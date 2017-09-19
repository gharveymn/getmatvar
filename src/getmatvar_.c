#include "getmatvar_.h"

typedef enum
{
	NOT_AN_ARGUMENT,
	THREAD_KWARG
} kwarg;

void mexFunction(int nlhs, mxArray* plhs[], int nrhs, const mxArray* prhs[])
{
	
	//init maps, needed so we don't confuse ending hooks in the case of error
	initializeMaps();
	is_multithreading = FALSE;
	addr_queue = NULL;
	varname_queue = NULL;
	header_queue = NULL;
	fd = -1;
	num_threads_to_use = -1;
	
	if(nrhs < 1)
	{
		readMXError("getmatvar:invalidNumInputs", "At least one input argument is required.\n\n");
	}
	else if(nlhs > 1)
	{
		readMXError("getmatvar:invalidNumOutputs", "Assignment cannot be made to more than one output.\n\n");
	}
	else
	{
		if(mxGetClassID(prhs[0]) != mxCHAR_CLASS)
		{
			readMXError("getmatvar:invalidFileNameType", "The file name must be a character vector\n\n");
		}
		const char* filename = mxArrayToString(prhs[0]);

		if(strstr(filename, ".mat") == NULL)
		{
			readMXError("getmatvar:invalidFilename", "The filename must end with .mat\n\n");
		}
		
		char** full_variable_names;
		int num_vars = 0;
		full_variable_names = malloc(((nrhs - 1) + 1) * sizeof(char*));
		kwarg kwarg_expected = NOT_AN_ARGUMENT;
		bool_t kwarg_flag = FALSE;
		for (int i = 0; i < nrhs - 1; i++)
		{
			if(kwarg_flag == TRUE)
			{
				switch(kwarg_expected)
				{
				case THREAD_KWARG:
					num_threads_to_use = MIN((int)mxGetScalar(prhs[i + 1]), NUM_TREE_MAPS);
					break;
				case NOT_AN_ARGUMENT:
				default:
					for(int j = num_vars - 1; j >= 0; j--)
					{
						free(full_variable_names[j]);
					}
					free(full_variable_names);
					readMXError("getmatvar:notAnArgument", "The specified keyword argument does not exist.\n\n");

				}
				kwarg_expected = NOT_AN_ARGUMENT;
				kwarg_flag = FALSE;
			}
			else
			{
				if(mxGetClassID(prhs[i + 1]) == mxCHAR_CLASS)
				{
					char* vn = mxArrayToString(prhs[i + 1]);
					if(strncmp(vn, "-", 1) == 0)
					{
						kwarg_flag = TRUE;
						if(strcmp(vn, "-threads") == 0)
						{
							kwarg_expected = THREAD_KWARG;
						}
					}
					else
					{
						kwarg_expected = NOT_AN_ARGUMENT;
						full_variable_names[num_vars] = malloc(strlen(vn) * sizeof(char)); /*this gets freed in getDataObjects*/
						strcpy(full_variable_names[num_vars], vn);
						num_vars++;
					}
				}
				else
				{
					for(int j = num_vars - 1; j >= 0; j--)
					{
						free(full_variable_names[j]);
					}
					free(full_variable_names);
					readMXError("getmatvar:invalidArgument", "Variable names and keyword identifiers must be character vectors.\n\n");
				}
			}
		}
		
		if(num_vars == 0)
		{
			free(full_variable_names);
			full_variable_names = malloc(2*sizeof(char*));
			full_variable_names[0] = "\0";
			num_vars = 1;
		}
		full_variable_names[num_vars] = NULL;
		
		Queue* error_objects = makeReturnStructure(plhs, num_vars, full_variable_names, filename);
		if(error_objects != NULL)
		{
			Data* front_object = peekQueue(error_objects, QUEUE_FRONT);
			char err_id[NAME_LENGTH], err_string[NAME_LENGTH];
			strcpy(err_id,front_object->name);
			strcpy(err_string,front_object->matlab_class);
			free(full_variable_names);
			freeQueue(error_objects);
			readMXError(err_id, err_string);
		}


		free(full_variable_names);

	}
	
}


Queue* makeReturnStructure(mxArray** uberStructure, const int num_elems, char** full_variable_names, const char* filename)
{

	mwSize ret_struct_dims[1] = {1};
	
	fprintf(stderr,"Fetching the objects... ");
	Queue* objects = getDataObjects(filename, full_variable_names, num_elems);
	Data* front_object = peekQueue(objects, QUEUE_FRONT);
	if((ERROR_DATA & front_object->type) == ERROR_DATA)
	{
		return objects;
	}
	fprintf(stderr,"success.\n");
	
	Data** super_objects = malloc((objects->length)*sizeof(Data*));
	char** varnames = malloc((objects->length)*sizeof(char*));
	int num_objs = 0;
	for (;objects->length  > 0; num_objs++)
	{
		super_objects[num_objs] = organizeObjects(objects);
		if (super_objects[num_objs] == NULL)
		{
			break;
		}
		else
		{
			varnames[num_objs] = malloc(NAME_LENGTH*sizeof(char));
			strcpy(varnames[num_objs], super_objects[num_objs]->name);
		}
	}

	//#pragma GCC diagnostic push
	//#pragma GCC diagnostic ignored "-Wincompatible-pointer-types"
	uberStructure[0] = mxCreateStructArray(1, ret_struct_dims, num_objs, varnames);
	//#pragma GCC diagnostic pop
	
	makeSubstructure(uberStructure[0], num_objs, super_objects, STRUCT_DATA);
	
	freeQueue(objects);
	free(super_objects);
	for (int i = 0; i < num_objs; i++)
	{
		free(varnames[i]);
	}
	free(varnames);

	fprintf(stderr,"\nProgram exited successfully.\n");
	
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
				readMXWarn("getmatvar:invalidOutputType", "Could not return a variable. Function-handles are not yet supported (proprietary).");
				mxRemoveField(returnStructure, mxGetFieldNumber(returnStructure, objects[index]->name));
				objects[index]->data_arrays.is_mx_used = FALSE;
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

void readMXError(const char error_id[], const char error_message[], ...)
{
	
	char message_buffer[ERROR_BUFFER_SIZE];

	va_list va;
	va_start(va, error_message);
	sprintf(message_buffer, error_message, va);
	strcat(message_buffer, MATLAB_HELP_MESSAGE);
	endHooks();
	va_end(va);
	mexErrMsgIdAndTxt(error_id, message_buffer);
	
}

void readMXWarn(const char warn_id[], const char warn_message[], ...)
{
	char message_buffer[WARNING_BUFFER_SIZE];

	va_list va;
	va_start(va, warn_message);
	sprintf(message_buffer, warn_message, va);
	strcat(message_buffer, MATLAB_WARN_MESSAGE);
	va_end(va);
	mexWarnMsgIdAndTxt(warn_id, warn_message);
}