#pragma once

typedef struct _task_que task_que_t;
typedef struct _task     task_t;

typedef void (*task_func_t)(task_que_t* q, task_t* task, void* params);

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>


#define TASK_PARAMS_MAX_SIZE 32


typedef enum {
	TASK_STATE_PENDING,
	TASK_STATE_RUNNING,
	TASK_STATE_SUCCESS,

	// failed for a reason that was predicted and
	// accounted for; if there's an error detail
	// param, it's valid and should be checked
	TASK_STATE_KNOWN_ERROR,

	// failed and internal state is likely borked;
	// do not even attempt to access any error param
	// the task might have
	TASK_STATE_UNKNOWN_ERROR,
	
	TASK_STATE_SHOULD_CANCEL,
	TASK_STATE_CANCELLED
} task_state_t;





// #define TASK_ID_SHOULD_EXIT 1



task_que_t* task_que_create();
void        task_que_destroy(task_que_t* q);

task_t*     task_que_add_task(
	task_que_t* q,
	task_func_t func,
	void*       params,
	size_t      sizeof_params
);

bool        task_que_get_are_any_incomplete_tasks(task_que_t* q);

bool        task_que_get_is_task_complete(task_que_t* q, task_t* task);
void        task_que_delete_completed_task(task_que_t* q, task_t* task);

// atomically change state to TASK_STATE_CANCELLED if currently TASK_STATE_SHOULD_CANCEL
// returns whether this was done
bool        task_que_task_set_cancelled_if_should_cancel(task_que_t* q, task_t* task);

// `out` must point to at least `sizeof_params` bytes
void         task_que_task_copy_params(task_que_t* q, task_t* task, void* out);

task_state_t task_que_task_get_state(task_que_t* q, task_t* task);
void         task_que_task_set_state(task_que_t* q, task_t* task, task_state_t state);

int          task_que_task_get_progress(task_que_t* q, task_t* task);
void         task_que_task_set_progress(task_que_t* q, task_t* task, int progress);

// todo set should exit

