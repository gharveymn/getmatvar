#include "../src/mapping.h"


int main(int argc, char* argv[])
{
	char* filename = "../../../res/my_struct1.mat";
	char** variable_name = malloc(sizeof(char*));
	variable_name[0] = "";
	//variable_name[0] = malloc(30*sizeof(char));
	//strcpy(variable_name[0], "");
	Queue* objects = getDataObjects(filename, variable_name, 1);
	Data** super_objects = malloc((objects->length)*sizeof(Data*));
	char** varnames = malloc((objects->length)*sizeof(char*));
	
	int num_objs = 0;
	for (;objects->length  > 0; num_objs++)
	{
		super_objects[num_objs] = organizeObjects(objects);
		if (super_objects[num_objs] == NULL)
		{
			break;
		}
		else
		{
			varnames[num_objs] = malloc(NAME_LENGTH*sizeof(char));
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

	free(variable_name);
	
	printf("\nProgram exited without errors\n\n");

	return 0;

}
