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

#define valid_node(n) Node_valid((Node*)(n))

/*
 * struct SkipListNode_t
 *
 * A container that wraps around the Node type to allow for skip list behavior as well.
 *
 * treenode - the Node that this SkipListNode wraps around
 * next - the next SkipListNode on the same level
 * down - defined such that if node = prev->next, node->down = prev->treenode.down->next
 */
typedef struct SkipListNode_t SkipListNode;
struct SkipListNode_t {
    Node treenode;
    SkipListNode *next, *down;
};

Node* Node_init(const float64_t length, const Point center) {
    SkipListNode *node = (SkipListNode*)malloc(sizeof(*node));
    *node = (SkipListNode){
        .treenode = (Node){
            .is_square = false,
            .length = length,
            .center = center,
            .down = NULL
#ifdef QUADTREE_TEST
            ,.id = QUADTREE_NODE_COUNT++
#endif
        },
        .next = NULL,
        .down = NULL
    };
    uint64_t i;
    for (i = 0; i < (1LL << D); i++) {
        node->treenode.children[i] = NULL;
    }
    return (Node*)node;
}

Quadtree* Quadtree_init(const float64_t length, const Point center) {
    Quadtree *tree = (Quadtree*)malloc(sizeof(*tree));
    *tree = (Quadtree){
        .height = 0,
        .root = Node_init(length, center),
        .center = center,
        .length = length
    };
    tree->root->is_square = true;
    return tree;
}

/*
 * Node_free_internal
 *
 * Frees the memory used to represent this node, inlined for internal use.
 *
 * node - the node to be freed
 */
static inline void Node_free_internal(const SkipListNode * const node) {
    free((void*)node);
}

void Node_free(const Node * const node) {
    Node_free_internal((SkipListNode*)node);
}

typedef enum {SUCCESS, FAILURE, EXISTENT, NONEXISTENT} Result;

// TODO: implement
/*
 * Quadtre_search_internal
 *
 * Does a horizontal traversal of the tree to find the square that should contain the target point,
 * which either contains the point, or serves as a drop-down location to the next level.
 *
 * root - the root of the subtree to start searching at
 * point - the point to search for
 *
 * Returns a result indicating whether the point is found.
 */
Result Quadtree_search_internal(const Node * const root, const Point * point) {
    // Check whether the root is valid and if the point is contained in the root.
    if (!valid_node(root) || !in_range(root, point)) {
        return FAILURE;
    }

    // Horizontally traverse to find the square that would contain the point if it exists.
    Node *parent = NULL, *node = (Node*)root;
    uint8_t quadrant;
    do {
        quadrant = get_quadrant(&node->center, point);
        parent = node;
        node = node->children[quadrant];
    } while (valid_node(node) && node->is_square && in_range(node, point));

    // Return EXISTENT if the point is found.
    if (valid_node(node) && Point_equals(&node->center, point)) {
        return EXISTENT;
    }

    // If there is a lower level, recurse down to find the point.
    if (valid_node(parent->down)) {
        return Quadtree_search_internal(parent->down, point);
    }

    // Otherwise, return NONEXISTENT.
    return FAILURE;
}

bool Quadtree_search(const Quadtree * const node, const Point point) {
    return EXISTENT == Quadtree_search_internal(node->root, &point);
}

/*
 * promote
 *
 * Promotes a node from the level one-lower to the current level. The tree is rooted at root, and
 * the node representing the point one level lower is given by down. Promoting where everything
 * down is NULL is equivalent to inserting the point fresh at the level of the root.
 *
 * root - the root of the tree to promote to, must be square and contains the point in down
 * head - the SkipListNode with the promoted node as its next
 * treedown - the Node of the promoting point that is one level lower, must not be square
 * down - the SkipListNode down for the promoted node
 * point - the Point being promoted
 *
 * Returns a Result detailing the success of the promotion.
 */
Result promote(SkipListNode * const root, SkipListNode * const head,
        SkipListNode * const treedown, SkipListNode * const down, const Point * const point) {
    // Check to make sure that root is valid, is square, and contains the point.
    if (!valid_node(root) || !root->treenode.is_square || !in_range(&root->treenode, point)) {
        return FAILURE;
    }

    // Check to make sure that head is valid.
    if (!valid_node(head)) {
        return FAILURE;
    }

    // Horizontal traversal of the tree to find insertion point.
    SkipListNode *parent = NULL, *sibling = root;
    uint8_t quadrant;
    do {
        quadrant = get_quadrant(&sibling->treenode.center, point);
        parent = sibling;
        sibling = (SkipListNode*)sibling->treenode.children[quadrant];
    } while (valid_node(sibling) && sibling->treenode.is_square &&
        in_range(&sibling->treenode, point));

    // Horizontal traversal of the skip list.
    SkipListNode *prev = NULL, *next = head;
    do {
        prev = next;
        next = prev->next;
    } while (valid_node(next) && 0 > Point_compare(&prev->treenode.center, &next->treenode.center));

    // Now, parent is the parent square and sibling is the sibling node of the new node.

    // Check to make sure node is not already in the tree.
    if (valid_node(sibling) && !sibling->treenode.is_square &&
            Point_equals(&sibling->treenode.center, point)) {
        return EXISTENT;
    }

    // Now that we've committed to creating a new node, we'll go ahead and create it.
    SkipListNode * const new_node = (SkipListNode*)Node_init(0, *point);

    // Set the appropriate pointers in the new node.
    new_node->next = next;
    new_node->down = down;
    new_node->treenode.down = (Node*)treedown;

    // The direct child of the parent. Is the new node by default, but could be a bounding square
    // if a there is a conflict for the quadrant in parent.
    SkipListNode *direct_child = new_node;

    // Insertion.
    if (valid_node(sibling)) {
        // Compute new containing square of new node and sibling.
        SkipListNode * const new_square = (SkipListNode*)Node_init(
            parent->treenode.length, parent->treenode.center);
        new_square->treenode.is_square = true;
        uint8_t n_quadrant = quadrant, s_quadrant;
        do {
            new_square->treenode.center = get_new_center((Node*)new_square, n_quadrant);
            new_square->treenode.length *= 0.5;
            n_quadrant = get_quadrant(&new_square->treenode.center, point);
            s_quadrant = get_quadrant(&new_square->treenode.center, &sibling->treenode.center);
        } while (n_quadrant == s_quadrant);

        // Find tree down of containing square.
        SkipListNode *down_square = (SkipListNode*)parent->treenode.down;
        if (valid_node(down_square)) {
            while ( abs(down_square->treenode.length - new_square->treenode.length) > PRECISION ||
                    !Point_equals(&down_square->treenode.center, &new_square->treenode.center)) {
                const uint8_t square_quadrant = get_quadrant(
                    &down_square->treenode.center, &new_square->treenode.center);
                down_square = (SkipListNode*)down_square->treenode.children[square_quadrant];
            }
        }

        // Connect containing square to new node, sibling, and tree down.
        new_square->treenode.children[n_quadrant] = (Node*)new_node;
        new_square->treenode.children[s_quadrant] = (Node*)sibling;
        new_square->treenode.down = (Node*)down_square;

        // Prepare to insert square into parent tree.
        direct_child = new_square;
    } else {
        // Prepare to insert child directly into tree.
        direct_child = new_node;
    }

    // Set the pointers of prev and parent to the correct nodes.
    parent->treenode.children[quadrant] = (Node*)direct_child;
    prev->next = new_node;

    // Return.
    return SUCCESS;
}

/*
 * Quadtree_add_internal
 *
 * Inserts the target point on the lowest level of the tree. As it progresses, nodes may be
 * promoted as necessary to preserve the 1-2-3 skip list invariants. Produces a Result.
 *
 * node - the top-most level tree node of the subtree to insert into
 * head - the top-most level head node of the sublist to insert into
 * root - the top-most level root node
 * point - the point to insert
 *
 * Returns a Result indicating the result of adding the point.
 */
Result Quadtree_add_internal(SkipListNode * const node, SkipListNode * const head,
        SkipListNode * const root, const Point * const point) {
    // Check to make sure node is valid, is square, and contains the point.
    if (!valid_node(node) || !node->treenode.is_square || !in_range((Node*)node, point)) {
        return FAILURE;
    }

    // Check to make sure root is valid, is square, and contains the node.
    if (!valid_node(root) || !root->treenode.is_square ||
            !in_range((Node*)root, &node->treenode.center)) {
        return FAILURE;
    }

    // Traverse skip list to find gap to drop into.
    SkipListNode *parent = NULL, *sibling = node;
    uint8_t quadrant;
    do {
        quadrant = get_quadrant(&sibling->treenode.center, point);
        parent = sibling;
        sibling = (SkipListNode*)sibling->treenode.children[quadrant];
    } while (valid_node(sibling) &&
        sibling->treenode.is_square &&
        in_range(&sibling->treenode, point));

    // Compute the previous node.
    SkipListNode *prev = NULL, *next = head;
    do {
        prev = next;
        next = prev->next;
    } while (valid_node(next) && 0 > Point_compare(&prev->treenode.center, &next->treenode.center));

    // If at bottom-most level, insert node and return.
    if (!valid_node(root->down)) {
        return promote(parent, prev, NULL, NULL, point);
    }

    // Determine whether to promote a node (if gap has 3 nodes).
    uint8_t gap_width = 0;
    SkipListNode * const prev_down = (SkipListNode*)prev->treenode.down;
    SkipListNode *gap_node = prev_down, *center_node = NULL;
    for (gap_node = gap_node->next;
            valid_node(gap_node) && (!valid_node(next) || next->treenode.down != (Node*)gap_node);
            gap_node = gap_node->next, gap_width++) {
        if (1 == gap_width) {
            center_node = gap_node;
        }
    }
    if (3 == gap_width) {
        // Promote the target node.
        const Result success =
            promote(parent, prev, center_node, prev_down->next, &center_node->treenode.center);
        if (SUCCESS != success) {
            return success;
        }
    }

    // Recurse down a level.
    return Quadtree_add_internal((SkipListNode*)parent->treenode.down, prev_down->next,
        (SkipListNode*)root->treenode.down, point);
}

bool Quadtree_add(Quadtree * const node, const Point point) {
    SkipListNode * const root = (SkipListNode*)node->root;

    const Result result = Quadtree_add_internal(root, root, root, &point);

    // Add new empty level if necessary, i.e. top-most level is no longer empty.
    uint64_t i;
    for (i = 0; i < (1LL << D); i++) {
        if (valid_node(root->treenode.children[i])) {
            SkipListNode * const node_root = (SkipListNode*)Node_init(node->length, node->center);
            node_root->treenode.is_square = true;
            node_root->treenode.down = node->root;
            node_root->down = root;
            node->root = &node_root->treenode;
            break;
        }
    }

    return SUCCESS == result;
}

// TODO: implement
bool Quadtree_remove(Quadtree * const node, const Point point) {
    return false;
}

/*
 * Quadtree_free_internal
 *
 * result - the result object to record data onto
 * node - the node to recursively free
 */
void Quadtree_free_internal(QuadtreeFreeResult * result, const SkipListNode * const node) {
    uint64_t i;
    bool is_leaf = true;
    for (i = 0; i < (1LL << D); i++) {
        if (valid_node(node->treenode.children[i])) {
            is_leaf = false;
            Quadtree_free_internal(result, (SkipListNode*)node->treenode.children[i]);
        }
    }
    Node_free_internal(node);
    result->total++;
    result->leaf += is_leaf;
}

QuadtreeFreeResult Quadtree_free(Quadtree * const tree) {
    QuadtreeFreeResult result = (QuadtreeFreeResult){ .total = 0, .leaf = 0, .levels = 0 };

    while (valid_node(tree->root)) {
        Node *root = tree->root;
        tree->root = root->down;
        Quadtree_free_internal(&result, (SkipListNode*)root);
        result.levels++;
    }

    free(tree);

    return result;
}
