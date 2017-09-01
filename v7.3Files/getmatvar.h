#include "mapping.h"
#include <mex.h>

//getmatvar.c
void makeReturnStructure(mxArray* uberStructure[], int num_elems, const char* full_variable_names[], const char* filename);
mxArray* makeSubstructure(mxArray* returnStructure, int num_elems, Data** objects, DataType super_structure_type);
mwSize* makeObjDims(const uint32_t* dims, mwSize num_obj_dims);
const char** getFieldNames(Data* object);
void readMXError(const char error_id[], const char error_message[]);
void readMXWarn(const char warn_id[], const char warn_message[]);

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
void setCellPtr(Data* object, mxArray* returnStructure, const char* varname, mwIndex index, DataType super_structure_type);
void setStructPtr(Data* object, mxArray* returnStructure, const char* varname, mwIndex index, DataType super_structure_type);
