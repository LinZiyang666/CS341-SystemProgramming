/**
 * parallel_make
 * CS 341 - Fall 2023
 */

#include "format.h"
#include "graph.h"
#include "parmake.h"
#include "parser.h"

int has_cycle(char *rule);
void get_rules_in_order(vector *targets);
// Method that is called when thread is created
void *solve(void *arg); 

// The graph and vector classes are not thread-safe! ->
// a.) rule_lock, rule_cv for `rules`
// b.) graph_lock, graph_cv for `g`
graph *g = NULL;
vector *rules = NULL;
pthread_cond_t rule_cv = PTHREAD_COND_INITIALIZER;
pthread_cond_t g_cv = PTHREAD_COND_INITIALIZER;
pthread_mutex_t rule_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t g_lock = PTHREAD_MUTEX_INITIALIZER;

// Detect graph cycle w/ "Tricolor algorithm" using `dictionary`
// White (0) -> !seen node
// Gray (1) -> have been discovered but that the algorithm is not done with yet; Frontier between White and Black
// Black (2) -> node that algorithm is done with.

int parmake(char *makefile, size_t num_threads, char **targets)
{
    // good luck!
    graph *g = parser_parse_makefile(makefile, targets); // contains empty sentinel rule w/ key ""
    vector *goal_rules = graph_neighbors(g, "");         // only work on rules that descend from this rule (i.e. the goal rules and all their descendants)
    size_t ngoals = vector_size(goal_rules);

    int isCycle = 0; 
    for (size_t i = 0; i < ngoals; ++i) {
        char* goal_rule = vector_get(goal_rules, i);
        if (has_cycle(goal_rule)) {
            print_cycle_failure(goal_rule);
            isCycle = 1;
        }
    }

    if (isCycle) {
        vector_destroy(goal_rules);
        graph_destroy(g);
        return 0;
    }
    else {
        rules = shallow_vector_create(); // to allow caller (ME) to perform memory management
        get_rules_in_order(goal_rules); //TODO: The rule’s commands need to be run sequentially in the order they appear in the command vector

        pthread_t threads[num_threads];
        for (int i = 0; i < num_threads; ++i) 
            pthread_create(threads + i, NULL, solve, NULL);
        for (int i = 0; i < num_threads; ++i) 
            pthread_join(threads[i], NULL);

        vector_destroy(rules);
        vector_destroy(goal_rules);
        graph_destroy(g);
        return 0;
    }
}
