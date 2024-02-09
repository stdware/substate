#include <iostream>
#include <utility>
#include <vector>
#include <charconv>
#include <cstdlib>
#include <string_view>
#include <map>

#include <substate/model.h>
#include <substate/fsengine.h>
#include <substate/vectornode.h>
#include <substate/structnode.h>
#include <substate/mappingnode.h>
#include <substate/sheetnode.h>
#include <substate/bytesnode.h>

#ifdef _WIN32
#  define CLEAR_SCREEN_COMMAND "cls"
#else
#  define CLEAR_SCREEN_COMMAND "clear"
#endif

using namespace Substate;

namespace {

    inline int stoi2(const std::string_view &s, int defaultValue = 0, bool *ok = nullptr) {
        auto res = std::from_chars(s.data(), s.data() + s.size(), defaultValue);
        if (ok) {
            *ok = res.ec == std::errc();
        }
        return defaultValue;
    }

    std::string join(const std::vector<std::string> &v, const std::string_view &delimiter) {
        if (v.empty())
            return {};

        std::string res;
        for (int i = 0; i < v.size() - 1; ++i) {
            res.append(v[i]);
            res.append(delimiter);
        }
        res.append(v.back());
        return res;
    }

    std::string join(const std::vector<std::string_view> &v, const std::string_view &delimiter) {
        if (v.empty())
            return {};

        std::string res;
        for (int i = 0; i < v.size() - 1; ++i) {
            res.append(v[i]);
            res.append(delimiter);
        }
        res.append(v.back());
        return res;
    }

    std::vector<std::string_view> parseCommand(const std::string_view &line) {
        std::vector<std::string_view> result;
        bool in_quotes = false;
        size_t start = 0;

        for (size_t i = 0; i < line.size(); ++i) {
            if (line[i] == '"' && (i == 0 || line[i - 1] != '\\')) {
                in_quotes = !in_quotes;
            } else if (line[i] == ' ' && !in_quotes) {
                if (i > start) {
                    result.emplace_back(line.data() + start, i - start);
                }
                start = i + 1;
            }
        }

        if (start < line.size()) {
            result.emplace_back(line.data() + start, line.size() - start);
        }
        return result;
    }

}

namespace {

    class Command;

    class Argument {
    public:
        inline Argument(std::string name = {});

        inline Argument &optional();
        inline Argument &digit();

    protected:
        bool m_digit;
        bool m_required;
        std::string m_name;

        friend class Command;
    };

    using CommandHandler = void (*)(const std::vector<std::string_view> &);

    class Command {
    public:
        inline Command(std::string name = {}, std::string help = {},
                       CommandHandler handler = nullptr);
        inline Command(std::initializer_list<std::string> names, std::string help = {},
                       CommandHandler handler = nullptr);

        inline Command &arg(Argument arg);
        inline Command &args(std::vector<Argument> args);
        inline bool empty() const;
        inline const std::vector<std::string> &names() const;

        bool run(const std::vector<std::string_view> &tokens) const;
        void help() const;

    protected:
        std::vector<std::string> m_names;
        std::string m_help;
        std::vector<Argument> m_args;
        CommandHandler m_handler;
    };

    inline Argument::Argument(std::string name)
        : m_digit(false), m_required(true), m_name(std::move(name)) {
    }

    inline Argument &Argument::optional() {
        m_required = false;
        return *this;
    }

    inline Argument &Argument::digit() {
        m_digit = true;
        return *this;
    }

    inline Command::Command(std::string name, std::string help, CommandHandler handler)
        : m_names({std::move(name)}), m_help(std::move(help)), m_handler(handler) {
    }

    inline Command::Command(std::initializer_list<std::string> names, std::string help,
                            CommandHandler handler)
        : m_names(names), m_help(std::move(help)), m_handler(handler) {
    }

    inline Command &Command::arg(Argument arg) {
        m_args.emplace_back(std::move(arg));
        return *this;
    }

    inline Command &Command::args(std::vector<Argument> args) {
        m_args = std::move(args);
        return *this;
    }

    inline bool Command::empty() const {
        return m_names.empty();
    }

    inline const std::vector<std::string> &Command::names() const {
        return m_names;
    }

    bool Command::run(const std::vector<std::string_view> &tokens) const {
        std::vector<std::string_view> args;
        for (int i = 0; i < m_args.size(); ++i) {
            const auto &arg = m_args[i];

            // Check if argument is enough
            if (i + 1 == tokens.size()) {
                if (!arg.m_required) {
                    args.emplace_back();
                    continue;
                }

                printf("Missing argument \"%s\".\n", arg.m_name.data());
                help();
                return false;
            }

            // Check if the token is a number
            const auto &token = tokens[i + 1];
            if (arg.m_digit) {
                bool ok;
                std::ignore = stoi2(token, 0, &ok);
                if (!ok) {
                    printf("The argument \"%s\" should be a digit.\n", arg.m_name.data());
                    help();
                    return false;
                }
            }
            args.push_back(token);
        }
        m_handler(args);
        return true;
    }

    void Command::help() const {
        std::string commands = join(m_names, std::string_view("/"));
        std::string args;
        for (const auto &arg : m_args) {
            args += ' ';
            if (arg.m_required) {
                args += '<' + arg.m_name + '>';
            } else {
                args += '[' + arg.m_name + ']';
            }
        }
        printf("%s%-10s%-40s%s\n", "    ", commands.data(), args.data(), m_help.data());
    }

}

namespace {

    class Environment {
    private:
        Environment();

    public:
        static Environment &instance();

    public:
        static const std::vector<Command> &commands();
        static const Command *findCommand(const std::string_view &name);

        static void cmd_help(const std::vector<std::string_view> &args);
        static void cmd_quit(const std::vector<std::string_view> &args);
        static void cmd_clear(const std::vector<std::string_view> &args);

        static void cmd_byte(const std::vector<std::string_view> &args);
        static void cmd_vec(const std::vector<std::string_view> &args);
        static void cmd_map(const std::vector<std::string_view> &args);
        static void cmd_sht(const std::vector<std::string_view> &args);

        static void cmd_bins(const std::vector<std::string_view> &args);
        static void cmd_brm(const std::vector<std::string_view> &args);
        static void cmd_bch(const std::vector<std::string_view> &args);

        static void cmd_vins(const std::vector<std::string_view> &args);
        static void cmd_vrm(const std::vector<std::string_view> &args);

        static void cmd_sins(const std::vector<std::string_view> &args);
        static void cmd_srm(const std::vector<std::string_view> &args);

        static void cmd_mins(const std::vector<std::string_view> &args);
        static void cmd_mrm(const std::vector<std::string_view> &args);

        static void cmd_info(const std::vector<std::string_view> &args);
        static void cmd_temp(const std::vector<std::string_view> &args);
        static void cmd_show(const std::vector<std::string_view> &args);
        static void cmd_root(const std::vector<std::string_view> &args);

        static void cmd_undo(const std::vector<std::string_view> &args);
        static void cmd_redo(const std::vector<std::string_view> &args);
        static void cmd_reset(const std::vector<std::string_view> &args);

    public:
        bool quit = false;

        std::map<int, Node *> tempItems;
        int maxTempId = -1;

        Model *model = nullptr;
    };

    Environment::Environment() = default;

    Environment &Environment::instance() {
        static Environment env;
        return env;
    }

    const std::vector<Command> &Environment::commands() {
        static std::vector<Command> commands = {
            Command({"help"}, "Show help information", cmd_help).arg(Argument("cmd").optional()),
            Command({"quit", "exit"}, "Exit", cmd_quit),
            Command({"cls", "clear"}, "Clear screen", cmd_clear),

            Command(),

            Command({"bytes", "bnew"}, "New bytes node", cmd_byte),
            Command({"vec", "vnew"}, "New vector node", cmd_vec),
            Command({"map", "mnew"}, "New mapping node", cmd_map),
            Command({"sheet", "snew"}, "New sheet node", cmd_sht),

            Command(),

            Command({"bins"}, "Insert bytes", cmd_bins)
                .arg(Argument("id").digit())
                .arg(Argument("idx").digit())
                .arg({"bytes"}),
            Command({"brm"}, "Remove bytes", cmd_brm)
                .arg(Argument("id").digit())
                .arg(Argument("idx").digit())
                .arg(Argument("cnt").digit()),
            Command({"bch"}, "Change bytes", cmd_bch)
                .arg(Argument("id").digit())
                .arg(Argument("idx").digit())
                .arg({"bytes"}),

            Command(),

            Command({"vins"}, "Insert vector item", cmd_vins)
                .arg(Argument("parent").digit())
                .arg(Argument("idx").digit())
                .arg(Argument("id").digit()),
            Command({"vrm"}, "Remove vector item", cmd_vrm)
                .arg(Argument("parent").digit())
                .arg(Argument("idx").digit())
                .arg(Argument("cnt").digit().optional()),

            Command(),

            Command({"sins"}, "Insert sheet item", cmd_sins)
                .arg(Argument("parent").digit())
                .arg(Argument("id").digit()),
            Command({"srm"}, "Remove sheet item", cmd_srm)
                .arg(Argument("parent").digit())
                .arg(Argument("idx").digit()),

            Command(),

            Command({"mins"}, "Insert map item", cmd_mins)
                .arg(Argument("parent").digit())
                .arg({"key"})
                .arg(Argument("id").digit()),
            Command({"mrm"}, "Remove map item", cmd_mrm)
                .arg(Argument("parent").digit())
                .arg({"key"}),

            Command(),

            Command({"info"}, "Show model information", cmd_info),
            Command({"temp"}, "Show temporary ids", cmd_temp),
            Command({"show", "ls"}, "Show node information", cmd_show).arg(Argument("id").digit()),
            Command({"root"}, "Set root node", cmd_root).arg(Argument("id").digit()),

            Command(),

            Command({"undo"}, "Undo", cmd_undo),
            Command({"redo"}, "Redo", cmd_redo),
            Command({"reset"}, "Reset model", cmd_reset),
        };
        return commands;
    }

    const Command *Environment::findCommand(const std::string_view &name) {
        static std::unordered_map<std::string_view, size_t> indexes = []() {
            std::unordered_map<std::string_view, size_t> res;
            const auto &commands = Environment::commands();
            for (size_t i = 0; i < commands.size(); ++i) {
                auto &cmd = commands.at(i);
                if (cmd.empty())
                    continue;

                for (const auto &name : cmd.names()) {
                    res.insert(std::make_pair(name.data(), i));
                }
            }
            return res;
        }();

        auto it = indexes.find(name);
        if (it == indexes.end())
            return nullptr;
        return &Environment::commands().at(it->second);
    }

    void Environment::cmd_help(const std::vector<std::string_view> &args) {
        auto &name = args.front();
        if (name.empty()) {
            for (const auto &cmd : commands()) {
                if (cmd.empty()) {
                    printf("\n");
                } else {
                    cmd.help();
                }
            }
            return;
        }

        auto cmd = findCommand(name);
        if (!cmd) {
            printf("Unknown command \"%s\".\n", name.data());
            return;
        }
        cmd->help();
    }

    void Environment::cmd_quit(const std::vector<std::string_view> &args) {
        QM_UNUSED(args)
        instance().quit = true;
    }

    void Environment::cmd_clear(const std::vector<std::string_view> &args) {
        QM_UNUSED(args)
        std::system(CLEAR_SCREEN_COMMAND);
    }

    void Environment::cmd_byte(const std::vector<std::string_view> &args) {
        auto &env = Environment::instance();

        auto node = new BytesNode();
        auto id = env.maxTempId--;
        env.tempItems.insert(std::make_pair(id, node));
        printf("Create new bytes node %d\n", id);
    }

    void Environment::cmd_vec(const std::vector<std::string_view> &args) {
        auto &env = Environment::instance();

        auto node = new VectorNode();
        auto id = env.maxTempId--;
        env.tempItems.insert(std::make_pair(id, node));
        printf("Create new vector node %d\n", id);
    }

    void Environment::cmd_map(const std::vector<std::string_view> &args) {
        auto &env = Environment::instance();

        auto node = new MappingNode();
        auto id = env.maxTempId--;
        env.tempItems.insert(std::make_pair(id, node));
        printf("Create new mapping node %d\n", id);
    }

    void Environment::cmd_sht(const std::vector<std::string_view> &args) {
        auto &env = Environment::instance();

        auto node = new SheetNode();
        auto id = env.maxTempId--;
        env.tempItems.insert(std::make_pair(id, node));
        printf("Create new sheet node %d\n", id);
    }

    bool findNodeCommon(int id, Node **nodeRef, bool free = false) {
        auto &env = Environment::instance();
        Node *node = nullptr;

        if (id < 0) {
            auto it = env.tempItems.find(id);
            if (it != env.tempItems.end()) {
                node = it->second;
            }
        } else {
            if (free) {
                printf("Node %d is not free.\n", id);
                return false;
            }
            node = env.model->indexOf(id);
        }

        if (!node) {
            printf("Cannot find node %d.\n", id);
            return false;
        }

        *nodeRef = node;
        return true;
    }

    void filterTempNodes() {
        auto &tempItems = Environment::instance().tempItems;
        for (auto it = tempItems.begin(); it != tempItems.end();) {
            if (it->second->index() > 0) {
                it = tempItems.erase(it);
            } else
                ++it;
        }
    }

    template <class ParentNode>
    bool findParentAndChild(int parentId, ParentNode **parentRef, int childId = 0,
                            Node **childRef = nullptr) {
        auto &env = Environment::instance();
        Node *parent = nullptr;

        if (parentId < 0) {
            auto it = env.tempItems.find(parentId);
            if (it != env.tempItems.end()) {
                parent = it->second;
            }
        } else {
            parent = env.model->indexOf(parentId);
        }

        if (!parent) {
            printf("Cannot find parent node %d.\n", parentId);
            return false;
        }

        Node *child = nullptr;
        if (childRef) {
            {
                auto it = env.tempItems.find(childId);
                if (it != env.tempItems.end()) {
                    child = it->second;
                }
            }

            if (!child) {
                printf("Cannot find child node %d.\n", childId);
                return false;
            }
        }

        auto realParent = dynamic_cast<ParentNode *>(parent);
        if (!realParent) {
            printf("Parent node %d type mismatch.\n", parentId);
            return false;
        }

        *parentRef = realParent;
        if (childRef)
            *childRef = child;
        return true;
    }

    inline void printOK() {
        printf("OK\n");
    }

    class TransactionGuard {
    public:
        explicit TransactionGuard(bool enable) {
            if (enable) {
                Environment::instance().model->beginTransaction();
            }
        }

        ~TransactionGuard() {
            auto model = Environment::instance().model;
            if (model->state() != Model::Idle) {
                model->commitTransaction({
                    {"action_index", std::to_string(model->maximumStep())}
                });
            }
        }
    };

    void Environment::cmd_bins(const std::vector<std::string_view> &args) {
        auto parentId = stoi2(args.front());
        auto idx = stoi2(args.at(1));
        auto bytes = args.at(2);

        BytesNode *parent;
        if (!findParentAndChild(parentId, &parent)) {
            return;
        }

        {
            TransactionGuard txGuard(parentId >= 0);
            if (!parent->insert(idx, bytes.data(), bytes.size())) {
                return;
            }
        }

        printOK();
    }

    void Environment::cmd_brm(const std::vector<std::string_view> &args) {
        auto parentId = stoi2(args.front());
        auto idx = stoi2(args.at(1));
        auto cnt = stoi2(args.at(2));

        BytesNode *parent;
        if (!findParentAndChild(parentId, &parent)) {
            return;
        }

        {
            TransactionGuard txGuard(parentId >= 0);
            if (!parent->remove(idx, cnt)) {
                return;
            }
        }

        printOK();
    }

    void Environment::cmd_bch(const std::vector<std::string_view> &args) {
        auto parentId = stoi2(args.front());
        auto idx = stoi2(args.at(1));
        auto bytes = args.at(2);

        BytesNode *parent;
        if (!findParentAndChild(parentId, &parent)) {
            return;
        }

        {
            TransactionGuard txGuard(parentId >= 0);
            if (!parent->replace(idx, bytes.data(), bytes.size())) {
                return;
            }
        }

        printOK();
    }

    void Environment::cmd_vins(const std::vector<std::string_view> &args) {
        auto parentId = stoi2(args.front());
        auto idx = stoi2(args.at(1));
        auto childId = stoi2(args.at(2));

        VectorNode *parent;
        Node *child;
        if (!findParentAndChild(parentId, &parent, childId, &child)) {
            return;
        }

        {
            TransactionGuard txGuard(parentId >= 0);
            if (idx >= parent->size() || idx < 0) {
                parent->append(child);
            } else {
                parent->insert(idx, child);
            }
        }

        filterTempNodes();
        printOK();
    }

    void Environment::cmd_vrm(const std::vector<std::string_view> &args) {
        auto parentId = stoi2(args.front());
        auto idx = stoi2(args.at(1));
        auto cnt = stoi2(args.at(2));

        VectorNode *parent;
        if (!findParentAndChild(parentId, &parent)) {
            return;
        }

        {
            TransactionGuard txGuard(parentId >= 0);
            if (!parent->remove(idx, cnt)) {
                return;
            }
        }

        printOK();
    }

    void Environment::cmd_sins(const std::vector<std::string_view> &args) {
        auto parentId = stoi2(args.front());
        auto childId = stoi2(args.at(1));

        SheetNode *parent;
        Node *child;
        if (!findParentAndChild(parentId, &parent, childId, &child)) {
            return;
        }

        {
            TransactionGuard txGuard(parentId >= 0);
            parent->insert(child);
        }

        filterTempNodes();
        printOK();
    }

    void Environment::cmd_srm(const std::vector<std::string_view> &args) {
        auto parentId = stoi2(args.front());
        auto idx = stoi2(args.at(1));

        SheetNode *parent;
        if (!findParentAndChild(parentId, &parent)) {
            return;
        }

        {
            TransactionGuard txGuard(parentId >= 0);
            if (!parent->remove(idx)) {
                return;
            }
        }

        printOK();
    }

    void Environment::cmd_mins(const std::vector<std::string_view> &args) {
        auto parentId = stoi2(args.front());
        auto key = args.at(1);
        auto childId = stoi2(args.at(2));

        MappingNode *parent;
        Node *child;
        if (!findParentAndChild(parentId, &parent, childId, &child)) {
            return;
        }

        {
            TransactionGuard txGuard(parentId >= 0);
            parent->setProperty(std::string(key), child);
        }

        filterTempNodes();
        printOK();
    }

    void Environment::cmd_mrm(const std::vector<std::string_view> &args) {
        auto parentId = stoi2(args.front());
        auto key = args.at(1);

        MappingNode *parent;
        if (!findParentAndChild(parentId, &parent)) {
            return;
        }

        {
            TransactionGuard txGuard(parentId >= 0);
            parent->clearProperty(std::string(key));
        }

        printOK();
    }

    static const char *nodeType2str(Node::Type type) {
        switch (type) {
            case Node::Bytes: {
                return "Bytes";
            }
            case Node::Vector: {
                return "Vector";
            }
            case Node::Mapping: {
                return "Mapping";
            }
            case Node::Sheet: {
                return "Sheet";
            }
            case Node::Struct: {
                return "Struct";
            }
            default:
                break;
        }
        return nullptr;
    }

    static void printNode(const std::string &msg, Node *node) {
        static int indent = 0;
        static const auto &printIndent = []() { printf("%s", std::string(indent, ' ').data()); };

        // Print name
        printIndent();
        if (!msg.empty())
            printf("%s: ", msg.data());
        printf("%s\n", nodeType2str(static_cast<Node::Type>(node->type())));

        indent += 2;

        // Print contents
        switch (node->type()) {
            case Node::Bytes: {
                auto n = static_cast<BytesNode *>(node);
                printIndent();
                printf("bytes: %s\n", std::string(n->data(), n->size()).data());
                break;
            }
            case Node::Vector: {
                auto n = static_cast<VectorNode *>(node);
                for (int i = 0; i < n->size(); ++i) {
                    auto child = n->at(i);
                    printNode(std::to_string(i), child);
                }
                break;
            }
            case Node::Mapping: {
                auto n = static_cast<MappingNode *>(node);
                const auto &data = n->data();
                for (const auto &item : data) {
                    auto &key = item.first;
                    auto &child = item.second;
                    if (child.isVariant()) {
                        printIndent();
                        printf("%s variant: %s\n", key.data(), child.variant().toString().data());
                    } else {
                        printNode(key, child.node());
                    }
                }
                break;
            }
            case Node::Sheet: {
                auto n = static_cast<SheetNode *>(node);
                std::map<int, Node *> data{n->data().begin(), n->data().end()};
                for (const auto &item : std::as_const(data)) {
                    printNode(std::to_string(item.first), item.second);
                }
                break;
            }
            case Node::Struct: {
                auto n = static_cast<StructNode *>(node);
                for (int i = 0; i < n->size(); ++i) {
                    auto val = n->at(i);
                    if (val.isVariant()) {
                        printIndent();
                        printf("%d variant: %s\n", i, val.variant().toString().data());
                    } else {
                        printNode(std::to_string(i), val.node());
                    }
                }
                break;
            }
            default:
                break;
        }

        indent -= 2;
    }

    void Environment::cmd_info(const std::vector<std::string_view> &args) {
        QM_UNUSED(args)

        auto model = Environment::instance().model;
        auto root = model->root();

        printf("Min step: %d\n", model->minimumStep());
        printf("Max step: %d\n", model->maximumStep());
        printf("Current step: %d\n", model->currentStep());
        printf("Root id: %d\n", root ? root->index() : 0);
    }

    void Environment::cmd_temp(const std::vector<std::string_view> &args) {
        auto &tempItems = Environment::instance().tempItems;

        for (const auto &item : std::as_const(tempItems)) {
            printf("%d ", item.first);
        }
        printf("\n");
    }

    void Environment::cmd_show(const std::vector<std::string_view> &args) {
        auto id = stoi2(args.front());

        Node *node;
        if (!findNodeCommon(id, &node)) {
            return;
        }

        printNode("Node", node);
    }

    void Environment::cmd_root(const std::vector<std::string_view> &args) {
        auto id = stoi2(args.front());

        Node *node;
        if (!findNodeCommon(id, &node, true)) {
            return;
        }

        {
            TransactionGuard txGuard(true);
            Environment::instance().model->setRoot(node);
        }

        filterTempNodes();
        printOK();
    }

    void Environment::cmd_undo(const std::vector<std::string_view> &args) {
        QM_UNUSED(args)

        Environment::instance().model->undo();

        printOK();
    }

    void Environment::cmd_redo(const std::vector<std::string_view> &args) {
        QM_UNUSED(args)

        Environment::instance().model->redo();

        printOK();
    }

    void Environment::cmd_reset(const std::vector<std::string_view> &args) {
        QM_UNUSED(args)

        Environment::instance().model->reset();

        printOK();
    }

}

int main(int argc, char *argv[]) {
    QM_UNUSED(argc)
    QM_UNUSED(argv)

#if 1
    static const std::filesystem::path model_path("model");

    auto engine = new FileSystemEngine();
    if (std::filesystem::is_directory(model_path)) {
        if (!engine->recover(model_path)) {
            printf("Recover Failed\n");
            return -1;
        }
        printf("Recover Success\n");
    } else {
        std::filesystem::create_directories(model_path);
        if (!engine->start(model_path)) {
            printf("Start Failed\n");
            return -1;
        }
    }

    Model model(engine);
#else
    Model model;
#endif

    auto &env = Environment::instance();
    env.model = &model;

    while (!env.quit) {
        printf("> ");

        std::string line;
        if (!std::getline(std::cin, line)) {
            break;
        }

        if (line.empty()) {
            continue;
        }

        auto tokens = parseCommand(line);
        if (tokens.empty()) {
            continue;
        }

        auto name = tokens.front();
        auto cmd = Environment::findCommand(name);
        if (!cmd) {
            printf("Unknown command \"%s\".\n", name.data());
            continue;
        }

        if (!cmd->run(tokens)) {
            continue;
        }
    }
    return 0;
}