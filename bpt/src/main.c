#include <string.h>
#include "bpt.h"

// MAIN



int main(int argc, char ** argv) {
	char file_name[64];
	char cmd[128];
	char val[VALUE_SIZE];
	//node * root;
	int64_t input;
	int range2;
	char instruction;
	char license_part;
	char buffer[] = "asdf";

	//----------------------------------------------------------//
	//                      presetting part                     //
	//			open new file or open existing file				//
	//----------------------------------------------------------//
	printf("preset > ");
	while (scanf("%s", cmd)) {
		printf("debug|scanf : %s\n", cmd);
		if (strcmp(cmd, "open") == 0) {    //TODO: change into cmd_pre array and switch
			scanf("%s", file_name);
			printf("debug|filename : %s\n", file_name);
			dbfile = fopen(file_name, "r+");
			if (dbfile == NULL) {
				dbfile = fopen(file_name, "w+");
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
		PRINT,
		XXD,
		DROP,
		TEST1,
		TEST2,
		OPERATION_COUNT
	};
	int op = OPERATION_COUNT;
	const char * op_string[] = {
		"insert",	//op_string[INSERT]
		"find",		//op_string[FIND]
		"delete",	//op_string[DELETE] 
		"print",	//op_string[PRINT]
		"xxd",		//op_string[XXD] 
		"drop",		//op_string[DROP] 
		"test1",
		"test2"
	};   //operation string input by user
	
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
			scanf("%I64i %s", &input, val);
			insert(input, val);
			PrintTree();
			break;
		case FIND:
			scanf("%I64i", &input);
			char* val_ptr = find(input);
			if (val_ptr == NULL) {
				printf("key%5d not exist!\n", input);
			} else {
				printf("exist! value : %s", val_ptr);
				free(val_ptr);
			}
			break;
		case PRINT:
			PrintTree();
			break;
		case XXD:
			XxdFile();
			break;
		case DROP:
			fclose(dbfile);
			remove(file_name);
			return 0;
		case TEST1:
			{
				IntrRecord  a[3];
				IntrRecord *b[3];
				int i;
				for (i = 0; i < 3; i++) {
					a[i].key	= i * 10;
					a[i].offset = PAGE_SIZE * (i+1);
					SetIntrRecord(PAGE_SIZE, i, a[i]);
					b[i] = GetIntrRecordPtr(PAGE_SIZE, i);
					continue;
				}
				i = i;
				for (i = 0; i < 3; i++) {
					b[i] = GetIntrRecordPtr(PAGE_SIZE, i);
				}
				i = i;
				SetKeyNum(PAGE_SIZE, 3);
				break;
			}
		case TEST2:
		{	// mem leaking here!
			LeafRecord  a[6];
			LeafRecord *b[6];
			int i;
			for (i = 0; i < 3; i++) {
				a[i].key = i * 5;
				strcpy(a[i].value, "aaaaaaaaaa");
				SetLeafRecord(PAGE_SIZE, i, a[i]);
				b[i] = GetLeafRecordPtr(PAGE_SIZE, i);
				continue;
			}
			i = i;
			for (i = 3; i < 6; i++) {
				b[i] = (LeafRecord *)malloc(sizeof(LeafRecord));
				a[i].key = i * 5;
				strcpy(a[i].value, "aaaaaaaaaa");
				//SetLeafRecord(PAGE_SIZE, i, a[i]);
				SetLeafKey(PAGE_SIZE, i, a[i].key);
				SetValue(PAGE_SIZE, i, a[i].value);
				b[i]->key = GetLeafKey(PAGE_SIZE, i);
				GetLeafValue(PAGE_SIZE, b[i]->value, i);
				//b[i] = GetLeafRecordPtr(PAGE_SIZE, i);
				continue;
			}
			i = i;

			for (i = 0; i < 6; i++) {
				b[i] = GetLeafRecordPtr(PAGE_SIZE, i);
			}
			i = i;
			SetKeyNum(PAGE_SIZE,  6);
			break;
		}
		default:
		case OPERATION_COUNT:   //DEFAULT
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

