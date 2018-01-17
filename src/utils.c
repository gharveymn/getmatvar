#include "headers/utils.h"
#include "headers/getDataObjects.h"


/*for use on a single level*/
Data* findSubObjectByShortName(Data* object, char* name)
{
	for(int i = 0; i < object->num_sub_objs; i++)
	{
		if(strcmp(object->sub_objects[i]->names.short_name, name) == 0)
		{
			return object->sub_objects[i];
		}
	}
	return NULL;
}

/*searches entire tree*/
Data* findObjectByHeaderAddress(address_t address)
{
	
	restartQueue(object_queue);
	do
	{
		Data* object = dequeue(object_queue);
		if(object->this_obj_address == address)
		{
			return object;
		}
	} while(object_queue->length > 0);
	
	return NULL;
	
}



uint16_t getRealNameLength(Data* object)
{
	uint16_t real_name_length = object->names.short_name_length;
	Data* curr = object->super_object;
	while(curr != NULL)
	{
		real_name_length += curr->names.short_name_length + 1;
		curr = curr->super_object;
	}
	return real_name_length;
}


void nullFreeFunction(void* param)
{
	//do nothing
}


void readMXError(const char error_id[], const char error_message[], ...)
{
	
	char message_buffer[ERROR_BUFFER_SIZE];
	
	va_list va;
	va_start(va, error_message);
	sprintf(message_buffer, error_message, va);
	strcat(message_buffer, MATLAB_HELP_MESSAGE);
	endHooks();
	va_end(va);

#ifdef NO_MEX
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-security"
	fprintf(stderr, message_buffer);
#pragma GCC diagnostic pop
	exit(1);
#else
	mexErrMsgIdAndTxt(error_id, message_buffer);
#endif

}


void readMXWarn(const char warn_id[], const char warn_message[], ...)
{
	
	if(will_suppress_warnings != TRUE)
	{
		char message_buffer[WARNING_BUFFER_SIZE];
		
		va_list va;
		va_start(va, warn_message);
		sprintf(message_buffer, warn_message, va);
		strcat(message_buffer, MATLAB_WARN_MESSAGE);
		va_end(va);

#ifdef NO_MEX
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-security"
		fprintf(stderr, message_buffer);
#pragma GCC diagnostic pop
#else
		mexWarnMsgIdAndTxt(warn_id, message_buffer);
#endif
	
	}
	
}