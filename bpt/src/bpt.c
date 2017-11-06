/*
*  bpt.c
*/
#define Version "1.14"
#include "bpt.h"
#include <stdio.h>


FILE * dbfile = NULL;

// FUNCTION DEFINITIONS.

// OUTPUT AND UTILITIES
typedef struct Node {
	Offset offset;
	struct Node* next;
} Node;


void enqueue(Node ** q_pointer, Offset new_offset) {

	Node * c;
	Node * q = *q_pointer;
	Node * new_node = (Node *)malloc(sizeof(Node));
	new_node->offset = new_offset;

	if (q == NULL) {
		q = new_node;
		q->next = NULL;

	}
	else {
		c = q;
		while (c->next != NULL) {
			c = c->next;
		}
		c->next = new_node;
		new_node->next = NULL;
	}
	return;
}

Offset dequeue(Node ** q_pointer) {
	Node* q = *q_pointer;
	Node * n = q;

	if (q == NULL) {
		return NULL_PAGE;
	}
	q = q->next;

	Offset ret = n->offset;
	free(n);

	return ret;
}

void PrintTree() {
	Node * q1 = NULL;
	Node * q2 = NULL;
	enqueue(&q1, GetHeadersRootPage());
	Offset ofs;

	// this while loop : move 1 depth down
	while (q1 != NULL) {

		// this while loop : move 1 node right
		// nodes of this depth are in q1
		// delete them and enque next depth nodes in q2
		ofs = -1;
		while (ofs != NULL_PAGE) {
			ofs = dequeue(&q1);

			int i;
			int key_num = GetKeyNum(ofs);
			bool is_leaf = (GetIsLeaf(ofs) == DEF_LEAF);

			if (is_leaf) {
				enqueue(&q2, GetChild(ofs, 0));
			}


			printf("[");
			for (i = 0; i < key_num; i++) {
				if (is_leaf == false) {
					enqueue(&q2, GetChild(ofs, i + 1));
				}
				printf("%ld,", GetKey(ofs, i));
			}
			printf("]");
			//now this node is done.
			//move to next node
		}
		q1 = q2;
		q2 = NULL;
		//now this depth is done
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

	while (GetIsLeaf(c) == DEF_INTERNAL) {
		i = 0;
		int key_num = GetKeyNum(c);
		while (i < key_num) {
			if (GetKey(c, i) < key) {
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


/* Finds and returns the record to which
* a key refers.
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
	}
	else {
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


/* Creates a new general node, which can be adapted
* to serve as either a leaf or an internal node.
*/
Offset make_node(void) {
	Offset ret = GetNewPage();
	if (ret == NULL_PAGE) {
		perror("Node creation.");
		exit(EXIT_FAILURE);
	}
	SetIsLeaf(ret, DEF_INTERNAL);
	SetKeyNum(ret, 0);
	SetParentPage(ret, NULL_PAGE);
	SetRightSibling(ret, NULL_PAGE);
	return ret;
}

Offset make_leaf() {
	Offset ret = make_node();
	SetIsLeaf(ret, DEF_LEAF);
	return ret;
}

/* Inserts a new pointer to a record and its corresponding
* key into a leaf.
* Returns the altered leaf.
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
		SetLeafRecord(leaf, i, *move);
	}
	SetLeafRecord(leaf, insert_index, r);
	SetKeyNum(leaf, key_num + 1);
	return SUCCESS;
}


/* Inserts a new key and pointer
* to a new record into a leaf so as to exceed
* the tree's order, causing the leaf to be split
* in half.
*/
int insert_into_leaf_after_splitting(Offset leaf, LeafRecord r) {
	int insert_index, split, i, j;
	int64_t key = r.key;
	Offset new_leaf;
	LeafRecord * temp_records[LEAF_DEGREE];
	LeafRecord * new_record = (LeafRecord *)malloc(sizeof(LeafRecord));;
	new_record->key = r.key;
	for (i = 0; i < VALUE_SIZE; i++) {
		new_record->value[i] = r.value[i];
	}


	insert_index = 0;
	while (insert_index < LEAF_DEGREE - 1 && GetKey(leaf, insert_index) < key) {
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

	split = cut(LEAF_DEGREE - 1);
	for (i = 0; i < split; i++) {
		SetLeafRecord(leaf, i, *temp_records[i]);
		free(temp_records[i]);
	}
	SetKeyNum(leaf, split);

	new_leaf = make_leaf();
	SetKeyNum(new_leaf, key_num + 1 - split);
	for (i = split; i < key_num + 1; i++) {
		SetLeafRecord(leaf, i - split, *temp_records[i]);
		free(temp_records[i]);
	}


	SetRightSibling(new_leaf, GetRightSibling(leaf));
	SetRightSibling(leaf, new_leaf);


	Offset parent = GetParentPage(leaf);

	int ret = insert_into_parent(parent, leaf, new_leaf);
	return ret;
}


/* Inserts a new key and pointer to a node
* into a node into which these can fit
* without violating the B+ tree properties.

node * insert_into_node(node * root, node * n,
	int left_index, int key, node * right) {
	int i;

	for (i = n->key_num; i > left_index; i--) {
		n->pointers[i + 1] = n->pointers[i];
		n->keys[i] = n->keys[i - 1];
	}
	n->pointers[left_index + 1] = right;
	n->keys[left_index] = key;
	n->key_num++;
	return root;
}
*/


/* Inserts a new node (leaf or internal node) into the B+ tree.
* Returns the root of the tree after insertion.
*/
int insert_into_parent(Offset node, Offset cur_child, Offset new_child) {
	int64_t      new_key = GetKey(new_child, 0);
	IntrRecord * new_record = (IntrRecord *) malloc(sizeof(IntrRecord));
	new_record->key = new_key;
	new_record->offset = new_child;

	if (node == NULL_PAGE) {
		return insert_into_new_root(cur_child, new_child);
	}

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
		SetKeyNum(node, key_num + 1);
		return SUCCESS;                 // jobs done

	}
	else {
		// ------case---------------------------case-------
		//		    no space for new_child node
		//	split this node and send middle value to parent
		// ------case---------------------------case-------
		int j;							//      : need to split again.
		IntrRecord* temp_records[INTR_DEGREE];
		for (i = j = 0; i < key_num; i++, j++) {
			if (i == insert_index) {
				temp_records[j] = new_record;
				j++;
			}
			temp_records[j] = GetIntrRecordPtr(node, i);
		}
		int split = cut(INTR_DEGREE - 1);
		for (i = 0; i < split; i++) {
			SetIntrRecord(node, i, *temp_records[i]);
			free(temp_records[i]);
		}
		SetKeyNum(node, split);         // original node contains half

		Offset new_leaf = make_node();
		SetKeyNum(new_leaf, key_num + 1 - split);
		for (i = split; i < key_num + 1; i++) {
			SetIntrRecord(node, i - split, *temp_records[i]);
			free(temp_records[i]);
		}


		Offset parent = GetParentPage(node);

		int ret = insert_into_parent(parent, node, new_leaf);

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
	SetKey  (root, 0, GetKey(new_child, 0));
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
int insert(int key, char value[VALUE_SIZE]) {
	int i;
	LeafRecord r;
	r.key = key;
	for(i = 0; i < VALUE_SIZE; i++)
		r.value[i] = value[i];  //rec will be only use in this function.
	Offset leaf;
	
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

	if (key_num < LEAF_DEGREE) {   // Case: leaf has room for key and pointer.
		leaf = insert_into_leaf(leaf, r);

	}
	else {                            // Case:  leaf must be split.
		insert_into_leaf_after_splitting(leaf, r);
	}
	return SUCCESS;
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
/*
//these code is in bpt.h
#define FIRST_ASSIGNE_PAGE_NUM  16
#define PAGE_SIZE               4096
#define (int) sizeof(LeafRecord)             256

#define TO_NEXT_FREE_PAGE_OFFSET 0      // Header   | Free    use it
#define TO_PARENT_PAGE_OFFSET    0      // Internal | Leaf
#define TO_ROOT_PAGE_OFFSET      8      // Header
#define TO_IS_LEAF               8      // Internal | Leaf
#define TO_KEY_NUM               12     // Internal | Leaf
#define TO_RIGHT_SIBLING_OFFSET  120    // Leaf
#define TO_KEYS                  128    // Leaf
#define TO_VALUES                136    // Leaf
#define TO_LEFT_CHILD_OFFSET     120
*/


//TODO : make it work without OS's buffering
void SetInstancesOnDB(Offset node_offset, void* value, int instance_pos, size_t size, size_t count) {
	int sum = 0;
	int add = 0;
	int wait_counter;
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
	printf("debug| start GetOffsetOnDB(offset=0x%lx, pos=%d)\n",
		(unsigned long)node_offset, instance_pos);
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
void * GetVoidPtrOnDB(Offset node_offset, int instance_pos, size_t size, size_t count) {
	char * value = (char * ) malloc(size * count);		// to move 1 Byte per ++, define value as char pointer
	fseek(dbfile, node_offset + instance_pos, SEEK_SET);
	int sum = 0, add = 0;
	while (sum < (int) count) {
		add = fread(value + (sum * size), size, count - sum, dbfile);
		sum += add;
		if (add < 0) {
			printf("ERROR: GetVoidPtrOnDB(node=%u, pos = %d) -> return %u\n",
				(unsigned int)node_offset, instance_pos, (unsigned int)value[0]);
		}
	}
	return (void *)value;
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
void SetNextFreePage(Offset node_offset, Offset value)	{ SetInstancesOnDB(node_offset, &value, TO_NEXT_FREE_PAGE_OFFSET, sizeof(Offset), 1); }
void SetParentPage	(Offset node_offset, Offset value)	{ SetInstancesOnDB(node_offset, &value, TO_PARENT_PAGE_OFFSET, sizeof(Offset), 1); }
void SetRightSibling(Offset node_offset, Offset value)	{ SetInstancesOnDB(node_offset, &value, TO_RIGHT_SIBLING_OFFSET, sizeof(Offset), 1); }
void SetIsLeaf		(Offset node_offset, int32_t value)	{ SetInstancesOnDB(node_offset, &value, TO_IS_LEAF, sizeof(int32_t), 1); }
void SetKeyNum		(Offset node_offset, int32_t value)	{ SetInstancesOnDB(node_offset, &value, TO_KEY_NUM, sizeof(int32_t), 1); }
void SetChild(Offset node_offset, int index, Offset value) { SetInstancesOnDB(node_offset, &value, TO_CHILDREN + index * (int) sizeof(IntrRecord), (int) sizeof(Offset ), 1); }  // for internal node
void SetKey	 (Offset node_offset, int index,int64_t value) { SetInstancesOnDB(node_offset, &value, TO_KEYS	   + index * (int) sizeof(IntrRecord), (int) sizeof(int64_t), 1); }  // for internal node
void SetHeadersPageNum(int64_t value)	{ SetInstancesOnDB(ADDR_HEADER, &value, TO_PAGE_NUM, sizeof(int64_t), 1); }
void SetHeadersRootPage(Offset value)	{ SetInstancesOnDB(ADDR_HEADER, &value, TO_ROOT_PAGE_OFFSET, sizeof(Offset), 1); }

void SetLeafRecord(Offset node_offset, int index, LeafRecord r) {
	SetInstancesOnDB(node_offset, &r, TO_KEYS + index * (int) sizeof(LeafRecord), sizeof(LeafRecord),1);
}
void SetIntrRecord(Offset node_offset, int index, IntrRecord r) {
	SetInstancesOnDB(node_offset, &r, TO_KEYS + index * (int) sizeof(IntrRecord), sizeof(IntrRecord), 1);
}


Offset GetNextFreePage	(Offset node_offset){ return GetOffsetOnDB(node_offset, TO_NEXT_FREE_PAGE_OFFSET); }
Offset GetParentPage	(Offset node_offset){ return GetOffsetOnDB(node_offset, TO_PARENT_PAGE_OFFSET); }
Offset GetRightSibling	(Offset node_offset){ return GetOffsetOnDB(node_offset, TO_RIGHT_SIBLING_OFFSET); }
int32_t GetIsLeaf		(Offset node_offset){ return GetInt32OnDB(node_offset, TO_IS_LEAF); }
int32_t GetKeyNum		(Offset node_offset){ return GetInt32OnDB(node_offset, TO_KEY_NUM); }
int64_t GetHeadersPageNum() { return GetInt64OnDB(ADDR_HEADER, TO_PAGE_NUM); }
Offset GetHeadersRootPage() { return GetOffsetOnDB(ADDR_HEADER, TO_ROOT_PAGE_OFFSET); }

Offset  GetChild(Offset node_offset, int index) { return GetOffsetOnDB(node_offset, TO_CHILDREN + index * (int) sizeof(IntrRecord)); }  // for internal node
int64_t GetKey(Offset node_offset, int index) {                                                                                 //for both node
	switch (GetIsLeaf(node_offset)) {
	case DEF_INTERNAL:
		return GetInt64OnDB(node_offset, TO_KEYS + index * (int) sizeof(IntrRecord));
	case DEF_LEAF:
		return GetInt64OnDB(node_offset, TO_KEYS + index * (int) sizeof(LeafRecord));
	default:
		printf("error! GetKey(Offset= 0x%lx(pagenum = %d), i); on this node, IsLeaf value is strange. not sure where to read\n",
			node_offset, (int) (node_offset / PAGE_SIZE));
		exit(1);
	}
}
LeafRecord * GetLeafRecordPtr(Offset node_offset, int index) {
	return (LeafRecord*)GetVoidPtrOnDB(node_offset, TO_KEYS + index * sizeof(LeafRecord), sizeof(LeafRecord), 1);
}
IntrRecord * GetIntrRecordPtr(Offset node_offset, int index){
	return (IntrRecord*)GetVoidPtrOnDB(node_offset, TO_KEYS + index * sizeof(IntrRecord), sizeof(IntrRecord), 1);
}
char* GetValuePtr(Offset node_offset, int index) {
	return (char*)		GetVoidPtrOnDB(node_offset, TO_VALUES + index * sizeof(IntrRecord), 1, VALUE_SIZE);
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
	printf("debug| MoreFreePage() called\n");
	int32_t page_num = GetHeadersPageNum();
	int32_t newpage_pos = page_num * PAGE_SIZE;
	printf("debug| B4change : headers pagenum was %d\n", page_num);


	Offset page_cur = ADDR_HEADER;                  //search from header
	Offset page_next = GetNextFreePage(page_cur);
	while (page_next != (Offset)NULL_PAGE) {		//while next page is not empty; if no free page, this loop not used
		page_cur = page_next;
		page_next = GetNextFreePage(page_cur);
	}                                              //now page_cur is last free page(or header)
	SetNextFreePage(page_cur, newpage_pos);			// set freepage 's liked list
	
	printf("debug| set newpage as offset=0x%lx, num=%d\n",
		(unsigned long)newpage_pos, (int)newpage_pos / PAGE_SIZE);

	const int limit = page_num + FREEPAGE_ADD_UNIT;
	int count = page_num;

	fflush(dbfile);
	int temp;
	scanf("%d", &temp);

	for (; count < limit; count++) {
		newpage_pos = count * PAGE_SIZE;

		fseek(dbfile, newpage_pos, SEEK_SET);
		fwrite(empty_page, PAGE_SIZE, 1, dbfile);

		SetNextFreePage(newpage_pos, newpage_pos + PAGE_SIZE);
	}
	SetNextFreePage(newpage_pos, NULL_PAGE);

	SetHeadersPageNum(limit);
	printf("debug|More Free Page() end. lim was 0x%x == %dth\n", limit * PAGE_SIZE, limit);
	printf("debug|heder's page num == %d\n", GetHeadersPageNum());
	
}



void FileInit(FILE* dbfile) {
	int i;
	
	for (i = 0; i < PAGE_SIZE; i++) {
		empty_page[i] = 0xe;
	}
	fseek(dbfile, 0, SEEK_END);
	fwrite(empty_page, PAGE_SIZE, 1, dbfile);
	SetNextFreePage(ADDR_HEADER, NULL_PAGE);	
	SetHeadersRootPage(NULL_PAGE);
	SetHeadersPageNum(1);

	MoreFreePage();
}