/*
*  bpt.c
*/
#define Version "1.14"
#include "bpt.h"

// GLOBALS.

/* The order determines the maximum and minimum
* number of entries (keys and pointers) in any
* node.  Every node has at most order - 1 keys and
* at least (roughly speaking) half that number.
* Every leaf has as many pointers to data as keys,
* and every internal node has one more pointer
* to a subtree than the number of keys.
* This global variable is initialized to the
* default value.
*/
int order = DEFAULT_ORDER;

/* The queue is used to print the tree in
* level order, starting from the root
* printing each entire rank on a separate
* line, finishing with the leaves.
*/
node * queue = NULL;

/* The user can toggle on and off the "verbose"
* property, which causes the pointer addresses
* to be printed out in hexadecimal notation
* next to their corresponding keys.
*/
bool verbose_output = false;

/* Only one file opened as main DB
* to prevent using this file pointer on every read/write, It became global
*/
FILE * dbfile = NULL;

// FUNCTION DEFINITIONS.

// OUTPUT AND UTILITIES


/* Helper function for printing the
* tree out.  See print_tree.
*/
void enqueue(node * new_node) {
	node * c;
	if (queue == NULL) {
		queue = new_node;
		queue->next = NULL;
	}
	else {
		c = queue;
		while (c->next != NULL) {
			c = c->next;
		}
		c->next = new_node;
		new_node->next = NULL;
	}
}


/* Helper function for printing the
* tree out.  See print_tree.
*/
node * dequeue(void) {
	node * n = queue;
	queue = queue->next;
	n->next = NULL;
	return n;
}


/* Prints the bottom row of keys
* of the tree (with their respective
* pointers, if the verbose_output flag is set.
*/
void print_leaves(node * root) {
	int i;
	node * c = root;
	if (root == NULL) {
		printf("Empty tree.\n");
		return;
	}
	while (!c->is_leaf)
		c = c->pointers[0];
	while (true) {
		for (i = 0; i < c->num_keys; i++) {
			if (verbose_output)
				printf("%lx ", (unsigned long)c->pointers[i]);
			printf("%d ", c->keys[i]);
		}
		if (verbose_output)
			printf("%lx ", (unsigned long)c->pointers[order - 1]);
		if (c->pointers[order - 1] != NULL) {
			printf(" | ");
			c = c->pointers[order - 1];
		}
		else
			break;
	}
	printf("\n");
}


/* Utility function to give the height
* of the tree, which length in number of edges
* of the path from the root to any leaf.
*/
int height(node * root) {
	int h = 0;
	node * c = root;
	while (!c->is_leaf) {
		c = c->pointers[0];
		h++;
	}
	return h;
}


/* Utility function to give the length in edges
* of the path from any node to the root.
*/
int path_to_root(node * root, node * child) {
	int length = 0;
	node * c = child;
	while (c != root) {
		c = c->parent;
		length++;
	}
	return length;
}



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
	int64_t i = 0;
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
	int num_keys = GetKeyNum(leaf);
	while (i < num_keys && GetKey(leaf, i) < r.key) {
		i++;
	}
	LeafRecord* move = NULL;
	int insert_index = i;
	for (i = num_keys; insert_index < i; i--) {
		move = GetLeafRecordPtr(leaf, i - 1);
		SetLeafRecord(leaf, i, *move);
	}
	SetLeafRecord(leaf, insert_index, r);
	SetKeyNum(leaf, num_keys + 1);
	return SUCCESS;
}


/* Inserts a new key and pointer
* to a new record into a leaf so as to exceed
* the tree's order, causing the leaf to be split
* in half.
*/
int insert_into_leaf_after_splitting(Offset leaf, int key, LeafRecord r) {
	int insert_index, split, new_key, i, j;
	Offset new_leaf;
	LeafRecord * temp_records[LEAF_DEGREE];
	LeafRecord * new_record = (LeafRecord *)malloc(sizeof(LeafRecord));;
	new_record->key = r.key;
	for (i = 0; i < VALUE_SIZE; i++) {
		new_record->value[i] = r.value[i];
	}


	insert_index = 0;
	while (insert_index < order - 1 && GetKey(leaf, insert_index) < key) {
		insert_index++;
	}

	int num_keys = GetKeyNum(leaf);
	for (i = j = 0; i < num_keys; i++, j++) {
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
	SetKeyNum(new_leaf, num_keys + 1 - split);
	for (i = split; i < num_keys + 1; i++) {
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
*/
node * insert_into_node(node * root, node * n,
	int left_index, int key, node * right) {
	int i;

	for (i = n->num_keys; i > left_index; i--) {
		n->pointers[i + 1] = n->pointers[i];
		n->keys[i] = n->keys[i - 1];
	}
	n->pointers[left_index + 1] = right;
	n->keys[left_index] = key;
	n->num_keys++;
	return root;
}


/* Inserts a new key and pointer to a node
* into a node, causing the node's size to exceed
* the order, and causing the node to split into two.
*/
int insert_into_node_after_splitting(node * root, node * old_node, int left_index,
	int key, node * right) {

	int i, j, split, k_prime;
	node * new_node, *child;
	int * temp_keys;
	node ** temp_pointers;

	/* First create a temporary set of keys and pointers
	* to hold everything in order, including
	* the new key and pointer, inserted in their
	* correct places.
	* Then create a new node and copy half of the
	* keys and pointers to the old node and
	* the other half to the new.
	*/

	temp_pointers = malloc((order + 1) * sizeof(node *));
	if (temp_pointers == NULL) {
		perror("Temporary pointers array for splitting nodes.");
		exit(EXIT_FAILURE);
	}
	temp_keys = malloc(order * sizeof(int));
	if (temp_keys == NULL) {
		perror("Temporary keys array for splitting nodes.");
		exit(EXIT_FAILURE);
	}

	for (i = 0, j = 0; i < old_node->num_keys + 1; i++, j++) {
		if (j == left_index + 1) j++;
		temp_pointers[j] = old_node->pointers[i];
	}

	for (i = 0, j = 0; i < old_node->num_keys; i++, j++) {
		if (j == left_index) j++;
		temp_keys[j] = old_node->keys[i];
	}

	temp_pointers[left_index + 1] = right;
	temp_keys[left_index] = key;

	/* Create the new node and copy
	* half the keys and pointers to the
	* old and half to the new.
	*/
	split = cut(order);
	new_node = make_node();
	old_node->num_keys = 0;
	for (i = 0; i < split - 1; i++) {
		old_node->pointers[i] = temp_pointers[i];
		old_node->keys[i] = temp_keys[i];
		old_node->num_keys++;
	}
	old_node->pointers[i] = temp_pointers[i];
	k_prime = temp_keys[split - 1];
	for (++i, j = 0; i < order; i++, j++) {
		new_node->pointers[j] = temp_pointers[i];
		new_node->keys[j] = temp_keys[i];
		new_node->num_keys++;
	}
	new_node->pointers[j] = temp_pointers[i];
	free(temp_pointers);
	free(temp_keys);
	new_node->parent = old_node->parent;
	for (i = 0; i <= new_node->num_keys; i++) {
		child = new_node->pointers[i];
		child->parent = new_node;
	}

	/* Insert a new key into the parent of the two
	* nodes resulting from the split, with
	* the old node to the left and the new to the right.
	*/

	return insert_into_parent(root, old_node, k_prime, new_node);
}



/* Inserts a new node (leaf or internal node) into the B+ tree.
* Returns the root of the tree after insertion.
*/
int insert_into_parent(Offset node, Offset cur_child, Offset new_child) {
	int64_t      new_key = GetKey(new_child, 0);
	IntrRecord * new_record = (IntrRecord *) malloc(sizeof(IntrRecord));
	new_record->key = new_key;
	new_record->offset = new_child;

	if (node == NULL_PAGE) {
		Offset noe_header = make_node();
		//root node splited. new root needed 
		//TODO : find some proper functions here. consider function down.
		//insert_into_new_root(left, key, right);
	}

	int i;
	int num_keys = GetKeyNum(node);
	for (i = 0; i < num_keys; i++) {
		if (new_key <= GetKey(node, i)) {
			break;                  // find place
		}
	}
	int insert_index = i;

	// ------case---------------------------case-------
	//		enough space for new_child node
	//		just put it in this node
	// ------case---------------------------case-------
	if (num_keys < INTR_DEGREE) {
		SetParentPage(new_child, node);  // I'm your father

		IntrRecord* rp = NULL;          // right-shift all nodes after insert_index
		for (i = num_keys; insert_index < i; i--) {
			rp = GetIntrRecordPtr(node, i - 1);
			SetIntrRecord(node, i, *rp);
			free(rp);
		}                               // and insert it
		SetIntrRecord(node, i, *new_record);
		SetKeyNum(node, num_keys + 1);
		return SUCCESS;                 // jobs done

	}
	else {
		// ------case---------------------------case-------
		//		    no space for new_child node
		//	split this node and send middle value to parent
		// ------case---------------------------case-------
		int j;							//      : need to split again.
		IntrRecord* temp_records[INTR_DEGREE];
		for (i = j = 0; i < num_keys; i++, j++) {
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
		SetKeyNum(new_leaf, num_keys + 1 - split);
		for (i = split; i < num_keys + 1; i++) {
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
int insert_into_new_root(node * left, int key, node * right) {




	node * root = make_node();
	root->keys[0] = key;
	root->pointers[0] = left;
	root->pointers[1] = right;
	root->num_keys++;
	root->parent = NULL;
	left->parent = root;
	right->parent = root;
	return SUCCESS;
}



/* First insertion:
* start a new tree.
*/
node * start_new_tree(int key, record * pointer) {

	node * root = make_leaf();
	root->keys[0] = key;
	root->pointers[0] = pointer;
	root->pointers[order - 1] = NULL;
	root->parent = NULL;
	root->num_keys++;
	return root;
}



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
	node * leaf;
	
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
	int num_keys = GetKeyNum(leaf);

	if (num_keys < LEAF_DEGREE) {   // Case: leaf has room for key and pointer.
		leaf = insert_into_leaf(leaf, r);

	}
	else {                            // Case:  leaf must be split.
		insert_into_leaf_after_splitting(leaf, key, r);
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
int get_neighbor_index(node * n) {

	int i;

	/* Return the index of the key to the left
	* of the pointer in the parent pointing
	* to n.
	* If n is the leftmost child, this means
	* return -1.
	*/
	for (i = 0; i <= n->parent->num_keys; i++)
		if (n->parent->pointers[i] == n)
			return i - 1;

	// Error state.
	printf("Search for nonexistent pointer to node in parent.\n");
	printf("Node:  %#lx\n", (unsigned long)n);
	exit(EXIT_FAILURE);
}


node * remove_entry_from_node(node * n, int key, node * pointer) {

	int i, num_pointers;

	// Remove the key and shift other keys accordingly.
	i = 0;
	while (n->keys[i] != key)
		i++;
	for (++i; i < n->num_keys; i++)
		n->keys[i - 1] = n->keys[i];

	// Remove the pointer and shift other pointers accordingly.
	// First determine number of pointers.
	num_pointers = n->is_leaf ? n->num_keys : n->num_keys + 1;
	i = 0;
	while (n->pointers[i] != pointer)
		i++;
	for (++i; i < num_pointers; i++)
		n->pointers[i - 1] = n->pointers[i];


	// One key fewer.
	n->num_keys--;

	// Set the other pointers to NULL for tidiness.
	// A leaf uses the last pointer to point to the next leaf.
	if (n->is_leaf)
		for (i = n->num_keys; i < order - 1; i++)
			n->pointers[i] = NULL;
	else
		for (i = n->num_keys + 1; i < order; i++)
			n->pointers[i] = NULL;

	return n;
}


node * adjust_root(node * root) {

	node * new_root;

	/* Case: nonempty root.
	* Key and pointer have already been deleted,
	* so nothing to be done.
	*/

	if (root->num_keys > 0)
		return root;

	/* Case: empty root.
	*/

	// If it has a child, promote 
	// the first (only) child
	// as the new root.

	if (!root->is_leaf) {
		new_root = root->pointers[0];
		new_root->parent = NULL;
	}

	// If it is a leaf (has no children),
	// then the whole tree is empty.

	else
		new_root = NULL;

	free(root->keys);
	free(root->pointers);
	free(root);

	return new_root;
}


/* Coalesces a node that has become
* too small after deletion
* with a neighboring node that
* can accept the additional entries
* without exceeding the maximum.
*/
node * coalesce_nodes(node * root, node * n, node * neighbor, int neighbor_index, int k_prime) {

	int i, j, neighbor_insert_index, n_end;
	node * tmp;

	/* Swap neighbor with node if node is on the
	* extreme left and neighbor is to its right.
	*/

	if (neighbor_index == -1) {
		tmp = n;
		n = neighbor;
		neighbor = tmp;
	}

	/* Starting point in the neighbor for copying
	* keys and pointers from n.
	* Recall that n and neighbor have swapped places
	* in the special case of n being a leftmost child.
	*/

	neighbor_insert_index = neighbor->num_keys;

	/* Case:  nonleaf node.
	* Append k_prime and the following pointer.
	* Append all pointers and keys from the neighbor.
	*/

	if (!n->is_leaf) {

		/* Append k_prime.
		*/

		neighbor->keys[neighbor_insert_index] = k_prime;
		neighbor->num_keys++;


		n_end = n->num_keys;

		for (i = neighbor_insert_index + 1, j = 0; j < n_end; i++, j++) {
			neighbor->keys[i] = n->keys[j];
			neighbor->pointers[i] = n->pointers[j];
			neighbor->num_keys++;
			n->num_keys--;
		}

		/* The number of pointers is always
		* one more than the number of keys.
		*/

		neighbor->pointers[i] = n->pointers[j];

		/* All children must now point up to the same parent.
		*/

		for (i = 0; i < neighbor->num_keys + 1; i++) {
			tmp = (node *)neighbor->pointers[i];
			tmp->parent = neighbor;
		}
	}

	/* In a leaf, append the keys and pointers of
	* n to the neighbor.
	* Set the neighbor's last pointer to point to
	* what had been n's right neighbor.
	*/

	else {
		for (i = neighbor_insert_index, j = 0; j < n->num_keys; i++, j++) {
			neighbor->keys[i] = n->keys[j];
			neighbor->pointers[i] = n->pointers[j];
			neighbor->num_keys++;
		}
		neighbor->pointers[order - 1] = n->pointers[order - 1];
	}

	root = delete_entry(root, n->parent, k_prime, n);
	free(n->keys);
	free(n->pointers);
	free(n);
	return root;
}


/* Redistributes entries between two nodes when
* one has become too small after deletion
* but its neighbor is too big to append the
* small node's entries without exceeding the
* maximum
*/
node * redistribute_nodes(node * root, node * n, node * neighbor, int neighbor_index,
	int k_prime_index, int k_prime) {

	int i;
	node * tmp;

	/* Case: n has a neighbor to the left.
	* Pull the neighbor's last key-pointer pair over
	* from the neighbor's right end to n's left end.
	*/

	if (neighbor_index != -1) {
		if (!n->is_leaf)
			n->pointers[n->num_keys + 1] = n->pointers[n->num_keys];
		for (i = n->num_keys; i > 0; i--) {
			n->keys[i] = n->keys[i - 1];
			n->pointers[i] = n->pointers[i - 1];
		}
		if (!n->is_leaf) {
			n->pointers[0] = neighbor->pointers[neighbor->num_keys];
			tmp = (node *)n->pointers[0];
			tmp->parent = n;
			neighbor->pointers[neighbor->num_keys] = NULL;
			n->keys[0] = k_prime;
			n->parent->keys[k_prime_index] = neighbor->keys[neighbor->num_keys - 1];
		}
		else {
			n->pointers[0] = neighbor->pointers[neighbor->num_keys - 1];
			neighbor->pointers[neighbor->num_keys - 1] = NULL;
			n->keys[0] = neighbor->keys[neighbor->num_keys - 1];
			n->parent->keys[k_prime_index] = n->keys[0];
		}
	}

	/* Case: n is the leftmost child.
	* Take a key-pointer pair from the neighbor to the right.
	* Move the neighbor's leftmost key-pointer pair
	* to n's rightmost position.
	*/

	else {
		if (n->is_leaf) {
			n->keys[n->num_keys] = neighbor->keys[0];
			n->pointers[n->num_keys] = neighbor->pointers[0];
			n->parent->keys[k_prime_index] = neighbor->keys[1];
		}
		else {
			n->keys[n->num_keys] = k_prime;
			n->pointers[n->num_keys + 1] = neighbor->pointers[0];
			tmp = (node *)n->pointers[n->num_keys + 1];
			tmp->parent = n;
			n->parent->keys[k_prime_index] = neighbor->keys[0];
		}
		for (i = 0; i < neighbor->num_keys - 1; i++) {
			neighbor->keys[i] = neighbor->keys[i + 1];
			neighbor->pointers[i] = neighbor->pointers[i + 1];
		}
		if (!n->is_leaf)
			neighbor->pointers[i] = neighbor->pointers[i + 1];
	}

	/* n now has one more key and one more pointer;
	* the neighbor has one fewer of each.
	*/

	n->num_keys++;
	neighbor->num_keys--;

	return root;
}


/* Deletes an entry from the B+ tree.
* Removes the record and its key and pointer
* from the leaf, and then makes all appropriate
* changes to preserve the B+ tree properties.
*/
node * delete_entry(node * root, node * n, int key, void * pointer) {

	int min_keys;
	node * neighbor;
	int neighbor_index;
	int k_prime_index, k_prime;
	int capacity;

	// Remove key and pointer from node.

	n = remove_entry_from_node(n, key, pointer);

	/* Case:  deletion from the root.
	*/

	if (n == root)
		return adjust_root(root);


	/* Case:  deletion from a node below the root.
	* (Rest of function body.)
	*/

	/* Determine minimum allowable size of node,
	* to be preserved after deletion.
	*/

	min_keys = n->is_leaf ? cut(order - 1) : cut(order) - 1;

	/* Case:  node stays at or above minimum.
	* (The simple case.)
	*/

	if (n->num_keys >= min_keys)
		return root;

	/* Case:  node falls below minimum.
	* Either coalescence or redistribution
	* is needed.
	*/

	/* Find the appropriate neighbor node with which
	* to coalesce.
	* Also find the key (k_prime) in the parent
	* between the pointer to node n and the pointer
	* to the neighbor.
	*/

	neighbor_index = get_neighbor_index(n);
	k_prime_index = neighbor_index == -1 ? 0 : neighbor_index;
	k_prime = n->parent->keys[k_prime_index];
	neighbor = neighbor_index == -1 ? n->parent->pointers[1] :
		n->parent->pointers[neighbor_index];

	capacity = n->is_leaf ? order : order - 1;

	/* Coalescence. */

	if (neighbor->num_keys + n->num_keys < capacity)
		return coalesce_nodes(root, n, neighbor, neighbor_index, k_prime);

	/* Redistribution. */

	else
		return redistribute_nodes(root, n, neighbor, neighbor_index, k_prime_index, k_prime);
}



/* Master deletion function.
*/
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
		for (i = 0; i < root->num_keys; i++)
			free(root->pointers[i]);
	else
		for (i = 0; i < root->num_keys + 1; i++)
			destroy_tree_nodes(root->pointers[i]);
	free(root->pointers);
	free(root->keys);
	free(root);
}


node * destroy_tree(node * root) {
	destroy_tree_nodes(root);
	return NULL;
}


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

	while (sum < count && 0 <= add) {
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
	char * value = (void * ) malloc(size * count);		// to move 1 Byte per ++, define value as char pointer
	fseek(dbfile, node_offset + instance_pos, SEEK_SET);
	int sum = 0, add = 0;
	while (sum < count) {
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
void SetNextFreePage(Offset node_offset, Offset value) { SetInstancesOnDB(node_offset, &value, TO_NEXT_FREE_PAGE_OFFSET, sizeof(Offset), 1); }
void SetParentPage(Offset node_offset, Offset value) { SetInstancesOnDB(node_offset, &value, TO_PARENT_PAGE_OFFSET, sizeof(Offset), 1); }
void SetRightSibling(Offset node_offset, Offset value) { SetInstancesOnDB(node_offset, &value, TO_RIGHT_SIBLING_OFFSET, sizeof(Offset), 1); }
void SetIsLeaf(Offset node_offset, int32_t value) { SetInstancesOnDB(node_offset, &value, TO_IS_LEAF, sizeof(int32_t), 1); }
void SetKeyNum(Offset node_offset, int32_t value) { SetInstancesOnDB(node_offset, &value, TO_KEY_NUM, sizeof(int32_t), 1); }
void SetHeadersPageNum(int32_t value) { SetInstancesOnDB(ADDR_HEADER, &value, TO_PAGE_NUM, sizeof(int32_t), 1); }
void SetHeadersRootPage(Offset value) { SetInstancesOnDB(ADDR_HEADER, &value, TO_ROOT_PAGE_OFFSET, sizeof(Offset), 1); }
void SetLeafRecord(Offset node_offset, int index, LeafRecord r) {
	SetInstancesOnDB(node_offset, &r, TO_KEYS + index * (int) sizeof(LeafRecord), sizeof(LeafRecord),1);
}
void SetIntrRecord(Offset node_offset, int index, IntrRecord r) {
	SetInstancesOnDB(node_offset, &r, TO_KEYS + index * (int) sizeof(IntrRecord), sizeof(IntrRecord), 1);
}


Offset GetNextFreePage(Offset node_offset) { return GetOffsetOnDB(node_offset, TO_NEXT_FREE_PAGE_OFFSET); }
Offset GetParentPage(Offset node_offset) { return GetOffsetOnDB(node_offset, TO_PARENT_PAGE_OFFSET); }
Offset GetRightSibling(Offset node_offset) { return GetOffsetOnDB(node_offset, TO_RIGHT_SIBLING_OFFSET); }
int32_t GetIsLeaf(Offset node_offset) { return GetInt32OnDB(node_offset, TO_IS_LEAF); }
int32_t GetKeyNum(Offset node_offset) { return GetInt32OnDB(node_offset, TO_KEY_NUM); }
int32_t GetHeadersPageNum() { return GetInt32OnDB(ADDR_HEADER, TO_PAGE_NUM); }
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
			node_offset, node_offset / PAGE_SIZE);
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
	int32_t page_num = GetHeadersPageNum();
	int32_t newpage_pos = page_num * PAGE_SIZE;
	printf("debug| B4change : headers pagenum was %d\n", page_num);


	Offset page_cur = ADDR_HEADER;                           //search from header
	Offset page_next = GetNextFreePage(page_cur);
	while (page_next != (Offset)NULL_PAGE) {//while next page is not empty; if no free page, this loop not used
		page_cur = page_next;
		page_next = GetNextFreePage(page_cur);
	}                                              //now page_cur is last free page(or header)

	SetNextFreePage(page_cur, newpage_pos);
	printf("debug| set newpage as offset=0x%lx, num=%d\n",
		(unsigned long)newpage_pos, (int)newpage_pos / PAGE_SIZE);

	const int limit = page_num + FREEPAGE_ADD_UNIT;
	int count = page_num;
	for (; count < limit; count++) {
		newpage_pos = count * PAGE_SIZE;
		SetNextFreePage(newpage_pos, newpage_pos + PAGE_SIZE);
	}
	fseek(dbfile, newpage_pos, SEEK_SET);
	fwrite(empty_page, PAGE_SIZE, 1, dbfile);

	SetHeadersPageNum(limit);
	printf("debug|More Free Page() end. lim was 0x%x == %dth\n", limit * PAGE_SIZE, limit);
	printf("debug|heder's page num == %d\n", GetHeadersPageNum());
}



void FileInit(FILE* dbfile) {
	int i;
	fseek(dbfile, 0, SEEK_END);
	for (i = 0; i < PAGE_SIZE; i++) {
		empty_page[i] = 0;
	}

	SetNextFreePage(ADDR_HEADER, NULL_PAGE);
	SetHeadersRootPage(NULL_PAGE);
	SetHeadersPageNum(1);

	MoreFreePage();



}

