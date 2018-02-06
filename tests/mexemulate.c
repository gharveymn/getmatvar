#include "../src/headers/getDataObjects.h"


typedef enum
{
	NOT_AN_ARGUMENT, THREAD_KWARG, MT_KWARG, SUPPRESS_WARN
} kwarg;

void readInput(int nrhs, char* prhs[]);
void makeReturnStructure(void);

int main(int argc, char* argv[])
{
	
	initialize();
	
	if(argc < 2)
	{
		readMXError("getmatvar:invalidNumInputs", "At least one input argument is required.\n\n");
	}
	else
	{
//		for(int j = 0; j < 1000; j++)
//		{
			readInput(argc - 1, argv + 1);
			makeReturnStructure();
			for(int i = 0; i < parameters.num_vars; i++)
			{
				mxFree(parameters.full_variable_names[i]);
			}
			mxFree(parameters.full_variable_names);
			mxFree(parameters.filename);
//		}

		fprintf(stderr, "\nProgram exited successfully.\n\n");
		
	}

}


void makeReturnStructure(void)
{
	
	if(getDataObjects(parameters.filename, parameters.full_variable_names, parameters.num_vars) != 0)
	{
		readMXError(error_id, error_message);
	}
	char** varnames = mxMalloc((virtual_super_object->num_sub_objs)*sizeof(char*));
	if(varnames == NULL)
	{
		readMXError("getmatvar:mallocErrMRSME","Memory allocation failed. Your system may be out of memory.\n\n");
		exit(1);
	}
	for(uint32_t i = 0; i < virtual_super_object->num_sub_objs; i++)
	{
		Data* obj = dequeue(virtual_super_object->sub_objects);
		varnames[i] = mxMalloc((obj->names.short_name_length + 1)*sizeof(char));
		if(varnames[i] == NULL)
		{
			for(uint32_t j = 0; j < i - 1; j++)
			{
				mxFree(varnames[i]);
			}
			mxFree(varnames);
			readMXError("getmatvar:mallocErrMRSVNI","Memory allocation failed. Your system may be out of memory.\n\n");
			exit(1);
		}
		strcpy(varnames[i], obj->names.short_name);
	}
	restartQueue(virtual_super_object->sub_objects);
	
	for(uint32_t i = 0; i < virtual_super_object->num_sub_objs; i++)
	{
		mxFree(varnames[i]);
	}
	mxFree(varnames);
	
	//enqueue(eval_objects, virtual_super_object->sub_objects[0]);
	//makeEvalArray();
	
	freeQueue(object_queue);
	
}


void readInput(int nrhs, char* prhs[])
{
	
	char* input;
	parameters.filename = mxMalloc((strlen(prhs[0]) + 1)*sizeof(char));
	strcpy(parameters.filename, prhs[0]);
	parameters.num_vars = 0;
	parameters.full_variable_names = mxMalloc(((nrhs - 1) + 1)*sizeof(char*));
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
			if(*input == 0)
			{
				readMXError("getmatvar:invalidArgument", "Variable names and keyword identifiers must have non-zero length.\n\n");
			}
			if(strncmp(input, "-", 1) == 0)
			{
				kwarg_flag = TRUE;
				if(strcmp(input, "-t") == 0 && strlen(input) == 2)
				{
					kwarg_expected = THREAD_KWARG;
				}
				else if((strcmp(input, "-suppress-warnings") == 0 && strlen(input) == strlen("-suppress-warnings")) || (strcmp(input, "-sw") == 0  && strlen(input) ==3))
				{
					will_suppress_warnings = TRUE;
					kwarg_flag = FALSE;
				}
				else if(strcmp(input, "-st") == 0 && strlen(input) == 3)
				{
					will_multithread = FALSE;
					kwarg_flag = FALSE;
				}
				else
				{
					readMXError("getmatvar:notAnArgument", "The specified keyword argument does not exist.\n\n");
				}
			}
			else
			{
				kwarg_expected = NOT_AN_ARGUMENT;
				parameters.full_variable_names[parameters.num_vars] = mxMalloc((strlen(input) + 1)*sizeof(char));
				strcpy(parameters.full_variable_names[parameters.num_vars], input);
				parameters.num_vars++;
			}
			
		}
	}
	
	if(parameters.num_vars == 0)
	{
		mxFree(parameters.full_variable_names);
		parameters.full_variable_names = mxMalloc(2*sizeof(char*));
		parameters.full_variable_names[0] = mxMalloc(sizeof(char));
		parameters.full_variable_names[0][0] = '\0';
		parameters.num_vars = 1;
	}
	parameters.full_variable_names[parameters.num_vars] = NULL;
}