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
        .down = NULL
#ifdef QUADTREE_TEST
        ,.id = QUADTREE_NODE_COUNT++
#endif
    };
    uint64_t i;
    for (i = 0; i < (1LL << D); i++) {
        node->children[i] = NULL;
    }
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
 * Node_free_internal
 *
 * Frees the memory used to represent this node, inlined for internal use.
 *
 * node - the node to be freed
 */
static inline void Node_free_internal(const Node * const node) {
    free((void*)node);
}

void Node_free(const Node * const node) {
    Node_free_internal(node);
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
/*
 * Quadtree_free_internal
 *
 * result - the result object to record data onto
 * node - the node to recursively free
 */
void Quadtree_free_internal(QuadtreeFreeResult * result, const Node * const node) {
    uint64_t i;
    bool is_leaf = true;
    for (i = 0; i < (1LL << D); i++) {
        if (Node_valid(node->children[i])) {
            is_leaf = false;
            Quadtree_free_internal(result, node->children[i]);
        }
    }
    Node_free_internal(node);
    result->total++;
    result->leaf += is_leaf;
}

QuadtreeFreeResult Quadtree_free(Quadtree * const tree) {
    QuadtreeFreeResult result = (QuadtreeFreeResult){ .total = 0, .leaf = 0, .levels = 0 };

    while (Node_valid(tree->root)) {
        Node *root = tree->root;
        tree->root = root->down;
        Quadtree_free_internal(&result, root);
        result.levels++;
    }

    free(tree);

    return result;
}
