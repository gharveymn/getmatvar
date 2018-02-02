#ifndef EZQ_H
#define EZQ_H

#define QUEUE_FRONT 0
#define QUEUE_BACK 1

#include <stdio.h>
#include <stdlib.h>

#ifdef NO_MEX
#define mxMalloc malloc
#define mxFree free
#define mxCalloc calloc
#define mxRealloc realloc
#else
#include <mex.h>
#define malloc mxMalloc
#define free mxFree
#define calloc mxCalloc
#define realloc mxRealloc
#endif

typedef int error_t;

typedef struct QueueNode_ QueueNode;
struct QueueNode_
{
	QueueNode* prev;
	QueueNode* next;
	void* data;
};

typedef struct
{
	QueueNode* abs_front; /* make it the queue's job to free all objects and data queued at the end */
	QueueNode* front;
	QueueNode* back;
	QueueNode* traverse_front;
	void (* free_function)(void*);
	size_t length;
	size_t abs_length;
} Queue;


Queue* initQueue(void (* free_function)(void*));
error_t enqueue(Queue* queue, void* data);
error_t priorityEnqueue(Queue* queue, void* data);
void* dequeue(Queue* queue);
void* peekQueue(Queue* queue, int queue_location);
error_t mergeQueue(Queue* new_queue, Queue** queues, size_t num_queues);
error_t flushQueue(Queue* queue);
void freeQueue(Queue* queue);
error_t resetQueue(Queue* queue);
error_t initTraversal(Queue* queue);
error_t initAbsTraversal(Queue* queue);
void* removeAtTraverseNode(Queue* queue);
void* peekTraverse(Queue* queue);
void* traverseQueue(Queue* queue);
error_t restartQueue(Queue* queue);
error_t cleanQueue(Queue* queue);

#endif //EZQ_H
