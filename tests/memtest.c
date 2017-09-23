#include "../src/mapping.h"


int main(int argc, char* argv[])
{
	
	initializeMaps();
	addr_queue = NULL;
	varname_queue = NULL;
	header_queue = NULL;
	fd = -1;
	num_threads_to_use = -1;
	will_multithread = TRUE;
	
	//char* filename = "res/hxdcopies/optData_ESTrade.mat";
	char* filename = argv[1];
	char** variable_name = malloc(sizeof(char*));
	variable_name[0] = "";
	//variable_name[0] = malloc(30*sizeof(char));
	//strcpy(variable_name[0], "");
	Queue* objects = getDataObjects(filename, variable_name, 1);
	Data* front_object = peekQueue(objects, QUEUE_FRONT);
	if((ERROR_DATA & front_object->type) == ERROR_DATA)
	{
		Data* front_object = peekQueue(objects, QUEUE_FRONT);
		char err_id[NAME_LENGTH], err_string[NAME_LENGTH];
		strcpy(err_id, front_object->name);
		strcpy(err_string, front_object->matlab_class);
		freeQueue(objects);
		free(variable_name);
		readMXError(err_id, err_string);
		exit(1);
	}
	
	
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
	
	printf("\nProgram exited without errors\n\n");

	return 0;

}
