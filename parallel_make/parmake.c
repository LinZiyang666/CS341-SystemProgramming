/**
 * parallel_make
 * CS 341 - Fall 2023
 */

#include "format.h"
#include "graph.h"
#include "parmake.h"
#include "parser.h"
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>
#include <pthread.h>
#include <set.h>

/*
Error codes for run status

ERR(0): error, don't run, mark failed
NOT_FINISHED(1): dependencies not finished yet
RUN_IMM(1): all deps are satisfied, run IMMEDIATELY ; rule is unsatisfied and available to be run
SAT_NO_RUN(2): should not run, don't mark as satisfied -> Basically, there is no need to satisfy a rule if it isn’t necessary to satisfy the goal rules or if we already know that all the goal rules it might satisfy are doomed to fail due to cycles
ALREADY_DONE(3): rule was already satisfied & run previously
*/
#define ERR -1
#define NOT_FINISHED 0
#define RUN_IMM 1
#define SAT_NO_RUN 2
#define ALREADY_DONE 3

// Same as #incldue <stdbool.h>
#define TRUE 1
#define FALSE 0

// Error codes for rules' states
#define DEFAULT 0
#define VALID 1
#define INVALID -1

// The graph and vector classes are not thread-safe! ->
// a.) rules_lock, rule_cv for `rules`
// b.) g_lock for `g`
graph *g = NULL;
vector *rules = NULL;                                  // Vector of rules in the order they should execute (i.e: ensures that if A depends on B, B can be found before A in vector 'rules'); Comptued using DFS and adding the nodes in a bottom-up (from leaves to root) approach, after we come back from the recursive call
pthread_cond_t rule_cv = PTHREAD_COND_INITIALIZER;     // CV & Mutex pattern to wake up the last waiting rule that has to be fulfilled
pthread_mutex_t rules_lock = PTHREAD_MUTEX_INITIALIZER; // Only one thread should modify (erase) rules from `rules` vector
pthread_mutex_t g_lock = PTHREAD_MUTEX_INITIALIZER;    // Only one thread should work on the graph at a time

int has_cycle(void *target);
int tricolor(dictionary *mp, void *target);

void get_rules_in_order(vector *targets);
void get_rules(set *cnt, vector *targets);

// ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------ //
// Multi-threaded methods

/* Return an integer depicitng the status of running the current rule
  -1: error, don't run, mark failed
  0: dependencies not finished yet
  1: all deps are satisfied, run IMMEDIATELY
  2: should not run, don't mark as satisfied -> Basically, there is no need to satisfy a rule if it isn’t necessary to satisfy the goal rules or if we already know that all the goal rules it might satisfy are doomed to fail due to cycles
  3: rule was already satisfied & run previously
*/
int run_status(void *target)
{
    rule_t *rule = (rule_t *)graph_get_vertex_value(g, target); // access MakeFile rule from graph

    if (rule->state != DEFAULT) // Rule's state changed before
        return ALREADY_DONE;    //  Once a rule is satisfied, it shouldn’t be run again.

    vector *dependencies = graph_neighbors(g, target);
    size_t num_dependencies = vector_size(dependencies);
    if (num_dependencies > 0)
    {
        int is_file_1 = (access((char *)target, F_OK) != ERR);
        if (is_file_1)
        { // The rule is the name of a file on disk
            for (size_t i = 0; i < num_dependencies; ++i)
            {
                char *dependency = vector_get(dependencies, i);
                int is_file_2 = (access(dependency, F_OK) != ERR);
                if (is_file_2)
                { //  it depends on another file

                    struct stat stat_file_1, stat_file_2;
                    int err_stat_1 = stat(target, &stat_file_1);
                    int err_stat_2 = stat(target, &stat_file_2);
                    if (err_stat_1 == ERR || err_stat_2 == ERR) // stat failed
                    {
                        vector_destroy(dependencies);
                        return ERR;
                    }
                    // Granularity of 1s -> provided by `st_mtime`
                    int diff = difftime(stat_file_1.st_mtime, stat_file_2.st_mtime);
                    if (diff < 0) // dependency is NEWER => rule is unsatisfied and available to be run
                    {
                        vector_destroy(dependencies);
                        return RUN_IMM;
                    }
                }
                else
                { // The rule depends on another rule that is not the name of a file on disk
                    vector_destroy(dependencies);
                    return RUN_IMM;
                }
            }

            vector_destroy(dependencies);
            return SAT_NO_RUN; // then the rule is already satisfied and does not need its commands executed.
        }
        else
        { // `target` not a file -> check if all dependencies succeeded (for any dependency: dependency->state = 1), if true -> 1,
            //  else propagate dependency's state by returning it: 0: if dependency not met yet, -1: if dependency had error

            pthread_mutex_lock(&g_lock); // graph class is not thread safe -> lock mutex to access its members
            // We now have to lock the mutex, as we access dep->state, to ensure that no other thread modifies it while we read it

            for (size_t i = 0; i < num_dependencies; ++i)
            {
                char *dependency = vector_get(dependencies, i);
                rule_t *sub_rule = (rule_t *)graph_get_vertex_value(g, dependency);
                if (sub_rule->state != VALID) // dependency not succeeded yet
                {
                    int st = sub_rule->state; // store in extra var. to resolve `data races` (i.e.: created by return sub_rule->state)
                    pthread_mutex_unlock(&g_lock);
                    vector_destroy(dependencies);
                    return st;
                }
            }

            // All subrules / dependencies suceeded :D  => run current rule immediately
            pthread_mutex_unlock(&g_lock);
            vector_destroy(dependencies);
            return RUN_IMM; //  A rule can be satisfied if and only if all of rules that it depends on have been satisfied
        }
    }
    else
    { // rule does not depend on any other rule
        vector_destroy(dependencies);
        int is_file_1 = (access((char *)target, F_OK) != ERR);
        if (!is_file_1) // not a file; ex: a: echo "A"
            return RUN_IMM;
        else
            return SAT_NO_RUN; // a file; ex. utils.c: echo "Hi"
    }
}

// Method that is called when thread is created
void *solve(void *arg)
{
    while (TRUE)
    {                                   // Solves artificially created spurious wakeups
        pthread_mutex_lock(&rules_lock); // The vector class is not thread-safe -> lock mutex to access the rules
        size_t num_rules = vector_size(rules);
        if (num_rules > 0)
        {

            for (size_t i = 0; i < num_rules; ++i) // check `rules` description at the top of the file -> by design:  A -- depends on --> B => B will appear earlier in the `rules` vector than A (i.e.: on a smaller index)
            {
                void *target = vector_get(rules, i);
                int stat_code = run_status(target);
                rule_t *rule = (rule_t *)graph_get_vertex_value(g, target); // access MakeFile rule from graph

                if (stat_code == RUN_IMM)
                {

                    vector_erase(rules, i); // remove rule
                    pthread_mutex_unlock(&rules_lock);

                    vector *commands = rule->commands;

                    int st = VALID;

                    size_t num_cmds = vector_size(commands);
                    for (size_t c = 0; c < num_cmds; ++c)
                    {
                        char *cmd = (char *)vector_get(commands, c);
                        if (system(cmd)) // use system() to run the commands associated with each rule; check if any such `cmd` failed
                            {
                                st = INVALID; // exit loop if so
                                break;
                            }
                    }

                    // Change state, and ensure thread-safeness by locking graph_lock -> no one else accesses the rule () while we modify it
                    pthread_mutex_lock(&g_lock);
                    rule->state = st;
                    pthread_cond_broadcast(&rule_cv); // let the other rules know about the change state in this rule; if there are instr. that depend on this rule, they can continue
                    pthread_mutex_unlock(&g_lock);
                    break;
                }
                else if (stat_code == SAT_NO_RUN || stat_code == ERR) // TODO: check correctness ?
                {

                    vector_erase(rules, i); // remove rule
                    pthread_mutex_unlock(&rules_lock);

                    pthread_mutex_lock(&g_lock);

                    if (stat_code == ERR)
                        rule->state = INVALID;
                    if (stat_code == SAT_NO_RUN)
                        rule->state = VALID; // parent should not run & marked as satisfied, so don't run this dependency of the parent either, just mark it as done

                    pthread_cond_broadcast(&rule_cv); // let the other rules know about hte change state in this rule; if there are instr. that depend on this rule, they can continue
                    pthread_mutex_unlock(&g_lock);
                    break;
                }
                else if (stat_code == ALREADY_DONE) // already satisfied & ran previously
                {
                    vector_erase(rules, i); // remove rule
                    pthread_mutex_unlock(&rules_lock);

                    break;
                }
                else if (i + 1 == num_rules) // last rule in the graph & deps not finished yet => WAIT (on cv)
                {
                    pthread_cond_wait(&rule_cv, &rules_lock); // unlock rules_lock, WAIT until signaled, re-lock rules_lock
                    pthread_mutex_unlock(&rules_lock);
                    break;
                }
                // if neither `if` case was true -> stat_code = 0 & not last rule to execute => SKIP rules[i], will try again later
            }
        }
        else
        {
            pthread_mutex_unlock(&rules_lock); // stop the thread, no more work to do :D
            break;
        }
    }

    return NULL; // don't need ret. val
}

// Multi-threaded methods END
// ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------ //

// ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------ //
// Cycle methods

// 1, if graph has cycle starting traversal from `target`, else 0
int has_cycle(void *target)
{
    dictionary *mp = string_to_int_dictionary_create();
    vector *nodes = graph_vertices(g);
    size_t num_nodes = vector_size(nodes);
    for (size_t i = 0; i < num_nodes; ++i)
    {
        int val = 0;
        dictionary_set(mp, vector_get(nodes, i), &val);
    }

    int isCycle = tricolor(mp, target);
    dictionary_destroy(mp); // TODO: LAB - Ask if this should be destroyed.
    vector_destroy(nodes); // Any vectors returned from graph functions must be destroyed manually to prevent memory leaks. Destroying these vectors will not destroy anything in the actual graph.
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
int tricolor(dictionary *mp, void *target)
{

    int *value = (int *)dictionary_get(mp, target);
    if (*value == 1) // got to a Grey node once again => CYCLE
        return 1;
    else if (*value == 2) // `target` is Black -> done, no cycle
        return 0;

    // mark as `in progress` (Gray)
    int *val = dictionary_get(mp, target);
    *val = 1; // update dict value

    vector *children = graph_neighbors(g, target);
    size_t num_children = vector_size(children);

    for (size_t i = 0; i < num_children; ++i)
    {
        int isCycle = tricolor(mp, vector_get(children, i));
        if (isCycle)
        {
            vector_destroy(children);
            return isCycle;
        }
    }

    // mark as `finished` (Black)
    int *final_val = dictionary_get(mp, target);
    *final_val = 2; // update dict value

    vector_destroy(children);
    return 0; // IF NONE of children produced cycle -> no cycle
}
// Detect graph cycle w/ "Tricolor algorithm" using `dictionary`
// White (0) -> !seen node
// Gray (1) -> have been discovered but that the algorithm is not done with yet; Frontier between White and Black
// Black (2) -> node that algorithm is done with.

// ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------ //
// Rules methods

void get_rules_in_order(vector *targets)
{
    set *cnt = shallow_set_create();
    get_rules(cnt, targets);
    set_destroy(cnt);
}

// Perform a DFS and enque the nodes when you get back from recursive call (i.e: target is enqueued AFTER all it's dependencies have been enqueued)
void get_rules(set *cnt, vector *targets)
{
    size_t num_targets = vector_size(targets);
    for (size_t i = 0; i < num_targets; ++i)
    {
        void *target = vector_get(targets, i);
        void *dependencies = graph_neighbors(g, target);
        get_rules(cnt, dependencies); // DFS

        int seen = set_contains(cnt, target);
        if (!seen)
        {
            set_add(cnt, target);            // visit
            vector_push_back(rules, target); // enqueue
        }
        vector_destroy(dependencies);
    }
}

// ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------ //

int parmake(char *makefile, size_t num_threads, char **targets)
{
    // good luck!
    g = parser_parse_makefile(makefile, targets); // contains empty sentinel rule w/ key ""
    vector *goal_rules = graph_neighbors(g, "");  // only work on rules that descend from this rule (i.e. the goal rules and all their descendants)
    size_t ngoals = vector_size(goal_rules);

    int isCycle = 0;
    for (size_t i = 0; i < ngoals; ++i)
    {
        void *goal_rule = vector_get(goal_rules, i);
        if (has_cycle(goal_rule))
        {
            print_cycle_failure((char *)goal_rule);
            isCycle = 1;
        }
    }

    if (isCycle)
    {
        vector_destroy(goal_rules);
        graph_destroy(g);
        return 0;
    }
    else
    {
        rules = shallow_vector_create(); // to allow caller (ME) to perform memory management
        get_rules_in_order(goal_rules);  // The rule’s commands need to be run sequentially in the order they appear in the command vector

        pthread_t threads[num_threads];
        for (size_t i = 0; i < num_threads; ++i)
            pthread_create(threads + i, NULL, solve, NULL);
        for (size_t i = 0; i < num_threads; ++i)
            pthread_join(threads[i], NULL);

        vector_destroy(rules);
        vector_destroy(goal_rules);
        graph_destroy(g);
        return 0;
    }
}
