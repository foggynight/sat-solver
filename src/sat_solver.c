#include "sat_solver.h"

#include <assert.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "AST.h"
#include "CNF.h"
#include "DA.h"
#include "util.h"

// Search for SAT solution given AST of expression and array of variables. Uses
// brute force by simply walking through each possible set of bindings linearly.
bool solve_brute_force(
    const CNF_Root *cnf_root,
    CNF_Var max_var,
    bool first_solution,
    DA_Solution *out_solutions)
{
    CNF_Binds cnf_binds = CNF_Binds_make_zeros(max_var);
    printf("Checking Binds:\n");
    for (size_t i = 0; i < (1u << max_var); ++i) {
        printf("  %ld: ", i);
        CNF_Binds_print(&cnf_binds);
        const bool result = CNF_Root_eval_with_binds(cnf_root, &cnf_binds);
        printf(" -> %c\n", result ? 'T' : 'F');
        if (result) {
            const Solution solution = CNF_Binds_copy(&cnf_binds);
            DA_APPEND(*out_solutions, solution);
            if (first_solution) { break; }
        }
        if (!CNF_Binds_inc(&cnf_binds)) {
            error_msg("solve_brute_force: failed to increment binds");
            return false;  // TODO: Signal error some other way.
        }
    }
    CNF_Binds_free(&cnf_binds);
    return out_solutions->count > 0;
}

// TODO
//static void unit_propagation(AST *ast, const DA_Var *vars) {
//
//}

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

static Polarity Polarity_resolve(Polarity prev, CNF_Lit lit) {
    const Polarity lit_pol = Polarity_lit(lit);
    if (prev == POLARITY_NULL) { return lit_pol; }
    if (lit_pol == POLARITY_NULL) { return prev; }
    if (prev != lit_pol) { return POLARITY_CONFLICT; }
    return lit_pol;
}

static Polarity DPLL_get_polarity(CNF_Root *root, CNF_Var var) {
    Polarity polarity = POLARITY_NULL;
    for (size_t clause_i = 0; clause_i < CNF_Root_len(root); ++clause_i) {
        const CNF_Clause clause = CNF_Root_index_clauses(root, clause_i);
        for (size_t lit_i = 0; lit_i < CNF_Clause_len(&clause); ++lit_i) {
            const CNF_Lit lit = CNF_Clause_index_lits(&clause, lit_i);
            if (imaxabs(lit) == var) {
                polarity = Polarity_resolve(polarity, lit);
            }
            if (polarity == POLARITY_CONFLICT) {
                break;
            }
        }
    }
    return polarity;
}

static bool DPLL(DA_CNF_State *state_stack) {
    //const CNF_State state = DA_TOP(*state_stack);
    return false;
}

// DPLL: Davis-Putnam-Logemann-Loveland Algorithm.
bool solve_DPLL(
    const CNF_Root *cnf_root,
    const CNF_Var max_var,
    bool first_solution,
    DA_Solution *out_solutions)
{
    CNF_Binds binds = CNF_Binds_make_vars(max_var);
    DA_CNF_State state_stack = {0};
    DA_APPEND(state_stack, ((CNF_State){ binds, {0} }));;
    bool result = DPLL(&state_stack);
    // TODO: free state_stack
    return result;
}
