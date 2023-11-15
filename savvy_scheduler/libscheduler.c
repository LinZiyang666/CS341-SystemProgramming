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


double turnaround_time;
double response_time;
double wait_time;
int n_jobs;

/**
 * The struct to hold the information about a given job
 */
typedef struct _job_info {
    int id;

    /* TODO: Add any other information and bookkeeping you need into this
     * struct. */

    // from `struct scheduler_info`
    double priority;
    double running_time;

    double start_time; 
    double arrival_time; // for fcfs
    double remaining_time; // for psrtf
    double next_time; // for RR = arrival time in ready queue

} job_info;

bool is_preemptive(scheme_t scheme) { // NOTE: RoundRobin is also pre-emptive, even though its name does not state that
    return scheme == PPRI || scheme == PSRTF || scheme == RR;
}

void scheduler_start_up(scheme_t s) {
    switch (s) {
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

static int break_tie(const void *a, const void *b) {
    return comparer_fcfs(a, b);
}

int comparer_fcfs(const void *a, const void *b) { // Priority: Arrival increasing
    // Implement me!

    job* j1 = (job*)a;
    job* j2 = (job*)b;

    job_info* j1_info = (job_info*) j1->metadata;
    job_info* j2_info = (job_info*) j2->metadata;

    if (j1_info->arrival_time < j2_info->arrival_time) return -1;
    else if (j1_info->arrival_time > j2_info->arrival_time) return 1;
}

int comparer_ppri(const void *a, const void *b) { 
    // Complete as is
    return comparer_pri(a, b);
}

int comparer_pri(const void *a, const void *b) { // Priority: priority increasing, if equal: break_tie()
    //  For the priority comparers, lower priority runs first

    // Implement me!
    job* j1 = (job*)a;
    job* j2 = (job*)b;

    job_info* j1_info = (job_info*) j1->metadata;
    job_info* j2_info = (job_info*) j2->metadata;

    if (j1_info->priority < j2_info->priority) return -1;
    else if (j1_info->priority > j2_info->priority) return 1;
    else return break_tie(a, b); //FCFS
}

int comparer_psrtf(const void *a, const void *b) { // Priority: Remaining increasing, if equal: break_tie()
    // Implement me!
    
    job* j1 = (job*)a;
    job* j2 = (job*)b;

    job_info* j1_info = (job_info*) j1->metadata;
    job_info* j2_info = (job_info*) j2->metadata;

    if (j1_info->remaining_time < j2_info->remaining_time) return -1;
    else if (j1_info->remaining_time > j2_info->remaining_time) return 1;
    else return break_tie(a, b); //FCFS
}

int comparer_rr(const void *a, const void *b) { // Priority: arriving time IN QUEUE (not to the scheduler) increasing 
    // Implement me!

    job* j1 = (job*)a;
    job* j2 = (job*)b;

    job_info* j1_info = (job_info*) j1->metadata;
    job_info* j2_info = (job_info*) j2->metadata;

    if (j1_info->next_time < j2_info->next_time) return -1;
    else if (j1_info->next_time > j2_info->next_time) return 1;
    else return break_tie(a, b); //FCFS
}

int comparer_sjf(const void *a, const void *b) { // Priority: running_time (total CPU time) increasing
    // Implement me!
    job* j1 = (job*)a;
    job* j2 = (job*)b;

    job_info* j1_info = (job_info*) j1->metadata;
    job_info* j2_info = (job_info*) j2->metadata;

    if (j1_info->running_time < j2_info->running_time) return -1;
    else if (j1_info->running_time > j2_info->running_time) return 1;
    else return break_tie(a, b);
}

// Do not allocate stack space or initialize ctx. These will be overwritten by
// gtgo
void scheduler_new_job(job *newjob, int job_number, double time,
                       scheduler_info *sched_data) { // Set-up a new job
    // Implement me!

    // Create & populate `job_info` struct, 
    job_info* my_job = calloc(1, sizeof(job_info));
    my_job->id = job_number;
    my_job->priority = sched_data->priority;
    my_job->running_time = sched_data->running_time;
    my_job->remaining_time = sched_data->running_time; // Initially: remaining = running
    my_job->arrival_time = time;
    my_job->start_time = -1; // Job has not started yet
    my_job->next_time = -1; // Job has not yet been added to the ready (priority) queue


    newjob->metadata = my_job; // The only field you will be using or modifying is metadata, where you must insert your job_info struct

        
    priqueue_offer(&pqueue, newjob);    // Once you’ve set up newjob offer it to the queue.

    n_jobs ++;
}

job *scheduler_quantum_expired(job *job_evicted, double time) {
    // TODO: Implement me!
    return NULL;
}

void scheduler_job_finished(job *job_done, double time) {
    // TODO: Implement me!
}

static void print_stats() {
    fprintf(stderr, "turnaround     %f\n", scheduler_average_turnaround_time());
    fprintf(stderr, "total_waiting  %f\n", scheduler_average_waiting_time());
    fprintf(stderr, "total_response %f\n", scheduler_average_response_time());
}

double scheduler_average_waiting_time() {
    // TODO: Implement me!
    return 9001;
}

double scheduler_average_turnaround_time() {
    // TODO: Implement me!
    return 9001;
}

double scheduler_average_response_time() {
    // TODO: Implement me!
    return 9001;
}

void scheduler_show_queue() {
    // OPTIONAL: Implement this if you need it!
}

void scheduler_clean_up() {
    priqueue_destroy(&pqueue);
    print_stats();
}
