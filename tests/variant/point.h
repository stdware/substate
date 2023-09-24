#ifndef POINT_H
#define POINT_H

#include <substate/stream.h>

class Point {
public:
    int x;
    int y;

    Point(int x = 0, int y = 0);
    Point(const Point &other);
    ~Point();

    friend Substate::IStream &operator>>(Substate::IStream &stream, Point &p);
    friend Substate::OStream &operator<<(Substate::OStream &stream, const Point &p);

    static int g_count();
};

#endif // POINT_H
