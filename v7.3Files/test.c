
#include "mapping.h"
int main()
{
	Data data;
	Data* objects;
	char* variable_name = (char *)malloc(50);

	strcpy(variable_name, "string");
	char string[9] = "Courtney";
	objects = mapping("my_struct.mat", variable_name);
	data = objects[0];
	assert(data.type == UINT16_T);
	assert(strcmp(data.matlab_class, "char") == 0);
	assert(data.dims[0] == 8);
	assert(data.dims[1] == 1);
	assert(data.double_data == NULL);
	assert(data.udouble_data == NULL);
	assert(data.ushort_data != NULL);
	assert(data.char_data == NULL);
	for (int i = 0; i < 8; i++)
	{
		assert(data.ushort_data[i] = string[i]);
	}

	//dbl
	strcpy(variable_name, "dbl");
	objects = mapping("my_struct.mat", variable_name);
	data = objects[0];
	assert(data.type == DOUBLE);
	assert(strcmp(data.matlab_class, "double") == 0);
	assert(data.dims[0] == 1);
	assert(data.dims[1] == 1);
	assert(data.double_data != NULL);
	assert(data.udouble_data == NULL);
	assert(data.ushort_data == NULL);
	assert(data.char_data == NULL);
	assert(data.double_data[0] == 2.2);

	//integer
	strcpy(variable_name, "integer");
	objects = mapping("my_struct.mat", variable_name);
	data = objects[0];
	assert(data.type == DOUBLE);
	assert(strcmp(data.matlab_class, "double") == 0);
	assert(data.dims[0] == 1);
	assert(data.dims[1] == 1);
	assert(data.double_data != NULL);
	assert(data.udouble_data == NULL);
	assert(data.ushort_data == NULL);
	assert(data.char_data == NULL);
	assert(data.double_data[0] == 1);

	//array
	strcpy(variable_name, "array");
	objects = mapping("my_struct.mat", variable_name);
	data = objects[0];
	assert(data.type == DOUBLE);
	assert(strcmp(data.matlab_class, "double") == 0);
	//dimensions are stored columns first???
	assert(data.dims[0] == 3);
	assert(data.dims[1] == 2);
	assert(data.double_data != NULL);
	assert(data.udouble_data == NULL);
	assert(data.ushort_data == NULL);
	assert(data.char_data == NULL);
	for (int i = 0; i < 6; i++)
	{
		assert(data.double_data[i] == 1);
	}

	//cell
	strcpy(variable_name, "cell");
	objects = mapping("my_struct.mat", variable_name);
	assert(objects[0].type == REF);
	assert(strcmp(objects[0].matlab_class, "cell") == 0);
	assert(objects[0].dims[0] == 3);
	assert(objects[0].dims[1] == 1);
	assert(objects[0].double_data == NULL);
	assert(objects[0].udouble_data != NULL);
	assert(objects[0].ushort_data == NULL);
	assert(objects[0].char_data == NULL);
	for (int i = 1; i < 4; i++)
	{
		data = objects[i];
		assert(data.type == DOUBLE);
		assert(strcmp(data.matlab_class, "double") == 0);
		assert(data.dims[0] == 1);
		assert(data.dims[1] == 1);
		assert(data.double_data != NULL);
		assert(data.udouble_data == NULL);
		assert(data.ushort_data == NULL);
		assert(data.char_data == NULL);
	}
	assert(objects[1].double_data[0] == 1);
	assert(objects[2].double_data[0] == 1.1);
	assert(objects[3].double_data[0] == 1.2);
	
	//my_struct.string
	strcpy(variable_name, "my_struct.string");
	objects = mapping("my_struct.mat", variable_name);
	data = objects[0];
	assert(data.type == UINT16_T);
	assert(strcmp(data.matlab_class, "char") == 0);
	assert(data.dims[0] == 8);
	assert(data.dims[1] == 1);
	assert(data.double_data == NULL);
	assert(data.udouble_data == NULL);
	assert(data.ushort_data != NULL);
	assert(data.char_data == NULL);
	for (int i = 0; i < 8; i++)
	{
		assert(data.ushort_data[i] = string[i]);
	}

	//my_struct.double
	strcpy(variable_name, "my_struct.double");
	objects = mapping("my_struct.mat", variable_name);
	data = objects[0];
	assert(data.type == DOUBLE);
	assert(strcmp(data.matlab_class, "double") == 0);
	assert(data.dims[0] == 1);
	assert(data.dims[1] == 1);
	assert(data.double_data != NULL);
	assert(data.udouble_data == NULL);
	assert(data.ushort_data == NULL);
	assert(data.char_data == NULL);
	assert(data.double_data[0] == 2.2);

	//my_struct.integer
	strcpy(variable_name, "my_struct.integer");
	objects = mapping("my_struct.mat", variable_name);
	data = objects[0];
	assert(data.type == DOUBLE);
	assert(strcmp(data.matlab_class, "double") == 0);
	assert(data.dims[0] == 1);
	assert(data.dims[1] == 1);
	assert(data.double_data != NULL);
	assert(data.udouble_data == NULL);
	assert(data.ushort_data == NULL);
	assert(data.char_data == NULL);
	assert(data.double_data[0] == 1);

	//my_struct.array
	strcpy(variable_name, "my_struct.array");
	objects = mapping("my_struct.mat", variable_name);
	data = objects[0];
	assert(data.type == DOUBLE);
	assert(strcmp(data.matlab_class, "double") == 0);
	assert(data.dims[0] == 3);
	assert(data.dims[1] == 2);
	assert(data.double_data != NULL);
	assert(data.udouble_data == NULL);
	assert(data.ushort_data == NULL);
	assert(data.char_data == NULL);
	for (int i = 0; i < 6; i++)
	{
		assert(data.double_data[i] == 1);
	}

	//my_struct.cell
	strcpy(variable_name, "my_struct.cell");
	objects = mapping("my_struct.mat", variable_name);
	assert(objects[0].type == REF);
	assert(strcmp(objects[0].matlab_class, "cell") == 0);
	assert(objects[0].dims[0] == 3);
	assert(objects[0].dims[1] == 1);
	assert(objects[0].double_data == NULL);
	assert(objects[0].udouble_data != NULL);
	assert(objects[0].ushort_data == NULL);
	assert(objects[0].char_data == NULL);
	for (int i = 1; i < 4; i++)
	{
		data = objects[i];
		assert(data.type == DOUBLE);
		assert(strcmp(data.matlab_class, "double") == 0);
		assert(data.dims[0] == 1);
		assert(data.dims[1] == 1);
		assert(data.double_data != NULL);
		assert(data.udouble_data == NULL);
		assert(data.ushort_data == NULL);
		assert(data.char_data == NULL);
	}
	assert(objects[1].double_data[0] == 1);
	assert(objects[2].double_data[0] == 1.1);
	assert(objects[3].double_data[0] == 1.2);
}