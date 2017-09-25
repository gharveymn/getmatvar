#include "../src/mapping.h"


typedef enum
{
	NOT_AN_ARGUMENT, THREAD_KWARG, MT_KWARG, SUPPRESS_WARN
} kwarg;

typedef struct
{
	int num_vars;
	char** full_variable_names;
	const char* filename;
} paramStruct;

void readInput(int nrhs, char* prhs[], paramStruct* parameters);
Queue* makeReturnStructure(int num_elems, char** full_variable_names, const char* filename);


int main(int argc, char* argv[])
{
	
	initialize();
	
	if(argc < 2)
	{
		readMXError("getmatvar:invalidNumInputs", "At least one input argument is required.\n\n");
	}
	else
	{
		
		paramStruct parameters;
		readInput(argc - 1, argv + 1, &parameters);
		
		Queue* error_objects = makeReturnStructure(parameters.num_vars, parameters.full_variable_names, parameters.filename);
		if(error_objects != NULL)
		{
			Data* front_object = peekQueue(error_objects, QUEUE_FRONT);
			char err_id[NAME_LENGTH], err_string[NAME_LENGTH];
			strcpy(err_id, front_object->name);
			strcpy(err_string, front_object->matlab_class);
			freeQueue(error_objects);
			readMXError(err_id, err_string);
		}
		
	}
	
}


void readInput(int nrhs, char* prhs[], paramStruct* parameters)
{
	
	char* input;
	parameters->filename = prhs[0];
	parameters->num_vars = 0;
	parameters->full_variable_names = malloc(((nrhs - 1) + 1)*sizeof(char*));
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
							for(int j = parameters->num_vars - 1; j >= 0; j--)
							{
								free(parameters->full_variable_names[j]);
							}
							free(parameters->full_variable_names);
							readMXError("getmatvar:invalidNumThreadsError", "Error in the number of threads requested.\n\n");
						}
					}
					
					char* endptr;
					long res = (int)strtol(input, &endptr, 10);
					num_threads_to_use = (int)res;
					if(endptr == input || ((res == LONG_MAX || res == LONG_MIN) && errno == ERANGE))
					{
						for(int j = parameters->num_vars - 1; j >= 0; j--)
						{
							free(parameters->full_variable_names[j]);
						}
						free(parameters->full_variable_names);
						readMXError("getmatvar:invalidNumThreadsError", "Error in the number of threads requested.\n\n");
					}
					
					if(num_threads_to_use < 0)
					{
						for(int j = parameters->num_vars - 1; j >= 0; j--)
						{
							free(parameters->full_variable_names[j]);
						}
						free(parameters->full_variable_names);
						readMXError("getmatvar:tooManyThreadsError", "Too many threads were requested.\n\n");
					}
					
					
					//TEMPORARY, REMOVE WHEN WE HAVE MT_KWARG WORKING
					if(num_threads_to_use == 0)
					{
						num_threads_to_use = -1;
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
						for(int j = parameters->num_vars - 1; j >= 0; j--)
						{
							free(parameters->full_variable_names[j]);
						}
						free(parameters->full_variable_names);
						readMXError("getmatvar:invalidMultithreadOption", "Multithreading argument options are: true, false, 1, 0, '1', '0', 't(rue)', 'f(alse)', 'on', or 'off'.\n\n");
					}
					break;
				case SUPPRESS_WARN:
					//this should not occur so fall through for debugging purposes
				case NOT_AN_ARGUMENT:
				default:
					for(int j = parameters->num_vars - 1; j >= 0; j--)
					{
						free(parameters->full_variable_names[j]);
					}
					free(parameters->full_variable_names);
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
				parameters->full_variable_names[parameters->num_vars] = malloc(strlen(input)*sizeof(char)); /*this gets freed in getDataObjects*/
				strcpy(parameters->full_variable_names[parameters->num_vars], input);
				parameters->num_vars++;
			}
			
		}
	}
	
	if(parameters->num_vars == 0)
	{
		free(parameters->full_variable_names);
		parameters->full_variable_names = malloc(2*sizeof(char*));
		parameters->full_variable_names[0] = "\0";
		parameters->num_vars = 1;
	}
	parameters->full_variable_names[parameters->num_vars] = NULL;
}


Queue* makeReturnStructure(const int num_elems, char** full_variable_names, const char* filename)
{
	
	Queue* objects = getDataObjects(filename, full_variable_names, num_elems);
	Data* front_object = peekQueue(objects, QUEUE_FRONT);
	if((ERROR_DATA & front_object->type) == ERROR_DATA)
	{
		return objects;
	}
	
	Data** super_objects = malloc((objects->length)*sizeof(Data*));
	char** varnames = malloc((objects->length)*sizeof(char*));
	int num_objs = 0;
	for(; objects->length > 0; num_objs++)
	{
		super_objects[num_objs] = organizeObjects(objects);
		if(super_objects[num_objs] == NULL)
		{
			break;
		}
		else
		{
			varnames[num_objs] = malloc(NAME_LENGTH*sizeof(char));
			strcpy(varnames[num_objs], super_objects[num_objs]->name);
		}
	}
	
	
	freeQueue(objects);
	free(super_objects);
	for(int i = 0; i < num_objs; i++)
	{
		free(varnames[i]);
	}
	free(varnames);
	
	fprintf(stderr, "\nProgram exited successfully.\n");
	
	return NULL;
	
}