#include "mapping.h"

void enqueuePair(Addr_Pair pair)
{
	if (queue.length >= MAX_Q_LENGTH)
	{
		printf("Not enough room in pair queue\n");
		exit(EXIT_FAILURE);
	}
	queue.pairs[queue.back].tree_address = pair.tree_address;
	queue.pairs[queue.back].heap_address = pair.heap_address;
	queue.length++;

	if (queue.back < MAX_Q_LENGTH - 1)
	{
		queue.back++;
	}
	else
	{
		queue.back = 0;
	}
}
void enqueueAddress(uint64_t address)
{
	if (header_queue.length >= MAX_Q_LENGTH)
	{
		printf("Not enough room in header queue\n");
		exit(EXIT_FAILURE);
	}
	header_queue.header_addresses[header_queue.back] = address;
	header_queue.length++;

	if (header_queue.back < MAX_Q_LENGTH - 1)
	{
		header_queue.back++;
	}
	else
	{
		header_queue.back = 0;
	}
}
void priorityEnqueuePair(Addr_Pair pair)
{
	if (queue.length >= MAX_Q_LENGTH)
	{
		printf("Trying to priority enqueue: Not enough room in pair queue\n");
		exit(EXIT_FAILURE);
	}
	if (queue.front - 1 < 0)
	{
		queue.pairs[MAX_Q_LENGTH - 1].tree_address = pair.tree_address;
		queue.pairs[MAX_Q_LENGTH - 1].heap_address = pair.heap_address;
		queue.front = MAX_Q_LENGTH - 1;
	}
	else
	{
		queue.pairs[queue.front - 1].tree_address = pair.tree_address;
		queue.pairs[queue.front - 1].heap_address = pair.heap_address;
		queue.front--;
	}
	queue.length++;
}
void priorityEnqueueAddress(uint64_t address)
{
	if (header_queue.length >= MAX_Q_LENGTH)
	{
		printf("Trying to priority enqueue: Not enough room in header queue\n");
		exit(EXIT_FAILURE);
	}
	if (header_queue.front - 1 < 0)
	{
		header_queue.header_addresses[MAX_Q_LENGTH - 1] = address;
		header_queue.front = MAX_Q_LENGTH - 1;
	}
	else
	{
		header_queue.header_addresses[header_queue.front - 1] = address;
		header_queue.front--;
	}
	header_queue.length++;
}
Addr_Pair dequeuePair()
{
	Addr_Pair pair;
	pair.tree_address = queue.pairs[queue.front].tree_address;
	pair.heap_address = queue.pairs[queue.front].heap_address;
	if (queue.front + 1 < MAX_Q_LENGTH)
	{
		queue.front++;
	}
	else
	{
		queue.front = 0;
	}
	queue.length--;
	return pair;
}
uint64_t dequeueAddress()
{
	uint64_t address;
	address = header_queue.header_addresses[header_queue.front];
	if (header_queue.front + 1 < MAX_Q_LENGTH)
	{
		header_queue.front++;
	}
	else
	{
		header_queue.front = 0;
	}
	header_queue.length--;
	return address;
}
void flushQueue()
{
	queue.length = 0;
	queue.front = 0;
	queue.back = 0;
}
void flushHeaderQueue()
{
	header_queue.length = 0;
	header_queue.front = 0;
	header_queue.back = 0;
}
