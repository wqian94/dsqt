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

typedef struct SkipQuadtreeNode_t Node;
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
 * struct SkipQuadtreeNode_t
 *
 * Stores information about a node in the quadtree
 *
 * is_square - true if node is a square, false if is a point
 * length - side length of the square. This means that the boundaries are length/2 distance from
 *     the center. Does not matter for a leaf node
 * center - center of the square, or coordinates of the point
 * down - the clone of the same node in the previous level; NULL if at lowest level
 * children - the four children of the square; each entry is NULL if there is no child
 *     there. Each index refers to a quadrant, such that children[0] is Q1, [1] is Q2,
 *     and so on. Should never be all NULL unless node is a point
 */
struct SkipQuadtreeNode_t {
    bool is_square;
    float64_t length;
    Point center;
    Node *down;
    Node *children[1LL << D];
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
 *     node (square). Must be >= 0.
 * center - the center of the node
 *
 * Returns a pointer to the created node.
 */
Node* Node_init(const float64_t length, const Point center);

/*
 * Node_free
 *
 * Frees the memory used to represent this node.
 *
 * node - the node to be freed
 */
void Node_free(const Node * const node);
#endif

/*
 * Quadtree_init
 *
 * Allocates memory for and initializes an empty quadtree.
 *
 * length - the edge length of the bounding box for the region that this Quadtree covers, >= 0.
 * center - the center of this region
 *
 * Returns a pointer to the created, empty quadtree.
 */
Quadtree* Quadtree_init(const float64_t length, const Point center);

/*
 * Quadtree_search
 *
 * Searches for the point in the quadtree, within a certain error tolerance.
 *
 * tree - the quadtree to query
 * point - the point we're searching for
 *
 * Returns whether point is in the quadtree.
 */
bool Quadtree_search(const Quadtree * const tree, const Point point);

/*
 * Quadtree_add
 *
 * Adds p to the quadtree.
 *
 * Will not add duplicate points to the tree, as defined by Point_equals. If the point is already
 * in the tree, this function will return false.
 *
 * tree - the quadtree to add the point to
 * point - the point being added
 *
 * Returns whether the add was successful. False indicates either an error, or that the node is
 * already in the tree.
 */
bool Quadtree_add(Quadtree * const tree, const Point point);

/*
 * Quadtree_remove
 *
 * Removes the point from the quadtree.
 *
 * tree - the tree to remove from
 * point - the point being removed
 *
 * Returns whether the remove was successful: false typically indicates that the node wasn't in the
 * tree to begin with.
 */
bool Quadtree_remove(Quadtree * const tree, const Point point);


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
    for (i = 0; i < D; i++) {
        if ((n->center.data[i] - bound > p->data[i]) || (n->center.data[i] + bound <= p->data[i])) {
            return false;
        }
    }
    return true;
}

/*
 * get_quadrant
 *
 * Returns the quadrant [0,2^D) that p is in, relative to the origin point.
 *
 * Let b = quadrant id in binary, with b[0] being the least significant bit. Then, b[0] corresponds
 * to the first dimension, b[1] corresponds to the second, etc. such that b[i] corresponds to the
 * (i + 1)th dimension.
 *
 * origin - the point representing the origin of the bounding square
 * p - the point we're trying to find the quadrant of
 *
 * Returns the quadrant that p is in, relative to origin.
 */
static uint64_t get_quadrant(const Point * const origin, const Point * const p) {
    register uint64_t i;
    uint64_t quadrant = 0;
    for (i = 0; i < D; i++) {
        quadrant |= ((p->data[i] >= origin->data[i] - PRECISION) & 1) << i;
    }
    return quadrant;
}

/*
 * get_new_center
 *
 * Given the current node and a quadrant, returns the Point representing
 * the center of the square that represents that quadrant.
 *
 * node - the parent node
 * quadrant - the quadrant to search for, [0, 2^D)
 *
 * Returns the center point for the given quadrant of node.
 */
static Point get_new_center(const Node * const node, const uint64_t quadrant) {
    Point point;
    register uint64_t i;
    for (i = 0; i < D; i++) {
        point.data[i] = node->center.data[i] + (((quadrant >> i) & 1) - 0.5) * 0.5 * node->length;
    }
    return point;
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

    char down_buffer[65] = "(nil)";
    if (NULL != node->down) {
        sprintf(down_buffer, "%llu", (unsigned long long)node->down->id);
    }

    sprintf(buffer, "Node{id = %llu, is_square = %s, length = %lf, center = %s, down = %s",
        (unsigned long long)node->id, (node->is_square ? "YES" : "NO"),
        node->length, center_buffer, down_buffer);

    uint64_t i;
    char child_buffer[33] = "(nil)";
    for (i = 0; i < (1LL << D); i++) {
        if (NULL != node->children[i]) {
            sprintf(child_buffer, "%llu", (unsigned long long)node->children[i]->id);
        } else {
            sprintf(child_buffer, "(nil)");
        }
        sprintf(buffer + strlen(buffer), ", children[%llu] = %s",
            (unsigned long long)i, child_buffer);
    }
    sprintf(buffer + strlen(buffer), "}");
}

/*
 * Quadtree_string
 *
 * Writes the value of the tree to the given string buffer.
 *
 * tree - the tree to write
 * buffer - the buffer to write to
 */
static inline void Quadtree_string(const Quadtree * const tree, char * const buffer) {
    char center_buffer[15 * D];
    Point_string(&tree->center, center_buffer);

    sprintf(buffer, "Quadtree{height = %llu, root = %p, length = %lf, center = %s}",
        (unsigned long long)tree->height, tree->root, tree->length, center_buffer);
}

// debugging function
extern void Point_string(const Point * const , char * const);
static void print(const Node * const n) {
    printf("pointer = %p", n);
    if (NULL != n) {
        char pbuf[100];
        Point_string((Point*)&n->center, pbuf);
        printf(", is_square = %s, length = %llu, center = %s, down = %p",
            (n->is_square ? "YES" : "NO"),
            (unsigned long long)n->length, pbuf, n->down);
        uint64_t i;
        for (i = 0; i < (1LL << D); i++) {
            printf(", children[%llu] = %p", (unsigned long long)i, n->children[i]);
        }
    }
    printf("\n");
}
#endif

// just for fun
#define Quadtree_plant Quadtree_init
#define Quadtree_climb Quadtree_search
#define Quadtree_grow Quadtree_add
#define Quadtree_prune Quadtree_remove
#define Quadtree_uproot Quadtree_free

#endif
