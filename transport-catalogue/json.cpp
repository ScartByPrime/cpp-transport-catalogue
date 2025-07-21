#include "json.h"

#include <cctype>

using namespace std;

namespace json {

    namespace {

        Node LoadNode(istream& input);

        Node LoadArray(istream& input) {
            Array result;

            for (char c; input >> c && c != ']';) {
                if (c != ',') {
                    input.putback(c);
                }
                result.push_back(LoadNode(input));
            }

            return Node(move(result));
        }

        Node LoadNumber(std::istream& input) {
            using namespace std::literals;

            std::string parsed_num;

            // Считывает в parsed_num очередной символ из input
            auto read_char = [&parsed_num, &input] {
                parsed_num += static_cast<char>(input.get());
                if (!input) {
                    throw ParsingError("Failed to read number from stream"s);
                }
                };

            // Считывает одну или более цифр в parsed_num из input
            auto read_digits = [&input, read_char] {
                if (input.peek() == ':') {
                    cerr << "LoadNode fail, ':' is in process" << endl;
                }
                if (!std::isdigit(input.peek())) {
                    throw ParsingError("A digit is expected"s);
                }
                while (std::isdigit(input.peek())) {
                    read_char();
                }
                };

            if (input.peek() == '-') {
                read_char();
            }
            // Парсим целую часть числа
            if (input.peek() == '0') {
                read_char();
                // После 0 в JSON не могут идти другие цифры
            }
            else {
                read_digits();
            }

            bool is_int = true;
            // Парсим дробную часть числа
            if (input.peek() == '.') {
                read_char();
                read_digits();
                is_int = false;
            }

            // Парсим экспоненциальную часть числа
            if (int ch = input.peek(); ch == 'e' || ch == 'E') {
                read_char();
                if (ch = input.peek(); ch == '+' || ch == '-') {
                    read_char();
                }
                read_digits();
                is_int = false;
            }

            try {
                if (is_int) {
                    // Сначала пробуем преобразовать строку в int
                    try {
                        return Node(std::stoi(parsed_num));
                    }
                    catch (...) {
                        // В случае неудачи, например, при переполнении,
                        // код ниже попробует преобразовать строку в double
                    }
                }
                return Node(std::stod(parsed_num));
            }
            catch (...) {
                throw ParsingError("Failed to convert "s + parsed_num + " to number"s);
            }
        }

        // Считывает содержимое строкового литерала JSON-документа
        // Функцию следует использовать после считывания открывающего символа ":
        Node LoadString(std::istream& input) {
            using namespace std::literals;

            auto it = std::istreambuf_iterator<char>(input);
            auto end = std::istreambuf_iterator<char>();
            std::string s;
            while (true) {
                if (it == end) {
                    // Поток закончился до того, как встретили закрывающую кавычку?
                    throw ParsingError("String parsing error");
                }
                const char ch = *it;
                if (ch == '"') {
                    // Встретили закрывающую кавычку
                    ++it;
                    break;
                }
                else if (ch == '\\') {
                    // Встретили начало escape-последовательности
                    ++it;
                    if (it == end) {
                        // Поток завершился сразу после символа обратной косой черты
                        throw ParsingError("String parsing error");
                    }
                    const char escaped_char = *(it);
                    // Обрабатываем одну из последовательностей: \\, \n, \t, \r, \"
                    switch (escaped_char) {
                    case 'n':
                        s.push_back('\n');
                        break;
                    case 't':
                        s.push_back('\t');
                        break;
                    case 'r':
                        s.push_back('\r');
                        break;
                    case '"':
                        s.push_back('"');
                        break;
                    case '\\':
                        s.push_back('\\');
                        break;
                    default:
                        // Встретили неизвестную escape-последовательность
                        throw ParsingError("Unrecognized escape sequence \\"s + escaped_char);
                    }
                }
                else if (ch == '\n' || ch == '\r') {
                    // Строковый литерал внутри- JSON не может прерываться символами \r или \n
                    throw ParsingError("Unexpected end of line"s);
                }
                else {
                    // Просто считываем очередной символ и помещаем его в результирующую строку
                    s.push_back(ch);
                }
                ++it;
            }
            return Node(s);
        }

        Node LoadDict(istream& input) {
            Dict result;

            for (char c; input >> c && c != '}';) {
                if (c == ',') {
                    input >> c;
                }

                if (c != '"') {
                    cerr << "before the key goes \' " << c << " \'" << endl;
                    throw ParsingError("Expected opening '\"' before key");
                }

                string key = LoadString(input).AsString();
                input >> c;
                result.insert({ move(key), LoadNode(input) });
            }

            return Node(move(result));
        }

        Node LoadNode(istream& input) {
            {
                char c1;
                if (!(input >> c1)) {
                    throw ParsingError("Empty stream or reading error");
                }

                if (c1 == '[' || c1 == '{') {
                    char c2;
                    if (!(input >> c2)) {
                        throw ParsingError("Unexpected end of container");
                    }
                    input.putback(c2);
                }

                input.putback(c1);
            }

            char c;
            input >> c;
            if (input.eof()) {
                std::cerr << "Reached end of file before expected!" << std::endl;
            }
            if (input.fail()) {
                std::cerr << "Input stream failure detected!" << std::endl;
            }
            if (c == '[') {
                return LoadArray(input);
            }
            else if (c == '{') {
                return LoadDict(input);
            }
            else if (c == '"') {
                return LoadString(input);
            }
            else if (c == 'n') {
                char buf[3];
                input.read(buf, 3);
                if (buf[0] == 'u' && buf[1] == 'l' && buf[2] == 'l') {
                    if (std::isalpha(input.peek())) {
                        throw ParsingError("Unexpected tokens after 'null'");
                    }
                    return Node();
                }
                else {
                    throw ParsingError("Unexpected tokens, expected 'null'");
                }
            }
            else if (c == 't') {
                char buf[3];
                input.read(buf, 3);
                if (buf[0] == 'r' && buf[1] == 'u' && buf[2] == 'e') {
                    if (std::isalpha(input.peek())) {
                        throw ParsingError("Unexpected tokens after 'true'");
                    }
                    return Node(true);
                }
                else {
                    throw ParsingError("Unexpected tokens, expected 'true'");
                }
            }
            else if (c == 'f') {
                char buf[4];
                input.read(buf, 4);
                if (buf[0] == 'a' && buf[1] == 'l' && buf[2] == 's' && buf[3] == 'e') {
                    char next = static_cast<char>(input.peek());
                    if (std::isalpha(next)) {
                        throw ParsingError("Unexpected token after 'false'");
                    }
                    return Node(false);
                }
                else {
                    throw ParsingError("Unexpected tokens, expected 'false'");
                }
            }

            else {
                input.putback(c);
                return LoadNumber(input);
            }
        }

    }  // namespace

    const Node::Value& Node::GetValue() const {
        return value_;
    }

    bool Node::IsInt() const {
        return holds_alternative<int>(value_);
    }

    bool Node::IsDouble() const {
        if (holds_alternative<double>(value_)
            || holds_alternative<int>(value_)) {
            return true;
        }
        return false;
    }

    bool Node::IsPureDouble() const {
        return holds_alternative<double>(value_);
    }

    bool Node::IsBool() const {
        return holds_alternative<bool>(value_);
    }

    bool Node::IsString() const {
        return holds_alternative<string>(value_);
    }

    bool Node::IsNull() const {
        return holds_alternative<nullptr_t>(value_);
    }

    bool Node::IsArray() const {
        return holds_alternative<Array>(value_);
    }

    bool Node::IsMap() const {
        return holds_alternative<Dict>(value_);
    }

    const Array& Node::AsArray() const {
        if (IsArray()) {
            return get<Array>(GetValue());
        }
        else throw std::logic_error("Unexpected node value, expected 'Array'");
    }

    const Dict& Node::AsMap() const {
        if (IsMap()) {
            return get<Dict>(GetValue());
        }
        else throw std::logic_error("Unexpected node value, expected 'Dict'");
    }

    int Node::AsInt() const {
        if (IsInt()) {
            return get<int>(GetValue());
        }
        else throw std::logic_error("Unexpected node value, expected 'Int'");
    }

    const string& Node::AsString() const {
        if (IsString()) {
            return get<string>(GetValue());
        }
        else throw std::logic_error("Unexpected node value, expected 'String'");
    }

    bool Node::AsBool() const {
        if (IsBool()) {
            return get<bool>(GetValue());
        }
        else throw std::logic_error("Unexpected node value, expected 'bool'");
    }

    double Node::AsDouble() const {
        if (IsPureDouble()) {
            return get<double>(GetValue());
        }
        else if (IsInt()) {
            return static_cast<double>(get<int>(GetValue()));
        }
        else throw std::logic_error("Unexpected node value, expected 'Double' or 'Int'");
    }

    void PrintValue(int val, const PrintContext& ctx) {
        ctx.out << val;
    }

    void PrintValue(double val, const PrintContext& ctx) {
        ctx.out << val;
    }

    void PrintValue(std::nullptr_t, const PrintContext& ctx) {
        ctx.out << "null"sv;
    }

    void PrintValue(const std::string& str, const PrintContext& ctx) {
        ctx.out << '"';
        for (char c : str) {
            switch (c) {
            case '\n': ctx.out << "\\n"; break;
            case '\r': ctx.out << "\\r"; break;
            case '\t': ctx.out << "\\t"; break;
            case '\\': ctx.out << "\\\\"; break;
            case '"':  ctx.out << "\\\""; break;
            default:   ctx.out << c; break;
            }
        }
        ctx.out << '"';
    }

    void PrintValue(bool val, const PrintContext& ctx) {
        ctx.out << std::boolalpha << val;
    }

    void PrintValue(const Array& array, const PrintContext& ctx) {
        ctx.out << "[\n";
        auto inner_ctx = ctx.Indented();
        for (size_t i = 0; i < array.size(); ++i) {
            inner_ctx.PrintIndent();
            PrintValue(array[i], inner_ctx);
            if (i + 1 != array.size()) {
                ctx.out << ',';
            }
            ctx.out << '\n';
        }
        ctx.PrintIndent();
        ctx.out << ']';
    }

    void PrintValue(const Dict& map, const PrintContext& ctx) {
        ctx.out << "{\n";
        auto inner_ctx = ctx.Indented();
        for (auto it = map.begin(); it != map.end(); ) {
            inner_ctx.PrintIndent();
            PrintValue(it->first, inner_ctx);
            ctx.out << ": ";
            PrintValue(it->second, inner_ctx);
            ++it;
            if (it != map.end()) {
                ctx.out << ',';
            }
            ctx.out << '\n';
        }
        ctx.PrintIndent();
        ctx.out << '}';
    }

    void PrintNode(const Node& node, std::ostream& out) {
        PrintContext ctx{ out };
        std::visit(
            [&ctx](const auto& value) { PrintValue(value, ctx); },
            node.GetValue());
    }

    void PrintValue(const Node& node, const PrintContext& ctx) {
        PrintNode(node, ctx.out);
    }

    Document::Document(Node root)
        : root_(move(root)) {
    }

    const Node& Document::GetRoot() const {
        return root_;
    }

    Document Load(istream& input) {
        return Document{ LoadNode(input) };
    }

    void Print(const Document& doc, std::ostream& output) {
        PrintNode(doc.GetRoot(), output);
    }

}  // namespace json