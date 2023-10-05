/**
 * deadlock_demolition
 * CS 341 - Fall 2023
 */
#include "graph.h"
#include "libdrm.h"
#include "set.h"
#include <bits/pthreadtypes.h>
#include <pthread.h>

// Protects `graph *g` critical sections
static pthread_mutex_t m_global = PTHREAD_MUTEX_INITIALIZER; // :warning: The provided graph data structure is not thread-safe.; this ensures thread safeness
graph *g = NULL; 
struct drm_t
{
    pthread_mutex_t m;
};
set *visited = NULL; // used in DFS (has_cycle) to detect cycles; gets reassigned to NULL after running the DFS


/*
Checks if there exists a cycle in the Directed Graph `g` that starts and ends in `node`
return: 1 (true), if cycle exists, 0 else
*/
int has_cycle(graph *g, void *node)
{                 // run DFS
    if (!visited) // lazy init
        visited = shallow_set_create();

    if (set_contains(visited, node))
        return 1;
    else
    {

        int was_visited = false;
        set_add(visited, node);
        vector *neigh_vector = graph_neighbors(g, node);

        VECTOR_FOR_EACH(neigh_vector, neigh, { // see vector's MACROs :D
            if (!set_contains(visited, neigh))
                was_visited |= (has_cycle(g, neigh)); 
        }); 

        return was_visited; // if at least one node `has_cycle` => return 1(true), else return 0
    }
}

drm_t *drm_init()
{
    drm_t *drm = (drm_t *)malloc(sizeof(drm_t));
    pthread_mutex_init(&drm->m, NULL);

    // Critical section for `graph *g` DS
    pthread_mutex_lock(&m_global);
    if (!g) // lazy init
        g = shallow_graph_create();
    graph_add_vertex(g, drm);
    pthread_mutex_unlock(&m_global);

    return drm;
}

int drm_post(drm_t *drm, pthread_t *thread_id)
{
    pthread_mutex_lock(&m_global);
    int exit_code = 0;

    if (!graph_contains_vertex(g, drm) || !graph_contains_vertex(g, thread_id)) //  both `drm` and `thread_id` should be vertexes that already exist
        exit_code = 0;
    else
    {

        if (graph_adjacent(g, drm, thread_id))
        {                                         // if (drm -> thread_id) edge exists in graph
            graph_remove_edge(g, drm, thread_id); // remove this edge
            pthread_mutex_unlock(&drm->m);        // unlock `drm` (binary semaphore)
        } 

        exit_code = 1; //TODO: ask in LAB what should we return if drm -> thread_id edge does not exist; I assume 1
    } 

    pthread_mutex_unlock(&m_global);

    return exit_code;
}

int drm_wait(drm_t *drm, pthread_t *thread_id)
{
    pthread_mutex_lock(&m_global);
    int exit_code = 0;

    if (!graph_contains_vertex(g, thread_id))
    { // Add the thread to the Resource Allocation Graph if not already present. Hint: what unique identifier can you use for each thread?
        graph_add_vertex(g, thread_id);
    }

    if (graph_adjacent(g, drm, thread_id))
    { // thread trying to lock a mutex it already owns => adding (thread_id -> drm) would have caused a cycle in RAG: thread_id -> drm -> thread_id, so DEADLOCK
        pthread_mutex_unlock(&m_global);
        return 0;
    }

    graph_add_edge(g, thread_id, drm); // Add the appropriate edge to the Resource Allocation Graph - attempt to lock (request resource)

    if (has_cycle(g, thread_id)) // Out attempt to lock the `drm` by the thread would cause deadlock. (cycle in RAG)
    {
        graph_remove_edge(g, thread_id, drm); // Remove (thread_id -> drm) edge
        exit_code = 0;
    }
    else
    {
        pthread_mutex_unlock(&m_global); // let `post` to unlock the `drm`, otherwise (PREV. WRONG IMPL., see commit history) we deadlock: `drm_wait` waits for `drm->m` held by `drm_post`, `drm_post` waits for `m_global` held by `drm_wait`
        pthread_mutex_lock(&drm->m);     // lock the `drm`;

        pthread_mutex_lock(&m_global);
        graph_remove_edge(g, thread_id, drm); // Our attempt did not create a deadlock => aquire resource
        graph_add_edge(g, drm, thread_id); //  `drm` owns/uses `thread_id`; `drm `is locked by one thread at a time <=> `drm` has only one outgoing edge at a time
        exit_code = 1;
    }

    visited = NULL;
    pthread_mutex_unlock(&m_global);
    return exit_code;
}

void drm_destroy(drm_t *drm)
{
    if (!drm)
        return;
    graph_remove_vertex(g, drm);
    pthread_mutex_destroy(&drm->m);
    free(drm);
    pthread_mutex_destroy(&m_global);
    return;
}
