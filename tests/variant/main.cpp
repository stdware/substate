#include <iostream>
#include <sstream>
#include <cassert>

#include <substate/variant.h>
#include <substate/private/substate_utils_p.h>

using namespace Substate;

static void testInt() {
    std::stringstream ss;

    Variant var(12);
    var.write(ss);

    Variant var2 = Variant::read(ss);
    assert(var2.toInt32() == 12);
}

static void testString() {
    std::stringstream ss;

    Variant var(std::string("what"));
    var.write(ss);

    Variant var2 = Variant::read(ss);
    assert(var2.toString() == "what");
}

static void testUserType() {
    static int g_cnt = 0;
    class Point {
    public:
        int x;
        int y;

        Point(int x = 0, int y = 0) : x(x), y(y) {
            g_cnt++;
        }
        Point(const Point &other) {
            x = other.x;
            y = other.y;

            g_cnt++;
        }
        ~Point() {
            g_cnt--;
        }
    };

    int type = Variant::User + 1;
    Variant::addUserType(type, {
                                   [](std::istream &in) -> void * {
                                       Point p;
                                       if (!readInt32(in, p.x) || !readInt32(in, p.y)) {
                                           return nullptr;
                                       }
                                       return new Point(p);
                                   },
                                   [](const void *buf, std::ostream &out) -> bool {
                                       auto &p = *reinterpret_cast<const Point *>(buf);
                                       return writeInt32(out, p.x) && writeInt32(out, p.y);
                                   },
                                   [](const void *buf) -> void * {
                                       return buf ? new Point(*reinterpret_cast<const Point *>(buf))
                                                  : new Point();
                                   },
                                   [](void *buf) {
                                       delete reinterpret_cast<Point *>(buf); //
                                   },
                               });

    std::stringstream ss;

    {
        Point p(2, 3);
        Variant var(type, &p);
        var.write(ss);
    }

    {
        Variant var = Variant::read(ss);
        const Point &p = *reinterpret_cast<const Point *>(var.constData());
        assert(p.x == 2 && p.y == 3);
    }

    assert(g_cnt == 0);

    Variant::removeUserType(type);
}

int main(int argc, char *argv[]) {

    testInt();
    testString();
    testUserType();

    return 0;
}