#include "point.h"

#include <substate/variant.h>

static int g_cnt = 0;

Point::Point(int x, int y) : x(x), y(y) {
    g_cnt++;
}

Point::Point(const Point &other) {
    x = other.x;
    y = other.y;

    g_cnt++;
}

Point::~Point() {
    g_cnt--;
}

bool Point::operator==(const Point &other) const {
    return x == other.x && y == other.y;
}

Substate::IStream &operator>>(Substate::IStream &stream, Point &p) {
    stream >> p.x >> p.y;
    return stream;
}

Substate::OStream &operator<<(Substate::OStream &stream, const Point &p) {
    stream << p.x << p.y;
    return stream;
}

int Point::g_count() {
    return g_cnt;
}
