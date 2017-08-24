#include "mapping.h"


void printDouble(Data *object);
void printShort(Data *object);
void printCell(Data *object);
void printStruct(Data *object);
void printChar(Data *object);


int main(int argc, char *argv[])
{
	char *filename = argv[1];
	char variable_name[30];
	strcpy(variable_name, argv[2]);
	int *num_objs = (int *) malloc(sizeof(int));
	Data *objects = getDataObject(filename, variable_name, num_objs);
	Data *hi_objects = organizeObjects(objects, *num_objs);
	int index = 0;
	
	while (hi_objects[index].type != UNDEF)
	{
		switch (hi_objects[index].type)
		{
			case DOUBLE:
				printDouble(&hi_objects[index]);
				break;
			case CHAR:
				printChar(&hi_objects[index]);
				break;
			case UNSIGNEDINT16:
				printShort(&hi_objects[index]);
				break;
			case REF:
				printCell(&hi_objects[index]);
				break;
			case STRUCT:
				printStruct(&hi_objects[index]);
				break;
			default:
				break;
		}
		index++;
	}
	freeDataObjects(objects, *num_objs);
	free(hi_objects);
}


void printDouble(Data *object)
{
	int num_elems = 1;
	int num_dims = 0;
	int i = 0;
	while (object->dims[i] > 0)
	{
		num_elems *= object->dims[i];
		num_dims++;
		i++;
	}
	
	printf("\n%s:\n", object->name);
	for (i = 0; i < num_elems; i++)
	{
		printf("%f ", object->double_data[i]);
		for (int j = 0; j < num_dims - 1; j++)
		{
			if ((i + 1) % object->dims[j] == 0)
			{
				printf("\n");
			}
		}
	}
	printf("\n");
}


void printShort(Data *object)
{
	int num_elems = 1;
	int num_dims = 0;
	int i = 0;
	while (object->dims[i] > 0)
	{
		num_elems *= object->dims[i];
		num_dims++;
		i++;
	}
	
	char *string = (char *) malloc(num_elems + 1);
	
	for (i = 0; i < num_elems; i++)
	{
		string[i] = object->ushort_data[i];
	}
	string[num_elems] = 0;
	printf("\n%s:\n", object->name);
	printf("%s\n\n", string);
}


void printCell(Data *object)
{
	printf("\n%s:\n", object->name);
	Data *cell_objects = object->sub_objects;
	
	int num_elems = 1;
	int num_dims = 0;
	int i = 0;
	while (object->dims[i] > 0)
	{
		num_elems *= object->dims[i];
		num_dims++;
		i++;
	}
	
	for (i = 0; i < num_elems; i++)
	{
		printf("%f ", cell_objects[i].double_data[0]);
		for (int j = 0; j < num_dims - 1; j++)
		{
			if ((i + 1) % object->dims[j] == 0)
			{
				printf("\n");
			}
		}
	}
	printf("\n");
}


void printStruct(Data *object)
{
	printf("\n%s fields: \n", object->name);
	
	int index = 0;
	while (object->sub_objects[index].type != UNDEF)
	{
		printf("%s\n", object->sub_objects[index].name);
		index++;
	}
	
	printf("\n");
}


void printChar(Data *object)
{
	printf("%s:\n", object->name);
	
	int num_elems = 1;
	int num_dims = 0;
	int i = 0;
	while (object->dims[i] > 0)
	{
		num_elems *= object->dims[i];
		num_dims++;
		i++;
	}
	
	for (i = 0; i < num_elems; i++)
	{
		printf("%d ", object->char_data[i]);
		for (int j = 0; j < num_dims - 1; j++)
		{
			if ((i + 1) % object->dims[j] == 0)
			{
				printf("\n");
			}
		}
	}
	printf("\n");
	
}