#include "mapping.h"


void enqueueTrio(Addr_Trio trio)
{
	if(queue.length >= MAX_Q_LENGTH)
	{
		fprintf(stderr, "Not enough room in trio queue\n");
		exit(EXIT_FAILURE);
	}
	queue.trios[queue.back].parent_obj_header_address = trio.parent_obj_header_address;
	queue.trios[queue.back].tree_address = trio.tree_address;
	queue.trios[queue.back].heap_address = trio.heap_address;
	queue.length++;
	
	if(queue.back < MAX_Q_LENGTH - 1)
	{
		queue.back++;
	}
	else
	{
		queue.back = 0;
	}
}

void enqueueObject(Object obj)
{
	if(header_queue.length >= MAX_Q_LENGTH)
	{
		fprintf(stderr, "Not enough room in header queue\n");
		exit(EXIT_FAILURE);
	}
	header_queue.objects[header_queue.back].parent_obj_header_address = obj.parent_obj_header_address;
	header_queue.objects[header_queue.back].this_obj_header_address = obj.this_obj_header_address;
	header_queue.objects[header_queue.back].name_offset = obj.name_offset;
	strcpy(header_queue.objects[header_queue.back].name, obj.name);
	header_queue.objects[header_queue.back].sub_tree_address = obj.sub_tree_address;
	header_queue.objects[header_queue.back].this_tree_address = obj.this_tree_address;
	header_queue.objects[header_queue.back].parent_tree_address = obj.parent_tree_address;
	header_queue.length++;
	
	if(header_queue.back < MAX_Q_LENGTH - 1)
	{
		header_queue.back++;
	}
	else
	{
		header_queue.back = 0;
	}
}


void enqueueVariableName(char* variable_name)
{
	variable_name_queue.variable_names[variable_name_queue.back] = variable_name;
	variable_name_queue.length++;
	variable_name_queue.back++;
}


void priorityEnqueueTrio(Addr_Trio trio)
{
	if(queue.length >= MAX_Q_LENGTH)
	{
		fprintf(stderr, "Trying to priority enqueue: Not enough room in trio queue\n");
		exit(EXIT_FAILURE);
	}
	if(queue.front - 1 < 0)
	{
		queue.trios[MAX_Q_LENGTH - 1].parent_obj_header_address = trio.parent_obj_header_address;
		queue.trios[MAX_Q_LENGTH - 1].tree_address = trio.tree_address;
		queue.trios[MAX_Q_LENGTH - 1].heap_address = trio.heap_address;
		queue.front = MAX_Q_LENGTH - 1;
	}
	else
	{
		queue.trios[queue.front - 1].parent_obj_header_address = trio.parent_obj_header_address;
		queue.trios[queue.front - 1].tree_address = trio.tree_address;
		queue.trios[queue.front - 1].heap_address = trio.heap_address;
		queue.front--;
	}
	queue.length++;
}


void priorityEnqueueObject(Object obj)
{
	if(header_queue.length >= MAX_Q_LENGTH)
	{
		fprintf(stderr, "Trying to priority enqueue: Not enough room in header queue\n");
		exit(EXIT_FAILURE);
	}
	if(header_queue.front - 1 < 0)
	{
		header_queue.objects[MAX_Q_LENGTH - 1].name_offset = obj.name_offset;
		header_queue.objects[MAX_Q_LENGTH - 1].parent_obj_header_address = obj.parent_obj_header_address;
		header_queue.objects[MAX_Q_LENGTH - 1].this_obj_header_address = obj.this_obj_header_address;
		strcpy(header_queue.objects[MAX_Q_LENGTH - 1].name, obj.name);
		header_queue.objects[MAX_Q_LENGTH - 1].sub_tree_address = obj.sub_tree_address;
		header_queue.objects[MAX_Q_LENGTH - 1].this_tree_address = obj.this_tree_address;
		header_queue.objects[MAX_Q_LENGTH - 1].parent_tree_address = obj.parent_tree_address;
		header_queue.front = MAX_Q_LENGTH - 1;
	}
	else
	{
		header_queue.objects[header_queue.front - 1].name_offset = obj.name_offset;
		header_queue.objects[header_queue.front - 1].parent_obj_header_address = obj.parent_obj_header_address;
		header_queue.objects[header_queue.front - 1].this_obj_header_address = obj.this_obj_header_address;
		strcpy(header_queue.objects[header_queue.front - 1].name, obj.name);
		header_queue.objects[header_queue.front - 1].sub_tree_address = obj.sub_tree_address;
		header_queue.objects[header_queue.front - 1].this_tree_address = obj.this_tree_address;
		header_queue.objects[header_queue.front - 1].parent_tree_address = obj.parent_tree_address;
		header_queue.front--;
	}
	header_queue.length++;
}


Addr_Trio dequeueTrio()
{
	Addr_Trio trio;
	trio.parent_obj_header_address = queue.trios[queue.front].parent_obj_header_address;
	trio.tree_address = queue.trios[queue.front].tree_address;
	trio.heap_address = queue.trios[queue.front].heap_address;
	if(queue.front + 1 < MAX_Q_LENGTH)
	{
		queue.front++;
	}
	else
	{
		queue.front = 0;
	}
	queue.length--;
	return trio;
}


Object dequeueObject()
{
	Object obj;
	obj.name_offset = header_queue.objects[header_queue.front].name_offset;
	obj.parent_obj_header_address = header_queue.objects[header_queue.front].parent_obj_header_address;
	obj.this_obj_header_address = header_queue.objects[header_queue.front].this_obj_header_address;
	strcpy(obj.name, header_queue.objects[header_queue.front].name);
	obj.sub_tree_address = header_queue.objects[header_queue.front].sub_tree_address;
	obj.parent_tree_address = header_queue.objects[header_queue.front].parent_tree_address;
	obj.this_tree_address = header_queue.objects[header_queue.front].this_tree_address;
	if(header_queue.front + 1 < MAX_Q_LENGTH)
	{
		header_queue.front++;
	}
	else
	{
		header_queue.front = 0;
	}
	header_queue.length--;
	return obj;
}


char* dequeueVariableName()
{
	char* variable_name = variable_name_queue.variable_names[variable_name_queue.front];
	variable_name_queue.front++;
	variable_name_queue.length--;
	return variable_name;
}


char* peekVariableName()
{
	return variable_name_queue.variable_names[variable_name_queue.front];
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


void flushVariableNameQueue()
{
	variable_name_queue.length = 0;
	variable_name_queue.front = 0;
	variable_name_queue.back = 0;
}
