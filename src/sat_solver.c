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
    printf("Checking Binds:\n");
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
        if (!CNF_Binds_inc(&cnf_binds)) { break; }
    }
    CNF_Binds_free(&cnf_binds);
    return out_solutions->count > 0;
}

// Polarity --------------------------------------------------------------------
// Determine the polarity of a variable in a CNF expression.

typedef enum {
    POLARITY_NULL,     // Variable not in expression, skip.
    POLARITY_CONFLICT, // Variable with opposite polarities, not pure.
    POLARITY_TRUE,     // Variable only true polarity, pure.
    POLARITY_FALSE,    // Variable only false polarity, pure.
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

static Polarity Polarity_from_cnf(CNF_Root *root, CNF_Var var) {
    Polarity polarity = POLARITY_NULL;
    for (size_t clause_i = 0; clause_i < CNF_Root_len(root); ++clause_i) {
        const CNF_Clause clause = CNF_Root_index_clauses(root, clause_i);
        for (size_t lit_i = 0; lit_i < CNF_Clause_len(&clause); ++lit_i) {
            const CNF_Lit lit = CNF_Clause_index_lits(&clause, lit_i);
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

// TODO: Apply pure literal elimination as a preprocessing step, not during
// each step of recursive search. Unit clause propagation is done at each step.

typedef struct {
    CNF_Lit lit;
    //size_t decision_level;
    //CNF_Clause *reason;
} DPLL_Event;
typedef DA(DPLL_Event) DPLL_History;

static void DPLL_History_push(DPLL_History *history, const DPLL_Event event) {
    assert(history != NULL);
    DA_PUSH(*history, event);
}

static DPLL_Event DPLL_History_pop(DPLL_History *history) {
    assert(history != NULL);
    assert(history->count > 0);
    return DA_POP(*history);
}

static DPLL_Event DPLL_History_top(DPLL_History *history) {
    assert(history != NULL);
    assert(history->count > 0);
    return DA_TOP(*history);
}

static DPLL_Event *DPLL_History_find_var(
    const DPLL_History *history,
    const CNF_Var var)
{
    for (size_t i = 0; i < history->count; ++i) {
        DPLL_Event *event = &(history->items[i]);
        if (var == imaxabs(event->lit)) {
            return event;
        }
    }
    return NULL;
}

typedef struct {
    // Constants passed in to DPLL solver as arguments.
    const CNF_Root * const root;
    const CNF_Var max_var;
    const bool first_solution;
    DA_Solution * const out_solutions;

    // Working state of DPLL solver.
    DPLL_History history;  // History stacked used for backtracking.
    CNF_Binds binds;       // Temporary location to store bindings.
    bool done;             // Should solver die early, solution found.
} DPLL_State;
//typedef DA(DPLL_State) DA_DPLL_State;

// Doesn't free the state struct or CNF_Root member, handled by caller.
static void DPLL_State_free(DPLL_State *state) {
    assert(state != NULL);
    DA_FREE(state->history);
}

static bool DPLL_update_binds_from_history(
    const DPLL_History *history,
    CNF_Binds *out_binds)
{
    for (size_t i = 0; i < history->count; ++i) {
        const DPLL_Event event = history->items[i];
        const CNF_Var var = imaxabs(event.lit);

        if ((size_t)var > out_binds->count) { return false; }

        CNF_Bind * const bind = &(out_binds->items[var]);
        bind->val = (event.lit > 0);
        bind->bound = true;
    }
    return true;
}

// Select variable for which to apply the splitting rule.
static CNF_Var DPLL_split_select_variable(const DPLL_State *state) {
    for (CNF_Var var = 1; var <= state->max_var; ++var) {
        if (!DPLL_History_find_var(&(state->history), var)) {
            return var;
        }
    }
    return CNF_VAR_NULL;
}

static bool DPLL(DPLL_State *state) {
    static size_t eval_count = 0;

    if (state->done) { return true; }

    // If all variables are assigned then evaluate, otherwise split.
    if (state->history.count == (size_t)state->max_var) {
        DPLL_update_binds_from_history(&(state->history), &(state->binds));
        printf("  %zu: ", eval_count++);
        CNF_Binds_print(&(state->binds));
        printf(" -> ");
        const bool result =
            CNF_Root_eval_with_binds(state->root, &(state->binds));
        printf("%c\n", result ? 'T' : 'F');
        if (result) {
            const Solution solution = CNF_Binds_copy(&(state->binds));
            DA_APPEND(*(state->out_solutions), solution);
            if (state->first_solution) { state->done = true; }
        }
        return result;
    }

    const CNF_Var var = DPLL_split_select_variable(state);
    assert(var != CNF_VAR_NULL);

    DPLL_Event event;
    bool result = false;

    // Check variable negative branch of split.
    event = (DPLL_Event){ -((CNF_Lit)var) };
    DPLL_History_push(&(state->history), event);
    if (DPLL(state)) { result = true; }
    DPLL_History_pop(&(state->history));

    if (result && state->first_solution) { return true; }

    // Check variable positive branch of split.
    event = (DPLL_Event){ (CNF_Lit)var };
    DPLL_History_push(&(state->history), event);
    if (DPLL(state)) { result = true; }
    DPLL_History_pop(&(state->history));

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
        {0}, binds, false,
    };
    printf("Searching for solutions...\n");
    const bool result = DPLL(&state);
    DPLL_State_free(&state);
    return result;
}
