#ifndef EZQ_H
#define EZQ_H

#define QUEUE_FRONT 0
#define QUEUE_BACK 1

#include <stdio.h>
#include <stdlib.h>

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
errno_t enqueue(Queue* queue, void* data);
errno_t priorityEnqueue(Queue* queue, void* data);
void* dequeue(Queue* queue);
void* peekQueue(Queue* queue, int queue_location);
errno_t mergeQueue(Queue* new_queue, Queue** queues, size_t num_queues);
errno_t flushQueue(Queue* queue);
void freeQueue(Queue* queue);
errno_t resetQueue(Queue* queue);
errno_t initTraversal(Queue* queue);
errno_t initAbsTraversal(Queue* queue);
void* removeAtTraverseNode(Queue* queue);
void* peekTraverse(Queue* queue);
void* traverseQueue(Queue* queue);
errno_t restartQueue(Queue* queue);
errno_t cleanQueue(Queue* queue);

#endif //EZQ_H
