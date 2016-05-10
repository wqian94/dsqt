/**
Point implementation
*/

#include "Point.h"

Point Point_from_array(float64_t data[D]) {
    Point p;
    memmove(&p.data, data, sizeof(p.data));
    return p;
}

int8_t Point_compare(const Point *a, const Point *b) {
    register uint64_t i;
    for (i = 0; i < D; i++) {
        if (abs(a->data[D - i - 1] - b->data[D - i - 1]) > PRECISION) {
            return (2 * (a->data[D - i - 1] > b->data[D - i - 1]) - 1);
        }
    }
}

bool Point_equals(const Point *a, const Point *b) {
    register uint64_t i;
    for (i = 0; i < D; i++) {
        if (abs(a->data[i] - b->data[i]) > PRECISION) {
            return false;
        }
    }
    return true;
}

void Point_copy(const Point* from, Point* to) {
    memcpy(&to->data, &from->data, sizeof(from->data));
}
