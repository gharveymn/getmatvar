#ifndef GETMATVAR__H
#define GETMATVAR__H

#define mxSTRING_CLASS 19

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


//getmatvar.c
void readInput(int nrhs, const mxArray* prhs[], paramStruct* parameters);
Queue* makeReturnStructure(mxArray** uberStructure, const int num_elems, char** full_variable_names, const char* filename);
mxArray* makeSubstructure(mxArray* returnStructure, const int num_elems, Data** objects, DataType super_structure_type);

//mexPointerSetters.c
void setUI8Ptr(Data* object, mxArray* returnStructure, const char* varname, mwIndex index, DataType super_structure_type);
void setI8Ptr(Data* object, mxArray* returnStructure, const char* varname, mwIndex index, DataType super_structure_type);
void setUI16Ptr(Data* object, mxArray* returnStructure, const char* varname, mwIndex index, DataType super_structure_type);
void setI16Ptr(Data* object, mxArray* returnStructure, const char* varname, mwIndex index, DataType super_structure_type);
void setUI32Ptr(Data* object, mxArray* returnStructure, const char* varname, mwIndex index, DataType super_structure_type);
void setI32Ptr(Data* object, mxArray* returnStructure, const char* varname, mwIndex index, DataType super_structure_type);
void setUI64Ptr(Data* object, mxArray* returnStructure, const char* varname, mwIndex index, DataType super_structure_type);
void setI64Ptr(Data* object, mxArray* returnStructure, const char* varname, mwIndex index, DataType super_structure_type);
void setSglPtr(Data* object, mxArray* returnStructure, const char* varname, mwIndex index, DataType super_structure_type);
void setDblPtr(Data* object, mxArray* returnStructure, const char* varname, mwIndex index, DataType super_structure_type);
void setFHPtr(Data* object, mxArray* returnStructure, const char* varname, mwIndex index, DataType super_structure_type);
void setCellPtr(Data* object, mxArray* returnStructure, const char* varname, mwIndex index, DataType super_structure_type);
void setStructPtr(Data* object, mxArray* returnStructure, const char* varname, mwIndex index, DataType super_structure_type);
mwSize* makeObjDims(const uint32_t* dims, const mwSize num_dims);
const char** getFieldNames(Data* object);
DataArrays rearrangeImaginaryData(Data* object);

#endif