#ifndef EZQ_H
#define EZQ_H

#define QUEUE_FRONT 0
#define QUEUE_BACK 1

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
	size_t total_length;
} Queue;


Queue* initQueue(void (* free_function)(void*));
void enqueue(Queue* queue, void* data);
void priorityEnqueue(Queue* queue, void* data);
void* dequeue(Queue* queue);
void* peekQueue(Queue* queue, int queue_location);
void mergeQueue(Queue* new_queue, Queue** queues, size_t num_queues);
void flushQueue(Queue* queue);
void freeQueue(Queue* queue);
void resetQueue(Queue* queue);
void initTraversal(Queue* queue);
void* traverseQueue(Queue* queue);
void restartQueue(Queue* queue);
void cleanQueue(Queue* queue);
void _nullFreeFunction(void*);

#endif //EZQ_H
