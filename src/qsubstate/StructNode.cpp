#include "StructNode.h"

#include <cassert>
#include <cstdint>

#include <substate/Codec.h>
#include <substate/private/Node_p.h>

#include "Property_p.h"

namespace ss {

    // A slot, as an end of a transfer.
    class StructNodeEndpoint : public TransferEndpoint {
    public:
        StructNodeEndpoint(StructNodeBase *node, int index) : m_node(node), m_index(index) {
        }

        Node *container() const override {
            return m_node;
        }

        std::vector<std::unique_ptr<Node>> take(int count) override {
            assert(count == 1);
            (void) count;
            std::vector<std::unique_ptr<Node>> taken;
            taken.push_back(PropertyPrivate::releaseChild(m_node->m_slots[m_index]));
            return taken;
        }

        void put(std::vector<std::unique_ptr<Node>> nodes) override {
            assert(nodes.size() == 1 && m_node->m_slots[m_index].isEmpty());
            m_node->m_slots[m_index] = Property(std::move(nodes.front()));
        }

        void write(Encoder &encoder) const override {
            encoder.stream() << int32_t(m_index);
        }

    private:
        StructNodeBase *m_node;
        int m_index;
    };

    StructNodeBase::~StructNodeBase() = default;

    std::unique_ptr<TransferEndpoint> StructNodeBase::readEndpoint(Decoder &decoder) {
        int32_t index = -1;
        decoder.stream() >> index;
        if (decoder.fail() || index < 0 || index >= m_size) {
            return nullptr;
        }
        return std::make_unique<StructNodeEndpoint>(this, index);
    }

    void StructNodeBase::writeContent(Encoder &encoder) const {
        encoder.stream() << int32_t(m_size);
        for (int i = 0; i < m_size; ++i) {
            PropertyPrivate::write(encoder, m_slots[i]);
        }
    }

    bool StructNodeBase::readContent(Decoder &decoder) {
        int32_t size = -1;
        decoder.stream() >> size;
        if (decoder.fail() || size != m_size) {
            return false;
        }
        for (int i = 0; i < m_size; ++i) {
            auto value = PropertyPrivate::read(decoder);
            if (decoder.fail()) {
                return false;
            }
            PropertyPrivate::assignFree(this, m_slots[i], std::move(value));
        }
        return true;
    }

    bool StructNodeBase::transferIn(int index, Node *node) {
        assert(isWritable() && !isFree());
        assert(index >= 0 && index < m_size);
        if (!m_slots[index].isEmpty()) {
            return false;
        }
        return NodePrivate::transfer(this, std::make_unique<StructNodeEndpoint>(this, index),
                                     {node});
    }

    std::unique_ptr<TransferEndpoint>
        StructNodeBase::endpointOf(const std::vector<Node *> &children) {
        if (children.size() != 1) {
            return nullptr;
        }
        for (int i = 0; i < m_size; ++i) {
            if (m_slots[i].child() == children.front()) {
                return std::make_unique<StructNodeEndpoint>(this, i);
            }
        }
        return nullptr;
    }

    void StructNodeBase::setAt(int index, Property value) {
        assert(isWritable());
        assert(index >= 0 && index < m_size);
        assert(PropertyPrivate::isAssignable(value, this));

        auto &slot = m_slots[index];
        if (slot == value) {
            return;
        }

        if (isFree()) {
            PropertyPrivate::assignFree(this, slot, std::move(value));
            return;
        }

        std::unique_ptr<StructAssignAction> action(
            new StructAssignAction(this, index, std::move(value)));
        NodePrivate::execute(model(), std::move(action));
    }

    Property StructNodeBase::take(int index) {
        assert(isFree());
        assert(index >= 0 && index < m_size);
        return PropertyPrivate::take(m_slots[index]);
    }

    void StructNodeBase::forEachChild(const std::function<void(Node *)> &func) const {
        for (int i = 0; i < m_size; ++i) {
            if (auto child = m_slots[i].child()) {
                func(child);
            }
        }
    }

    void StructNodeBase::cloneSlotsFrom(const StructNodeBase &source) {
        assert(isFree() && m_size == source.m_size);
        for (int i = 0; i < m_size; ++i) {
            assert(m_slots[i].isEmpty());
            PropertyPrivate::assignFree(this, m_slots[i], source.m_slots[i].clone());
        }
    }

    StructAssignAction::StructAssignAction(StructNodeBase *parent, int index, Property value)
        : PropertyAction(StructAssign, parent, parent->at(index), std::move(value)),
          m_index(index) {
    }

    StructAssignAction::StructAssignAction(StructNodeBase *parent, int index, QVariant oldVariant,
                                           Node *oldChild, Property value)
        : PropertyAction(StructAssign, parent, std::move(oldVariant), oldChild, std::move(value)),
          m_index(index) {
    }

    StructAssignAction::~StructAssignAction() = default;

    void StructAssignAction::execute(Operation operation) {
        (void) operation;
        exchange(static_cast<StructNodeBase *>(parent())->m_slots[m_index]);
    }

    void StructAssignAction::write(Encoder &encoder) const {
        encoder.writeReference(parent());
        encoder.stream() << int32_t(m_index);
        writeValues(encoder);
    }

    std::unique_ptr<Action> StructAssignAction::read(Decoder &decoder) {
        auto parent = dynamic_cast<StructNodeBase *>(decoder.readReference());
        int32_t index = -1;
        decoder.stream() >> index;
        if (decoder.fail() || !parent || index < 0 || index >= parent->size()) {
            decoder.setFailed();
            return nullptr;
        }
        auto value = PropertyPrivate::read(decoder);
        auto old = PropertyPrivate::readReference(decoder);
        if (decoder.fail()) {
            return nullptr;
        }
        return std::unique_ptr<Action>(new StructAssignAction(parent, index, std::move(old.first),
                                                              old.second, std::move(value)));
    }

}
