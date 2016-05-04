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
#define random() Marsaglia_random()

#define WRAP(statement) {RLU_THREAD_INIT(rlu_self);statement;RLU_THREAD_FINISH(rlu_self);}

typedef struct {
    bool on;
    uint32_t *food;
    uint32_t *first;
    uint32_t *last;
} TestRandTrough;

static TestRandTrough test_rand_trough = { .on = false, .food = NULL, .first = NULL, .last = NULL };

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
        if (test_rand_trough.food == test_rand_trough.last) {
            test_rand_trough.food = test_rand_trough.first;
        } else {
            test_rand_trough.food++;
        }
    }
    return next;
}

static ASSERT_STATUS TESTS_STATUS = ALL;
static ASSERT_STATUS SUITES_STATUS = ALL;
static ASSERT_STATUS MESSAGES_STATUS = ALL;

#define setTests(status) set_tests(status)
#define setSuites(status) set_suites(status)
#define setMessages(status) set_messages(status)

#define messagesOn() (ALL == MESSAGES_STATUS)

static inline void set_tests(const ASSERT_STATUS status) { TESTS_STATUS = status; }
static inline void set_suites(const ASSERT_STATUS status) { SUITES_STATUS = status; }
static inline void set_messages(const ASSERT_STATUS status) { MESSAGES_STATUS = status; }

static uint64_t TOTAL_TESTS = 0, PASSED_TESTS = 0;
static uint64_t TOTAL_SUITES = 0, PASSED_SUITES = 0;
static uint64_t PREV_TEST_TOTAL_ASSERTS = 0, PREV_TEST_PASSED_ASSERTS = 0;
static const char *TEST_NAME = "";

static inline bool display_result(const ASSERT_STATUS status, const bool passed) {
    return ALL == status || PASSING == status && passed || FAILING == status && !passed;
}

void start_suite(void (*func)(), const char *suite_name) {
    uint64_t prev_total_asserts = total_assertions();
    uint64_t prev_passed_asserts = passed_assertions();
    if (ALL == MESSAGES_STATUS) {
        printf("\n===Suite: %s===\n", suite_name);
    }
    func();
    if (ALL == MESSAGES_STATUS) {
        printf("\n");
    }
    test_rand_close();

    const bool passed = (passed_assertions() - prev_passed_asserts) == (total_assertions() - prev_total_asserts);
    TOTAL_SUITES++;
    PASSED_SUITES += passed;

    if (display_result(SUITES_STATUS, passed)) {
        printf("\n...%s (suite %s)\n", passed ? "\033[0;32mOK\033[m" : "\033[1;31mFAILED\033[m", suite_name);
    }
}

void start_test(const char *test_name) {
    PREV_TEST_TOTAL_ASSERTS = total_assertions();
    PREV_TEST_PASSED_ASSERTS = passed_assertions();
    TEST_NAME = test_name;
    if (strlen(TEST_NAME) > 0 && ALL == MESSAGES_STATUS) {
        printf("\n--Test: %s--\n", TEST_NAME);
    }
}

void end_test() {
    const bool passed = (passed_assertions() - PREV_TEST_PASSED_ASSERTS) == (total_assertions() - PREV_TEST_TOTAL_ASSERTS);
    TOTAL_TESTS++;
    PASSED_TESTS += passed;
    PREV_TEST_TOTAL_ASSERTS = 0;
    PREV_TEST_PASSED_ASSERTS = 0;

    if (strlen(TEST_NAME) > 0 && display_result(TESTS_STATUS, passed)) {
        printf("\n  ...%s (test %s)\n", passed ? "\033[0;32mOK\033[m" : "\033[1;31mFAILED\033[m", TEST_NAME);
    }
    TEST_NAME = "";
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
