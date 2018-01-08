#include "../src/headers/getDataObjects.h"


typedef enum
{
	NOT_AN_ARGUMENT, THREAD_KWARG, MT_KWARG, SUPPRESS_WARN
} kwarg;

void readInput(int nrhs, char* prhs[]);
void makeReturnStructure(int num_elems);
void makeEvalArray(void);

int main(int argc, char* argv[])
{
	
	initialize();
	
	if(argc < 2)
	{
		readMXError("getmatvar:invalidNumInputs", "At least one input argument is required.\n\n");
	}
	else
	{
		
		readInput(argc - 1, argv + 1);
		makeReturnStructure(parameters.num_vars);
		for(int i = 0; i < parameters.num_vars; i++)
		{
			free(parameters.full_variable_names[i]);
		}
		free(parameters.full_variable_names);
		
	}
	
}


void makeReturnStructure(const int num_elems)
{
	object_queue = initQueue(freeDataObject);
	eval_objects = initQueue(NULL);
	
	getDataObjects(parameters.filename, parameters.full_variable_names, parameters.num_vars);
	if(error_flag == TRUE)
	{
		readMXError(error_id, error_message);
	}
	char** varnames = malloc((virtual_super_object->num_sub_objs)*sizeof(char*));
	for(int i = 0; i < virtual_super_object->num_sub_objs; i++)
	{
		varnames[i] = malloc((virtual_super_object->sub_objects[i]->names.short_name_length + 1)*sizeof(char));
		strcpy(varnames[i], virtual_super_object->sub_objects[i]->names.short_name);
	}
	
	for(int i = 0; i < virtual_super_object->num_sub_objs; i++)
	{
		free(varnames[i]);
	}
	free(varnames);
	
	//enqueue(eval_objects, virtual_super_object->sub_objects[0]);
	makeEvalArray();
	
	freeQueue(eval_objects);
	freeQueue(object_queue);
	
	fprintf(stderr, "\nProgram exited successfully.\n\n");
	
}


void readInput(int nrhs, char* prhs[])
{
	
	char* input;
	parameters.filename = prhs[0];
	parameters.num_vars = 0;
	parameters.full_variable_names = malloc(((nrhs - 1) + 1)*sizeof(char*));
	kwarg kwarg_expected = NOT_AN_ARGUMENT;
	bool_t kwarg_flag = FALSE;
	for(int i = 1; i < nrhs; i++)
	{
		
		if(kwarg_flag == TRUE)
		{
			switch(kwarg_expected)
			{
				case THREAD_KWARG:
					
					input = prhs[i];
					
					//verify all chars are numeric
					for(int k = 0; k < strlen(prhs[i]); k++)
					{
						if((input[k] - '0') > 9 || (input[k] - '0') < 0)
						{
							readMXError("getmatvar:invalidNumThreadsError", "Error in the number of threads requested.\n\n");
						}
					}
					
					char* endptr;
					long res = (int)strtol(input, &endptr, 10);
					num_threads_user_def = (int)res;
					if(endptr == input && errno == ERANGE)
					{
						readMXError("getmatvar:invalidNumThreadsError", "Error in the number of threads requested.\n\n");
					}
					
					if(num_threads_user_def < 0)
					{
						readMXError("getmatvar:tooManyThreadsError", "Too many threads were requested.\n\n");
					}
					
					
					//TEMPORARY, REMOVE WHEN WE HAVE MT_KWARG WORKING
					if(num_threads_user_def == 0)
					{
						num_threads_user_def = -1;
					}
					
					break;
				case MT_KWARG:
					
					input = prhs[i];
					if(strncmp(input, "f", 1) == 0 || strcmp(input, "off") == 0 || strcmp(input, "\x30") == 0)
					{
						will_multithread = FALSE;
					}
					else if(strncmp(input, "t", 1) == 0 || strcmp(input, "on") == 0 || strcmp(input, "\x31") == 0)
					{
						will_multithread = TRUE;
					}
					else
					{
						readMXError("getmatvar:invalidMultithreadOption", "Multithreading argument options are: true, false, 1, 0, '1', '0', 't(rue)', 'f(alse)', 'on', or 'off'.\n\n");
					}
					break;
				case SUPPRESS_WARN:
					//this should not occur so fall through for debugging purposes
				case NOT_AN_ARGUMENT:
				default:
					readMXError("getmatvar:notAnArgument", "The specified keyword argument does not exist.\n\n");
			}
			kwarg_expected = NOT_AN_ARGUMENT;
			kwarg_flag = FALSE;
		}
		else
		{
			input = prhs[i];
			if(strncmp(input, "-", 1) == 0)
			{
				kwarg_flag = TRUE;
				if(strncmp(input, "-t", 2) == 0)
				{
					kwarg_expected = THREAD_KWARG;
				}
				else if(strncmp(input, "-m", 2) == 0)
				{
					kwarg_expected = MT_KWARG;
				}
				else if(strcmp(input, "-suppress-warnings") == 0 || strcmp(input, "-sw") == 0)
				{
					will_suppress_warnings = TRUE;
					kwarg_flag = FALSE;
				}
			}
			else
			{
				kwarg_expected = NOT_AN_ARGUMENT;
				parameters.full_variable_names[parameters.num_vars] = malloc((strlen(input) + 1)*sizeof(char));
				strcpy(parameters.full_variable_names[parameters.num_vars], input);
				parameters.num_vars++;
			}
			
		}
	}
	
	if(parameters.num_vars == 0)
	{
		free(parameters.full_variable_names);
		parameters.full_variable_names = malloc(2*sizeof(char*));
		parameters.full_variable_names[0] = malloc(sizeof(char));
		parameters.full_variable_names[0][0] = '\0';
		parameters.num_vars = 1;
	}
	parameters.full_variable_names[parameters.num_vars] = NULL;
}

void makeEvalArray(void)
{
	uint32_t total_length = 0;
	Data* eval_obj;
	while(eval_objects->length > 0)
	{
		eval_obj = dequeue(eval_objects);
		//long_name=expression;
		uint16_t real_name_length = getRealNameLength(eval_obj);
		memmove(eval_obj->names.long_name,
			   eval_obj->names.long_name + eval_obj->names.long_name_length - real_name_length,
			   real_name_length+1);
		
		eval_obj->names.long_name_length = real_name_length;
		
		total_length += eval_obj->names.long_name_length + 1 + eval_obj->num_elems + 1;
	}
	
	char* eval_vector = malloc(total_length * sizeof(uint16_t));
	
	uint32_t offset = 0;
	restartQueue(eval_objects);
	while(eval_objects->length > 0)
	{
		eval_obj = dequeue(eval_objects);
		//long_name=expression;
		for(int i = 0; i < eval_obj->names.long_name_length; i++, offset++)
		{
			memcpy(&eval_vector[offset], &eval_obj->names.long_name[i], sizeof(uint8_t));
		}
		eval_vector[offset] = '=';
		offset++;
		memcpy(&eval_vector[offset], eval_obj->data_arrays.data, eval_obj->num_elems * sizeof(uint16_t));
		offset += eval_obj->num_elems;
		eval_vector[offset] = ';';
		offset++;
	}
	
	free(eval_vector);
	
}