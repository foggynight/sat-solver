#include "sat_solver.h"

#include <assert.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "AST.h"
#include "CNF.h"
#include "DA.h"
#include "util.h"

// Brute Force -----------------------------------------------------------------

// Search for SAT solution given AST of expression and array of variables. Uses
// brute force by walking through each possible set of bindings.
bool solve_brute_force(
    const CNF_Root *cnf_root,
    CNF_Var max_var,
    bool first_solution,
    DA_Solution *out_solutions)
{
    CNF_Binds cnf_binds = CNF_Binds_make_zeros(max_var);
    printf("Searching...\n");
    for (size_t i = 0; true; ++i) {
        printf("  %ld: ", i);
        CNF_Binds_print(&cnf_binds);
        const bool result = CNF_Root_eval_with_binds(cnf_root, &cnf_binds);
        printf(" -> %c\n", result ? 'T' : 'F');
        if (result) {
            const Solution solution = CNF_Binds_copy(&cnf_binds);
            DA_APPEND(*out_solutions, solution);
            if (first_solution) { break; }
        }
        if (!CNF_Binds_increment(&cnf_binds)) { break; }
    }
    CNF_Binds_free(&cnf_binds);
    return out_solutions->count > 0;
}

// Polarity --------------------------------------------------------------------
// Determine the polarity of a variable in a CNF expression.

typedef enum {
    POLARITY_NULL,      // Variable not in expression, skip.
    POLARITY_CONFLICT,  // Variable with opposite polarities, not pure.
    POLARITY_TRUE,      // Variable only true polarity, pure.
    POLARITY_FALSE,     // Variable only false polarity, pure.
} Polarity;

static Polarity Polarity_lit(CNF_Lit lit) {
    if (lit > 0) { return POLARITY_TRUE; }
    if (lit < 0) { return POLARITY_FALSE; }
    return POLARITY_NULL;
}

static Polarity Polarity_prev_and_lit(Polarity prev, CNF_Lit lit) {
    const Polarity lit_pol = Polarity_lit(lit);
    if (prev == POLARITY_NULL) { return lit_pol; }
    if (lit_pol == POLARITY_NULL) { return prev; }
    if (prev != lit_pol) { return POLARITY_CONFLICT; }
    return lit_pol;
}

static Polarity Polarity_from_cnf(const CNF_Root *root, CNF_Var var) {
    Polarity polarity = POLARITY_NULL;
    for (size_t clause_i = 0; clause_i < CNF_Root_len(root); ++clause_i) {
        const CNF_Clause *clause = CNF_Root_index_clauses(root, clause_i);
        for (size_t lit_i = 0; lit_i < CNF_Clause_len(clause); ++lit_i) {
            const CNF_Lit lit = CNF_Clause_index_lits(clause, lit_i);
            if (imaxabs(lit) == var) {
                polarity = Polarity_prev_and_lit(polarity, lit);
            }
            if (polarity == POLARITY_CONFLICT) {
                break;
            }
        }
    }
    return polarity;
}

// DPLL ------------------------------------------------------------------------
// Davis-Putnam-Logemann-Loveland Algorithm

typedef struct {
    CNF_Lit lit;   // Assigned literal (variable and polarity).
    size_t depth;  // Decision depth of assignment.
} DPLL_VarAssign;
typedef DA(DPLL_VarAssign) DPLL_History_VA;

typedef struct {
    size_t index;  // Index of clause in CNF_Root.
    size_t depth;  // Decision depth of deletion.
} DPLL_ClauseDelete;
typedef DA(DPLL_ClauseDelete) DPLL_History_CD;

typedef struct {
    // Constants passed in to DPLL solver as arguments.
    const CNF_Root * const cnf_root;
    const CNF_Var max_var;
    const bool first_solution;
    DA_Solution * const out_solutions;

    // Working state of DPLL solver.
    bool is_done;                // Should solver stop, first solution found.
    DPLL_History_VA history_va;  // History stack of literal assignments.
    DPLL_History_CD history_cd;  // History stack of clause deletions.
    CNF_Binds binds;             // Temporary location to store bindings.
} DPLL_State;

// Doesn't free the state struct or CNF_Root member, handled by caller.
static void DPLL_State_free(DPLL_State *state) {
    assert(state != NULL);
    DA_FREE(state->history_va);
    DA_FREE(state->history_cd);
    // TODO: Free binds?
}

static void DPLL_assign_var(
    DPLL_State *state, CNF_Var var, bool val, size_t depth)
{
    const CNF_Lit lit = val ? var : -var;
    const DPLL_VarAssign va = { lit, depth };
    DA_PUSH(state->history_va, va);
}

static CNF_Lit DPLL_find_assign(const DPLL_State *state, CNF_Var var) {
    for (size_t i = 0; i < state->history_va.count; ++i) {
        const DPLL_VarAssign va = state->history_va.items[i];
        if (var == imaxabs(va.lit)) {
            return va.lit;
        }
    }
    return 0;
}

static void DPLL_delete_clauses_with_lit(
    DPLL_State *state, CNF_Lit lit, size_t depth)
{
    for (size_t i_clause = 0; i_clause < state->cnf_root->count; ++i_clause) {
        CNF_Clause *clause = CNF_Root_index_clauses(state->cnf_root, i_clause);
        if (!clause->is_deleted && CNF_Clause_contains_lit(clause, lit)) {
            clause->is_deleted = true;
            const DPLL_ClauseDelete cd = { i_clause, depth };
            DA_PUSH(state->history_cd, cd);
        }
    }
}

static void DPLL_pure_literal_elimination(DPLL_State *state, size_t depth) {
    for (CNF_Var var = 1; var <= state->max_var; ++var) {
        const Polarity pol = Polarity_from_cnf(state->cnf_root, var);
        if (pol == POLARITY_TRUE || pol == POLARITY_FALSE) {
            const CNF_Lit lit = (pol == POLARITY_TRUE) ? var : -var;
            printf("PLE: Found pure literal: %zd\n", lit);
            DPLL_assign_var(state, var, (pol == POLARITY_TRUE), depth);  // TODO: Use lit computed above.
            DPLL_delete_clauses_with_lit(state, lit, depth);
        }
    }
}

// TODO: Replace with eval of CNF directly from history?
static void DPLL_update_binds_from_history(
    const DPLL_History_VA *history_va,
    CNF_Binds *out_binds)
{
    for (size_t i = 0; i < history_va->count; ++i) {
        const DPLL_VarAssign va = history_va->items[i];
        const CNF_Var var = imaxabs(va.lit);
        CNF_Bind * const bind = &(out_binds->items[var]);
        bind->val = (va.lit > 0);
        bind->bound = true;
    }
}

// Select unassigned variable for which to apply the splitting rule.
static CNF_Var DPLL_split_select_variable(const DPLL_State *state) {
    for (CNF_Var var = 1; var <= state->max_var; ++var) {
        if (!DPLL_find_assign(state, var)) {
            return var;
        }
    }
    return CNF_VAR_NULL;
}

// TODO: Should depth = 0 count as root split or only preprocessing?
static bool DPLL(DPLL_State *state, size_t depth) {
    static size_t eval_count = 0;

    // Stop DPLL search early. Used when searching for only first solution.
    if (state->is_done) { return true; }

    // If all variables are assigned then evaluate, otherwise split.
    if (state->history_va.count == (size_t)state->max_var) {
        DPLL_update_binds_from_history(&(state->history_va), &(state->binds));
        printf("  %zu: ", eval_count++);
        CNF_Binds_print(&(state->binds));
        printf(" -> ");
        const bool result =
            CNF_Root_eval_with_binds(state->cnf_root, &(state->binds));
        printf("%c\n", result ? 'T' : 'F');
        if (result) {
            const Solution solution = CNF_Binds_copy(&(state->binds));
            DA_APPEND(*(state->out_solutions), solution);
            if (state->first_solution) { state->is_done = true; }
        }
        return result;
    }

    const CNF_Var var = DPLL_split_select_variable(state);
    assert(var != CNF_VAR_NULL);

    printf("Splitting on: %zu\n", var);

    DPLL_VarAssign va;
    bool result = false;

    // Check variable negative branch of split.
    va = (DPLL_VarAssign){ -((CNF_Lit)var), depth };
    DA_PUSH(state->history_va, va);
    if (DPLL(state, depth + 1)) { result = true; }
    UNUSED(DA_POP(state->history_va));

    if (result && state->first_solution) { return true; }

    // Check variable positive branch of split.
    va = (DPLL_VarAssign){ +((CNF_Lit)var), depth };
    DA_PUSH(state->history_va, va);
    if (DPLL(state, depth + 1)) { result = true; }
    UNUSED(DA_POP(state->history_va));

    return result;
}

bool solve_DPLL(
    const CNF_Root *cnf_root,
    const CNF_Var max_var,
    const bool first_solution,
    DA_Solution *out_solutions)
{
    CNF_Binds binds = CNF_Binds_make_vars(max_var);
    DPLL_State state = (DPLL_State){
        cnf_root, max_var, first_solution, out_solutions,
        false, {0}, {0}, binds,
    };

    printf("Eliminating pure variables...\n");
    DPLL_pure_literal_elimination(&state, 0);

    printf("Searching...\n");
    const bool result = DPLL(&state, 0); // TODO: First depth = 0 or 1?

    DPLL_State_free(&state);
    return result;
}
