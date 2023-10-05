/**
 * deadlock_demolition
 * CS 341 - Fall 2023
 */
#include "graph.h"
#include "libdrm.h"
#include "set.h"
#include <bits/pthreadtypes.h>
#include <pthread.h>

pthread_mutex_t m_global = PTHREAD_MUTEX_INITIALIZER;
graph *g = NULL;
struct drm_t
{
    pthread_mutex_t m;
};
set *visited = NULL;

drm_t *drm_init()
{
    /* Your code here */
    drm_t *drm = (drm_t *)malloc(sizeof(drm_t));
    pthread_mutex_init(&m_global, NULL);

    pthread_mutex_lock(&m_global);

    if (!g)
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
    if (!graph_contains_vertex(g, drm))
        exit_code = 0;
    else
    {

        if (graph_adjacent(g, drm, thread_id)) { // if drm -> thred edge exists in graph
          graph_remove_edge(g, drm, thread_id);  // remove edge
          pthread_mutex_unlock(&drm->m); // unlock drm
        }
        exit_code = 1;
    }

    pthread_mutex_lock(&m_global);

    return exit_code;
}

int drm_wait(drm_t *drm, pthread_t *thread_id)
{
    /* Your code here */
    return 0;
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
