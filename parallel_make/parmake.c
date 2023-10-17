/**
 * parallel_make
 * CS 341 - Fall 2023
 */

#include "format.h"
#include "graph.h"
#include "parmake.h"
#include "parser.h"

int has_cycle(void *target);
int tricolor(dictionary *mp, void *target);

void get_rules_in_order(vector *targets);
void get_rules(dictionary *cnt, vector *targets);


// Method that is called when thread is created
void* solve(void *arg); 

// The graph and vector classes are not thread-safe! ->
// a.) rule_lock, rule_cv for `rules`
// b.) graph_lock, graph_cv for `g`
graph *g = NULL;
vector *rules = NULL;
pthread_cond_t rule_cv = PTHREAD_COND_INITIALIZER;
pthread_cond_t g_cv = PTHREAD_COND_INITIALIZER;
pthread_mutex_t rule_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t g_lock = PTHREAD_MUTEX_INITIALIZER;


/* Helper Functions */

// Allocates a string -> int dictionary and returns it
dictionary *dictionary_init() {
    dictionary *mp = string_to_int_dictionary_create();
    vector *nodes = graph_vertices(g);
    size_t num_nodes = vector_size(nodes);
    for (size_t i = 0; i < num_nodes; ++i) {
        int val = 0; 
        dictionary_set(mp, vector_get(nodes, i), &val);
    }
    return mp;
}

// ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------ // 
// Cycle methods

// 1, if graph has cycle starting traversal from `target`, else 0
int has_cycle(void *target) {
    dictionary *mp = dictionary_init();
    int isCycle = tricolor(mp, target); 
    dictionary_destroy(mp); //TODO: LAB - Ask if this hsould be destroyed.
    return isCycle;
}

/*
Source: https://www.cs.cornell.edu/courses/cs2112/2012sp/lectures/lec24/lec24-12sp.html
Performs Tricolor algorithm to detect cycle
Each entry in the `mp` dictionary is in one of the following 3 states:
mp[key] = 0 -> (White) Not visited yet 
mp[key] = 1 -> (Grey) in progress
mp[key] = 2 -> (Black) finished
Returns: 1, if cycle found, else 0
*/
int tricolor (dictionary *mp, void *target) {

    int *value = (int *)dictionary_get(mp, target);
    if (*value == 1) // got to a Grey node once again => CYCLE
        return 1; 
    else if (*value == 2) // `target` is Black -> done, no cycle
        return 0; 
    
    // mark as `in progress` (Gray)
    dictionary_set(mp, target, 1);
    vector *children = graph_neighbors(g, target); 
    size_t num_children = vector_size(children);

    for (size_t i = 0; i < num_children; ++i) {
        int isCycle = tricolor(mp, vector_get(children, i));
        if (isCycle) {
            vector_destroy(children);
            return isCycle;
        }
    }
    
    //mark as `finished` (Black)
    dictionary_set(mp, target, 2);
    vector_destroy(children);
    return 0; // IF NONE of children produced cycle -> no cycle
}
// Detect graph cycle w/ "Tricolor algorithm" using `dictionary`
// White (0) -> !seen node
// Gray (1) -> have been discovered but that the algorithm is not done with yet; Frontier between White and Black
// Black (2) -> node that algorithm is done with.

// ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------ // 
// Rules methods

void get_rules_in_order(vector *targets) {
    dictionary *cnt = dictionary_init();
    get_rules(cnt, targets);
    dictionary_destroy(cnt);
}

// Perform a DFS and enque the nodes when you get back from recursive call (i.e: target is enqueued AFTER all it's dependencies have been enqueued)
// Each entry in `cnt` dictionary can be in one of the following states:
// 0 -> !visited yet
// 1 -> visited
void get_rules (dictionary *cnt, vector *targets) {
    size_t num_targets = vector_size(targets);
    for (size_t i = 0; i < num_targets; ++i) {
        void *target = vector_get(targets, i);
        void *dependencies = graph_neighbors(g, target);
        get_rules(cnt, dependencies); // DFS

        int seen = (*(int*)dictionary_get(g, target));
        if (!seen) {
            dictionary_set(cnt, target, 1); // mark as seen
            vector_push_back(rules, target); // enqueue
        }
        vector_destroy(dependencies);
    }
}

// ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------ // 





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
            print_cycle_failure((char *)goal_rule);
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
