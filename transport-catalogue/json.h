#pragma once

#include <iostream>
#include <map>
#include <string>
#include <variant>
#include <vector>

namespace json {

    class Node;
    // Сохраните объявления Dict и Array без изменения
    using Dict = std::map<std::string, Node>;
    using Array = std::vector<Node>;

    // Эта ошибка должна выбрасываться при ошибках парсинга JSON
    class ParsingError : public std::runtime_error {
    public:
        using runtime_error::runtime_error;
    };

    struct PrintContext {
        std::ostream& out;
        int indent_step = 4;
        int indent = 0;

        void PrintIndent() const {
            for (int i = 0; i < indent; ++i) {
                out.put(' ');
            }
        }

        // Возвращает новый контекст вывода с увеличенным смещением
        PrintContext Indented() const {
            return { out, indent_step, indent_step + indent };
        }
    };

    class Node {
    public:

        using Value = std::variant<std::nullptr_t, Array, Dict, double, int, std::string, bool>;

        Node() = default;

        template <typename T>
        Node(T value)
            : value_(value) {
        }

        const Value& GetValue() const;

        int AsInt() const;

        bool AsBool() const;

        double AsDouble() const;

        const std::string& AsString() const;

        const Array& AsArray() const;

        const Dict& AsMap() const;

        bool IsInt() const;

        bool IsDouble() const;

        bool IsPureDouble() const;

        bool IsBool() const;

        bool IsString() const;

        bool IsNull() const;

        bool IsArray() const;

        bool IsMap() const;

        auto operator<=>(const Node&) const = default;

    private:
        Value value_;
    };

    class Document {
    public:
        Document() = default;

        explicit Document(Node root);

        const Node& GetRoot() const;

        auto operator<=>(const Document&) const = default;
    private:
        Node root_;
    };

    Document Load(std::istream& input);

    void PrintValue(std::nullptr_t, const PrintContext& ctx);

    void PrintValue(const Array& array, const PrintContext& ctx);

    void PrintValue(const Dict& map, const PrintContext& ctx);

    void PrintValue(const std::string& str, const PrintContext& ctx);

    void PrintValue(bool val, const PrintContext& ctx);

    void PrintValue(int val, const PrintContext& ctx);

    void PrintValue(double val, const PrintContext& ctx);

    void PrintValue(const Node& node, const PrintContext& ctx);

    void PrintNode(const Node& node, std::ostream& out);

    void Print(const Document& doc, std::ostream& output);

}  // namespace json