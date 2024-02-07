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

#ifdef _WIN32
#  define CLEAR_SCREEN_COMMAND "cls"
#else
#  define CLEAR_SCREEN_COMMAND "clear"
#endif

using namespace Substate;

namespace {

    inline int stoi2(const std::string_view &s, int defaultValue, bool *ok = nullptr) {
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
        inline Command(std::initializer_list<std::string> names = {}, std::string help = {},
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

                printf("Missing argument \"%s\".", arg.m_name.data());
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

        static void cmd_newVector(const std::vector<std::string_view> &args);

    public:
        bool quit = false;

        std::map<int, Node *> tempItems;
        int maxTempId = -1;
    };

    Environment::Environment() = default;

    Environment &Environment::instance() {
        static Environment env;
        return env;
    }

    const std::vector<Command> &Environment::commands() {
        static std::vector<Command> commands = {
            Command("help", "Show help information", cmd_help).arg(Argument("cmd").optional()),
            Command({"quit", "exit"}, "Exit", cmd_quit),
            Command({"cls", "clear"}, "Clear screen", cmd_clear),
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

    void Environment::cmd_newVector(const std::vector<std::string_view> &args) {
        auto &env = Environment::instance();

        auto item = new VectorNode();
        auto id = env.maxTempId--;
        env.tempItems.insert(std::make_pair(id, item));
        printf("Create new vector item %d\n", id);
    }

}

int main(int argc, char *argv[]) {
    QM_UNUSED(argc)
    QM_UNUSED(argv)

    auto &env = Environment::instance();
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