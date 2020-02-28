#include "task_que.h"

// #ifdef __STDC_NO_THREADS__
#include "../../third_party/tinycthread/tinycthread.h"
// #endif

#include <stdlib.h>
#include <string.h> // memcpy
#include <stdio.h>

#define MAX_TASKS 32

static int _task_queue_thread(void* _q);

struct _task {
	task_func_t  func;
	int32_t      progress; // [0,100]
	task_state_t state;
	bool         is_complete;
	// 32 bytes of inout data
	uint8_t      params[TASK_PARAMS_MAX_SIZE] __attribute__((aligned(16)));
	size_t       sizeof_params;
};

struct _task_que {
	thrd_t thread;
	mtx_t  mutex;
	cnd_t  cond;
	task_t tasks[MAX_TASKS];
	bool   tasks_empty[MAX_TASKS];
	bool   should_quit;
};

static void _task_que_init(task_que_t* q) {
	mtx_init(&q->mutex, mtx_plain);
	cnd_init(&q->cond);

	for (int i = 0; i < MAX_TASKS; i++) {
		q->tasks_empty[i] = true;
	}

	q->should_quit = false;

	thrd_create(&q->thread, &_task_queue_thread, (void*)q);
}

static void _task_que_deinit(task_que_t* q) {
	mtx_lock(&q->mutex);
	q->should_quit = true;
	mtx_unlock(&q->mutex);

	cnd_broadcast(&q->cond);
	thrd_join(q->thread, NULL);

	mtx_destroy(&q->mutex);
	cnd_destroy(&q->cond);
}

task_que_t* task_que_create() {
	task_que_t* q = malloc(sizeof(task_que_t));
	if (!q) return NULL;
	_task_que_init(q);
	return q;
}

void task_que_destroy(task_que_t* q) {
	_task_que_deinit(q);
	free(q);
}

static task_t* _find_pending_task(task_que_t* q) {
	task_t* result = NULL;

	for (int i = 0; i < MAX_TASKS; i++) {
		if (!q->tasks_empty[i]) {
			task_t* task = q->tasks + i;
			if (task->state == TASK_STATE_PENDING) {
				result = task;
			}
		}
	}

	return result;
}

static task_t* _find_empty_slot(task_que_t* q) {
	for (int i = 0; i < MAX_TASKS; i++) {
		if (q->tasks_empty[i]) {
			q->tasks_empty[i] = false;
			return q->tasks + i;
		}
	}

	return NULL;
}

static int _task_queue_thread(void* _q) {
	task_que_t* q = (task_que_t*) _q;

	task_t* task = NULL;
	while (1) {
		mtx_lock(&q->mutex);

		if (q->should_quit) {
			mtx_unlock(&q->mutex);
			break;
		}

		task = _find_pending_task(q);
		if (!task) {
			cnd_wait(&q->cond, &q->mutex);
		}
		task = _find_pending_task(q);
		if (!task) {
			// Still nothing.
			// Spurious wakeup? Or maybe should_quit has been set?
			mtx_unlock(&q->mutex);
			continue;
		}

		task->state = TASK_STATE_RUNNING;
		task->progress = 0;

		mtx_unlock(&q->mutex);

		task->func(q, task, (void*)task->params);

		mtx_lock(&q->mutex);

		task->is_complete = true;

		if ((task->state == TASK_STATE_RUNNING) || (task->state == TASK_STATE_PENDING)) {
			// That's a problem- should have been set to
			// _SUCCESS/ERROR/CANCELLED by now.
			task->state = TASK_STATE_UNKNOWN_ERROR;
		}

		if (task->state == TASK_STATE_SHOULD_CANCEL) {
			// Similarly
			task->state = TASK_STATE_CANCELLED;
		}

		if (task->state == TASK_STATE_SUCCESS) {
			task->progress = 100;
		}

		mtx_unlock(&q->mutex);
	}

	return 0;
}

task_t*     task_que_add_task(
	task_que_t* q,
	task_func_t func,
	void*       params,
	size_t      sizeof_params
) {
	if (sizeof_params > TASK_PARAMS_MAX_SIZE) {
		return NULL;
	}

	mtx_lock(&q->mutex);

	task_t* task = _find_empty_slot(q);
	if (!task) {
		mtx_unlock(&q->mutex);
		return NULL;
	}

	task->func = func;
	task->progress = 0;
	task->state = TASK_STATE_PENDING;
	if (sizeof_params > 0) {
		memcpy(task->params, params, sizeof_params);
	}
	task->sizeof_params = sizeof_params;
	task->is_complete = false;

	mtx_unlock(&q->mutex);
	cnd_broadcast(&q->cond);

	return task;
}

bool task_que_get_is_task_complete(task_que_t* q, task_t* task) {
	bool result;
	mtx_lock(&q->mutex);
	result = task->is_complete;
	mtx_unlock(&q->mutex);
	return result;
}

bool task_que_get_are_any_incomplete_tasks(task_que_t* q) {
	bool result = false;
	mtx_lock(&q->mutex);

	for (int i = 0; i < MAX_TASKS; i++) {
		if (!q->tasks_empty[i]) {
			task_t* task = q->tasks + i;
			if (!task->is_complete) {
				result = true;
			}
		}
	}

	mtx_unlock(&q->mutex);
	return result;
}

bool task_que_task_set_cancelled_if_should_cancel(task_que_t* q, task_t* task) {
	bool result = false;
	mtx_lock(&q->mutex);

	if (task->state == TASK_STATE_SHOULD_CANCEL) {
		task->state = TASK_STATE_CANCELLED;
		result = true;
	}

	mtx_unlock(&q->mutex);
	return result;
}

void task_que_task_copy_params(task_que_t* q, task_t* task, void* out) {
	mtx_lock(&q->mutex);
	memcpy(out, task->params, task->sizeof_params);
	mtx_unlock(&q->mutex);
}

task_state_t task_que_task_get_state(task_que_t* q, task_t* task) {
	task_state_t result;
	mtx_lock(&q->mutex);
	result = task->state;
	mtx_unlock(&q->mutex);
	return result;
}

void task_que_task_set_state(task_que_t* q, task_t* task, task_state_t state) {
	mtx_lock(&q->mutex);
	task->state = state;
	mtx_unlock(&q->mutex);
}

void task_que_delete_completed_task(task_que_t* q, task_t* task) {
	mtx_lock(&q->mutex);

	int task_index = -1;
	for (int i = 0; i < MAX_TASKS; i++) {
		if ((q->tasks + i) == task) {
			task_index = i;
		}
	}

	if (task_index != -1) {
		q->tasks_empty[task_index] = true;
	}

	mtx_unlock(&q->mutex);
}

int task_que_task_get_progress(task_que_t* q, task_t* task) {
	int result;
	mtx_lock(&q->mutex);
	result = task->progress;
	mtx_unlock(&q->mutex);
	return result;
}

void task_que_task_set_progress(task_que_t* q, task_t* task, int progress) {
	mtx_lock(&q->mutex);
	task->progress = progress;
	mtx_unlock(&q->mutex);
}

