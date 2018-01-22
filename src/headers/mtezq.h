#ifndef MTEZQ_H
#define MTEZQ_H

#include <pthread.h>
#include "ezq.h"

typedef struct
{
	QueueNode* abs_front; /* make it the queue's job to free all objects and data queued at the end */
	QueueNode* front;
	QueueNode* back;
	void (* free_function)(void*);
	size_t length;
	size_t total_length;
	pthread_mutex_t lock;
} MTQueue;


MTQueue* mt_initQueue(void (* free_function)(void*));
void mt_enqueue(MTQueue* queue, void* data);
void mt_priorityEnqueue(MTQueue* queue, void* data);
void* mt_dequeue(MTQueue* queue);
void* mt_peekQueue(MTQueue* queue, int queue_location);
void mt_mergeQueue(MTQueue* new_queue, Queue** queues, size_t num_queues);
void mt_mergeMTQueue(MTQueue* new_queue, MTQueue** queues, size_t num_queues);
void mt_flushQueue(MTQueue* queue);
void mt_freeQueue(MTQueue* queue);
void mt_resetQueue(MTQueue* queue);
void mt_restartQueue(MTQueue* queue);
void mt_cleanQueue(MTQueue* queue);
void _mt_nullFreeFunction(void*);

#endif //MTEZQ_H