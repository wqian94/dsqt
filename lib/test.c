/**
Testing suite for correctness of Quadtrees
*/

#include "test.h"

//extern __thread rlu_thread_data_t *rlu_self;
extern bool in_range(const Node*, const Point*);
extern void Point_string(const Point*, char*);

/*
 * uniform_point
 *
 * Given a value, creates a D-dimensional Point where each dimension has the given value.
 *
 * value - the value to put in every one of the D dimensions
 *
 * Returns a Point where each dimension has the given value.
 */
static Point uniform_point(float64_t value) {
    float64_t coords[D];
    uint64_t i;
    for (i = 0; i < D; i++) {
        coords[i] = value;
    }
    return Point_from_array(coords);
}

void test_sizes() {
    if (messagesOn()) {
        printf("dimensions        = %lu\n", (unsigned long)D);
        printf("sizeof(Quadtree)  = %lu\n", sizeof(Quadtree));
        printf("sizeof(Node)      = %lu\n", sizeof(Node));
        printf("sizeof(bool)      = %lu\n", sizeof(bool));
        printf("sizeof(uint64_t)  = %lu\n", sizeof(uint64_t));
        printf("sizeof(Node*)     = %lu\n", sizeof(Node*));
        printf("sizeof(float64_t) = %lu\n", sizeof(float64_t));
        printf("sizeof(Point)     = %lu\n", sizeof(Point));
    }

    start_test("Quadtree size");

    // Quadtree is 24 bytes + 8 bytes for each dimension.
    #ifndef PARALLEL
    assertLong(8 * D + 24, sizeof(Quadtree), "sizeof(Quadtree)");
    #endif

    end_test();
    start_test("Node size");

    // Node is 24 bytes + 8 bytes per dimension + 8 bytes per 2^D children for each node, but with
    // the addition of the id, it's 32 bytes + 8 * D bytes + 8 * (2^D) bytes.
    #ifndef PARALLEL
    assertLong(8 * (1LL << D) + 8 * D + 32, sizeof(Node), "sizeof(Node)");
    #endif

    end_test();
}

void test_in_range() {
    char buffer[256 + 30 * D], node_buffer[128 + 15 * D], point_buffer[15 * D];

    start_test("same point");

    Point point1 = uniform_point(0);

    float64_t length1 = 1;
    Node *node1 = Node_init(length1, point1);

    sprintf(node_buffer, "Node{center = ");
    Point_string(&point1, node_buffer + strlen(node_buffer));
    sprintf(node_buffer + strlen(node_buffer), ", length = %lf}", node1->length);

    Point_string(&point1, point_buffer);
    sprintf(buffer, "in_range(%s, %s)", node_buffer, point_buffer);
    assertTrue(in_range(node1, &point1), buffer);

    end_test();
    start_test("boundary upper right (exclusive)");

    Point point2 = uniform_point(0.5);

    Point_string(&point2, point_buffer);
    sprintf(buffer, "in_range(%s, %s)", node_buffer, point_buffer);
    assertFalse(in_range(node1, &point2), buffer);

    end_test();
    start_test("boundary bottom left (inclusive)");

    Point point3 = uniform_point(-0.5);

    Point_string(&point3, point_buffer);
    sprintf(buffer, "in_range(%s, %s)", node_buffer, point_buffer);
    assertTrue(in_range(node1, &point3), buffer);

    end_test();
    start_test("exclude higher coordinates");

    Point point4 = uniform_point(1);

    Point_string(&point4, point_buffer);
    sprintf(buffer, "in_range(%s, %s)", node_buffer, point_buffer);
    assertFalse(in_range(node1, &point4), buffer);

    end_test();
    start_test("exclude lower coordinates");

    Point point5 = uniform_point(-1);

    Point_string(&point5, point_buffer);
    sprintf(buffer, "in_range(%s, %s)", node_buffer, point_buffer);
    assertFalse(in_range(node1, &point5), buffer);

    end_test();

    Node_free(node1);
}

void test_get_quadrant() {
    char buffer[256 + 30 * D], origin_buffer[15 * D], point_buffer[15 * D];
    uint64_t i, j;

    Point point1 = uniform_point(0);

    start_test("same point");

    Point_string(&point1, origin_buffer);
    Point_string(&point1, point_buffer);
    sprintf(buffer, "get_quadrant(%s, %s)", origin_buffer, point_buffer);
    assertLong((1LL << D) - 1, get_quadrant(&point1, &point1), buffer);

    end_test();
    start_test("different points each direction");

    Point point2 = uniform_point(-1);

    for (i = 0; i < (1LL << D); i++) {
        for (j = 0; j < D; j++) {
            point2.data[j] = 2 * ((i >> j) & 1) - 1.0;
        }
        Point_string(&point1, origin_buffer);
        Point_string(&point2, point_buffer);
        sprintf(buffer, "get_quadrant(%s, %s)", origin_buffer, point_buffer);
        assertLong(i, get_quadrant(&point1, &point2), buffer);
    }

    end_test();
}

void test_get_new_center() {
    char buffer[256 + 15 * D], node1_buffer[128 + 15 * D], node3_buffer[128 + 15 * D];
    uint64_t i, j;

    start_test("each direction from origin");

    Point point1 = uniform_point(0);
    Point point2 = uniform_point(0.5);

    float64_t length1 = 2;
    Node *node1 = Node_init(length1, point1);

    sprintf(node1_buffer, "Node{center = ");
    Point_string(&point1, node1_buffer + strlen(node1_buffer));
    sprintf(node1_buffer + strlen(node1_buffer), ", length = %lf}", length1);

    for (i = 0; i < (1LL << D); i++) {
        for (j = 0; j < D; j++) {
            point2.data[j] = ((i >> j) & 1) - 0.5;
        }
        sprintf(buffer, "get_new_center(%s, %llu)", node1_buffer, (unsigned long long)i);
        assertPoint(point2, get_new_center(node1, i), buffer);
    }

    end_test();
    start_test("each direction from non-origin");

    Point point3 = uniform_point(1);
    Point point4 = uniform_point(2);

    float64_t length3 = 4;
    Node *node3 = Node_init(length3, point3);

    sprintf(node3_buffer, "Node{center = ");
    Point_string(&point3, node3_buffer + strlen(node3_buffer));
    sprintf(node3_buffer + strlen(node3_buffer), ", length = %lf}", length3);

    for (i = 0; i < (1LL << D); i++) {
        for (j = 0; j < D; j++) {
            point4.data[j] = 2.0 * ((i >> j) & 1);
        }
        sprintf(buffer, "get_new_center(%s, %llu)", node3_buffer, (unsigned long long)i);
        assertPoint(point4, get_new_center(node3, i), buffer);
    }

    end_test();

    Node_free(node1);
    Node_free(node3);
}

void test_node_create() {
    char buffer[256 + 30 * (1LL << D) + 15 * D];
    char node_buffer[128 + 30 * (1LL << D) + 15 * D], point_buffer[15 * D];
    uint64_t i;

    start_test("");

    Point point1 = uniform_point(-42);
    float64_t length1 = 5;
    Node *node1 = Node_init(length1, point1);

    Point_string(&point1, point_buffer);
    Node_string(node1, node_buffer);

    sprintf(buffer, "square status of %s", node_buffer);
    assertFalse(node1->is_square, buffer);

    sprintf(buffer, "length of %s", node_buffer);
    assertDouble(length1, node1->length, buffer);

    sprintf(buffer, "center of %s", node_buffer);
    assertPoint(point1, node1->center, buffer);

    sprintf(buffer, "NULL down of %s", node_buffer);
    assertTrue(NULL == node1->down, buffer);

    for (i = 0; i < (1LL << D); i++) {
        sprintf(buffer, "NULL children[%llu] of %s", (unsigned long long)i, node_buffer);
        assertTrue(NULL == node1->children[i], buffer);
    }

    end_test();

    Node_free(node1);
}

void test_quadtree_create() {
    char buffer[256 + 15 * D];
    char tree_buffer[128 + 15 * D], point_buffer[15 * D];
    uint64_t i;

    start_test("");

    Point point1 = uniform_point(5);
    float64_t length1 = 42;
    Quadtree *tree1 = Quadtree_init(length1, point1);

    Point_string(&point1, point_buffer);
    Quadtree_string(tree1, tree_buffer);

    sprintf(buffer, "height of %s", tree_buffer);
    assertLong(0, tree1->height, buffer);

    sprintf(buffer, "NULL root of %s", tree_buffer);
    assertTrue(NULL == tree1->root, buffer);

    sprintf(buffer, "length of %s", tree_buffer);
    assertDouble(length1, tree1->length, buffer);

    sprintf(buffer, "center of %s", tree_buffer);
    assertPoint(point1, tree1->center, buffer);

    end_test();

    Quadtree_free(tree1);
}

void test_quadtree_add() {
    char buffer[256 + 15 * D];
    char tree_buffer[128 + 15 * D], point_buffer[15 * D];
    uint64_t i, j;

    start_test("center point");

    Point point1 = uniform_point(0);

    float64_t length1 = 2;
    Quadtree *tree1 = Quadtree_init(length1, point1);

    Point_string(&point1, point_buffer);
    Quadtree_string(tree1, tree_buffer);

    sprintf(buffer, "addition of %s into %s", point_buffer, tree_buffer);
    assertTrue(Quadtree_add(tree1, point1), buffer);
    sprintf(buffer, "repeated addition of %s into %s", point_buffer, tree_buffer);
    assertFalse(Quadtree_add(tree1, point1), buffer);

    end_test();
    start_test("all-positive point");

    Point point2 = uniform_point(0.5);
    Point_string(&point2, point_buffer);

    sprintf(buffer, "addition of %s into %s", point_buffer, tree_buffer);
    assertTrue(Quadtree_add(tree1, point2), buffer);
    sprintf(buffer, "repeated addition of %s into %s", point_buffer, tree_buffer);
    assertFalse(Quadtree_add(tree1, point2), buffer);

    end_test();
    start_test("all-negative, non-terminating point");

    Point point3 = uniform_point(-1.0 / 3);
    Point_string(&point3, point_buffer);

    sprintf(buffer, "addition of %s into %s", point_buffer, tree_buffer);
    assertTrue(Quadtree_add(tree1, point3), buffer);
    sprintf(buffer, "repeated addition of %s into %s", point_buffer, tree_buffer);
    assertFalse(Quadtree_add(tree1, point3), buffer);

    end_test();
    start_test("creating smaller square");

    Point point4 = uniform_point(1.0 / 32);
    Point_string(&point4, point_buffer);

    sprintf(buffer, "addition of %s into %s", point_buffer, tree_buffer);
    assertTrue(Quadtree_add(tree1, point4), buffer);
    sprintf(buffer, "repeated addition of %s into %s", point_buffer, tree_buffer);
    assertFalse(Quadtree_add(tree1, point4), buffer);

    end_test();
    start_test("point off all-same-values line");

    Point point5 = uniform_point(0.125);
    point5.data[0] = -0.0625;
    Point_string(&point5, point_buffer);

    sprintf(buffer, "addition of %s into %s", point_buffer, tree_buffer);
    assertTrue(Quadtree_add(tree1, point5), buffer);
    sprintf(buffer, "repeated addition of %s into %s", point_buffer, tree_buffer);
    assertFalse(Quadtree_add(tree1, point5), buffer);

    end_test();
    start_test("does not add out-of-bounds point");

    Point point6 = uniform_point(2);

    for (i = 0; i < (1LL << D); i++) {
        for (j = 0; j < D; j++) {
            point6.data[j] = 2 * ((i >> j) & 1) - 1.0;
        }
        Point_string(&point6, point_buffer);
        sprintf(buffer, "addition of out-of-bounds %s into %s", point_buffer, tree_buffer);
        assertTrue(Quadtree_add(tree1, point6), buffer);
    }

    end_test();

    Quadtree_free(tree1);
}

void test_quadtree_search() {
    char buffer[256 + 15 * D];
    char tree_buffer[128 + 15 * D], point_buffer[15 * D];
    uint64_t i;

    start_test("center point");

    Point point1 = uniform_point(0);

    float64_t length1 = 2;
    Quadtree *tree1 = Quadtree_init(length1, point1);

    Point_string(&point1, point_buffer);
    Quadtree_string(tree1, tree_buffer);

    sprintf(buffer, "searching for non-existent %s in %s", point_buffer, tree_buffer);
    assertFalse(Quadtree_search(tree1, point1), buffer);
    sprintf(buffer, "addition of %s into %s", point_buffer, tree_buffer);
    assertTrue(Quadtree_add(tree1, point1), buffer);
    sprintf(buffer, "searching for existent %s in %s", point_buffer, tree_buffer);
    assertTrue(Quadtree_search(tree1, point1), buffer);

    end_test();
    start_test("all-positive point");

    Point point2 = uniform_point(0.5);
    Point_string(&point2, point_buffer);

    sprintf(buffer, "searching for non-existent %s in %s", point_buffer, tree_buffer);
    assertFalse(Quadtree_search(tree1, point2), buffer);
    sprintf(buffer, "addition of %s into %s", point_buffer, tree_buffer);
    assertTrue(Quadtree_add(tree1, point2), buffer);
    sprintf(buffer, "searching for existent %s in %s", point_buffer, tree_buffer);
    assertTrue(Quadtree_search(tree1, point2), buffer);

    end_test();
    start_test("all-negative, non-terminating point");

    Point point3 = uniform_point(-1.0 / 3);
    Point_string(&point3, point_buffer);

    sprintf(buffer, "searching for non-existent %s in %s", point_buffer, tree_buffer);
    assertFalse(Quadtree_search(tree1, point3), buffer);
    sprintf(buffer, "addition of %s into %s", point_buffer, tree_buffer);
    assertTrue(Quadtree_add(tree1, point3), buffer);
    sprintf(buffer, "searching for existent %s in %s", point_buffer, tree_buffer);
    assertTrue(Quadtree_search(tree1, point3), buffer);

    end_test();
    start_test("creating smaller square");

    Point point4 = uniform_point(1.0 / 32);
    Point_string(&point4, point_buffer);

    sprintf(buffer, "searching for non-existent %s in %s", point_buffer, tree_buffer);
    assertFalse(Quadtree_search(tree1, point4), buffer);
    sprintf(buffer, "addition of %s into %s", point_buffer, tree_buffer);
    assertTrue(Quadtree_add(tree1, point4), buffer);
    sprintf(buffer, "searching for existent %s in %s", point_buffer, tree_buffer);
    assertTrue(Quadtree_search(tree1, point4), buffer);

    end_test();
    start_test("point off all-same-values line");

    Point point5 = uniform_point(0.125);
    point5.data[0] = -0.0625;
    Point_string(&point5, point_buffer);

    sprintf(buffer, "searching for non-existent %s in %s", point_buffer, tree_buffer);
    assertFalse(Quadtree_search(tree1, point5), buffer);
    sprintf(buffer, "addition of %s into %s", point_buffer, tree_buffer);
    assertTrue(Quadtree_add(tree1, point5), buffer);
    sprintf(buffer, "searching for existent %s in %s", point_buffer, tree_buffer);
    assertTrue(Quadtree_search(tree1, point5), buffer);

    end_test();
    start_test("all added points post-addition");

    Point_string(&point1, point_buffer);
    sprintf(buffer, "searching for existent %s in %s", point_buffer, tree_buffer);
    assertTrue(Quadtree_search(tree1, point1), buffer);

    Point_string(&point2, point_buffer);
    sprintf(buffer, "searching for existent %s in %s", point_buffer, tree_buffer);
    assertTrue(Quadtree_search(tree1, point2), buffer);

    Point_string(&point3, point_buffer);
    sprintf(buffer, "searching for existent %s in %s", point_buffer, tree_buffer);
    assertTrue(Quadtree_search(tree1, point3), buffer);

    Point_string(&point4, point_buffer);
    sprintf(buffer, "searching for existent %s in %s", point_buffer, tree_buffer);
    assertTrue(Quadtree_search(tree1, point4), buffer);

    Point_string(&point5, point_buffer);
    sprintf(buffer, "searching for existent %s in %s", point_buffer, tree_buffer);
    assertTrue(Quadtree_search(tree1, point5), buffer);

    end_test();

    Quadtree_free(tree1);
}

void test_quadtree_remove() {
    char buffer[256 + 15 * D];
    char tree_buffer[128 + 15 * D], point_buffer[15 * D];
    uint64_t i;

    start_test("center point");

    Point point1 = uniform_point(0);

    float64_t length1 = 2;
    Quadtree *tree1 = Quadtree_init(length1, point1);

    Point_string(&point1, point_buffer);
    Quadtree_string(tree1, tree_buffer);

    sprintf(buffer, "searching for non-existent %s in %s", point_buffer, tree_buffer);
    assertFalse(Quadtree_remove(tree1, point1), buffer);
    sprintf(buffer, "addition of %s into %s", point_buffer, tree_buffer);
    assertTrue(Quadtree_add(tree1, point1), buffer);
    sprintf(buffer, "removing for existent %s in %s", point_buffer, tree_buffer);
    assertTrue(Quadtree_remove(tree1, point1), buffer);

    end_test();
    start_test("all-positive point");

    Point point2 = uniform_point(0.5);
    Point_string(&point2, point_buffer);

    sprintf(buffer, "removing for non-existent %s in %s", point_buffer, tree_buffer);
    assertFalse(Quadtree_remove(tree1, point2), buffer);
    sprintf(buffer, "addition of %s into %s", point_buffer, tree_buffer);
    assertTrue(Quadtree_add(tree1, point2), buffer);
    sprintf(buffer, "removing for existent %s in %s", point_buffer, tree_buffer);
    assertTrue(Quadtree_remove(tree1, point2), buffer);

    end_test();
    start_test("all-negative, non-terminating point");

    Point point3 = uniform_point(-1.0 / 3);
    Point_string(&point3, point_buffer);

    sprintf(buffer, "removing for non-existent %s in %s", point_buffer, tree_buffer);
    assertFalse(Quadtree_remove(tree1, point3), buffer);
    sprintf(buffer, "addition of %s into %s", point_buffer, tree_buffer);
    assertTrue(Quadtree_add(tree1, point3), buffer);
    sprintf(buffer, "removing for existent %s in %s", point_buffer, tree_buffer);
    assertTrue(Quadtree_remove(tree1, point3), buffer);

    end_test();
    start_test("creating smaller square");

    Point point4 = uniform_point(1.0 / 32);
    Point_string(&point4, point_buffer);

    sprintf(buffer, "removing for non-existent %s in %s", point_buffer, tree_buffer);
    assertFalse(Quadtree_remove(tree1, point4), buffer);
    sprintf(buffer, "addition of %s into %s", point_buffer, tree_buffer);
    assertTrue(Quadtree_add(tree1, point4), buffer);
    sprintf(buffer, "removing for existent %s in %s", point_buffer, tree_buffer);
    assertTrue(Quadtree_remove(tree1, point4), buffer);

    end_test();
    start_test("point off all-same-values line");

    Point point5 = uniform_point(0.125);
    point5.data[0] = -0.0625;
    Point_string(&point5, point_buffer);

    sprintf(buffer, "removing for non-existent %s in %s", point_buffer, tree_buffer);
    assertFalse(Quadtree_remove(tree1, point5), buffer);
    sprintf(buffer, "addition of %s into %s", point_buffer, tree_buffer);
    assertTrue(Quadtree_add(tree1, point5), buffer);
    sprintf(buffer, "removing for existent %s in %s", point_buffer, tree_buffer);
    assertTrue(Quadtree_remove(tree1, point5), buffer);

    end_test();
    start_test("all added points post-readdition");

    Point_string(&point1, point_buffer);
    sprintf(buffer, "searching for removed %s in %s", point_buffer, tree_buffer);
    assertFalse(Quadtree_search(tree1, point1), buffer);

    Point_string(&point2, point_buffer);
    sprintf(buffer, "searching for removed %s in %s", point_buffer, tree_buffer);
    assertFalse(Quadtree_search(tree1, point2), buffer);

    Point_string(&point3, point_buffer);
    sprintf(buffer, "searching for removed %s in %s", point_buffer, tree_buffer);
    assertFalse(Quadtree_search(tree1, point3), buffer);

    Point_string(&point4, point_buffer);
    sprintf(buffer, "searching for removed %s in %s", point_buffer, tree_buffer);
    assertFalse(Quadtree_search(tree1, point4), buffer);

    Point_string(&point5, point_buffer);
    sprintf(buffer, "searching for removed %s in %s", point_buffer, tree_buffer);
    assertFalse(Quadtree_search(tree1, point5), buffer);

    Point_string(&point1, point_buffer);
    sprintf(buffer, "adding %s in %s", point_buffer, tree_buffer);
    assertTrue(Quadtree_add(tree1, point1), buffer);

    Point_string(&point2, point_buffer);
    sprintf(buffer, "adding %s in %s", point_buffer, tree_buffer);
    assertTrue(Quadtree_add(tree1, point2), buffer);

    Point_string(&point3, point_buffer);
    sprintf(buffer, "adding %s in %s", point_buffer, tree_buffer);
    assertTrue(Quadtree_add(tree1, point3), buffer);

    Point_string(&point4, point_buffer);
    sprintf(buffer, "adding %s in %s", point_buffer, tree_buffer);
    assertTrue(Quadtree_add(tree1, point4), buffer);

    Point_string(&point5, point_buffer);
    sprintf(buffer, "adding %s in %s", point_buffer, tree_buffer);
    assertTrue(Quadtree_add(tree1, point5), buffer);

    Point_string(&point1, point_buffer);
    sprintf(buffer, "searching for added %s in %s", point_buffer, tree_buffer);
    assertTrue(Quadtree_search(tree1, point1), buffer);

    Point_string(&point2, point_buffer);
    sprintf(buffer, "searching for added %s in %s", point_buffer, tree_buffer);
    assertTrue(Quadtree_search(tree1, point2), buffer);

    Point_string(&point3, point_buffer);
    sprintf(buffer, "searching for added %s in %s", point_buffer, tree_buffer);
    assertTrue(Quadtree_search(tree1, point3), buffer);

    Point_string(&point4, point_buffer);
    sprintf(buffer, "searching for added %s in %s", point_buffer, tree_buffer);
    assertTrue(Quadtree_search(tree1, point4), buffer);

    Point_string(&point5, point_buffer);
    sprintf(buffer, "searching for added %s in %s", point_buffer, tree_buffer);
    assertTrue(Quadtree_search(tree1, point5), buffer);

    Point_string(&point2, point_buffer);
    sprintf(buffer, "searching for added %s in %s", point_buffer, tree_buffer);
    assertTrue(Quadtree_search(tree1, point2), buffer);

    Point_string(&point3, point_buffer);
    sprintf(buffer, "searching for added %s in %s", point_buffer, tree_buffer);
    assertTrue(Quadtree_search(tree1, point3), buffer);

    Point_string(&point4, point_buffer);
    sprintf(buffer, "searching for existent %s in %s", point_buffer, tree_buffer);
    assertTrue(Quadtree_search(tree1, point4), buffer);

    Point_string(&point5, point_buffer);
    sprintf(buffer, "searching for existent %s in %s", point_buffer, tree_buffer);
    assertTrue(Quadtree_search(tree1, point5), buffer);

    Point_string(&point1, point_buffer);
    sprintf(buffer, "removing %s in %s", point_buffer, tree_buffer);
    assertTrue(Quadtree_remove(tree1, point1), buffer);

    Point_string(&point2, point_buffer);
    sprintf(buffer, "removing %s in %s", point_buffer, tree_buffer);
    assertTrue(Quadtree_remove(tree1, point2), buffer);

    Point_string(&point3, point_buffer);
    sprintf(buffer, "removing %s in %s", point_buffer, tree_buffer);
    assertTrue(Quadtree_remove(tree1, point3), buffer);

    Point_string(&point4, point_buffer);
    sprintf(buffer, "removing %s in %s", point_buffer, tree_buffer);
    assertTrue(Quadtree_remove(tree1, point4), buffer);

    Point_string(&point5, point_buffer);
    sprintf(buffer, "removing %s in %s", point_buffer, tree_buffer);
    assertTrue(Quadtree_remove(tree1, point5), buffer);

    Point_string(&point1, point_buffer);
    sprintf(buffer, "searching for removed %s in %s", point_buffer, tree_buffer);
    assertFalse(Quadtree_search(tree1, point1), buffer);

    Point_string(&point2, point_buffer);
    sprintf(buffer, "searching for removed %s in %s", point_buffer, tree_buffer);
    assertFalse(Quadtree_search(tree1, point2), buffer);

    Point_string(&point3, point_buffer);
    sprintf(buffer, "searching for removed %s in %s", point_buffer, tree_buffer);
    assertFalse(Quadtree_search(tree1, point3), buffer);

    Point_string(&point4, point_buffer);
    sprintf(buffer, "searching for removed %s in %s", point_buffer, tree_buffer);
    assertFalse(Quadtree_search(tree1, point4), buffer);

    Point_string(&point5, point_buffer);
    sprintf(buffer, "searching for removed %s in %s", point_buffer, tree_buffer);
    assertFalse(Quadtree_search(tree1, point5), buffer);

    end_test();

    Quadtree_free(tree1);
}

void test_randomized() {
    char buffer[256 + 30 * D];
    char tree_buffer[128 + 15 * D], point_buffer[15 * D];
    uint64_t i, j;

    start_test("10 random points, all within bounds");

    Point point1 = uniform_point(1);
    float64_t length1 = 2;
    Quadtree *tree1 = Quadtree_init(length1, point1);

    Quadtree_string(tree1, tree_buffer);

    Point points1[10];
    // Points are constructed to not coincide.
    for (i = 0; i < 10; i++) {
        for (j = 0; j< D; j++) {
            float64_t value = 2 * random() - 1;
            float64_t sign = (value < 0 ? -1 : 1);
            points1[i].data[j] = (value + sign * i) * length1 / 20.1;
        }
        Point_string(&points1[i], point_buffer);
        sprintf(buffer, "point %s successfully added to %s", point_buffer, tree_buffer);
        assertTrue(Quadtree_add(tree1, points1[i]), buffer);
    }
    for (i = 0; i < 10; i++) {
        Point_string(&points1[i], point_buffer);
        sprintf(buffer, "point %s exists in %s", point_buffer, tree_buffer);
        assertTrue(Quadtree_search(tree1, points1[i]), buffer);
    }
    for (i = 0; i < 10; i++) {
        Point_string(&points1[9 - i], point_buffer);
        sprintf(buffer, "point %s successfully removed from %s", point_buffer, tree_buffer);
        assertTrue(Quadtree_remove(tree1, points1[9 - i]), buffer);
    }
    for (i = 0; i < 10; i++) {
        Point_string(&points1[i], point_buffer);
        sprintf(buffer, "point %s no longer exists in %s", point_buffer, tree_buffer);
        assertFalse(Quadtree_search(tree1, points1[i]), buffer);
    }

    end_test();

    Quadtree_free(tree1);

    start_test("100 random points within bounds, 100 random points out of bounds");

    Point point2 = uniform_point(1);
    float64_t length2 = 2;
    Quadtree *tree2 = Quadtree_init(length2, point2);

    Quadtree_string(tree2, tree_buffer);

    Point points2[200];
    // Points are constructed to not coincide.
    for (i = 0; i < 100; i++) {
        for (j = 0; j< D; j++) {
            float64_t value = 2 * random() - 1;
            float64_t sign = (value < 0 ? -1 : 1);
            points2[i].data[j] = (value + sign * i) * length2 / 200.1;
        }
        Point_string(&points2[i], point_buffer);
        sprintf(buffer, "point %s successfully added to %s", point_buffer, tree_buffer);
        assertTrue(Quadtree_add(tree2, points2[i]), buffer);
    }
    for (i = 100; i < 200; i++) {
        for (j = 0; j< D; j++) {
            float64_t value = 2 * random() - 1;
            float64_t sign = (value < 0 ? -1 : 1);
            points2[i].data[j] = (value + sign * i) * length2 / 199.9;
        }
        Point_string(&points2[i], point_buffer);
        sprintf(buffer, "point %s successfully added to %s", point_buffer, tree_buffer);
        assertFalse(Quadtree_add(tree2, points2[i]), buffer);
    }
    for (i = 0; i < 100; i++) {
        Point_string(&points2[i], point_buffer);
        sprintf(buffer, "point %s exists in %s", point_buffer, tree_buffer);
        assertTrue(Quadtree_search(tree2, points2[i]), buffer);
    }
    for (i = 100; i < 200; i++) {
        Point_string(&points2[i], point_buffer);
        sprintf(buffer, "point %s absent from %s", point_buffer, tree_buffer);
        assertFalse(Quadtree_search(tree2, points2[i]), buffer);
    }
    for (i = 0; i < 100; i++) {
        Point_string(&points2[99 - i], point_buffer);
        sprintf(buffer, "point %s successfully removed from %s", point_buffer, tree_buffer);
        assertTrue(Quadtree_remove(tree2, points2[99 - i]), buffer);
    }
    for (i = 0; i < 100; i++) {
        Point_string(&points2[i], point_buffer);
        sprintf(buffer, "point %s no longer exists in %s", point_buffer, tree_buffer);
        assertFalse(Quadtree_search(tree2, points2[i]), buffer);
    }

    end_test();

    Quadtree_free(tree2);

}

/*
 * prepare_assertions
 *
 * length - the number of arguments to process
 * args - the string arguments to process
 *
 * Returns whether to abort running the program due to parameter errors.
 */
static bool prepare_assertions(const int length, char * const args[]) {
    uint64_t i, j;
    for (i = 0; i < length; i++) {
        for (j = 0; args[i][j] != '\0'; j++) {
            if ('=' == args[i][j]) {
                args[i][j] = '\0';
                j++;
                break;
            }
        }
        char type[100] = "", value[100] = "";
        sscanf(args[i], "%s", type);
        sscanf(args[i] + j, "%s", value);

        ASSERT_STATUS status = ERROR;
        if (!strcmp("ALL", value)) {
            status = ALL;
        } else if (!strcmp("PASSING", value)) {
            status = PASSING;
        } else if (!strcmp("FAILING", value)) {
            status = FAILING;
        } else if (!strcmp("NONE", value)) {
            status = NONE;
        }

        if (!strcmp("ASSERTS", type)) {
            if (ERROR != status) {
                setAssertions(status);
            }
        } else if (!strcmp("TESTS", type)) {
            if (ERROR != status) {
                setTests(status);
            }
        } else if (!strcmp("SUITES", type)) {
            if (ERROR != status) {
                setSuites(status);
            }
        } else if (!strcmp("MESSAGES", type)) {
            if (ALL == status || NONE == status) {
                setMessages(status);
            } else {
                status = ERROR;
            }
        } else {
            status = ALL;
        }

        // If status is still ERROR, then we have an invalid input for a valid type.
        if (ERROR == status) {
            printf("Invalid status for type %s. Valid statuss are: ALL,%s NONE.\n",
                type, strcmp("MESSAGES", type) ? " PASSING, FAILING," : "");
            return true;
        }
    }
    return false;
}

int main(int argc, char *argv[]) {
    setbuf(stdout, 0);

    printf("Turn reports on and off: [ASSERTS | TESTS | SUITES]=[ALL | PASSING | FAILING | NONE]\nTurn messages on and off: MESSAGES=[ALL | NONE]\n");

    // Set assertions settings
    if (prepare_assertions(argc - 1, argv + 1)) {
        return 1;
    }
    
    mtrace();
    Marsaglia_srand(time(NULL));
    printf("[Beginning tests]\n");

    // Initialize RLU
    RLU_INIT(RLU_TYPE_FINE_GRAINED, 8);
    rlu_self = (rlu_thread_data_t*)malloc(sizeof(*rlu_self));

    start_suite(test_sizes, "Struct sizes");
    start_suite(test_in_range, "in_range");
    start_suite(test_get_quadrant, "get_quadrant");
    start_suite(test_get_new_center, "get_new_center");
    start_suite(test_node_create, "Node_init");
    start_suite(test_quadtree_create, "Quadtree_init");
    start_suite(test_quadtree_add, "Quadtree_add");
    start_suite(test_quadtree_search, "Quadtree_search");
    start_suite(test_quadtree_remove, "Quadtree_remove");
    start_suite(test_randomized, "Randomized input");

    // End RLU
    free(rlu_self);

    printf("\n[Ending tests]\n");
    printf("\033[7;33m===================================================================\n");
    printf("                    TESTS AND ASSERTIONS REPORT                    \n");
    printf("                         DIMENSIONS: %5llu                         \n", (unsigned long long)D);
    printf("===================================================================\033[m\n");
    printf("\033[1;36mTOTAL  SUITES: %4lld\033\[m | \033[1;36mTOTAL  TESTS: %4lld\033\[m | \033\[1;36mTOTAL  ASSERTIONS: %5lld\033\[m\n", total_suites(), total_tests(), total_assertions());
    printf("\033[3;32mPASSED SUITES: %4lld\033\[m | \033[3;32mPASSED TESTS: %4lld\033\[m | \033\[3;32mPASSED ASSERTIONS: %5lld\033\[m\n", passed_suites(), passed_tests(), passed_assertions());
    printf("\033[3;31mFAILED SUITES: %4lld\033\[m | \033[3;31mFAILED TESTS: %4lld\033\[m | \033\[3;31mFAILED ASSERTIONS: %5lld\033\[m\n", total_suites() - passed_suites(), total_tests() - passed_tests(), total_assertions() - passed_assertions());
    printf("=============================================\n");

    printf("Turn reports on and off: [ASSERTS | TESTS | SUITES]=[ALL | PASSING | FAILING | NONE]\nTurn messages on and off: MESSAGES=[ALL | NONE]\n");
    return 0;
}
