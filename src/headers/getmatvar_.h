#ifndef GET_MAT_VAR__H
#define GET_MAT_VAR__H

#define mxSTRING_CLASS 19

typedef enum
{
	NOT_AN_ARGUMENT, THREAD_KWARG, MT_KWARG, SUPPRESS_WARN
} kwarg;


//getmatvar.c
void readInput(int nrhs, const mxArray* prhs[]);
void makeReturnStructure(mxArray** super_structure, int nlhs);
mxArray* makeSubstructure(mxArray* returnStructure, const int num_elems, Queue* objects, mxClassID super_structure_type);
void makeEvalArray(mxArray** super_structure);

//mexPointerSetters.c
void setNumericPtr(Data* object, mxArray* returnStructure, const char* varname, mwIndex index, mxClassID super_structure_type);
void setLogicPtr(Data* object, mxArray* returnStructure, const char* varname, mwIndex index, mxClassID super_structure_type);
void setCharPtr(Data* object, mxArray* returnStructure, const char* varname, mwIndex index, mxClassID super_structure_type);
void setSpsPtr(Data* object, mxArray* returnStructure, const char* varname, mwIndex index, mxClassID super_structure_type);
void setCellPtr(Data* object, mxArray* returnStructure, const char* varname, mwIndex index, mxClassID super_structure_type);
void setStructPtr(Data* object, mxArray* returnStructure, const char* varname, mwIndex index, mxClassID super_structure_type);
mwSize* makeObjDims(const uint32_t* dims, const mwSize num_dims);
char** getFieldNames(Data* object);
DataArrays rearrangeImaginaryData(Data* object);

bool_t warnedObjectVar;
bool_t warnedUnknownVar;

#endif //GET_MAT_VAR