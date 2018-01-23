#include "headers/getDataObjects.h"
#include "headers/ezq.h"


void mexFunction(int nlhs, mxArray* plhs[], int nrhs, const mxArray* prhs[])
{
	
	initialize();
	warnedObjectVar = FALSE;
	warnedUnknownVar = FALSE;
	
	if(nrhs < 1)
	{
		readMXError("getmatvar:invalidNumInputs", "At least one input argument is required.\n\n");
	}
	else if(nlhs > 2)
	{
		readMXError("getmatvar:invalidNumOutputs", "Assignment cannot be made to more than two outputs.\n\n");
	}
	else
	{
		readInput(nrhs, prhs);
		makeReturnStructure(plhs, nlhs);
		for(int i = 0; i < parameters.num_vars; i++)
		{
			mxFree(parameters.full_variable_names[i]);
		}
		free(parameters.full_variable_names);
		mxFree(parameters.filename);

		fprintf(stderr, "\nProgram exited successfully.\n");
	}
	
}


void makeReturnStructure(mxArray** super_structure, int nlhs)
{
	object_queue = initQueue(freeDataObject);
	
	mwSize ret_struct_dims[1] = {1};
	
	getDataObjects(parameters.filename, parameters.full_variable_names, parameters.num_vars);
	if(error_flag == TRUE)
	{
		readMXError(error_id, error_message);
	}
	char** field_names = getFieldNames(virtual_super_object);

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wincompatible-pointer-types"
	super_structure[0] = mxCreateStructArray(1, ret_struct_dims, virtual_super_object->num_sub_objs, field_names);
#pragma GCC diagnostic pop
	
	makeSubstructure(super_structure[0], virtual_super_object->num_sub_objs, virtual_super_object->sub_objects, mxSTRUCT_CLASS);
	
	free(field_names);
	
	for(Data* obj = dequeue(object_queue); object_queue->length > 0; obj = dequeue(object_queue))
	{
		freeQueue(obj->sub_objects);
		obj->sub_objects = NULL;
	}
	freeQueue(object_queue);
	object_queue = NULL;
	
}


mxArray* makeSubstructure(mxArray* returnStructure, const int num_elems, Queue* objects, mxClassID super_structure_type)
{
	
	if(num_elems == 0)
	{
		return NULL;
	}
	
	if(((Data*)peekQueue(objects, QUEUE_FRONT))->data_flags.is_struct_array == TRUE)
	{
		for(mwIndex index = 0; index < num_elems; index++)
		{
			Data* obj = dequeue(objects);
			makeSubstructure(returnStructure, obj->num_sub_objs, obj->sub_objects, mxSTRUCT_CLASS);
		}
		return returnStructure;
	}
	
	for(mwIndex index = 0; index < num_elems; index++)
	{
		
		Data* obj = dequeue(objects);
		
		obj->data_flags.is_mx_used = TRUE;
		
		switch(obj->matlab_internal_type)
		{
			case mxINT8_CLASS:
			case mxUINT8_CLASS:
			case mxINT16_CLASS:
			case mxUINT16_CLASS:
			case mxINT32_CLASS:
			case mxUINT32_CLASS:
			case mxINT64_CLASS:
			case mxUINT64_CLASS:
			case mxSINGLE_CLASS:
			case mxDOUBLE_CLASS:
				setNumericPtr(obj, returnStructure, obj->names.short_name, obj->s_c_array_index, super_structure_type);
				break;
			case mxSPARSE_CLASS:
				setSpsPtr(obj, returnStructure, obj->names.short_name, obj->s_c_array_index, super_structure_type);
				break;
			case mxLOGICAL_CLASS:
				setLogicPtr(obj, returnStructure, obj->names.short_name, obj->s_c_array_index, super_structure_type);
				break;
			case mxCHAR_CLASS:
				setCharPtr(obj, returnStructure, obj->names.short_name, obj->s_c_array_index, super_structure_type);
				break;
			case mxCELL_CLASS:
				setCellPtr(obj, returnStructure, obj->names.short_name, obj->s_c_array_index, super_structure_type);
				//Indicate we should free any memory used by this
				obj->data_flags.is_mx_used = FALSE;
				break;
			case mxSTRUCT_CLASS:
				setStructPtr(obj, returnStructure, obj->names.short_name, obj->s_c_array_index, super_structure_type);
				obj->data_flags.is_mx_used = FALSE;
				break;
			case mxFUNCTION_CLASS:
			case mxOBJECT_CLASS:
			case mxOPAQUE_CLASS:
				if(warnedObjectVar == FALSE)
				{
					//run only once
					readMXWarn("getmatvar:invalidOutputType", "Could not return a variable. Objects are not yet supported.");
					warnedObjectVar = TRUE;
				}
//				if (super_structure_type == mxSTRUCT_CLASS)
//				{
//					mxSetField(returnStructure, index, obj->names.short_name, NULL);
//				}
//				else if (super_structure_type == mxCELL_CLASS)
//				{
//					//is a cell array
//					mxSetCell(returnStructure, index, NULL);
//				}
				obj->data_flags.is_mx_used = FALSE;
				break;
			default:
//				if(warnedUnknownVar == FALSE)
//				{
//					//run only once
//					readMXWarn("getmatvar:unknownOutputType", "Could not return a variable. Unknown variable type.");
//					warnedUnknownVar = TRUE;
//				}
//				if (super_structure_type == mxSTRUCT_CLASS)
//				{
//					mxSetField(returnStructure, index, obj->names.short_name, NULL);
//				}
//				else if (super_structure_type == mxCELL_CLASS)
//				{
//					//is a cell array
//					mxSetCell(returnStructure, index, NULL);
//				}
				//this will happen for NULL objects
				obj->data_flags.is_mx_used = FALSE;
				break;
		}
		
	}
	
	return returnStructure;
	
}


void readInput(int nrhs, const mxArray* prhs[])
{
	if(mxGetClassID(prhs[0]) != mxCHAR_CLASS)
	{
		readMXError("getmatvar:invalidFileNameType", "The file name must be a character vector\n\n");
	}
	
	char* input = NULL;
	parameters.filename = mxArrayToString(prhs[0]);
	parameters.num_vars = 0;
	parameters.full_variable_names = malloc(nrhs*sizeof(char*));
	kwarg kwarg_expected = NOT_AN_ARGUMENT;
	bool_t kwarg_flag = FALSE;
	for(int i = 1; i < nrhs; i++)
	{
		
		if(mxGetClassID(prhs[i]) == mxSTRING_CLASS)
		{
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
						num_threads_user_def = (int)mxGetScalar(prhs[i]);
					}
					else if(mxIsChar(prhs[i]))
					{
						
						input = mxArrayToString(prhs[i]);
						
						//verify all chars are numeric
						for(int k = 0; k < mxGetNumberOfElements(prhs[i]); k++)
						{
							if((input[k] - '0') > 9 || (input[k] - '0') < 0)
							{
								readMXError("getmatvar:invalidNumThreadsError", "Error in the number of threads requested.\n\n");
							}
						}
						
						char* endptr;
						long res = (int)strtol(input, &endptr, 10);
						num_threads_user_def = (int)res;
						if(endptr == input || ((res == LONG_MAX || res == LONG_MIN) && errno == ERANGE))
						{
							readMXError("getmatvar:invalidNumThreadsError", "Error in the number of threads requested.\n\n");
						}
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
					if(mxIsLogical(prhs[i]) == TRUE)
					{
						will_multithread = *(bool_t*)mxGetLogicals(prhs[i]);
					}
					else if(mxIsNumeric(prhs[i]) == TRUE)
					{
						will_multithread = *(bool_t*)mxGetData(prhs[i]);
					}
					else if(mxIsChar(prhs[i]) == TRUE)
					{
						input = mxArrayToString(prhs[i]);
						if(strncmp(input, "f", 1) == 0 || strcmp(input, "off") == 0 || strcmp(input, "\x48") == 0)
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
							if(num_threads_user_def == -1)
							{
								num_threads_user_def = getNumProcessors();
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
				if(*input == 0)
				{
					mxFree(input);
					input = NULL;
					readMXError("getmatvar:invalidArgument", "Variable names and keyword identifiers must have non-zero length.\n\n");
				}
				else if(strncmp(input, "-", 1) == 0)
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
					else if(strcmp(input, "-st") == 0)
					{
						will_multithread = FALSE;
						kwarg_flag = FALSE;
					}
					
						mxFree(input);
						input = NULL;
				}
				else
				{
					kwarg_expected = NOT_AN_ARGUMENT;
					parameters.full_variable_names[parameters.num_vars] = input;
					parameters.num_vars++;
				}
			}
			else
			{
				readMXError("getmatvar:invalidArgument", "Variable names and keyword identifiers must be character vectors.\n\n");
			}
		}
		
		
	}
	
	if(parameters.num_vars == 0)
	{
		free(parameters.full_variable_names);
		parameters.full_variable_names = malloc(2*sizeof(char*));
		parameters.full_variable_names[0] = mxMalloc(sizeof(char));
		parameters.full_variable_names[0][0] = '\0';
		parameters.num_vars = 1;
	}
	parameters.full_variable_names[parameters.num_vars] = NULL;
}