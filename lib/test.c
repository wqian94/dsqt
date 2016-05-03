/**
Testing suite for correctness of Quadtrees
*/

#include "test.h"

//extern __thread rlu_thread_data_t *rlu_self;
extern bool in_range(const Node*, const Point*);
extern void Point_string(const Point*, char*);

void test_sizes() {
    printf("dimensions        = %lu\n", (unsigned long)D);
    printf("sizeof(Quadtree)  = %lu\n", sizeof(Quadtree));
    printf("sizeof(uint64_t)  = %lu\n", sizeof(uint64_t));
    printf("sizeof(Node*)     = %lu\n", sizeof(Node*));
    printf("sizeof(float64_t) = %lu\n", sizeof(float64_t));
    printf("sizeof(Point)     = %lu\n", sizeof(Point));
    printf("\n===Testing Quadtree size===\n");
    // Quadtree is 24 bytes + 8 bytes for each dimension.
    #ifndef PARALLEL
    assertLong(8 * D + 24, sizeof(Quadtree), "sizeof(Quadtree)");
    #endif
}

void test_in_range() {
}

void test_get_quadrant() {
}

void test_get_new_center() {
}

void test_quadtree_create() {
}

void test_quadtree_add() {
}

void test_quadtree_search() {
}

void test_quadtree_remove() {
}

void test_randomized() {
}

void test_performance() {
}

int main(int argc, char *argv[]) {
    setbuf(stdout, 0);
    mtrace();
    Marsaglia_srand(time(NULL));
    printf("[Beginning tests]\n");

    // initialize RLU
    RLU_INIT(RLU_TYPE_FINE_GRAINED, 8);
    rlu_self = (rlu_thread_data_t*)malloc(sizeof(*rlu_self));
    
    start_test(test_sizes, "Struct sizes");
    /*start_test(test_in_range, "in_range");
    start_test(test_get_quadrant, "get_quadrant");
    start_test(test_get_new_center, "get_new_center");
    start_test(test_quadtree_create, "Quadtree_init");
    start_test(test_quadtree_add, "Quadtree_add");
    start_test(test_quadtree_search, "Quadtree_search");
    start_test(test_quadtree_remove, "Quadtree_remove");
    start_test(test_randomized, "Randomized (in-environment)");
    //start_test(test_performance, "Performance tests");*/

    // end RLU
    free(rlu_self);

    printf("\n[Ending tests]\n");
    printf("\033[7;33m=============================================\n");
    printf("         TESTS AND ASSERTIONS REPORT         \n");
    printf("              DIMENSIONS: %5llu              \n", (unsigned long long)D);
    printf("=============================================\033[m\n");
    printf("\033[1;36mTOTAL  TESTS: %4lld\033\[m | \033\[1;36mTOTAL  ASSERTIONS: %5lld\033\[m\n", total_tests(), total_assertions());
    printf("\033[3;32mPASSED TESTS: %4lld\033\[m | \033\[3;32mPASSED ASSERTIONS: %5lld\033\[m\n", passed_tests(), passed_assertions());
    printf("\033[3;31mFAILED TESTS: %4lld\033\[m | \033\[3;31mFAILED ASSERTIONS: %5lld\033\[m\n", total_tests() - passed_tests(), total_assertions() - passed_assertions());
    printf("=============================================\n");
    return 0;
}
