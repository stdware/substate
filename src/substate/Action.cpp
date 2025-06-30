#include "Action.h"

#include "Stream.h"

namespace ss {

    void Action::write(Action &action, std::ostream &os) {
        OStream stream(&os);
        stream << action.type();
        action.write(os);
    }

    std::unique_ptr<Action> ActionReader::readAction(std::istream &is) {
        IStream stream(&is);
        int type;
        stream >> type;
        return readAction(type, is);
    }

}