/**
 * deadlock_demolition
 * CS 341 - Fall 2023
 */
#include "graph.h"
#include "libdrm.h"
#include "set.h"
#include <bits/pthreadtypes.h>
#include <pthread.h>

static pthread_mutex_t m_global = PTHREAD_MUTEX_INITIALIZER; // :warning: The provided graph data structure is not thread-safe.; this ensures thread safeness
graph *g = NULL;
struct drm_t
{
    pthread_mutex_t m;
};
set *visited = NULL;

int has_cycle(graph *g, void *node) { // run DFS
  if (!visited) // lazy init
    visited = shallow_set_create();  
  
  if (set_contains(visited, node)) return 1;
  else {

    int was_visited = false;
    set_add(visited, node);
    vector *neigh_vector = graph_neighbors(g, node);

    VECTOR_FOR_EACH(neigh_vector, neigh, {
        if (!set_contains(visited, neigh)) was_visited |= (has_cycle(g, neigh));
    });

    return was_visited;
  }

}

drm_t *drm_init()
{
    /* Your code here */
    drm_t *drm = (drm_t *)malloc(sizeof(drm_t));
    pthread_mutex_init(&m_global, NULL);

    pthread_mutex_lock(&m_global);
  
    if (!g) // lazy init
        g = shallow_graph_create();

    graph_add_vertex(g, drm);

    pthread_mutex_unlock(&m_global);

    return drm;
}

int drm_post(drm_t *drm, pthread_t *thread_id)
{
    /* Your code here */

    pthread_mutex_lock(&m_global);
    int exit_code = 0;
    if (!graph_contains_vertex(g, drm) || !graph_contains_vertex(g, thread_id))
        exit_code = 0;
    else
    {

        if (graph_adjacent(g, drm, thread_id)) { // if drm -> thred edge exists in graph
          graph_remove_edge(g, drm, thread_id);  // remove edge
          pthread_mutex_unlock(&drm->m); // unlock drm
        }
        exit_code = 1;
    }

    pthread_mutex_unlock(&m_global);

    return exit_code;
}

int drm_wait(drm_t *drm, pthread_t *thread_id)
{
    pthread_mutex_lock(&m_global);
    int exit_code = 0;

    if (!graph_contains_vertex(g, thread_id)) { // Add the thread to the Resource Allocation Graph if not already present. Hint: what unique identifier can you use for each thread?
        graph_add_vertex(g, thread_id);
    }

    if (graph_adjacent(g, drm, thread_id)) { // thread trying to lock a mutex it already owns
        pthread_mutex_unlock(&m_global);
        return 0;
    } 

    graph_add_edge(g, thread_id, drm); // Add the appropriate edge to the Resource Allocation Graph. - attempt to lock

    // int thread_already_owns_mutex = 0;
    // vector *thread_neigh = graph_neighbors(g, thread_id);

    if (has_cycle(g, thread_id)) // the attempt to lock the drm by the thread would cause deadlock.
    {
       graph_remove_edge(g, thread_id, drm); // Add the appropriate edge to the Resource Allocation Graph. - attempt to lock
       exit_code = 0; 
    }
    else {


        pthread_mutex_unlock(&m_global); // let `post` to unlock the `drm`, otherwise we deadlock: wait waits for drm->m, post waits for m_global
        pthread_mutex_lock(&drm->m); // lock the drm; 
        pthread_mutex_lock(&m_global);

        graph_remove_edge(g, thread_id, drm);
        graph_add_edge(g, drm, thread_id); //  DRM is locked by one thread at a time <=> drm has only one outgoing edge at a time
        exit_code = 1;
    }

    visited = NULL;
    pthread_mutex_unlock(&m_global);
    return exit_code;
}

void drm_destroy(drm_t *drm)
{
    /* Your code here */
    if (!drm)
        return;
    graph_remove_vertex(g, drm);
    pthread_mutex_destroy(&drm->m);
    free(drm);
    pthread_mutex_destroy(&m_global);
    return;
}
