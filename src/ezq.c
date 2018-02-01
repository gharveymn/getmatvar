#include "headers/ezq.h"


Queue* initQueue(void (* free_function)(void*))
{
	Queue* new_queue = malloc(sizeof(Queue));
	new_queue->abs_front = NULL;
	new_queue->front = NULL;
	new_queue->back = NULL;
	new_queue->traverse_front = NULL;
	new_queue->length = 0;
	new_queue->abs_length = 0;
	new_queue->free_function = free_function;
	return new_queue;
}


void enqueue(Queue* queue, void* data)
{
	QueueNode* new_node = malloc(sizeof(QueueNode));
	new_node->data = data;
	if(queue->abs_length == 0)
	{
		new_node->next = NULL;
		new_node->prev = NULL;
		queue->abs_front = new_node;
		queue->front = new_node;
		queue->back = queue->front;
	}
	else
	{
		new_node->prev = queue->back;
		new_node->next = queue->back->next;
		
		if(queue->back->next != NULL)
		{
			queue->back->next->prev = new_node;
		}
		queue->back->next = new_node;
		queue->back = new_node;
		if(queue->length == 0)
		{
			queue->front = new_node;
		}
	}
	queue->length++;
	queue->abs_length++;
}


void priorityEnqueue(Queue* queue, void* data)
{
	QueueNode* new_node = malloc(sizeof(QueueNode));
	new_node->data = data;
	if(queue->abs_length == 0)
	{
		new_node->next = NULL;
		new_node->prev = NULL;
		queue->abs_front = new_node;
		queue->front = new_node;
		queue->back = queue->front;
	}
	else if(queue->length == 0)
	{
		//means the queue is reset, so just normal queue instead so we don't confuse the total length
		new_node->prev = queue->back;
		new_node->next = queue->back->next;
		if(queue->back->next != NULL)
		{
			queue->back->next->prev = new_node;
		}
		queue->back->next = new_node;
		queue->back = new_node;
		queue->front = new_node;
		
	}
	else
	{
		new_node->prev = queue->front->prev;
		new_node->next = queue->front;
		
		if(queue->front == queue->abs_front)
		{
			queue->abs_front = new_node;
		}
		else
		{
			queue->front->prev->next = new_node;
		}
		
		queue->front->prev = new_node;
		queue->front = new_node;
		
	}
	queue->length++;
	queue->abs_length++;
}


void resetQueue(Queue* queue)
{
	queue->front = queue->back;
	queue->length = 0;
}

void restartQueue(Queue* queue)
{
	queue->front = queue->abs_front;
	queue->length = queue->abs_length;
}


void* dequeue(Queue* queue)
{
	if(queue->front != NULL && queue->length > 0)
	{
		void* to_return = queue->front->data;
		QueueNode* new_front = queue->front->next;
		queue->front = new_front;
		queue->length--;
		return to_return;
	}
	else
	{
		return NULL;
	}
}


void* peekQueue(Queue* queue, int queue_location)
{
	if(queue->front != NULL && queue->length > 0)
	{
		if(queue_location == QUEUE_FRONT)
		{
			return queue->front->data;
		}
		else if(queue_location == QUEUE_BACK)
		{
			return queue->back->data;
		}
		else
		{
			return NULL;
		}
	}
	else
	{
		return NULL;
	}
}


void mergeQueue(Queue* new_queue, Queue** queues, size_t num_queues)
{
	for(int i = 0; i < num_queues; i++)
	{
		size_t q_len = queues[i]->length;
		for(int j = 0; j < q_len; j++)
		{
			enqueue(new_queue, dequeue(queues[i]));
		}
		restartQueue(queues[i]);
	}
}


void flushQueue(Queue* queue)
{
	if(queue != NULL)
	{
		while(queue->abs_length > 0)
		{
			QueueNode* next = queue->abs_front->next;
			if(queue->abs_front->data != NULL && queue->free_function != NULL)
			{
				queue->free_function(queue->abs_front->data);
			}
			free(queue->abs_front);
			queue->abs_front = next;
			queue->abs_length--;
		}
		queue->abs_front = NULL;
		queue->traverse_front = NULL;
		queue->front = NULL;
		queue->back = NULL;
		queue->length = 0;
	}
}


void cleanQueue(Queue* queue)
{
	//move the absolute front to the same position as front and free up the queue objects along the way
	if(queue != NULL)
	{
		while(queue->abs_front != queue->front)
		{
			QueueNode* next = queue->abs_front->next;
			if(queue->abs_front->data != NULL)
			{
				queue->free_function(queue->abs_front->data);
			}
			queue->abs_front->prev = NULL;
			queue->abs_front->data = NULL;
			free(queue->abs_front);
			queue->abs_front = next;
			queue->abs_length--;
		}
	}
}


void initTraversal(Queue* queue)
{
	if(queue != NULL)
	{
		queue->traverse_front = queue->front;
	}
}


void initAbsTraversal(Queue* queue)
{
	if(queue != NULL)
	{
		queue->traverse_front = queue->abs_front;
	}
}


void* traverseQueue(Queue* queue)
{
	//give back the
	if(queue != NULL)
	{
		if(queue->traverse_front != NULL)
		{
			void* to_return = queue->traverse_front->data;
			QueueNode* new_front = queue->traverse_front->next;
			queue->traverse_front = new_front;
			return to_return;
		}
	}
	return NULL;
}

void* peekTraverse(Queue* queue)
{
	if(queue != NULL)
	{
		if(queue->traverse_front != NULL)
		{
			return queue->traverse_front->data;
		}
	}
	return NULL;
}

void* removeAtTraverseNode(Queue* queue)
{
	if(queue != NULL)
	{
		QueueNode* tf = queue->traverse_front;
		if(tf != NULL)
		{
			queue->traverse_front = tf->next;
			if(tf->prev != NULL)
			{
				tf->prev->next = tf->next;
			}
			
			if(tf->next != NULL)
			{
				tf->next->prev = tf->prev;
			}
			
			if(tf == queue->front)
			{
				queue->front = tf->next;
			}
			
			if(tf == queue->abs_front)
			{
				queue->abs_front = tf->next;
			}
			
			queue->abs_length--;
			if(queue->abs_length < queue->length)
			{
				queue->length--;
			}
			queue->free_function(tf->data);
			free(tf);
			if(queue->traverse_front != NULL)
			{
				return queue->traverse_front->data;
			}
		}
	}
	return NULL;
}


void freeQueue(Queue* queue)
{
	if(queue != NULL)
	{
		flushQueue(queue);
		free(queue);
	}
}
