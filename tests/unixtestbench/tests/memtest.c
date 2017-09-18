#include "../src/mapping.h"


void main(int argc, char* argv[])
{
	char* filename = "../../res/t.mat";
	char** variable_name = malloc(sizeof(char*));
	variable_name[0] = malloc(30*sizeof(char));
	strcpy(variable_name[0], "t");
	Queue* objects = getDataObjects(filename, variable_name, 1);
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