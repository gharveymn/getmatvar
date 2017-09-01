#include <mex.h>
#include "mapping.h"

#define MATLAB_HELP_MESSAGE "Usage:\n \tgetmatvar(filename,variable)\n" \
					"\tgetmatvar(filename,variable1,...,variableN)\n\n" \
					"\tfilename\t\ta character vector of the name of the file with a .mat extension\n" \
					"\tvariable\t\ta character vector of the variable to extract from the file\n\n" \
					"Example:\n\ts = getmatvar('my_workspace.mat', 'my_struct')"

#define MATLAB_WARN_MESSAGE ""

void makeReturnStructure(mxArray* uberStructure[], int num_elems, const char* varnames[], const char* filename);
mxArray* makeSubstructure(mxArray* returnStructure, int num_elems, Data** objects, DataType super_structure_type);

void setDblPtr(Data* object, mxArray* returnStructure, const char* varname, mwIndex index, DataType super_structure_type);
void setCharPtr(Data* object, mxArray* returnStructure, const char* varname, mwIndex index, DataType super_structure_type);
void setIntPtr(Data* object, mxArray* returnStructure, const char* varname, mwIndex index, DataType super_structure_type);
void setCellPtr(Data* object, mxArray* returnStructure, const char* varname, mwIndex index, DataType super_structure_type);
void setStructPtr(Data* object, mxArray* returnStructure, const char* varname, mwIndex index, DataType super_structure_type);

mwSize* makeObjDims(const uint32_t* dims, mwSize num_obj_dims);
void getNums(Data* object, mwSize* num_obj_dims, mwSize* num_elems);
const char** getFieldNames(Data* object);

void readMXError(const char error_id[], const char error_message[]);
void readMXWarn(const char warn_id[], const char warn_message[]);

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

		const char** varnames = malloc((nrhs - 1) * sizeof(char*));
		for(int i = 0; i < nrhs - 1; i++)
		{
			varnames[i] = mxArrayToString(prhs[i + 1]);
		}
		makeReturnStructure(plhs, nrhs - 1, varnames, filename);
		free(varnames);
		
	}
	
}


void makeReturnStructure(mxArray* uberStructure[], const int num_elems, const char* varnames[], const char* filename)
{
	mwSize ret_struct_dims[1] = {1};
	uberStructure[0] = mxCreateStructArray(1, ret_struct_dims, num_elems, varnames);
	DataType super_structure_type = STRUCT;
	
	for(mwIndex i = 0; i < num_elems; i++)
	{
		
		Data* hi_objects = findDataObject(filename, varnames[i]);
		
		int index = 0;
		while(hi_objects[index].type != UNDEF)
		{
			switch(hi_objects[index].type)
			{
				case DOUBLE:
					setDblPtr(&hi_objects[index], uberStructure[0], varnames[i], i, super_structure_type);
					break;
				case CHAR:
					setCharPtr(&hi_objects[index], uberStructure[0], varnames[i], i, super_structure_type);
					break;
				case UNSIGNEDINT16:
					setIntPtr(&hi_objects[index], uberStructure[0], varnames[i], i, super_structure_type);
					break;
				case REF:
					setCellPtr(&hi_objects[index], uberStructure[0], hi_objects[index].name, i, super_structure_type);
					break;
				case STRUCT:
					setStructPtr(&hi_objects[index], uberStructure[0], hi_objects[index].name, i, super_structure_type);
					break;
				default:
					break;
			}
			index++;
		}
		
		freeDataObjectTree(hi_objects);
		
	}
}

/*this will run only after one of the setXXXX methods*/
mxArray* makeSubstructure(mxArray* returnStructure, const int num_elems, Data** objects, DataType super_structure_type)
{
	
	for(mwIndex index = 0; index < num_elems; index++)
	{
		switch(objects[index]->type)
		{
			case DOUBLE:
				setDblPtr(objects[index], returnStructure, objects[index]->name, index, super_structure_type);
				break;
			case CHAR:
				setCharPtr(objects[index], returnStructure, objects[index]->name, index, super_structure_type);
				break;
			case UNSIGNEDINT16:
				setIntPtr(objects[index], returnStructure, objects[index]->name, index, super_structure_type);
				break;
			case REF:
				setCellPtr(objects[index], returnStructure, objects[index]->name, index, super_structure_type);
				break;
			case STRUCT:
				setStructPtr(objects[index], returnStructure, objects[index]->name, index, super_structure_type);
				break;
			default:
				break;
		}
		
	}
	
	return returnStructure;
	
}


void setDblPtr(Data* object, mxArray* returnStructure, const char* varname, mwIndex index, DataType super_structure_type)
{
	mwSize num_obj_dims = 0, num_obj_elems = 0;
	getNums(object, &num_obj_dims, &num_obj_elems);
	mwSize* obj_dims = makeObjDims(object->dims, num_obj_dims);
	mxArray* mxDblPtr = mxCreateNumericArray(num_obj_dims, obj_dims, mxDOUBLE_CLASS, mxREAL);
	double* mxDblPtrPr = mxGetPr(mxDblPtr);
	
	for(int j = 0; j < num_obj_elems; j++)
	{
		mxDblPtrPr[j] = object->double_data[j];
	}
	
	if(super_structure_type == STRUCT)
	{
		mxSetField(returnStructure, 0, varname, mxDblPtr);
	}
	else if(super_structure_type == REF)
	{
		//is a cell array
		mxSetCell(returnStructure, index, mxDblPtr);
	}
}


void setCharPtr(Data* object, mxArray* returnStructure, const char* varname, mwIndex index, DataType super_structure_type)
{
	mwSize num_obj_dims = 0, num_obj_elems = 0;
	getNums(object, &num_obj_dims, &num_obj_elems);
	char* mxCharPtrPr = mxCalloc(num_obj_elems, sizeof(char));
	
	for(int j = 0; j < num_obj_elems; j++)
	{
		mxCharPtrPr[j] = (char)object->ushort_data[j];
	}
	mxArray* mxCharPtr = mxCreateString(mxCharPtrPr);
	
	if(super_structure_type == STRUCT)
	{
		mxSetField(returnStructure, 0, varname, mxCharPtr);
	}
	else if(super_structure_type == REF)
	{
		//is a cell array
		mxSetCell(returnStructure, index, mxCharPtr);
	}
	mxFree(mxCharPtrPr);
}


void setIntPtr(Data* object, mxArray* returnStructure, const char* varname, mwIndex index, DataType super_structure_type)
{
	mwSize num_obj_dims = 0, num_obj_elems = 0;
	getNums(object, &num_obj_dims, &num_obj_elems);
	char* mxIntPtrPr = mxCalloc(num_obj_elems, sizeof(char));
	
	for(int j = 0; j < num_obj_elems; j++)
	{
		mxIntPtrPr[j] = (char)object->ushort_data[j];
	}
	mxArray* mxIntPtr = mxCreateString(mxIntPtrPr);
	
	if(super_structure_type == STRUCT)
	{
		mxSetField(returnStructure, 0, varname, mxIntPtr);
	}
	else if(super_structure_type == REF)
	{
		//is a cell array
		mxSetCell(returnStructure, index, mxIntPtr);
	}
	mxFree(mxIntPtrPr);
}

void setCellPtr(Data* object, mxArray* returnStructure, const char* varname, mwIndex index, DataType super_structure_type)
{
	
	DataType this_stucture_type = REF;
	
	mwSize num_obj_dims = 0, num_obj_elems = 0;
	getNums(object, &num_obj_dims, &num_obj_elems);
	mwSize* obj_dims = makeObjDims(object->dims, num_obj_dims);
	int num_fields = object->num_sub_objs;
	mxArray* mxCellPtr = mxCreateCellArray(num_obj_dims, obj_dims);
	
	if(super_structure_type == STRUCT)
	{
		mxSetField(returnStructure, 0, varname, makeSubstructure(mxCellPtr, num_fields, object->sub_objects, this_stucture_type));
	}
	else if(super_structure_type == REF)
	{
		mxSetCell(returnStructure, index, makeSubstructure(mxCellPtr, num_fields, object->sub_objects, this_stucture_type));
	}
	
}

void setStructPtr(Data* object, mxArray* returnStructure, const char* varname, mwIndex index, DataType super_structure_type)
{
	
	DataType this_stucture_type = STRUCT;
	
	mwSize num_obj_dims = 0, num_obj_elems = 0;
	getNums(object, &num_obj_dims, &num_obj_elems);
	mwSize* obj_dims = makeObjDims(object->dims, num_obj_dims);
	int num_fields = object->num_sub_objs;
	const char** field_names = getFieldNames(object);
	mxArray* mxStructPtr = mxCreateStructArray(num_obj_dims, obj_dims, num_fields, field_names);
	
	if(super_structure_type == STRUCT)
	{
		mxSetField(returnStructure, 0, varname, makeSubstructure(mxStructPtr, num_fields, object->sub_objects, this_stucture_type));
	}
	else if(super_structure_type == REF)
	{
		mxSetCell(returnStructure, index, makeSubstructure(mxStructPtr, num_fields, object->sub_objects, this_stucture_type));
	}

	free(field_names);
	
}


const char** getFieldNames(Data* object)
{
	const char** varnames = malloc(object->num_sub_objs * sizeof(char*));
	for(int index = 0; index < object->num_sub_objs; index++)
	{
		varnames[index] = object->sub_objects[index]->name;
	}
	return varnames;
}


void getNums(Data* object, mwSize* num_obj_dims, mwSize* num_elems)
{
	mwSize ne = 1;
	mwSize nd = 0;
	int i = 0;
	while(object->dims[i] > 0)
	{
		ne *= object->dims[i];
		nd++;
		i++;
	}
	*num_obj_dims = nd;
	*num_elems = ne;
}


mwSize* makeObjDims(const uint32_t* dims, const mwSize num_obj_dims)
{
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
	mexErrMsgIdAndTxt(warn_id, warn_message);
}
