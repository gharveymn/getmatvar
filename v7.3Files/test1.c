#include "mapping.h"


void arrayTest(Data* objects);


void cellTest(Data* objects);


void integerTest(Data* objects);


void doubleTest(Data* objects);


int main()
{
	Data* objects;
	char* variable_name = (char*) malloc(50);
	int* num_objs = (int*) malloc(sizeof(int));
	
	/*strcpy(variable_name, "my_struct.array");
	objects = getDataObject("my_struct1.mat", variable_name, num_objs);
	arrayTest(objects);
	freeDataObjects(objects, 1);
	printf("array test succeeded.\n");*/
	
	//strcpy(variable_name, "my_struct.logical");
	//objects = getDataObject("my_struct1.mat", variable_name, num_objs);
	//freeDataObjects(objects);
	
	strcpy(variable_name, "cell");
	objects = getDataObject("my_struct1.mat", variable_name, num_objs);
	cellTest(objects);
	freeDataObjects(objects, *num_objs);
	printf("cell test succeeded\n");
	
	strcpy(variable_name, "my_struct.your_struct.integer");
	objects = getDataObject("my_struct1.mat", variable_name, num_objs);
	integerTest(objects);
	freeDataObjects(objects, *num_objs);
	printf("integer test succeeded\n");
	
	strcpy(variable_name, "my_struct.your_struct.double");
	objects = getDataObject("my_struct1.mat", variable_name, num_objs);
	doubleTest(objects);
	freeDataObjects(objects, *num_objs);
	printf("double test succeeded.\n");
}


void arrayTest(Data* objects)
{
	Data data = objects[0];
	assert(data.type == DOUBLE);
	assert(strcmp(data.matlab_class, "double") == 0);
	assert(data.dims[0] == 102);
	assert(data.dims[1] == 101);
	assert(data.dims[2] == 100);
	assert(data.data_arrays.double_data != NULL);
	assert(data.data_arrays.udouble_data == NULL);
	assert(data.data_arrays.ui16_data == NULL);
	assert(data.data_arrays.ui8_data == NULL);
	assert(strcmp(data.name, "array") == 0);
	for(int i = 0; i < 102 * 101 * 100; i++)
	{
		assert(data.data_arrays.double_data[i] == 1);
	}
}


void cellTest(Data* objects)
{
	Data data;
	assert(objects[0].type == REF);
	assert(strcmp(objects[0].matlab_class, "cell") == 0);
	assert(objects[0].dims[0] == 3);
	assert(objects[0].dims[1] == 1);
	assert(objects[0].data_arrays.double_data == NULL);
	assert(objects[0].data_arrays.udouble_data != NULL);
	assert(objects[0].data_arrays.ui16_data == NULL);
	assert(objects[0].data_arrays.ui8_data == NULL);
	assert(strcmp(objects[0].name, "cell") == 0);
	
	data = objects[1];
	assert(data.type == UINT16);
	assert(strcmp(data.matlab_class, "char") == 0);
	assert(data.dims[0] == 4);
	assert(data.dims[1] == 1);
	assert(data.data_arrays.double_data == NULL);
	assert(data.data_arrays.udouble_data == NULL);
	assert(data.data_arrays.ui16_data != NULL);
	assert(data.data_arrays.ui8_data == NULL);
	char* string = (char*) malloc(5 * sizeof(char));
	for(int i = 0; i < 4; i++)
	{
		string[i] = data.data_arrays.ui16_data[i];
	}
	assert(strcmp(string, "John") == 0);
	free(string);
	
	data = objects[2];
	assert(data.type == DOUBLE);
	assert(strcmp(data.matlab_class, "double") == 0);
	assert(data.dims[0] == 1);
	assert(data.dims[1] == 1);
	assert(data.data_arrays.double_data != NULL);
	assert(data.data_arrays.udouble_data == NULL);
	assert(data.data_arrays.ui16_data == NULL);
	assert(data.data_arrays.ui8_data == NULL);
	assert(data.data_arrays.double_data[0] == 2.0);
	
	data = objects[3];
	assert(data.type == UINT16);
	assert(strcmp(data.matlab_class, "char") == 0);
	assert(data.dims[0] == 5);
	assert(data.dims[1] == 1);
	assert(data.data_arrays.double_data == NULL);
	assert(data.data_arrays.udouble_data == NULL);
	assert(data.data_arrays.ui16_data != NULL);
	assert(data.data_arrays.ui8_data == NULL);
	string = (char*) malloc(6 * sizeof(char));
	for(int i = 0; i < 5; i++)
	{
		string[i] = data.data_arrays.ui16_data[i];
	}
	assert(strcmp(string, "Smith") == 0);
	free(string);
}


void integerTest(Data* objects)
{
	Data data = objects[0];
	assert(data.type == DOUBLE);
	assert(strcmp(data.matlab_class, "double") == 0);
	assert(data.dims[0] == 1);
	assert(data.dims[1] == 1);
	assert(data.data_arrays.double_data != NULL);
	assert(data.data_arrays.udouble_data == NULL);
	assert(data.data_arrays.ui16_data == NULL);
	assert(data.data_arrays.ui8_data == NULL);
	assert(data.data_arrays.double_data[0] == 1);
	assert(strcmp(data.name, "integer") == 0);
}


void doubleTest(Data* objects)
{
	Data data = objects[0];
	assert(data.type == DOUBLE);
	assert(strcmp(data.matlab_class, "double") == 0);
	assert(data.dims[0] == 1);
	assert(data.dims[1] == 1);
	assert(data.data_arrays.double_data != NULL);
	assert(data.data_arrays.udouble_data == NULL);
	assert(data.data_arrays.ui16_data == NULL);
	assert(data.data_arrays.ui8_data == NULL);
	assert(data.data_arrays.double_data[0] == 2.0);
}