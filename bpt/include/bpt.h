#ifndef __BPT_H__
#define __BPT_H__

//#define _CRT_SECURE_NO_WARNINGS


// Uncomment the line below if you are compiling on Windows.
// #define WINDOWS
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#ifdef WINDOWS
#define bool char
#define false 0
#define true 1
#endif

// Default order is 4.
#define DEFAULT_ORDER 32

// Minimum order is necessarily 3.  We set the maximum
// order arbitrarily.  You may change the maximum order.
#define MIN_ORDER 3
#define MAX_ORDER 32

// Constants for printing part or all of the GPL license.
#define LICENSE_FILE "LICENSE.txt"
#define LICENSE_WARRANTEE 0
#define LICENSE_WARRANTEE_START 592
#define LICENSE_WARRANTEE_END 624
#define LICENSE_CONDITIONS 1
#define LICENSE_CONDITIONS_START 70
#define LICENSE_CONDITIONS_END 625


// Constants for DB file
#define FREEPAGE_ADD_UNIT       16
#define PAGE_SIZE               4096 //64 for debug
#define RECORD_SIZE             128
#define VALUE_SIZE              120
#define INTERNAL_RECORD_SIZE    16

#define LEAF_DEGREE             3//for debug //31
#define INTR_DEGREE             3//for debug//248

#define ADDR_HEADER             0
#define NULL_PAGE               0
enum {
	PAGE_FREE = 0,
	PAGE_INTERNAL,
	PAGE_LEAF,
	PAGE_HEADER
};

//#define DEF_INTERNAL            0
//#define DEF_LEAF                1
#define SUCCESS                 0

#define TO_NEXT_FREE_PAGE_OFFSET 0      // Header   | Free    use it
#define TO_PARENT_PAGE_OFFSET    0      // Internal | Leaf
#define TO_ROOT_PAGE_OFFSET      8      // Header
#define TO_IS_LEAF               8      // Internal | Leaf
#define TO_KEY_NUM               12     // Internal | Leaf
#define TO_PAGE_NUM              16     // Header
#define TO_CHILDREN              120    // Internal
#define TO_RIGHT_SIBLING_OFFSET  120    // Leaf
#define TO_KEYS                  128    // Leaf
#define TO_VALUES                136    // Leaf




// TYPES.

/* Type representing the record
* to which a given key refers.
* In a real B+ tree system, the
* record would hold data (in a database)
* or a file (in an operating system)
* or some other information.
* Users can rewrite this part of the code
* to change the type and content
* of the value field.
*/
typedef unsigned long Offset;
typedef struct IntrRecord {
	int64_t key;            //not support internal node
	Offset offset;
} IntrRecord;
typedef struct LeafRecord {      //leaf node's record
	int64_t key;            //not support internal node
	char value[VALUE_SIZE];
}LeafRecord;

typedef struct record {
	int value;
} record;

/* Type representing a node in the B+ tree.
* This type is general enough to serve for both
* the leaf and the internal node.
* The heart of the node is the array
* of keys and the array of corresponding
* pointers.  The relation between keys
* and pointers differs between leaves and
* internal nodes.  In a leaf, the index
* of each key equals the index of its corresponding
* pointer, with a maximum of order - 1 key-pointer
* pairs.  The last pointer points to the
* leaf to the right (or NULL in the case
* of the rightmost leaf).
* In an internal node, the first pointer
* refers to lower nodes with keys less than
* the smallest key in the keys array.  Then,
* with indices i starting at 0, the pointer
* at i + 1 points to the subtree with keys
* greater than or equal to the key in this
* node at index i.
* The num_keys field is used to keep
* track of the number of valid keys.
* In an internal node, the number of valid
* pointers is always num_keys + 1.
* In a leaf, the number of valid pointers
* to data is always num_keys.  The
* last leaf pointer points to the next leaf.
*/


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
//extern int order;

/* The queue is used to print the tree in
* level order, starting from the root
* printing each entire rank on a separate
* line, finishing with the leaves.
*/
//extern node * queue;

/* The user can toggle on and off the "verbose"
* property, which causes the pointer addresses
* to be printed out in hexadecimal notation
* next to their corresponding keys.
*/
//extern bool verbose_output;



/* Only one file opened as main DB
* to prevent using this file pointer on every read/write, It became global
*/
extern FILE* dbfile;
extern int debug_enable;

char empty_page[PAGE_SIZE];

// FUNCTION PROTOTYPES.

// Output and utility.
void PrintTree();
/*
void enqueue( node * new_node );
node * dequeue( void );
int height( node * root );
int path_to_root( node * root, node * child );
void print_leaves( node * root );
void print_tree( node * root );
/void find_and_print(node * root, int key, bool verbose);
void find_and_print_range(node * root, int range1, int range2, bool verbose);
int find_range( node * root, int key_start, int key_end, bool verbose,
int returned_keys[], void * returned_pointers[]);
node * find_leaf( node * root, int key, bool verbose );
record * find( node * root, int key, bool verbose );
int cut( int length );
*/
// Insertion.
//int get_left_index(node * parent, node * left);

Offset make_node(void);
Offset make_leaf(void);

int insert_into_leaf_after_splitting(Offset leaf, LeafRecord r);
int insert_into_new_root(Offset cur_child, Offset new_child);
int insert_into_parent(Offset node, Offset cur_child, Offset new_child);



//under here, my funtions
int open_db(char file_name[]);
void FileInit(FILE* fp);
Offset GetNewPage();
void MoreFreePage();
void SetInstancesOnDB(Offset node_offset, void* value, int instance_pos, size_t size, size_t count);
Offset GetOffsetOnDB(Offset node_offset, int instance_pos);
int32_t GetInt32OnDB(Offset node_offset, int instance_pos);

void SetNextFreePage(Offset node_offset, Offset value);
void SetParentPage(Offset node_offset, Offset value);
void SetRightSibling(Offset node_offset, Offset value);
void SetNodeType(Offset node_offset, int32_t value);
void SetKeyNum(Offset node_offset, int32_t value);
void SetHeadersPageNum(int64_t value);
void SetHeadersRootPage(Offset value);
void SetLeafRecord(Offset node_offset, int index, LeafRecord r);
void SetIntrRecord(Offset node_offset, int index, IntrRecord r);
void SetChild(Offset node_offset, int index, Offset value);
void SetKey(Offset node_offset, int index, int64_t value);
void SetIntrKey(Offset node_offset, int index, int64_t value);
void SetLeafKey(Offset node_offset, int index, int64_t value);
void SetValue(Offset node_offset, int index, char* value);



Offset GetNextFreePage(Offset node_offset);
Offset GetParentPage(Offset node_offset);
Offset GetRightSibling(Offset node_offset);
int32_t GetNodeType(Offset node_offset);
int32_t GetKeyNum(Offset node_offset);
int64_t GetHeadersPageNum();
Offset GetHeadersRootPage();
Offset  GetChild(Offset node_offset, int index);
int64_t GetKey(Offset node_offset, int index);
int64_t GetLeafKey(Offset node_offset, int index);
int64_t GetIntrKey(Offset node_offset, int index);
void GetLeafValue(Offset node_offset, char alloced[], int index);
char* GetValuePtr(Offset node_offset, int index);
LeafRecord * GetLeafRecordPtr(Offset node_offset, int index);
IntrRecord * GetIntrRecordPtr(Offset node_offset, int index);



#endif /* __BPT_H__*/
