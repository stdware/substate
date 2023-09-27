#include <iostream>
#include <sstream>
#include <cassert>

#include <substate/stream.h>
#include <substate/variant.h>

#include "point.h"

using namespace Substate;

static void testInt() {
    std::stringstream ss;

    {
        OStream stream(&ss);
        Variant var(12);
        stream << var;
    }

    {
        IStream stream(&ss);
        Variant var2;
        stream >> var2;
        assert(var2.toInt32() == 12);
    }
}

static void testString() {
    std::stringstream ss;

    {
        OStream stream(&ss);
        Variant var(std::string("what"));
        stream << var;
    }

    {
        IStream stream(&ss);
        Variant var2;
        stream >> var2;
        assert(var2.toString() == "what");
    }
}

static void testUserType() {
    std::stringstream ss;
    {
        OStream stream(&ss);
        Point p(2, 3);
        Variant var = Variant::fromValue(p);
        stream << var;
    }

    {
        IStream stream(&ss);
        Variant var2;
        stream >> var2;

        const Point &p = *reinterpret_cast<const Point *>(var2.constData());
        assert(p.x == 2 && p.y == 3);
    }

    { Variant var1 = Variant::fromValue(uint64_t(&ss)); }

    assert(Point::g_count() == 0);
}

int main(int argc, char *argv[]) {

    testInt();
    testString();
    testUserType();

    return 0;
}