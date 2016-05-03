/**
Naive serial implementation of deterministic compressed skip quadtree
*/

#include <assert.h>
#include <stdlib.h>

#include "../types.h"
#include "../Quadtree.h"
#include "../Point.h"

// rlu_self, included to make compiler happy
__thread rlu_thread_data_t *rlu_self = NULL;

// quadtree counter
#ifdef QUADTREE_TEST
uint64_t QUADTREE_NODE_COUNT = 0;
#endif

Node* Node_init(const float64_t length, const Point center) {
    Node *node = (Node*)malloc(sizeof(*node));
    *node = (Node){
        .is_square = false,
        .length = length,
        .center = center,
        .right = NULL,
        .down = NULL
#ifdef QUADTREE_TEST
        ,.id = QUADTREE_NODE_COUNT++
#endif
    };
    return node;
}

Quadtree* Quadtree_init(const float64_t length, const Point center) {
    Quadtree *tree = (Quadtree*)malloc(sizeof(*tree));
    *tree = (Quadtree){
        .height = 0,
        .root = NULL,
        .center = center,
        .length = length
    };
    return tree;
}

/*
 * Node_free
 *
 * Frees the memory used to represent this node.
 *
 * node - the node to be freed
 */
static inline void Node_free(Node * const node) {
    free((void*)node);
}

// TODO: implement
bool Quadtree_search(const Quadtree * const node, const Point p) {
    return false;
}

// TODO: implement
bool Quadtree_add(Quadtree * const node, const Point p) {
    return false;
}

// TODO: implement
bool Quadtree_remove(Quadtree * const node, const Point p) {
    return false;
}

// TODO: implement
QuadtreeFreeResult Quadtree_free(Quadtree * const root) {
    QuadtreeFreeResult result = (QuadtreeFreeResult){ .total = 0, .leaf = 0, .levels = 0 };
    return result;
}
