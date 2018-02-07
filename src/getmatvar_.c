#include "headers/getDataObjects.h"


void mexFunction(int nlhs, mxArray* plhs[], int nrhs, const mxArray* prhs[])
{
	
	initialize();
	mexAtExit(endHooks);
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
		makeReturnStructure(plhs);
		for(int i = 0; i < parameters.num_vars; i++)
		{
			mxFree(parameters.full_variable_names[i]);
		}
		mxFree(parameters.full_variable_names);
		parameters.full_variable_names = NULL;
		mxFree(parameters.filename);
		parameters.filename = NULL;
		
		mexAtExit(nullFunction);
		fprintf(stderr, "\nProgram exited successfully.\n");
	}
	
}


void makeReturnStructure(mxArray** super_structure)
{
	mwSize ret_struct_dims[1] = {1};
	
	if(getDataObjects(parameters.filename, parameters.full_variable_names, parameters.num_vars) != 0)
	{
		readMXError(error_id, error_message);
	}
	char** field_names = getFieldNames(virtual_super_object);

	super_structure[0] = mxCreateStructArray(1, ret_struct_dims, virtual_super_object->num_sub_objs, (const char**)field_names);
	
	makeSubstructure(super_structure[0], virtual_super_object->num_sub_objs, virtual_super_object->sub_objects, mxSTRUCT_CLASS);
	
	mxFree(field_names);
	
	freeQueue(object_queue);
	object_queue = NULL;
	
}


mxArray* makeSubstructure(mxArray* returnStructure, const int num_elems, Queue* objects, mxClassID super_structure_type)
{
	
	if(num_elems == 0)
	{
		return NULL;
	}
	
	initTraversal(objects);
	if(((Data*)peekTraverse(objects))->data_flags.is_struct_array == TRUE)
	{
		//struct arrays are spoofed as cell arrays in getmatvar
		while(objects->traverse_length > 0)
		{
			Data* obj = traverseQueue(objects);
			makeSubstructure(returnStructure, obj->num_sub_objs, obj->sub_objects, mxSTRUCT_CLASS);
		}
	}
	else
	{
		
		while(objects->traverse_length > 0)
		{
			
			Data* obj = traverseQueue(objects);
			
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
					setNumericPtr(obj, returnStructure, obj->names.short_name, (mwIndex)obj->s_c_array_index, super_structure_type);
					break;
				case mxSPARSE_CLASS:
					setSpsPtr(obj, returnStructure, obj->names.short_name, (mwIndex)obj->s_c_array_index, super_structure_type);
					break;
				case mxLOGICAL_CLASS:
					setLogicPtr(obj, returnStructure, obj->names.short_name, (mwIndex)obj->s_c_array_index, super_structure_type);
					break;
				case mxCHAR_CLASS:
					setCharPtr(obj, returnStructure, obj->names.short_name, (mwIndex)obj->s_c_array_index, super_structure_type);
					break;
				case mxCELL_CLASS:
					setCellPtr(obj, returnStructure, obj->names.short_name, (mwIndex)obj->s_c_array_index, super_structure_type);
					//Indicate we should free any memory used by this
					obj->data_flags.is_mx_used = FALSE;
					break;
				case mxSTRUCT_CLASS:
					setStructPtr(obj, returnStructure, obj->names.short_name, (mwIndex)obj->s_c_array_index, super_structure_type);
					obj->data_flags.is_mx_used = FALSE;
					break;
				case mxFUNCTION_CLASS:
				case mxOBJECT_CLASS:
				case mxOPAQUE_CLASS:
					if(warnedObjectVar == FALSE)
					{
						//run only once
						readMXWarn("getmatvar:invalidOutputType", "Could not return variable '%s'. Objects are not yet supported.", obj->names.long_name);
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
	parameters.full_variable_names = mxMalloc(nrhs*sizeof(char*));
	if(parameters.full_variable_names == NULL)
	{
		readMXError("getmatvar:mallocErrFullVarNames","Memory allocation failed. Your system may be out of memory.\n\n");
	}
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
					
					if(mxIsNumeric(prhs[i]) && (mxGetNumberOfElements(prhs[i]) == 1))
					{
						num_threads_user_def = (int)mxGetScalar(prhs[i]);
					}
					else if(mxIsChar(prhs[i]))
					{
						
						input = mxArrayToString(prhs[i]);
						
						//verify all chars are numeric
						for(size_t k = 0; k < mxGetNumberOfElements(prhs[i]); k++)
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
					readMXError("getmatvar:invalidArgument", "Variable names and keyword identifiers must have non-zero length.\n\n");
				}
				else if(*input == '-')
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
					
					mxFree(input);
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
		mxFree(parameters.full_variable_names);
		parameters.full_variable_names = mxMalloc(2*sizeof(char*));
		parameters.full_variable_names[0] = mxMalloc(sizeof(char));
		parameters.full_variable_names[0][0] = '\0';
		parameters.num_vars = 1;
	}
	parameters.full_variable_names[parameters.num_vars] = NULL;
}
