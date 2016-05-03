/**
Interface for Quadtree data structure
*/

#ifndef QUADTREE_H
#define QUADTREE_H

#include <errno.h>
#include <pthread.h>
#include <stdio.h>


#include "types.h"
#include "util.h"
#include "Point.h"

typedef struct DeterministicSkipQuadtreeNode_t Node;
typedef struct Quadtree_t Quadtree;

extern __thread rlu_thread_data_t *rlu_self;

/*
 * struct Quadtree_t
 *
 * Stores header information about the entire quadtree.
 *
 * height - the height of the tree
 * root - the node with no parents with highest height
 * length - the supremum of the L-infinity norm of a Point contained in this Quadtree
 * center - the center of the region covered by this Quadtree
 */
struct Quadtree_t {
    uint64_t height;
    Node *root;
    float64_t length;
    Point center;
};

/*
 * struct DeterministicSkipQuadtreeNode_t
 *
 * Stores information about a node in the quadtree
 *
 * is_square - true if node is a square, false if is a point
 * length - side length of the square. This means that the boundaries are length/2 distance from
 *     the center. Does not matter for a leaf node
 * center - center of the square, or coordinates of the point
 * right - the next node on the same level
 * down - the next node of the node representing the same point as this node's previous node, one
 *     level down
 */
struct DeterministicSkipQuadtreeNode_t {
    bool is_square;
    float64_t length;
    Point center;
    Node *right;
    Node *down;
#ifdef QUADTREE_TEST
    uint64_t id;
#endif
};

/*
 * struct QuadtreeFreeResult_t
 *
 * Contains information about how many total nodes were freed and how many leaf nodes were freed
 * during a freeing operation. In addition, also contains information about how many levels were
 * freed.
 *
 * total - the total number of nodes freed, including intermediate and dirty nodes
 * clean - the total number of non-dirty nodes freed
 * leaf - the number of non-dirty leaf nodes freed
 * levels - the number of non-dirty levels freed
 */
typedef struct QuadtreeFreeResult_t {
    uint64_t total, clean, leaf, levels;
} QuadtreeFreeResult;

#ifdef QUADTREE_TEST
/*
 * Node_init
 *
 * Allocates memory for and initializes an empty leaf node in the quadtree.
 *
 * length - the "length" of the node -- irrelevant for a leaf node, but necessary for an internal
 *     node (square)
 * center - the center of the node
 *
 * Returns a pointer to the created node.
 */
Node* Node_init(const float64_t length, const Point center);
#endif

/*
 * Quadtree_init
 *
 * Allocates memory for and initializes an empty quadtree.
 *
 * length - the edge length of the bounding box for the region that this Quadtree covers
 * center - the center of this region
 *
 * Returns a pointer to the created, empty quadtree.
 */
Quadtree* Quadtree_init(const float64_t length, const Point center);

/*
 * Quadtree_search
 *
 * Searches for p in the quadtree, within a certain error tolerance.
 *
 * tree - the quadtree to query
 * p - the point we're searching for
 *
 * Returns whether p is in the quadtree.
 */
bool Quadtree_search(const Quadtree * const tree, const Point p);

/*
 * Quadtree_add
 *
 * Adds p to the quadtree.
 *
 * Will not add duplicate points to the tree, as defined by Point_equals. If p is already in the
 * tree, this function will return false.
 *
 * tree - the quadtree to add the point to
 * p - the point being added
 *
 * Returns whether the add was successful. False indicates either an error, or that the node is
 * already in the tree.
 */
bool Quadtree_add(Quadtree * const tree, const Point p);

/*
 * Quadtree_remove
 *
 * Removes p from the quadtree.
 *
 * tree - the tree to remove from
 * p - the point being removed
 *
 * Returns whether the remove was successful: false typically indicates that the node wasn't in the
 * tree to begin with.
 */
bool Quadtree_remove(Quadtree * const tree, const Point p);


/*
 * Quadtree_free
 *
 * Frees any dynamically-allocated memory in the entire tree, one node at a time.
 *
 * The freeing order is such that, if a free fails, retrying won't cause memory leaks, though the
 * tree may enter into an invalid state.
 *
 * Returns a QuadtreeFreeResult.
 */
QuadtreeFreeResult Quadtree_free(Quadtree * const tree);

/*
 * Node_valid
 *
 * Returns false if node is either NULL or is dirty, and true otherwise. A value of true indicates
 * that the node is both non-NULL and not logically deleted.
 *
 * node - the node to check
 *
 * Returns whether the node is valid for use.
 */
static inline bool Node_valid(const Node * const node) {
    return NULL != node;
}

/*
 * in_range
 *
 * Returns true if p is within the boundaries of n, false otherwise.
 *
 * On-boundary counts as being within if on the left or bottom boundaries.
 *
 * n - the square node to check at
 * p - the point to check for
 *
 * Returns whether p is within the boundaries of n.
 */
static bool in_range(const Node * const n, const Point * const p) {
    register float64_t bound = n->length * 0.5;
    register uint64_t i;
    for (i = 0; i < D; i++)
        if ((n->center.data[i] - bound > p->data[i]) || (n->center.data[i] + bound <= p->data[i]))
            return false;
    return true;
}

#ifdef QUADTREE_TEST
/*
 * Node_string
 *
 * Writes value of node to the given string buffer.
 *
 * node - the node to write
 * buffer - the buffer to write to
 */
static inline void Node_string(const Node * const node, char * const buffer) {
    char center_buffer[15 * D];
    Point_string(&node->center, center_buffer);

    char right_buffer[65] = "(null)", down_buffer[65] = "(null)";
    if (NULL != node->right)
        sprintf(right_buffer, "%llu", (unsigned long long)node->right->id);
    if (NULL != node->down)
        sprintf(down_buffer, "%llu", (unsigned long long)node->down->id);

    sprintf(buffer, "Node{id = %llu, is_square = %s, center = %s, length = %lf, right = %s, down = %s}",
        (unsigned long long)node->id,
        (node->is_square ? "YES" : "NO"), center_buffer, node->length,
        right_buffer, down_buffer);
}
#endif

// debugging function
extern void Point_string(const Point * const , char * const);
static void print(const Node * const n) {
    printf("pointer = %p", n);
    if (NULL != n) {
        char pbuf[100];
        Point_string((Point*)&n->center, pbuf);
        printf(", is_square = %s, center = %s, length = %llu, right = %p, down = %p\n",
            (n->is_square ? "YES" : "NO"),
            pbuf, (unsigned long long)n->length, n->right, n->down);
    }
    printf("\n");
}

// just for fun
#define Quadtree_plant Quadtree_init
#define Quadtree_climb Quadtree_search
#define Quadtree_grow Quadtree_add
#define Quadtree_prune Quadtree_remove
#define Quadtree_uproot Quadtree_free

#endif
