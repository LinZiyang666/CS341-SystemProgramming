/**
 * savvy_scheduler
 * CS 341 - Fall 2023
 */
#include "libpriqueue/libpriqueue.h"
#include "libscheduler.h"

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "print_functions.h"

double turnaround_time; // Time from when process arives to when it ends executing
double response_time;   // Total latency time: from when the process arrives to the CPU, to when it actually starts working
double wait_time;       // The total time that a process waits on the ready (priority) queue
int n_jobs;

/**
 * The struct to hold the information about a given job
 */
typedef struct _job_info
{
    int id;

    /* TODO: Add any other information and bookkeeping you need into this
     * struct. */

    // from `struct scheduler_info`
    double priority;
    double running_time;

    double start_time;
    double arrival_time;   // for fcfs
    double remaining_time; // for psrtf
    double next_time;      // for RR = arrival time in ready queue

    double start_after_preempt;

} job_info;

bool is_preemptive(scheme_t scheme)
{ // NOTE: RoundRobin is also pre-emptive, even though its name does not state that
    return scheme == PPRI || scheme == PSRTF || scheme == RR;
}

void scheduler_start_up(scheme_t s)
{
    switch (s)
    {
    case FCFS:
        comparision_func = comparer_fcfs;
        break;
    case PRI:
        comparision_func = comparer_pri;
        break;
    case PPRI:
        comparision_func = comparer_ppri;
        break;
    case PSRTF:
        comparision_func = comparer_psrtf;
        break;
    case RR:
        comparision_func = comparer_rr;
        break;
    case SJF:
        comparision_func = comparer_sjf;
        break;
    default:
        printf("Did not recognize scheme\n");
        exit(1);
    }
    priqueue_init(&pqueue, comparision_func);
    pqueue_scheme = s;
    // Put any additional set up code you may need here

    n_jobs = 0;
    turnaround_time = response_time = wait_time = 0.0;
}

static int break_tie(const void *a, const void *b)
{
    return comparer_fcfs(a, b);
}

int comparer_fcfs(const void *a, const void *b)
{ // Priority: Arrival increasing
    // Implement me!

    job *j1 = (job *)a;
    job *j2 = (job *)b;

    job_info *j1_info = (job_info *)j1->metadata;
    job_info *j2_info = (job_info *)j2->metadata;

    if (j1_info->arrival_time < j2_info->arrival_time)
        return -1;
    else if (j1_info->arrival_time > j2_info->arrival_time)
        return 1;
    else
        return 0;
}

int comparer_ppri(const void *a, const void *b)
{
    // Complete as is
    return comparer_pri(a, b);
}

int comparer_pri(const void *a, const void *b)
{ // Priority: priority increasing, if equal: break_tie()
    //  For the priority comparers, lower priority runs first

    // Implement me!
    job *j1 = (job *)a;
    job *j2 = (job *)b;

    job_info *j1_info = (job_info *)j1->metadata;
    job_info *j2_info = (job_info *)j2->metadata;

    if (j1_info->priority < j2_info->priority)
        return -1;
    else if (j1_info->priority > j2_info->priority)
        return 1;
    else
        return break_tie(a, b); // FCFS
}

int comparer_psrtf(const void *a, const void *b)
{ // Priority: Remaining increasing, if equal: break_tie()
    // Implement me!

    job *j1 = (job *)a;
    job *j2 = (job *)b;

    job_info *j1_info = (job_info *)j1->metadata;
    job_info *j2_info = (job_info *)j2->metadata;

    if (j1_info->remaining_time < j2_info->remaining_time)
        return -1;
    else if (j1_info->remaining_time > j2_info->remaining_time)
        return 1;
    else
        return break_tie(a, b); // FCFS
}

int comparer_rr(const void *a, const void *b)
{ // Priority: arriving time IN QUEUE (not to the scheduler) increasing
    // Implement me!

    job *j1 = (job *)a;
    job *j2 = (job *)b;

    job_info *j1_info = (job_info *)j1->metadata;
    job_info *j2_info = (job_info *)j2->metadata;

    if (j1_info->next_time < j2_info->next_time)
        return -1;
    else if (j1_info->next_time > j2_info->next_time)
        return 1;
    else
        return break_tie(a, b); // FCFS
}

int comparer_sjf(const void *a, const void *b)
{ // Priority: running_time (total CPU time) increasing
    // Implement me!
    job *j1 = (job *)a;
    job *j2 = (job *)b;

    job_info *j1_info = (job_info *)j1->metadata;
    job_info *j2_info = (job_info *)j2->metadata;

    if (j1_info->running_time < j2_info->running_time)
        return -1;
    else if (j1_info->running_time > j2_info->running_time)
        return 1;
    else
        return break_tie(a, b);
}

// Do not allocate stack space or initialize ctx. These will be overwritten by
// gtgo
void scheduler_new_job(job *newjob, int job_number, double time,
                       scheduler_info *sched_data)
{ // Set-up a new job
    // Implement me!

    // Create & populate `job_info` struct,
    job_info *my_job = calloc(1, sizeof(job_info));
    my_job->id = job_number;
    my_job->priority = sched_data->priority;
    my_job->running_time = sched_data->running_time;
    my_job->remaining_time = sched_data->running_time; // Initially: remaining = running
    my_job->arrival_time = time;
    my_job->start_time = -1; // Job has not started yet
    my_job->next_time = -1;  // Job has not yet been added to the ready (priority) queue

    my_job->start_after_preempt = time; // The tme at which `my_job` will start again, after preempt; used by PSRTF to calculate `remaining time` of each job

    newjob->metadata = my_job;       // The only field you will be using or modifying is metadata, where you must insert your job_info struct
    priqueue_offer(&pqueue, newjob); // Once you’ve set up newjob offer it to the queue.
}

job *scheduler_quantum_expired(job *job_evicted, double time)
{
    job *j = priqueue_peek(&pqueue);

    if (!j && !job_evicted) return NULL;

    if (j) {
    job_info *j_info = j->metadata;
    if (j_info->start_time == -1) // Intialise actual start time of a job
        j_info->start_time = time;
    }

    if (!job_evicted) // No jobs are running
    {
        // Schedule the first in the queue
        job *next_job = priqueue_peek(&pqueue);
        job_info *next_job_info = (job_info *)next_job->metadata;
        next_job_info->start_after_preempt = time;
        return next_job;
    }
    else
    {
        // Mark that the `job_evicted` was added to the ready queue again at time `next_time`
        job_info *j_ev_info = (job_info *)job_evicted->metadata;
        j_ev_info->next_time = time;

        if (is_preemptive(pqueue_scheme))
        {
            // place it back on the queue an (Note, it is possible for the next job to be the same as job_evicted
            job *next_job = priqueue_poll(&pqueue);
            job_info *next_job_info = (job_info *)next_job->metadata;
            next_job_info->start_after_preempt = time;
            priqueue_offer(&pqueue, next_job);

            j_ev_info->remaining_time -= time - j_ev_info->start_after_preempt;
            return priqueue_peek(&pqueue); // ret. a pointer to the next job that should run
        }
        else
        {
            return job_evicted;
        }
    }
}

void scheduler_job_finished(job *job_done, double time)
{
    // Implement me!
    // update any statistics you are collecting & free any buffers you may have allocated for this job’s metadata.
    // (!) : poll from PQ

    job_info *j_info = (job_info *)job_done->metadata;

    n_jobs++;
    turnaround_time += time - j_info->arrival_time;
    response_time += j_info->start_time - j_info->arrival_time;
    wait_time += time - j_info->arrival_time - j_info->running_time;

    free(j_info);           // Free metadata
    priqueue_poll(&pqueue); // Remove `job_done` from queue; as `scheduler_job_finished` is called w/ `job_done`, this means that this job is at the top of the PQ
}

static void print_stats()
{
    fprintf(stderr, "turnaround     %f\n", scheduler_average_turnaround_time());
    fprintf(stderr, "total_waiting  %f\n", scheduler_average_waiting_time());
    fprintf(stderr, "total_response %f\n", scheduler_average_response_time());
}

double scheduler_average_waiting_time()
{
    // Implement me!
    return wait_time * 1.0 / n_jobs;
}

double scheduler_average_turnaround_time()
{
    // Implement me!
    return turnaround_time * 1.0 / n_jobs;
}

double scheduler_average_response_time()
{
    // Implement me!
    return response_time * 1.0 / n_jobs;
}

void scheduler_show_queue()
{
    // OPTIONAL: Implement this if you need it!
}

void scheduler_clean_up()
{
    priqueue_destroy(&pqueue);
    print_stats();
}
