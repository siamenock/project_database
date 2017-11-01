#include <string.h>
#include "bpt.h"

// MAIN



int main(int argc, char ** argv) {
	char input_file[64];
	char cmd[128];
	node * root;
	int input, range2;
	char instruction;
	char license_part;
	char buffer[] = "asdf";
	root = NULL;
	verbose_output = false;

	// dbfile is global var. only 1 File used now
	dbfile = fopen("this_is_sample_file", "w");
	fwrite(buffer, sizeof(char), sizeof(buffer), dbfile);
	fclose(dbfile);


	//----------------------------------------------------------//
	//                      presetting part                     //
	//----------------------------------------------------------//
	//printf("preset > ");
	while (scanf("%s", cmd)) {
		printf("debug|scanf : %s\n", cmd);
		if (strcmp(cmd, "open") == 0) {    //TODO: change into cmd_pre array and switch
			scanf("%s", input_file);
			printf("debug|filename : %s\n", input_file);
			dbfile = fopen(input_file, "r+");
			if (dbfile == NULL) {
				dbfile = fopen(input_file, "w+");
				if (dbfile == NULL) {
					perror("Failure  open input file.");
					exit(EXIT_FAILURE);
				}
				else {
					FileInit(dbfile);
					printf("file not found. opening NEW FILE\n");
					break;
				}
			}
			else {
				printf("opening EXISTING FILE\n");
				break;
			}
		}
		else {
			printf("cmd on presetting step\n\topen FILE_PATH\n");
			continue;
		}
	}

	//----------------------------------------------------------//
	//                         main loop                        //
	//----------------------------------------------------------//    
	enum {
		INSERT = 0,
		FIND,
		DELETE,
		OPERATION_COUNT
	};
	int op = OPERATION_COUNT;
	const char * op_string[3];   //operation string used by user
	op_string[INSERT] = "insert";
	op_string[FIND] = "find";
	op_string[DELETE] = "delete";

	printf("> ");
	while (scanf("%s", cmd) != EOF) {
		for (op = 0; op < OPERATION_COUNT; op++) {
			if (strcmp(cmd, op_string[op]) == 0) { //if find operation string
				break;                           //now, op == INSERT||FIND||DELETE||...
			}
		}
		switch (op) {
		case DELETE:
			scanf("%d", &input);
			//root = delete(root, input);
			//print_tree(root);
			break;
		case INSERT:
			scanf("%d", &input);
			//root = insert(root, input, input);
			//print_tree(root);
			break;
		case FIND:
			break;
		case OPERATION_COUNT:   //DEFAULT
								//usage_2();
			printf("your input : %s\nusage is wrong\n", cmd);
			break;
			/*
			case 'p':
			scanf("%d", &input);
			find_and_print(root, input, instruction == 'p');
			break;
			case 'r':
			scanf("%d %d", &input, &range2);
			if (input > range2) {
			int tmp = range2;
			range2 = input;
			input = tmp;
			}
			find_and_print_range(root, input, range2, instruction == 'p');
			break;
			case 'l':
			print_leaves(root);
			break;
			case 'q':
			while (getchar() != (int)'\n');
			return EXIT_SUCCESS;
			break;
			case 't':
			print_tree(root);
			break;
			case 'v':
			verbose_output = !verbose_output;
			break;
			case 'x':
			if (root)
			root = destroy_tree(root);
			print_tree(root);
			break;
			*/

		}
		while (getchar() != (int)'\n');
		printf("> ");
	}
	printf("\n");

	return EXIT_SUCCESS;
}
