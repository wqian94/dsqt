/**
Testing suite utilities header
*/

#ifndef QUADTREE_TEST_H
#define QUADTREE_TEST_H

#include <omp.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <assert.h>
#include <mcheck.h>

#include "assertions.h"
#include "types.h"
#include "util.h"
#include "Quadtree.h"

extern __thread rlu_thread_data_t *rlu_self;
extern uint32_t Marsaglia_rand();
#define rand() Marsaglia_rand()

#define WRAP(statement) {RLU_THREAD_INIT(rlu_self);statement;RLU_THREAD_FINISH(rlu_self);}

typedef struct {
    bool on;
    uint32_t *food;
    uint32_t *first;
    uint32_t *last;
} TestRandTrough;

static TestRandTrough test_rand_trough = {.on = false, .food = NULL, .first = NULL, .last = NULL};

void test_rand_feed(uint32_t *food, uint32_t length) {
    test_rand_trough.on = true;
    test_rand_trough.first = food;
    test_rand_trough.last = food + length - 1;
    test_rand_trough.food = test_rand_trough.first;
}

void test_rand_on() {
    test_rand_trough.on = true;
}

void test_rand_off() {
    test_rand_trough.on = false;
}

void test_rand_close() {
    test_rand_trough.on = false;
    test_rand_trough.first = NULL;
    test_rand_trough.last = NULL;
    test_rand_trough.food = NULL;
}

int test_rand() {
    int next = rand();
    if (test_rand_trough.on) {
        next = *test_rand_trough.food;
        if (test_rand_trough.food == test_rand_trough.last)
            test_rand_trough.food = test_rand_trough.first;
        else
            test_rand_trough.food++;
    }
    return next;
}

static uint64_t TOTAL_TESTS = 0, PASSED_TESTS = 0;
static uint64_t TOTAL_SUITES = 0, PASSED_SUITES = 0;
static uint64_t PREV_TEST_TOTAL_ASSERTS = 0, PREV_TEST_PASSED_ASSERTS = 0;

void start_suite(void (*func)(), const char *suite_name) {
    uint64_t prev_total_asserts = total_assertions();
    uint64_t prev_passed_asserts = passed_assertions();
    printf("\n===Suite: %s===\n", suite_name);
    func();
    printf("\n");
    test_rand_close();
    TOTAL_SUITES++;
    PASSED_SUITES += (passed_assertions() - prev_passed_asserts) == (total_assertions() - prev_total_asserts);
}

void start_test(const char *test_name) {
    PREV_TEST_TOTAL_ASSERTS = total_assertions();
    PREV_TEST_PASSED_ASSERTS = passed_assertions();
    if (strlen(test_name) > 0)
        printf("\n--Test: %s--\n", test_name);
}

void end_test() {
    TOTAL_TESTS++;
    PASSED_TESTS += (passed_assertions() - PREV_TEST_PASSED_ASSERTS) == (total_assertions() - PREV_TEST_TOTAL_ASSERTS);
    PREV_TEST_TOTAL_ASSERTS = 0;
    PREV_TEST_PASSED_ASSERTS = 0;
}

unsigned long long total_tests() {
    return TOTAL_TESTS;
}

unsigned long long passed_tests() {
    return PASSED_TESTS;
}

unsigned long long total_suites() {
    return TOTAL_SUITES;
}

unsigned long long passed_suites() {
    return PASSED_SUITES;
}

#endif
