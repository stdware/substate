#ifndef SUBSTATE_TESTS_CODECTESTTOOLS_H
#define SUBSTATE_TESTS_CODECTESTTOOLS_H

#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include <substate/Codec.h>
#include <substate/Model.h>
#include <substate/ModelObserver.h>
#include <substate/private/Node_p.h>

/// Returns the encoding of \a node and its descendants.
inline std::string encodeNode(const ss::Node *node) {
    std::ostringstream stream(std::ios::binary);
    ss::OBinaryStream out(stream);
    ss::Encoder encoder(out);
    encoder.writeNode(node);
    return encoder.fail() ? std::string() : stream.str();
}

/// Decodes the node of \a bytes into \a model, or as a free node if \a model is \c nullptr.
/// Returns \c nullptr on failure, and also if \a bytes contains more than the node.
inline std::unique_ptr<ss::Node> decodeNode(const ss::Codec &codec, const std::string &bytes,
                                            ss::Model *model) {
    std::istringstream stream(bytes, std::ios::binary);
    ss::IBinaryStream in(stream);
    ss::Decoder decoder(codec, in, model);
    auto node = decoder.readNode();
    if (decoder.fail() || stream.peek() != std::char_traits<char>::eof()) {
        return nullptr;
    }
    return node;
}

/// Decodes the action of \a bytes for \a model. Returns \c nullptr on failure, and also if
/// \a bytes contains more than the action.
inline std::unique_ptr<ss::Action> decodeAction(const ss::Codec &codec, const std::string &bytes,
                                                ss::Model *model) {
    std::istringstream stream(bytes, std::ios::binary);
    ss::IBinaryStream in(stream);
    ss::Decoder decoder(codec, in, model);
    auto action = decoder.readAction();
    if (decoder.fail() || stream.peek() != std::char_traits<char>::eof()) {
        return nullptr;
    }
    return action;
}

/// An observer that encodes every action at its first execution, as a storage engine with a
/// log does, and collects the encodings of the transaction in progress.
class ActionRecorder : public ss::ModelObserver {
public:
    /// The encodings of the actions since the last clear().
    std::vector<std::string> actions;

    /// Whether an encoding failed.
    bool failed = false;

    inline void actionApplied(const ss::Action &action, ss::Action::Operation operation) override {
        if (operation != ss::Action::Execute) {
            return;
        }
        std::ostringstream stream(std::ios::binary);
        ss::OBinaryStream out(stream);
        ss::Encoder encoder(out);
        encoder.writeAction(action);
        failed = failed || encoder.fail();
        actions.push_back(stream.str());
    }
};

/// Decodes \a actions for \a model and executes them as one transaction, as the recovery of a
/// storage engine does. Returns whether every action was decoded.
inline bool replay(ss::Model &model, const ss::Codec &codec,
                   const std::vector<std::string> &actions) {
    model.beginTransaction();
    for (const auto &bytes : actions) {
        auto action = decodeAction(codec, bytes, &model);
        if (!action) {
            model.abortTransaction();
            return false;
        }
        ss::NodePrivate::execute(&model, std::move(action));
    }
    model.commitTransaction();
    return true;
}

#endif // SUBSTATE_TESTS_CODECTESTTOOLS_H
