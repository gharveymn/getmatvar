#include "mapping.h"


void mexFunction(int nlhs, mxArray* plhs[], int nrhs, const mxArray* prhs[])
{
	
	initialize();
	
	if(nrhs < 1)
	{
		readMXError("getmatvar:invalidNumInputs", "At least one input argument is required.\n\n");
	}
	else if(nlhs > 1)
	{
		readMXError("getmatvar:invalidNumOutputs", "Assignment cannot be made to more than two outputs.\n\n");
	}
	else
	{
		
		paramStruct parameters;
		readInput(nrhs, prhs, &parameters);
		
		Queue* error_objects = makeReturnStructure(plhs, parameters.num_vars, parameters.full_variable_names, parameters.filename);
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


Queue* makeReturnStructure(mxArray** uberStructure, const int num_elems, char** full_variable_names, const char* filename)
{
	
	mwSize ret_struct_dims[1] = {1};
	
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
	
	#pragma GCC diagnostic push
	#pragma GCC diagnostic ignored "-Wincompatible-pointer-types"
	uberStructure[0] = mxCreateStructArray(1, ret_struct_dims, num_objs, varnames);
	#pragma GCC diagnostic pop
	
	makeSubstructure(uberStructure[0], num_objs, super_objects, STRUCT_DATA);
	
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


mxArray* makeSubstructure(mxArray* returnStructure, const int num_elems, Data** objects, DataType super_structure_type)
{
	
	for(mwIndex index = 0; index < num_elems; index++)
	{
		
		objects[index]->data_arrays.is_mx_used = TRUE;
		
		switch(objects[index]->type)
		{
			case UINT8_DATA:
				setUI8Ptr(objects[index], returnStructure, objects[index]->name, index, super_structure_type);
				break;
			case INT8_DATA:
				setI8Ptr(objects[index], returnStructure, objects[index]->name, index, super_structure_type);
				break;
			case UINT16_DATA:
				setUI16Ptr(objects[index], returnStructure, objects[index]->name, index, super_structure_type);
				break;
			case INT16_DATA:
				setI16Ptr(objects[index], returnStructure, objects[index]->name, index, super_structure_type);
				break;
			case UINT32_DATA:
				setUI32Ptr(objects[index], returnStructure, objects[index]->name, index, super_structure_type);
				break;
			case INT32_DATA:
				setI32Ptr(objects[index], returnStructure, objects[index]->name, index, super_structure_type);
				break;
			case UINT64_DATA:
				setUI64Ptr(objects[index], returnStructure, objects[index]->name, index, super_structure_type);
				break;
			case INT64_DATA:
				setI64Ptr(objects[index], returnStructure, objects[index]->name, index, super_structure_type);
				break;
			case SINGLE_DATA:
				setSglPtr(objects[index], returnStructure, objects[index]->name, index, super_structure_type);
				break;
			case DOUBLE_DATA:
				setDblPtr(objects[index], returnStructure, objects[index]->name, index, super_structure_type);
				break;
			case REF_DATA:
				setCellPtr(objects[index], returnStructure, objects[index]->name, index, super_structure_type);
				//Indicate we should free any memory used by this
				objects[index]->data_arrays.is_mx_used = FALSE;
				break;
			case STRUCT_DATA:
				setStructPtr(objects[index], returnStructure, objects[index]->name, index, super_structure_type);
				objects[index]->data_arrays.is_mx_used = FALSE;
				break;
			case FUNCTION_HANDLE_DATA:
				//setUI16Ptr(objects[index], returnStructure, objects[index]->name, index, super_structure_type);
				
				/*remove this section when system is ready*/
				readMXWarn("getmatvar:invalidOutputType", "Could not return a variable. Function handles are not yet supported.");
				mxRemoveField(returnStructure, mxGetFieldNumber(returnStructure, objects[index]->name));
				objects[index]->data_arrays.is_mx_used = FALSE;
				/*remove this section when system is ready*/
				
				break;
			case TABLE_DATA:
				readMXWarn("getmatvar:invalidOutputType", "Could not return a variable. Tables are not yet supported.");
				mxRemoveField(returnStructure, mxGetFieldNumber(returnStructure, objects[index]->name));
				objects[index]->data_arrays.is_mx_used = FALSE;
				break;
			case NULLTYPE_DATA:
				//do nothing, this is an empty array
				break;
			case UNDEF_DATA:
				//in this case we want to actually remove the whole thing because it is triggered by the object not being found, so fall through
			default:
				mxRemoveField(returnStructure, mxGetFieldNumber(returnStructure, objects[index]->name));
				objects[index]->data_arrays.is_mx_used = FALSE;
				break;
		}
		
	}
	
	return returnStructure;
	
}


void readInput(int nrhs, const mxArray* prhs[], paramStruct* parameters)
{
	if(mxGetClassID(prhs[0]) != mxCHAR_CLASS)
	{
		readMXError("getmatvar:invalidFileNameType", "The file name must be a character vector\n\n");
	}
	
	char* input;
	parameters->filename = mxArrayToString(prhs[0]);
	parameters->num_vars = 0;
	parameters->full_variable_names = malloc(((nrhs - 1) + 1)*sizeof(char*));
	kwarg kwarg_expected = NOT_AN_ARGUMENT;
	bool_t kwarg_flag = FALSE;
	for(int i = 1; i < nrhs; i++)
	{
		
		if(mxGetClassID(prhs[i]) == mxSTRING_CLASS)
		{
			for(int j = parameters->num_vars - 1; j >= 0; j--)
			{
				free(parameters->full_variable_names[j]);
			}
			free(parameters->full_variable_names);
			readMXError("getmatvar:invalidInputType", "This function does not support strings.\n\n");
		}
		
		if(kwarg_flag == TRUE)
		{
			switch(kwarg_expected)
			{
				case THREAD_KWARG:
					
					//this overrides the max setting
					
					if(mxIsNumeric(prhs[i]) && mxIsScalar(prhs[i]))
					{
						num_threads_to_use = (int)mxGetScalar(prhs[i]);
					}
					else if(mxIsChar(prhs[i]))
					{
						
						input = mxArrayToString(prhs[i]);
						
						//verify all chars are numeric
						for(int k = 0; k < mxGetNumberOfElements(prhs[i]); k++)
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
					if(mxIsLogical(prhs[i]) == TRUE)
					{
						will_multithread = *mxGetLogicals(prhs[i]);
					}
					else if(mxIsNumeric(prhs[i]) == TRUE)
					{
						will_multithread = *(bool_t*)mxGetData(prhs[i]);
					}
					else if(mxIsChar(prhs[i]) == TRUE)
					{
						input = mxArrayToString(prhs[i]);
						if(strncmp(input, "f",1) == 0 || strcmp(input, "off") == 0 || strcmp(input, "\x48") == 0)
						{
							will_multithread = FALSE;
						}
						else if(strncmp(input, "t", 1) == 0 || strcmp(input, "on") == 0 || strcmp(input, "\x31") == 0)
						{
							will_multithread = TRUE;
						}
						else if(strncmp(input, "max", 3) == 0)
						{
							//overridden by the threads setting, so if still -1 don't execute
							if(num_threads_to_use == -1)
							{
								num_threads_to_use = getNumProcessors();
							}
						}
						else
						{
							goto mt_error;
						}
					}
					else
					{
					mt_error:
						for(int j = parameters->num_vars - 1; j >= 0; j--)
						{
							free(parameters->full_variable_names[j]);
						}
						free(parameters->full_variable_names);
						readMXError("getmatvar:invalidMultithreadOption", "Multithreading argument options are: true, false, 1, 0, '1', '0', 't(rue)', 'f(alse)', 'on', or 'off'.\n\n");
					}
					
					if(will_multithread != FALSE && will_multithread != TRUE)
					{
						goto mt_error;
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
			if(mxIsChar(prhs[i]))
			{
				input = mxArrayToString(prhs[i]);
				if(strncmp(input, "-", 1) == 0)
				{
					kwarg_flag = TRUE;
					if(strcmp(input, "-t") == 0)
					{
						kwarg_expected = THREAD_KWARG;
					}
					else if(strcmp(input, "-m") == 0)
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
			else
			{
				for(int j = parameters->num_vars - 1; j >= 0; j--)
				{
					free(parameters->full_variable_names[j]);
				}
				free(parameters->full_variable_names);
				readMXError("getmatvar:invalidArgument", "Variable names and keyword identifiers must be character vectors.\n\n");
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