#include "../src/mapping.h"


void printDouble(Data* object);
void printShort(Data* object);
void printCell(Data* object);
void printStruct(Data* object);
void printChar(Data* object);


int main(int argc, char* argv[])
{
	char* filename = argv[1];
	char variable_name[30];
	strcpy(variable_name, argv[2]);
	Queue* objects = getDataObjects(filename, full_variable_names, num_elems);
	Data** super_objects = malloc((objects->length)*sizeof(Data*));
	char** varnames = malloc((objects->length)*sizeof(char*));
	
	int num_objs = 0;
	for (;objects->length  > 0; num_objs++)
	{
		varnames[num_objs] = malloc(NAME_LENGTH*sizeof(char));
		super_objects[num_objs] = organizeObjects(objects);
		if (super_objects[num_objs] == NULL)
		{
			break;
		}
		else
		{
			strcpy(varnames[num_objs], super_objects[num_objs]->name);
		}
	}

	freeQueue(objects);
	free(super_objects);
	for (int i = 0; i < num_objs; i++)
	{
		free(varnames[i]);
	}
	free(varnames);
	
	printf("\nProgram exited without errors\n\n");
}


void printDouble(Data* object)
{
	int num_elems = 1;
	int num_dims = 0;
	int i = 0;
	while(object->dims[i] > 0)
	{
		num_elems *= object->dims[i];
		num_dims++;
		i++;
	}
	
	fprintf(stderr, "\n%s:\n", object->name);
	for(i = 0; i < num_elems; i++)
	{
		fprintf(stderr, "%f ", object->data_arrays.double_data[i]);
		for(int j = 0; j < num_dims - 1; j++)
		{
			if((i + 1) % object->dims[j] == 0)
			{
				fprintf(stderr, "\n");
			}
		}
	}
	fprintf(stderr, "\n");
}


void printShort(Data* object)
{
	int num_elems = 1;
	int num_dims = 0;
	int i = 0;
	while(object->dims[i] > 0)
	{
		num_elems *= object->dims[i];
		num_dims++;
		i++;
	}
	
	char* string = (char*) malloc(num_elems + 1);
	
	for(i = 0; i < num_elems; i++)
	{
		string[i] = (char)object->data_arrays.ui16_data[i];
	}
	string[num_elems] = 0;
	fprintf(stderr, "\n%s:\n", object->name);
	fprintf(stderr, "%s\n\n", string);
}


void printCell(Data* object)
{
	fprintf(stderr, "\n%s:\n", object->name);
	Data** cell_objects = object->sub_objects;
	
	int num_elems = 1;
	int num_dims = 0;
	int i = 0;
	while(object->dims[i] > 0)
	{
		num_elems *= object->dims[i];
		num_dims++;
		i++;
	}
	
	for(i = 0; i < num_elems; i++)
	{
		fprintf(stderr, "%f ", cell_objects[i]->data_arrays.double_data[0]);
		for(int j = 0; j < num_dims - 1; j++)
		{
			if((i + 1) % object->dims[j] == 0)
			{
				fprintf(stderr, "\n");
			}
		}
	}
	fprintf(stderr, "\n");
}


void printStruct(Data* object)
{
	fprintf(stderr, "\n%s fields: \n", object->name);
	
	int index = 0;
	while(object->sub_objects[index]->type != NULLTYPE)
	{
		fprintf(stderr, "%s\n", object->sub_objects[index]->name);
		index++;
	}
	
	fprintf(stderr, "\n");
}


void printChar(Data* object)
{
	fprintf(stderr, "%s:\n", object->name);
	
	int num_elems = 1;
	int num_dims = 0;
	int i = 0;
	while(object->dims[i] > 0)
	{
		num_elems *= object->dims[i];
		num_dims++;
		i++;
	}
	
	for(i = 0; i < num_elems; i++)
	{
		fprintf(stderr, "%d ", object->data_arrays.ui8_data[i]);
		for(int j = 0; j < num_dims - 1; j++)
		{
			if((i + 1) % object->dims[j] == 0)
			{
				fprintf(stderr, "\n");
			}
		}
	}
	fprintf(stderr, "\n");
	
}