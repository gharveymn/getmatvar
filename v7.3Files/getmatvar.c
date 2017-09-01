#include "getmatvar.h"

void mexFunction(int nlhs, mxArray* plhs[], int nrhs, const mxArray* prhs[])
{
	if(nrhs < 2)
	{
		readMXError("getmatvar:invalidNumInputs", "At least two input arguments are required.\n\n");
	}
	else if(nlhs > 1)
	{
		readMXError("getmatvar:invalidNumOutputs", "Assignment cannot be made to more than one output.\n\n");
	}
	else
	{
		
		for (int i = 0; i < nrhs; i++)
		{
			if (mxGetClassID(prhs[i]) != mxCHAR_CLASS)
			{
				readMXError("getmatvar:invalidInputType", "All inputs must be character vectors\n\n");
			}
		}

		const char* filename = mxArrayToString(prhs[0]);

		if(strstr(filename, ".mat") == NULL)
		{
			readMXError("getmatvar:invalidFilename", "The filename must end with .mat\n\n");
		}

		const char** full_variable_names = malloc((nrhs - 1) * sizeof(char*));
		for(int i = 0; i < nrhs - 1; i++)
		{
			full_variable_names[i] = mxArrayToString(prhs[i + 1]);
		}

		makeReturnStructure(plhs, nrhs - 1, full_variable_names, filename);


		free(full_variable_names);

	}
	
}


void makeReturnStructure(mxArray* uberStructure[], const int num_elems, const char* full_variable_names[], const char* filename)
{

	Data** pseudo_object = malloc(sizeof(Data*));
	char** varnames = malloc(num_elems*sizeof(char*));
	char* last_delimit;

	for (mwIndex i = 0; i < num_elems; i++)
	{
		varnames[i] = malloc(NAME_LENGTH*sizeof(char));
		last_delimit = strrchr(full_variable_names[i], ".");
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
	uberStructure[0] = mxCreateStructArray(1, ret_struct_dims, num_elems, varnames);

	Data** objects;
	Data* hi_objects;

	for(mwIndex i = 0; i < num_elems; i++)
	{
		
		objects = getDataObjects(filename, full_variable_names[i]);
		hi_objects = organizeObjects(objects);
		pseudo_object[0] = hi_objects;
		makeSubstructure(uberStructure[0], 1, pseudo_object, STRUCT);
		freeMXDataObjects(objects);

	}

	free(pseudo_object);
	free(varnames);

}

mxArray* makeSubstructure(mxArray* returnStructure, const int num_elems, Data** objects, DataType super_structure_type)
{
	
	for(mwIndex index = 0; index < num_elems; index++)
	{
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
				break;
			case STRUCT:
				setStructPtr(objects[index], returnStructure, objects[index]->name, index, super_structure_type);
				break;
			case FUNCTION_HANDLE:
				//readMXWarn("getmatvar:invalidOutputType", "Could not return a variable. Function-handles are not yet supported (proprietary).");
			case TABLE:
				//readMXWarn("getmatvar:invalidOutputType", "Could not return a variable. Tables are not yet supported.");
			default:
				break;
		}
		
	}
	
	return returnStructure;
	
}


const char** getFieldNames(Data* object)
{
	const char** varnames = malloc(object->num_sub_objs * sizeof(char*));
	for(uint16_t index = 0; index < object->num_sub_objs; index++)
	{
		varnames[index] = object->sub_objects[index]->name;
	}
	return varnames;
}


mwSize* makeObjDims(const uint32_t* dims, const mwSize num_obj_dims)
{
	
	//ie. flip them around...
	
	mwSize* obj_dims = malloc(num_obj_dims);
	for(int i = 0; i < num_obj_dims; i++)
	{
		obj_dims[i] = (mwSize) dims[num_obj_dims - 1 - i];
	}
	return obj_dims;
}

void readMXError(const char error_id[], const char error_message[])
{
	char message_buffer[1000];
	strcpy(message_buffer, error_message);
	strcat(message_buffer, MATLAB_HELP_MESSAGE);
	mexErrMsgIdAndTxt(error_id, message_buffer);
}

void readMXWarn(const char warn_id[], const char warn_message[])
{
	char message_buffer[1000];
	strcpy(message_buffer, warn_message);
	strcat(message_buffer, MATLAB_WARN_MESSAGE);
	mexWarnMsgIdAndTxt(warn_id, warn_message);
}
