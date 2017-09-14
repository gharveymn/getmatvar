#include "getmatvar_.h"

void mexFunction(int nlhs, mxArray* plhs[], int nrhs, const mxArray* prhs[])
{

	if(nrhs < 2)
	{
		readMXError("getmatvar:invalidNumInputs", "At least two input arguments are required.\n\n", "");
	}
	else if(nlhs > 1)
	{
		readMXError("getmatvar:invalidNumOutputs", "Assignment cannot be made to more than one output.\n\n", "");
	}
	else
	{
		
		for (int i = 0; i < nrhs; i++)
		{
			if (mxGetClassID(prhs[i]) != mxCHAR_CLASS)
			{
				readMXError("getmatvar:invalidInputType", "All inputs must be character vectors\n\n", "");
			}
		}

		const char* filename = mxArrayToString(prhs[0]);

		if(strstr(filename, ".mat") == NULL)
		{
			readMXError("getmatvar:invalidFilename", "The filename must end with .mat\n\n", "");
		}

		const char** full_variable_names = malloc(((nrhs - 1) + 1) * sizeof(char*));
		for(int i = 0; i < nrhs - 1; i++)
		{
			full_variable_names[i] = mxArrayToString(prhs[i + 1]);
		}
		full_variable_names[nrhs-1] = NULL;

		Data** error_objects = makeReturnStructure(plhs, nrhs - 1, full_variable_names, filename);
		if(error_objects != NULL)
		{
			char* err_id = error_objects[0]->name;
			char* err_string = error_objects[0]->matlab_class;
			free(full_variable_names);
			freeDataObjects(error_objects);
			readMXError(err_id, err_string);
		}


		free(full_variable_names);

	}
	
}


Data** makeReturnStructure(mxArray* uberStructure[], const int num_elems, const char* full_variable_names[], const char* filename)
{
	
	char** varnames = malloc(num_elems*sizeof(char*));

	for (mwIndex i = 0; i < num_elems; i++)
	{
		varnames[i] = malloc(NAME_LENGTH*sizeof(char));
		char* last_delimit = strrchr(full_variable_names[i], '.');
		if (last_delimit == NULL)
		{
			strcpy(varnames[i], full_variable_names[i]);
		}
		else
		{
			strcpy(varnames[i], last_delimit + 1);
		}
	}

	mwSize ret_struct_dims[1] = { 1 };

	#pragma GCC diagnostic push
	#pragma GCC diagnostic ignored "-Wincompatible-pointer-types"
	uberStructure[0] = mxCreateStructArray(1, ret_struct_dims, num_elems, varnames);
	#pragma GCC diagnostic pop
	
	fprintf(stderr,"Fetching the objects... ");
	Data** objects = getDataObjects(filename, full_variable_names, num_elems);
	if((ERROR & objects[0]->type) == ERROR)
	{
		for(mwIndex i = 0; i < num_elems; i++)
		{
			free(varnames[i]);
		}
		free(varnames);
		return objects;
	}
	fprintf(stderr,"success.\n");
	
	Data** pseudo_object = malloc(sizeof(Data*));
	
	int starting_pos = 0;
	for(mwIndex i = 0; i < num_elems; i++)
	{
		fprintf(stderr,"Organizing... ");
		pseudo_object[0] = organizeObjects(objects, &starting_pos);
		fprintf(stderr,"success.\n");
		fprintf(stderr,"Creating the mx structure... ");
		makeSubstructure(uberStructure[0], 1, pseudo_object, STRUCT);
		fprintf(stderr,"success.\n");
		free(varnames[i]);
	}
	
	fprintf(stderr,"Freeing the data objects... ");
	freeDataObjects(objects);
	fprintf(stderr,"success.\n");

	free(pseudo_object);
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
			case UINT8:
				setUI8Ptr(objects[index], returnStructure, objects[index]->name, index, super_structure_type);
				break;
			case INT8:
				setI8Ptr(objects[index], returnStructure, objects[index]->name, index, super_structure_type);
				break;
			case UINT16:
				setUI16Ptr(objects[index], returnStructure, objects[index]->name, index, super_structure_type);
				break;
			case INT16:
				setI16Ptr(objects[index], returnStructure, objects[index]->name, index, super_structure_type);
				break;
			case UINT32:
				setUI32Ptr(objects[index], returnStructure, objects[index]->name, index, super_structure_type);
				break;
			case INT32:
				setI32Ptr(objects[index], returnStructure, objects[index]->name, index, super_structure_type);
				break;
			case UINT64:
				setUI64Ptr(objects[index], returnStructure, objects[index]->name, index, super_structure_type);
				break;
			case INT64:
				setI64Ptr(objects[index], returnStructure, objects[index]->name, index, super_structure_type);
				break;
			case SINGLE:
				setSglPtr(objects[index], returnStructure, objects[index]->name, index, super_structure_type);
				break;
			case DOUBLE:
				setDblPtr(objects[index], returnStructure, objects[index]->name, index, super_structure_type);
				break;
			case REF:
				setCellPtr(objects[index], returnStructure, objects[index]->name, index, super_structure_type);
				//Indicate we should free any memory used by this
				objects[index]->data_arrays.is_mx_used = FALSE;
				break;
			case STRUCT:
				setStructPtr(objects[index], returnStructure, objects[index]->name, index, super_structure_type);
				objects[index]->data_arrays.is_mx_used = FALSE;
				break;
			case FUNCTION_HANDLE:
				readMXWarn("getmatvar:invalidOutputType", "Could not return a variable. Function-handles are not yet supported (proprietary).");
				mxRemoveField(returnStructure, mxGetFieldNumber(returnStructure, objects[index]->name));
				objects[index]->data_arrays.is_mx_used = FALSE;
				break;
			case TABLE:
				readMXWarn("getmatvar:invalidOutputType", "Could not return a variable. Tables are not yet supported.");
				mxRemoveField(returnStructure, mxGetFieldNumber(returnStructure, objects[index]->name));
				objects[index]->data_arrays.is_mx_used = FALSE;
				break;
			case NULLTYPE:
				//do nothing, this is an empty array
				break;
			case UNDEF:
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
	
	char message_buffer[1000];

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
	char message_buffer[1000];

	va_list va;
	va_start(va, warn_message);
	sprintf(message_buffer, warn_message, va);
	strcat(message_buffer, MATLAB_WARN_MESSAGE);
	va_end(va);
	mexWarnMsgIdAndTxt(warn_id, warn_message);
}