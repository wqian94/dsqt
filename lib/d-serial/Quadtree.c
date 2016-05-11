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

/*
 * Quadtre_search_internal
 *
 * Does a horizontal traversal of the tree to find the square that should contain the target point,
 * which either contains the point, or serves as a drop-down location to the next level.
 *
 * node - the root-most level node in the tree to start searching at
 * point - the point to search for
 *
 * Returns a result indicating whether the point is found.
 */
Result Quadtree_search_internal(const Node * const node, const Point * point) {
    // Check whether the root is valid and if the point is contained in the root.
    if (!valid_node(node) || !in_range(node, point)) {
        return FAILURE;
    }

    // Horizontally traverse to find the square that would contain the point if it exists.
    Node *parent = NULL, *target = (Node*)node;
    uint8_t quadrant;
    do {
        quadrant = get_quadrant(&target->center, point);
        parent = target;
        target = target->children[quadrant];
    } while (valid_node(target) && target->is_square && in_range(target, point));

    // Return EXISTENT if the point is found.
    if (valid_node(target) && Point_equals(&target->center, point)) {
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
 * root - the root of the tree to promote to, must be square and contains the point
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
        SkipListNode * promote_root = root;
        if (in_range((Node*)parent, &center_node->treenode.center)) {
            promote_root = parent;
        }
        const Result success =
            promote(promote_root, prev, center_node, prev_down->next,
            &center_node->treenode.center);
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
/*
 * demote
 *
 * Demotes a node from the current level. The tree is rooted at root, and the previous node in the
 * skip list is given by head. If deleting a node causes its parent to have < 2 children and it is
 * a non-root square, then we collapse the square as well, resetting the ``grandparent" node to
 * point to the sibling of the demoted node.
 *
 * root - the root of the tree to demote from, must be square and contains the point
 * head - the SkipListNode with the demoted node as its next
 * node - the SkipListNode node for the demoted node
 * point - the Point being demoted
 *
 * Returns a Result detailing the success of the demotion.
 */
Result demote(SkipListNode * const root, SkipListNode * const head, const Point * const point) {
    // Check to make sure that root is valid, is square, and contains the point.
    if (!valid_node(root) || !root->treenode.is_square || !in_range(&root->treenode, point)) {
        return FAILURE;
    }

    // Check to make sure that head is valid and that the point comes after.
    if (!valid_node(head)) {
        return FAILURE;
    }

    // Find parent and grandparent from tree.
    SkipListNode *grandparent = NULL, *parent = NULL, *node = root;
    uint8_t quadrant = 0, parent_quadrant = 0;
    do {
        parent_quadrant = quadrant;
        quadrant = get_quadrant(&node->treenode.center, point);
        grandparent = parent;
        parent = node;
        node = (SkipListNode*)node->treenode.children[quadrant];
    } while (valid_node(node) && node->treenode.is_square && in_range(&node->treenode, point));

    // Find previous and next in skip list.
    SkipListNode *prev = NULL, *next = head;
    do {
        prev = next;
        next = next->next;
    } while (next != node);  // We expect to always find the node in the list.
    next = next -> next;

    // Deletion.

    // Determine whether the parent node should be collapsed.
    bool collapse = false;
    SkipListNode *sibling = NULL;
    uint64_t i;
    for (i = 0; i < (1LL << D); i++) {
        SkipListNode * const child = (SkipListNode*)parent->treenode.children[i];

        if (!valid_node(child) || child == node) {  // Ignore if child is not a valid sibling.
            continue;
        } else if (!valid_node(sibling)) {  // On first sibling, track sibling and flag collapse.
            sibling = child;
            collapse = true;
        } else {  // On future siblings, deflag collapse and break.
            collapse = false;
            break;
        }
    }
    collapse = collapse && valid_node(grandparent);  // Collapse only if parent is not a root node.

    // Detach node from parent.
    parent->treenode.children[quadrant] = NULL;

    if (collapse) {
        // Reset grandparent's pointer to parent to now point to the sibling.
        grandparent->treenode.children[parent_quadrant] = (Node*)sibling;

        // Free parent node.
        Node_free_internal(parent);
    }

    // Reset pointers of previous and next node in skip list.
    prev->next = next;
    if (valid_node(next)) {
        next->down = node->down;
    }

    // Free target node.
    Node_free_internal(node);

    // Return.
    return SUCCESS;
}

/*
 * gap_length
 *
 * Measures the number of nodes between two nodes given, exclusive.
 *
 * head - the SkipListNode to start at, must not be NULL
 * tail - the SkipListNode to end at, may be NULL
 *
 * Returns the number of nodes between head and tail, exclusive.
 */
uint64_t gap_length(const SkipListNode * const head, const SkipListNode * const tail) {
    uint64_t gap = 0;
    SkipListNode *node = head->next;
    while (valid_node(node) && tail != node) {
        node = node->next;
        gap++;
    }
    return gap;
}

/*
 * Quadtree_remove_internal
 *
 * Inserts the target point on the lowest level of the tree. As it progresses, nodes may be
 * promoted as necessary to preserve the 1-2-3 skip list invariants. Produces a Result.
 *
 * grandroot - the parent of root, may be NULL
 * root - the top-most level tree node of the subtree to delete from
 * grandhead - the prev of head, may be NULL
 * head - the top-most level head node of the sublist to delete from
 * point - the point to delete
 *
 * Returns a Result indicating the result of adding the point.
 */
Result Quadtree_remove_internal(SkipListNode * const grandroot, SkipListNode * const root,
        SkipListNode * const grandhead, SkipListNode * const head, const Point * const point) {
    // Check to make sure root is valid, is square, and contains the node.
    if (!valid_node(root) || !root->treenode.is_square || !in_range((Node*)root, point)) {
        return FAILURE;
    }

    // Horizontally traverse the tree to find the corresponding node.
    SkipListNode *grandparent = NULL, *parent = grandroot, *node = root;
    uint64_t quadrant;
    do {
        quadrant = get_quadrant(&node->treenode.center, point);
        grandparent = parent;
        parent = node;
        node = (SkipListNode*)node->treenode.children[quadrant];
    } while (valid_node(node) && node->treenode.is_square && in_range((Node*)node, point));

    // Horizontally traverse the skip list.
    SkipListNode *prevprev = NULL, *prev = grandhead, *next = head, *nextnext = NULL;
    do {
        prevprev = prev;
        prev = next;
        next = next->next;
    } while (valid_node(next) && 0 > Point_compare(&next->treenode.center, point));
    // If the target point exists on this level, then next must point to the node representing it.
    if (valid_node(next)) {
        nextnext = next->next;
    }

    // If we're at lowest level, node better contain the node we're searching for.
    if (!valid_node(root->treenode.down)) {
        if (!valid_node(next) || !Point_equals(&next->treenode.center, point)) {
            return NONEXISTENT;
        }
        return demote(grandparent, prev, point);
    }

    // We aim to drop into the gap between prev and next.

    SkipListNode *grandroot_down = NULL;
    if (valid_node(grandroot)) {
        grandroot_down = (SkipListNode*)grandroot->treenode.down;
    }
    SkipListNode *root_down = (SkipListNode*)root->treenode.down;
    SkipListNode *prev_down = (SkipListNode*)prev->treenode.down;
    SkipListNode *next_down = NULL;
    if (valid_node(next)) {
        next_down = (SkipListNode*)next->treenode.down;
    }
    SkipListNode *nextnext_down = NULL;
    if (valid_node(nextnext)) {
        nextnext_down = (SkipListNode*)nextnext->treenode.down;
    }
    if (!valid_node(prevprev)) {  // If trying to drop into the first gap.
        // If the gap is == 1, demote next.
        if (gap_length(prev_down, next_down)) {
            // If the second gap is >= 2 (> 1), promote the first node in the second gap.
            if (valid_node(next) && 1 < gap_length(next_down, nextnext_down)) {
                promote(root, next, next_down->next, next_down->next,
                    &next_down->next->treenode.center);
            }

            // Demote next.
            if (valid_node(next)) {
                demote(root, prev, &next->treenode.center);
            }
        }

        // Drop into gap between prev and next.
        return Quadtree_remove_internal(grandroot_down, root_down, prev_down, prev_down->next,
            point);
    } else {  // Non-first gap.
        SkipListNode *prevprev_down = (SkipListNode*)prevprev->treenode.down;
        // If the previous gap is length > 1, promote the last node in that gap.
        if (1 < gap_length(prevprev_down, prev_down)) {
            SkipListNode *promote_node = prevprev_down;
            while (promote_node->next != prev_down) {
                promote_node = promote_node->next;
            }
            promote(root, prevprev, promote_node, promote_node, &promote_node->treenode.center);
        }

        // Demote prev.
        demote(root, prevprev, &prev->treenode.center);

        // Drop into gap between prev and next.
        return Quadtree_remove_internal(grandroot_down, root_down, prev_down, prev_down->next,
            point);
    }

    return FAILURE;
}

bool Quadtree_remove(Quadtree * const node, const Point point) {
    SkipListNode * const root = (SkipListNode*)node->root;

    const Result result = Quadtree_remove_internal(NULL, root, NULL, root, &point);

    // If two top-most root nodes are both empty, delete the top-most root node.
    if (valid_node(root->down)) {
        bool both_empty = true;
        uint64_t i;
        for (i = 0; i < (1LL << D); i++) {
            both_empty = !valid_node(root->treenode.children[i]) &&
                !valid_node(root->treenode.down->children[i]);
        }
        if (both_empty) {
            node->root = root->treenode.down;
            Node_free_internal(root);
        }
    }

    return SUCCESS == result;
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
