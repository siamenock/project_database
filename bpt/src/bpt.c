/*
*  bpt.c
*/
#define Version "1.14"
#include "bpt.h"
#include <stdio.h>

int debug_enable = 1;
FILE * dbfile = NULL;

// FUNCTION DEFINITIONS.

// OUTPUT AND UTILITIES
typedef struct Node {
	Offset offset;
	struct Node* next;
} Node;


void enqueue(Node ** q_pointer, Offset new_offset) {
	Node * c;
	Node * new_node = (Node *)malloc(sizeof(Node));
	new_node->offset = new_offset;
	new_node->next = NULL;
	if (*q_pointer == NULL) {
		*q_pointer = new_node;
	} else {
		c = *q_pointer;
		while (c->next != NULL) {
			c = c->next;
		}
		c->next = new_node;
	}
	return;
}

Offset dequeue(Node ** q_pointer) {
	Node* q;

	if (*q_pointer == NULL) {
		return NULL_PAGE;
	}
	
	Offset ret = (*q_pointer)->offset;
	q = (*q_pointer)->next;
	free(*q_pointer);
	*q_pointer = q;
	return ret;
}
Offset seek(Node ** q_pointer) {
	if (*q_pointer == NULL) {
		return NULL_PAGE;
	} else {
		return (*q_pointer)->offset;
	}
}
void PrintTree() {
	int depth = 0;
	Node * q1 = NULL;
	Node * q2 = NULL;
	enqueue(&q1, GetHeadersRootPage());
	Offset ofs;
	//printf("debug| call PrintTree()\n");
	// this while loop : move 1 depth down
	for (depth = 0; q1 != NULL; depth++) {
		//printf("debug| PrintTree() : start next depth==%2d from page %d\n", depth, seek(&q1) / PAGE_SIZE);
		// this while loop : move 1 node right
		// nodes of this depth are in q1
		// delete them and enque next depth nodes in q2
		ofs = -1;

		while (q1 != NULL) {
			do {
				ofs = dequeue(&q1);
			} while (ofs == NULL_PAGE && q1 != NULL);
			if (ofs == NULL_PAGE && q1 == NULL) { break; }
			int i;
			int key_num = GetKeyNum(ofs);
			bool is_intr = ! (GetNodeType(ofs) == PAGE_LEAF);

			if (is_intr) {
				enqueue(&q2, GetChild(ofs, 0));
			}


			printf("[");
			for (i = 0; i < key_num; i++) {
				if (is_intr) {
					enqueue(&q2, GetChild(ofs, i + 1));
				}
				printf("%lld,", GetKey(ofs, i));
			}
			printf("]");
			//now this node is done.
			//move to next node
		}
		printf("\n");
		q1 = q2;
		q2 = NULL;
		//now this depth is done
	}
}

void XxdPage(Offset pos) {
	int page_num = pos / PAGE_SIZE;
	printf("PageNum %4d, NextFree|Parent %4ld, %s, KeyNum %4d\t",
		page_num, GetParentPage(pos) / PAGE_SIZE, GetNodeType(pos) == PAGE_INTERNAL ? "intr" : "leaf", GetKeyNum(pos));
	int j;
	for (j = 0; j < GetKeyNum(pos); j++) {
		printf("|%3lld", GetKey(pos, j));
	}
	printf("\n");
}

void XxdFile() {			//fucking window don't have xxd command like linux. so i made better one for this project
	int i = 0;
	int64_t page_num_max;
	Offset pos = ADDR_HEADER;
	page_num_max = GetHeadersPageNum();

	printf("PageNum %4d, NextFree|Parent %4ld, Rootpos %4d, TotalPages %lld\n",
		i, GetParentPage(pos) / PAGE_SIZE, GetHeadersRootPage()/PAGE_SIZE, GetHeadersPageNum());
	for (i = 1; i < page_num_max; i++) {
		Offset pos = i * PAGE_SIZE;
		XxdPage(pos);
	}
}



/* Utility function to give the height
* of the tree, which length in number of edges
* of the path from the root to any leaf.

int height(node * root) {
int h = 0;
node * c = root;
while (!c->is_leaf) {
c = c->pointers[0];
h++;
}
return h;
}
*/
/* Utility function to give the length in edges
* of the path from any node to the root.

int path_to_root(node * root, node * child) {
int length = 0;
node * c = child;
while (c != root) {
c = c->parent;
length++;
}
return length;
}
*/


/* Traces the path from the root to a leaf, searching
* by key.  Displays information about the path
* if the verbose flag is set.
* Returns the leaf containing the given key.
*/
Offset find_leaf(int64_t key) {
	int i = 0;
	Offset c = GetHeadersRootPage();
	if (c == NULL_PAGE) {
		return c;
	}

	while (GetNodeType(c) == PAGE_INTERNAL) {
		i = 0;
		int key_num = GetKeyNum(c);
		while (i < key_num) {
			if (GetKey(c, i) <= key) {
				i++;
			}
			else {
				break;
			}
		}
		c = GetChild(c, i);
	}
	return c;
}

/* Finds and returns the value
*  which key refers.
*/
char* find(int64_t key) {
	int i = 0;
	Offset c = find_leaf(key);
	if (c == NULL_PAGE) {
		return NULL;
	}
	int key_num = GetKeyNum(c);
	for (i = 0; i < key_num; i++) {
		if (GetKey(c, i) == key) {
			break;
		}
	}
	if (i == key_num) {
		return NULL;
	} else {
		char* ret = GetValuePtr(c, i);
		return ret;
	}
}

/* Finds the appropriate place to
* split a node that is too big into two.
*/
int cut(int length) {
	if (length % 2 == 0)
		return length / 2;
	else
		return length / 2 + 1;
}


// INSERTION

/* Creates a new INTERNAL node from spare free pages.
	if not exist, call MoreFreePage to finish job
*/
Offset make_node(void) {
	Offset ret = GetNewPage();
	if (ret == NULL_PAGE) {
		perror("Node creation.");
		exit(EXIT_FAILURE);
	}
	SetNodeType(ret, PAGE_INTERNAL);
	SetKeyNum(ret, 0);
	SetParentPage(ret, NULL_PAGE);
	SetRightSibling(ret, NULL_PAGE);
	return ret;
}

/* Creates a new LEAF node from spare free pages.
	if not exist, call MoreFreePage to finish job
*/
Offset make_leaf() {
	Offset ret = make_node();
	SetNodeType(ret, PAGE_LEAF);
	return ret;
}

/* Inserts a new new leaf record 
	into a leaf with ENOUGH SPACE!!!(warning)
	Returns #define SUCCESS if success.
*/
int insert_into_leaf(Offset leaf, LeafRecord r) {
	int i = 0;
	int key_num = GetKeyNum(leaf);
	while (i < key_num && GetKey(leaf, i) < r.key) {
		i++;
	}
	LeafRecord* move = NULL;
	int insert_index = i;
	for (i = key_num; insert_index < i; i--) {
		move = GetLeafRecordPtr(leaf, i - 1);
		SetLeafRecord(leaf, i, * move);
		free(move);
	}
	//SetLeafRecord(leaf, insert_index, r);
	SetLeafKey(leaf, insert_index, r.key);
	//SetValue(leaf, insert_index, r.value);
	
	int64_t key = GetKey(leaf, insert_index);
	//printf("debug| insert check: key, %I64i\n", key, r);
	SetKeyNum(leaf, key_num + 1);




	//printf("debug| insert check: key, val -> %I64i, %s\n", r.key, r.value);
	key = GetKey(leaf,insert_index);
	char*	val = GetValuePtr(leaf, insert_index);
	//printf("debug| insert check: key, val -> %I64i, %s\n", key, val);
	free(val);
	LeafRecord* val_lr = GetLeafRecordPtr(leaf, insert_index);
	//printf("debug| insert check: key, val -> %I64i, %s\n", val_lr->key, val_lr->value);
	
	return SUCCESS;
}


/* Inserts a new LEAF RECORD  into a FULL leaf (warning)
	split node & send new sibling to parent
	Returns #define SUCCESS if success.
*/
int insert_into_leaf_after_splitting(Offset leaf, LeafRecord r) {
	int insert_index, split, i, j;
	int64_t key = r.key;
	Offset new_leaf;
	LeafRecord * temp_records[LEAF_DEGREE + 1];
	LeafRecord * new_record = (LeafRecord *)malloc(sizeof(LeafRecord));;
	new_record->key = r.key;
	for (i = 0; i < VALUE_SIZE; i++) { new_record->value[i] = r.value[i];}	// copying new input done


	insert_index = 0;
	while (insert_index < LEAF_DEGREE && GetKey(leaf, insert_index) < key) {
		insert_index++;
	}

	int key_num = GetKeyNum(leaf);
	for (i = j = 0; i < key_num; i++, j++) {
		if (i == insert_index) {
			temp_records[j] = new_record;
			j++;
		}
		temp_records[j] = GetLeafRecordPtr(leaf, i);
	}
	if (i == j) {
		temp_records[j] = new_record;
		j++;
	}

	split = cut(LEAF_DEGREE);
	for (i = 0; i < split; i++) {
		SetLeafRecord(leaf, i, *temp_records[i]);
		free(temp_records[i]);
	}
	SetKeyNum(leaf, split);

	new_leaf = make_leaf();
	SetKeyNum(new_leaf, key_num + 1 - split);
	for (i = split; i < key_num + 1; i++) {
		SetLeafRecord(new_leaf, i - split, *temp_records[i]);
		free(temp_records[i]);
	}


	SetRightSibling(new_leaf, GetRightSibling(leaf));
	SetRightSibling(leaf, new_leaf);


	Offset parent = GetParentPage(leaf);

	int ret = insert_into_parent(parent, leaf, new_leaf);
	return ret;
}



int64_t GetMinKeyFromChildren(Offset node) {		//TODO: reform insert_into_parent to remove this function
	if (GetNodeType(node) == PAGE_LEAF) {
		return GetKey(node, 0);	//		actually this is not needed but I made it for easy coding
	} else {
		return GetMinKeyFromChildren(GetChild(node, 0));
	}
}


/* parameters : [parent, current_child, new_child]
 Inserts a new child to parent.
 this new child mush occure from current_child's split
*/
int insert_into_parent(Offset node, Offset cur_child, Offset new_child) {
if (node == NULL_PAGE) {
		return insert_into_new_root(cur_child, new_child);
	}
	int64_t      new_key	= GetMinKeyFromChildren(new_child);
	IntrRecord * new_record = (IntrRecord *)malloc(sizeof(IntrRecord));
	new_record->key			= new_key;
	new_record->offset		= new_child;

	int i;
	int key_num = GetKeyNum(node);
	for (i = 0; i < key_num; i++) {
		if (new_key <= GetKey(node, i)) {
			break;                  // find place
		}
	}
	int insert_index = i;

	// ------case---------------------------case-------
	//		enough space for new_child node
	//		just put it in this node
	// ------case---------------------------case-------
	if (key_num < INTR_DEGREE) {
		SetParentPage(new_child, node);  // I'm your father

		IntrRecord* rp = NULL;          // right-shift all nodes after insert_index
		for (i = key_num; insert_index < i; i--) {
			rp = GetIntrRecordPtr(node, i - 1);
			SetIntrRecord(node, i, *rp);
			free(rp);
		}                               // and insert it
		SetIntrRecord(node, i, *new_record);
		free(new_record);
		SetKeyNum(node, key_num + 1);
		return SUCCESS;                 // jobs done

	}
	else {
		// ------case---------------------------case-------
		//		    no space for new_child node
		//	split this node and send middle value to parent
		// ------case---------------------------case-------
		int j;							//      : need to split again.
		IntrRecord* temp_records[INTR_DEGREE + 1];
		for (i = j = 0; i < key_num; i++, j++) {
			if (i == insert_index) {temp_records[j++] = new_record;	}
			temp_records[j] = GetIntrRecordPtr(node, i);
		}
		if (i == insert_index) { temp_records[j++] = new_record; }
		int split = cut(INTR_DEGREE);		//now ready to re-copy

		for (i = 0; i < split; i++) {
			SetIntrRecord(node, i, *temp_records[i]);
			free(temp_records[i]);
		}
		SetKeyNum(node, split);         
		
		Offset new_sibling = make_node();
		SetChild(new_sibling, 0, temp_records[i]->offset);
		SetParentPage(temp_records[i]->offset, new_sibling);
		free(temp_records[i++]);				// not contains half point key

		SetKeyNum(new_sibling, key_num - split);	//so +1 not included here

		

		for (i = split + 1; i < key_num + 1; i++) {
			SetIntrRecord(new_sibling, i - (split + 1), *temp_records[i]);
			SetParentPage(temp_records[i]->offset, new_sibling);
			free(temp_records[i]);
		}

		

		Offset parent = GetParentPage(node);

		int ret = insert_into_parent(parent, node, new_sibling);

		return ret;
	}
}


/* Creates a new root for two subtrees
* and inserts the appropriate key into
* the new root.
*/
int insert_into_new_root(Offset cur_child, Offset new_child) {

	Offset root = make_node();
	SetParentPage(root, NULL_PAGE);
	SetChild(root, 0, cur_child);
	SetChild(root, 1, new_child);
	SetKey(root, 0, GetMinKeyFromChildren(new_child));
	SetKeyNum(root, 1);


	SetHeadersRootPage(root);
	SetParentPage(cur_child, root);
	SetParentPage(new_child, root);

	return SUCCESS;
}



/* First insertion:
* start a new tree.

node * start_new_tree(int key, record * pointer) {

node * root = make_leaf();
root->keys[0] = key;
root->pointers[0] = pointer;
root->pointers[order - 1] = NULL;
root->parent = NULL;
root->key_num++;
return root;
}
*/


/* Master insertion function.
* Inserts a key and an associated value into
* the B+ tree, causing the tree to be adjusted
* however necessary to maintain the B+ tree
* properties.
*/
int insert(int64_t key, char value[VALUE_SIZE]) {
	int i;
	LeafRecord r;
	r.key = key;
	for (i = 0; i < VALUE_SIZE; i++)
		r.value[i] = value[i];  //rec will be only use in this function.
	Offset leaf;
	//printf("debug| insert check: key, val -> %I64i, %s\n", r.key, r.value);

	if (GetHeadersRootPage() == NULL_PAGE) { // case : no root yet!
		Offset header = make_leaf();
		SetHeadersRootPage(header);
		insert_into_leaf(header, r);
		
		return SUCCESS;
	}

	char* find_result = find(key);
	if (find_result != NULL) {   // in case of duplication
		free(find_result);
		return -1;              // ignore input command
	}



	/* Case: the tree already exists.
	* (Rest of function body.)
	*/
	leaf = find_leaf(key);
	int key_num = GetKeyNum(leaf);
	int ret;
	if (key_num < LEAF_DEGREE) {   // Case: leaf has room for key and pointer.
		ret = insert_into_leaf(leaf, r);
		fflush(dbfile);
		return ret;

	} else {                            // Case:  leaf must be split.
		ret = insert_into_leaf_after_splitting(leaf, r);
		fflush(dbfile);
		return ret;
	}
	
}

// DELETION.

/* Utility function for deletion.  Retrieves
* the index of a node's nearest neighbor (sibling)
* to the left if one exists.  If not (the node
* is the leftmost child), returns -1 to signify
* this special case.
*/
/*
int get_neighbor_index(node * n) {

int i;

Return the index of the key to the left
* of the pointer in the parent pointing
* to n.
* If n is the leftmost child, this means
* return -1.

for (i = 0; i <= n->parent->key_num; i++)
if (n->parent->pointers[i] == n)
return i - 1;

// Error state.
printf("Search for nonexistent pointer to node in parent.\n");
printf("Node:  %#lx\n", (unsigned long)n);
exit(EXIT_FAILURE);
}
*/
/*
node * remove_entry_from_node(node * n, int key, node * pointer) {

int i, num_pointers;

// Remove the key and shift other keys accordingly.
i = 0;
while (n->keys[i] != key)
i++;
for (++i; i < n->key_num; i++)
n->keys[i - 1] = n->keys[i];

// Remove the pointer and shift other pointers accordingly.
// First determine number of pointers.
num_pointers = n->is_leaf ? n->key_num : n->key_num + 1;
i = 0;
while (n->pointers[i] != pointer)
i++;
for (++i; i < num_pointers; i++)
n->pointers[i - 1] = n->pointers[i];


// One key fewer.
n->key_num--;

// Set the other pointers to NULL for tidiness.
// A leaf uses the last pointer to point to the next leaf.
if (n->is_leaf)
for (i = n->key_num; i < order - 1; i++)
n->pointers[i] = NULL;
else
for (i = n->key_num + 1; i < order; i++)
n->pointers[i] = NULL;

return n;
}
*/


/* Coalesces a node that has become
* too small after deletion
* with a neighboring node that
* can accept the additional entries
* without exceeding the maximum.
*/
//node * coalesce_nodes(node * root, node * n, node * neighbor, int neighbor_index, int k_prime);

/* Redistributes entries between two nodes when
* one has become too small after deletion
* but its neighbor is too big to append the
* small node's entries without exceeding the
* maximum

node * redistribute_nodes(node * root, node * n, node * neighbor, int neighbor_index,
int k_prime_index, int k_prime) {

int i;
node * tmp;

// Case: n has a neighbor to the left.
// Pull the neighbor's last key-pointer pair over
// from the neighbor's right end to n's left end.

if (neighbor_index != -1) {
if (!n->is_leaf)
n->pointers[n->key_num + 1] = n->pointers[n->key_num];
for (i = n->key_num; i > 0; i--) {
n->keys[i] = n->keys[i - 1];
n->pointers[i] = n->pointers[i - 1];
}
if (!n->is_leaf) {
n->pointers[0] = neighbor->pointers[neighbor->key_num];
tmp = (node *)n->pointers[0];
tmp->parent = n;
neighbor->pointers[neighbor->key_num] = NULL;
n->keys[0] = k_prime;
n->parent->keys[k_prime_index] = neighbor->keys[neighbor->key_num - 1];
}
else {
n->pointers[0] = neighbor->pointers[neighbor->key_num - 1];
neighbor->pointers[neighbor->key_num - 1] = NULL;
n->keys[0] = neighbor->keys[neighbor->key_num - 1];
n->parent->keys[k_prime_index] = n->keys[0];
}
}

// Case: n is the leftmost child.
// Take a key-pointer pair from the neighbor to the right.
// Move the neighbor's leftmost key-pointer pair
// to n's rightmost position.


else {
if (n->is_leaf) {
n->keys[n->key_num] = neighbor->keys[0];
n->pointers[n->key_num] = neighbor->pointers[0];
n->parent->keys[k_prime_index] = neighbor->keys[1];
}
else {
n->keys[n->key_num] = k_prime;
n->pointers[n->key_num + 1] = neighbor->pointers[0];
tmp = (node *)n->pointers[n->key_num + 1];
tmp->parent = n;
n->parent->keys[k_prime_index] = neighbor->keys[0];
}
for (i = 0; i < neighbor->key_num - 1; i++) {
neighbor->keys[i] = neighbor->keys[i + 1];
neighbor->pointers[i] = neighbor->pointers[i + 1];
}
if (!n->is_leaf)
neighbor->pointers[i] = neighbor->pointers[i + 1];
}

// n now has one more key and one more pointer;
// the neighbor has one fewer of each.
//

n->key_num++;
neighbor->key_num--;

return root;
}
*/

/* Deletes an entry from the B+ tree.
* Removes the record and its key and pointer
* from the leaf, and then makes all appropriate
* changes to preserve the B+ tree properties.

node * delete_entry(node * root, node * n, int key, void * pointer) */

/* Master deletion function.

node * delete(node * root, int key) {

node * key_leaf;
record * key_record;

key_record = find(root, key, false);
key_leaf = find_leaf(root, key, false);
if (key_record != NULL && key_leaf != NULL) {
root = delete_entry(root, key_leaf, key, key_record);
free(key_record);
}
return root;
}


void destroy_tree_nodes(node * root) {
int i;
if (root->is_leaf)
for (i = 0; i < root->key_num; i++)
free(root->pointers[i]);
else
for (i = 0; i < root->key_num + 1; i++)
destroy_tree_nodes(root->pointers[i]);
free(root->pointers);
free(root->keys);
free(root);
}


node * destroy_tree(node * root) {
destroy_tree_nodes(root);
return NULL;
}
*/



//under here, my funtions

int open_db(char file_name[]) {
	//printf("debug|filename : %s\n", file_name);
	dbfile = fopen(file_name, "r+");
	if (dbfile == NULL) {
		//file not exist
		dbfile = fopen(file_name, "w+");
		if (dbfile == NULL) {
			//perror("Failure  open new input file.");
			//exit(EXIT_FAILURE);
			return 1;
		} else {
			FileInit(dbfile);
			//printf("open NEW FILE\n");
			fflush(dbfile);
			return 0;
		}
	} else {
		//printf("opening EXISTING FILE\n");
		//fflush(dbfile);
		return 0;
	}
	return 1;
}

//TODO : make it work without OS's buffering
void SetInstancesOnDB(Offset node_offset, void* value, int instance_pos, size_t size, size_t count) {
	int sum = 0;
	int add = 0;
	int wait_counter = 0;
	int64_t debug_val = *((int64_t*) value);
	fseek(dbfile, node_offset + instance_pos, SEEK_SET);

	while (sum < (int)count && 0 <= add) {
		add = fwrite(value, size, count, dbfile);
		sum += add;
		if (add == 0) {
			wait_counter++;
		}
		else {
			wait_counter = 0;
		}
		if (10 <wait_counter) {
			printf("SetInstanceOnDB(offset=0x%lx, val, pos=%d, size=%d, count=%d)      not response.\n",
				(unsigned long)node_offset, instance_pos, (int)size, (int)count);
			return;
		}
		if (add < 0) {
			printf("SetInstanceOnDB(offset=0x%lx, val, pos=%d, size=%d, count=%d)      failed\n",
				(unsigned long)node_offset, instance_pos, (int)size, (int)count);
			return;
		}
	}
	//debug
	fseek(dbfile, node_offset + instance_pos, SEEK_SET);
	fread(value, size, count, dbfile);
	int64_t debug_val2 = *((int64_t*)value);
	if (debug_val != debug_val2 || (
		((int)size)==8 && (debug_val < -100 || 1000 < debug_val))	) {
		printf("FUCK! fwrite int64 is suck!\n");
	}
	if (0 <= add) {
		return;
	}
	else {   //error
		printf("ERROR from SetInstancesOnDB(page num=%d, &val, instance_pos = %d, size = %d, count = %d)",
			(int)node_offset / PAGE_SIZE, instance_pos, (int)size, (int)count);
	}
}

// read/write basic functions (this common code will be applied to specifified funtions)
Offset GetOffsetOnDB(Offset node_offset, int instance_pos) {
	Offset value[1];
	fseek(dbfile, node_offset + instance_pos, SEEK_SET);
	int result = 0;
	//printf("debug| start GetOffsetOnDB(offset=0x%lx, pos=%d)\n",(unsigned long)node_offset, instance_pos);
	while (0 == result) {
		result = fread(value, sizeof(Offset), 1, dbfile);
	}
	if (result < 0) {
		printf("ERROR: GetOffsetOnDB(node=%u, pos = %d) -> return %u\n",
			(unsigned int)node_offset, instance_pos, (unsigned int)value[0]);
	}
	return value[0];
}
int32_t GetInt32OnDB(Offset node_offset, int instance_pos) {
	int32_t value[1];
	fseek(dbfile, node_offset + instance_pos, SEEK_SET);
	int result = fread(value, sizeof(int32_t), 1, dbfile);
	if (result < 0) {
		printf("ERROR: GetInt32OnDB(node=%u, pos = %d) -> return %u\n",
			(unsigned int)node_offset, instance_pos, (unsigned int)value[0]);
	}
	return value[0];
}
int64_t GetInt64OnDB(Offset node_offset, int instance_pos) {
	int64_t value[1];
	fseek(dbfile, node_offset + instance_pos, SEEK_SET);
	int result = fread(value, sizeof(int64_t), 1, dbfile);
	if (result < 0) {
		printf("ERROR: GetInt64OnDB(node=%u, pos = %d) -> return %u\n",
			(unsigned int)node_offset, instance_pos, (unsigned int)value[0]);
	}
	return value[0];
}
//must free allocated return value!!!!!!

void* GetVoidValOnDB(Offset node_offset, char* buff, int instance_pos, size_t size, size_t count) {
	fseek(dbfile, node_offset + instance_pos, SEEK_SET);
	int sum = 0, add = 0;
	while (sum < (int)count) {
		add = fread(buff + (sum * size), size, count - sum, dbfile);
		sum += add;
		if (add < 0) {
			printf("ERROR: GetVoidPtrOnDB(node=%u, pos = %d) -> return %u\n",
				(unsigned int)node_offset, instance_pos, (unsigned int) buff[0]);
		}
	}
	return (void *)buff;
}
void * GetVoidPtrOnDB(Offset node_offset, int instance_pos, size_t size, size_t count) {
	char * value = (char *)malloc(size * count);		// to move 1 Byte per ++, define value as char pointer
	return GetVoidValOnDB(node_offset, value, instance_pos, size, count);
}

//==============================================================================//
//  *     *                  [dbfile] GET, SET funtions             *       *   //
//  general format : Set[instance_name](node_offset, instance_value)            //
//                   Get[instance_name](node_offset)                            //
//  get, set value on dbfile.                                                   //
//  WARNING : you must free Get[변수명]Ptr functions.(pointer 무조건 free 해)	//
//																				//
//  To select which node's value you want, it requires node_offset              //
//  But, Instance only used in header don't require it                          //
//  header form    : SetHeaders[instance_name](instance_value)                  //
//                   SetHeaders[instance_name]()                                //
//==============================================================================//
void SetNextFreePage(Offset node_offset, Offset value ) { SetInstancesOnDB(node_offset, &value, TO_NEXT_FREE_PAGE_OFFSET, sizeof(Offset), 1); }
void SetParentPage	(Offset node_offset, Offset value ) { SetInstancesOnDB(node_offset, &value, TO_PARENT_PAGE_OFFSET,	 sizeof(Offset ), 1); }
void SetRightSibling(Offset node_offset, Offset value ) { SetInstancesOnDB(node_offset, &value, TO_RIGHT_SIBLING_OFFSET, sizeof(Offset ), 1); }
void SetNodeType	(Offset node_offset, int32_t value) { SetInstancesOnDB(node_offset, &value, TO_IS_LEAF,				 sizeof(int32_t), 1); }
void SetKeyNum		(Offset node_offset, int32_t value) { SetInstancesOnDB(node_offset, &value, TO_KEY_NUM,				 sizeof(int32_t), 1); }
void SetChild		(Offset node_offset, int index, Offset value) { SetInstancesOnDB(node_offset, &value, TO_CHILDREN + index * (int) sizeof(IntrRecord), (int) sizeof(Offset), 1); }  // for internal node
void SetHeadersPageNum(int64_t value) { SetInstancesOnDB(ADDR_HEADER, &value, TO_PAGE_NUM, sizeof(int64_t), 1); }
void SetHeadersRootPage(Offset value) { SetInstancesOnDB(ADDR_HEADER, &value, TO_ROOT_PAGE_OFFSET, sizeof(Offset), 1); }
void SetIntrKey(Offset node_offset, int index, int64_t value) { SetInstancesOnDB(node_offset, &value, TO_KEYS + index * (int) sizeof(IntrRecord), sizeof(int64_t), 1); }
void SetLeafKey(Offset node_offset, int index, int64_t value) { SetInstancesOnDB(node_offset, &value, TO_KEYS  + index * (int) sizeof(LeafRecord), sizeof(int64_t), 1); }
void SetValue(Offset node_offset, int index, char* value)	  {
	int64_t pos = node_offset + TO_VALUES + (index * (int) sizeof(LeafRecord));
	printf("setting Leaf value at 0x%llx\n", pos);
	int len = strlen(value);
	SetInstancesOnDB(node_offset, value, TO_VALUES+ (index * (int) sizeof(LeafRecord)), strlen(value), 1);
}
void SetLeafRecord(Offset node_offset, int index, LeafRecord r) {
	SetLeafKey(node_offset, index, r.key);
	SetValue  (node_offset, index, r.value);
}
void SetIntrRecord(Offset node_offset, int index, IntrRecord r) {
	SetIntrKey(node_offset, index, r.key);
	SetChild(node_offset, index + 1, r.offset);
}
void SetKey(Offset node_offset, int index, int64_t value) {
	switch (GetNodeType(node_offset)) {
	case PAGE_INTERNAL:		SetIntrKey(node_offset, index, value);	break;
	case PAGE_LEAF:			SetLeafKey(node_offset, index, value);	break;
	default:				printf("SetKey() err. can't know what type of page is\n");
	}	
}  // for internal node

Offset GetNextFreePage(Offset node_offset) { return GetOffsetOnDB(node_offset, TO_NEXT_FREE_PAGE_OFFSET); }
Offset GetParentPage(Offset node_offset) { return GetOffsetOnDB(node_offset, TO_PARENT_PAGE_OFFSET); }
Offset GetRightSibling(Offset node_offset) { return GetOffsetOnDB(node_offset, TO_RIGHT_SIBLING_OFFSET); }
int32_t GetNodeType(Offset node_offset) { return GetInt32OnDB(node_offset, TO_IS_LEAF); }
int32_t GetKeyNum(Offset node_offset) { return GetInt32OnDB(node_offset, TO_KEY_NUM); }
int64_t GetHeadersPageNum() { return GetInt64OnDB(ADDR_HEADER, TO_PAGE_NUM); }
Offset GetHeadersRootPage() { return GetOffsetOnDB(ADDR_HEADER, TO_ROOT_PAGE_OFFSET); }
Offset  GetChild(Offset node_offset, int index) { return GetOffsetOnDB(node_offset, TO_CHILDREN + index * (int) sizeof(IntrRecord)); }  // for internal node
int64_t GetIntrKey(Offset node_offset, int index) { return GetInt64OnDB(node_offset, TO_KEYS + index * (int) sizeof(IntrRecord)); }
int64_t GetLeafKey(Offset node_offset, int index) { return GetInt64OnDB(node_offset, TO_KEYS + index * (int) sizeof(LeafRecord)); }
int64_t GetKey(Offset node_offset, int index) {                                                                                 //for both node
	switch (GetNodeType(node_offset)) {
	case PAGE_INTERNAL:	return GetIntrKey(node_offset, index);
	case PAGE_LEAF:		return GetLeafKey(node_offset, index);
	default:			printf("GetKey() ERROR\n");
	}
}
LeafRecord * GetLeafRecordPtr(Offset node_offset, int index) {
	LeafRecord* value = (LeafRecord*)malloc (sizeof(LeafRecord) ) ;
	value->key	 = GetLeafKey(node_offset, index);
	GetLeafValue(node_offset, value->value, index);
	return value;
}
IntrRecord * GetIntrRecordPtr(Offset node_offset, int index) {
	IntrRecord* value = (IntrRecord*)malloc(sizeof(IntrRecord));
	value->key		= GetIntrKey(node_offset, index);
	value->offset	= GetChild	(node_offset, index + 1);
	return value;
}
void GetLeafValue(Offset node_offset, char* buff, int index) {
	int64_t pos = node_offset + TO_VALUES + (index * (int) sizeof(LeafRecord));
	printf("getting Leaf value at 0x%llx\n", pos);
	GetVoidValOnDB	   (node_offset, buff, TO_VALUES + index * sizeof(LeafRecord), 1, VALUE_SIZE);
}
char* GetValuePtr(Offset node_offset, int index) {
	return (char*)GetVoidPtrOnDB(node_offset, TO_VALUES + index * sizeof(LeafRecord), 1, VALUE_SIZE);
}

Offset GetNewPage() {
	Offset ret = GetNextFreePage(ADDR_HEADER);
	if (ret == NULL_PAGE) {
		MoreFreePage();
		ret = GetNextFreePage(ADDR_HEADER);
	}

	Offset next = GetNextFreePage(ret);
	SetNextFreePage(ADDR_HEADER, next);
	return ret;
}



void MoreFreePage() {
	//printf("debug| MoreFreePage() called\n");
	int64_t page_num = GetHeadersPageNum();
	Offset newpage_pos = page_num * PAGE_SIZE;
	//printf("debug| B4change : headers pagenum was %lld\n", page_num);


	Offset page_cur = ADDR_HEADER;                  //search from header
	Offset page_next = GetNextFreePage(page_cur);
	while (page_next != (Offset)NULL_PAGE) {		//while next page is not empty; if no free page, this loop not used
		page_cur = page_next;
		page_next = GetNextFreePage(page_cur);
	}                                              //now page_cur is last free page(or header)
	SetNextFreePage(page_cur, newpage_pos);			// set freepage 's liked list

	//printf("debug| set newpage as offset=0x%lx, num=%d\n",(unsigned long)newpage_pos, (int)newpage_pos / PAGE_SIZE);

	int64_t limit = page_num + FREEPAGE_ADD_UNIT;
	int64_t count = page_num;

	fflush(dbfile);

	for (; count < limit; count++) {
		newpage_pos = count * PAGE_SIZE;

		fseek(dbfile, newpage_pos, SEEK_SET);
		fwrite(empty_page, PAGE_SIZE, 1, dbfile);

		SetNextFreePage(newpage_pos, newpage_pos + PAGE_SIZE);
	}
	SetNextFreePage(newpage_pos, NULL_PAGE);

	SetHeadersPageNum(limit);
	//printf("debug|More Free Page() end. lim was 0x%llx == %lldth\n", limit * PAGE_SIZE, limit);
	//printf("debug|heder's page num == %lld\n", GetHeadersPageNum());

}



void FileInit(FILE* dbfile) {
	int i;

	for (i = 0; i < PAGE_SIZE; i++) {
		empty_page[i] = 0x00;
	}
	fseek(dbfile, 0, SEEK_END);
	fwrite(empty_page, PAGE_SIZE, 1, dbfile);
	SetNextFreePage(ADDR_HEADER, NULL_PAGE);
	SetHeadersRootPage(NULL_PAGE);
	SetHeadersPageNum(1);

	MoreFreePage();
}
