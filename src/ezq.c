#include "headers/ezq.h"


Queue* initQueue(void (* free_function)(void*))
{
	Queue* new_queue = mxMalloc(sizeof(Queue));
	if(new_queue != NULL)
	{
		new_queue->abs_front = NULL;
		new_queue->front = NULL;
		new_queue->back = NULL;
		new_queue->traverse_front = NULL;
		new_queue->length = 0;
		new_queue->abs_length = 0;
		new_queue->free_function = free_function;
	}
	return new_queue;
}


error_t enqueue(Queue* queue, void* data)
{
	if(queue != NULL)
	{
		QueueNode* new_node = mxMalloc(sizeof(QueueNode));
#ifdef NO_MEX
		if(new_node == NULL)
		{
			
			return 1;
		}
#endif
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
		return 0;
	}
	else
	{
		return 2;
	}
}


error_t priorityEnqueue(Queue* queue, void* data)
{
	if(queue != NULL)
	{
		QueueNode* new_node = mxMalloc(sizeof(QueueNode));
#ifdef NO_MEX
		if(new_node == NULL)
		{
			
			return 1;
		}
#endif
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
		return 0;
	}
	else
	{
		return 2;
	}
}


error_t resetQueue(Queue* queue)
{
	if(queue != NULL)
	{
		queue->front = queue->back;
		queue->length = 0;
		return 0;
	}
	else
	{
		return 2;
	}
}


error_t restartQueue(Queue* queue)
{
	if(queue != NULL)
	{
		queue->front = queue->abs_front;
		queue->length = queue->abs_length;
		return 0;
	}
	else
	{
		return 2;
	}
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


error_t mergeQueue(Queue* new_queue, Queue** queues, size_t num_queues)
{
	if(new_queue != NULL)
	{
		for(size_t i = 0; i < num_queues; i++)
		{
			size_t q_len = queues[i]->length;
			for(size_t j = 0; j < q_len; j++)
			{
				enqueue(new_queue, dequeue(queues[i]));
			}
			restartQueue(queues[i]);
		}
		return 0;
	}
	else
	{
		return 2;
	}
}


error_t flushQueue(Queue* queue)
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
			mxFree(queue->abs_front);
			queue->abs_front = next;
			queue->abs_length--;
		}
		queue->abs_front = NULL;
		queue->traverse_front = NULL;
		queue->front = NULL;
		queue->back = NULL;
		queue->length = 0;
		return 0;
	}
	else
	{
		return 2;
	}
}


error_t cleanQueue(Queue* queue)
{
	//move the absolute front to the same position as front and mxFree up the queue objects along the way
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
			mxFree(queue->abs_front);
			queue->abs_front = next;
			queue->abs_length--;
		}
		return 0;
	}
	else
	{
		return 2;
	}
}


error_t initTraversal(Queue* queue)
{
	if(queue != NULL)
	{
		queue->traverse_front = queue->front;
		queue->traverse_length = queue->length;
		return 0;
	}
	else
	{
		return 2;
	}
}


error_t initAbsTraversal(Queue* queue)
{
	if(queue != NULL)
	{
		queue->traverse_front = queue->abs_front;
		queue->traverse_length = queue->abs_length;
		return 0;
	}
	else
	{
		return 2;
	}
}


void* traverseQueue(Queue* queue)
{
	if(queue != NULL)
	{
		if(queue->traverse_front != NULL)
		{
			void* to_return = queue->traverse_front->data;
			QueueNode* new_front = queue->traverse_front->next;
			queue->traverse_front = new_front;
			queue->traverse_length--;
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
			mxFree(tf);
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
		mxFree(queue);
	}
	//queue being NULL is not an error
}
