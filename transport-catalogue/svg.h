#pragma once

#include <cstdint>
#include <iostream>
#include <optional>
#include <sstream>
#include <variant>
#include <vector>
#include <memory>
#include <string>

namespace svg {
    struct Rgb {
        uint8_t red = 0;
        uint8_t green = 0;
        uint8_t blue = 0;
    };

    struct Rgba {
        uint8_t red = 0;
        uint8_t green = 0;
        uint8_t blue = 0;
        double opacity = 1.0;
    };

    using Color = std::variant<std::monostate, std::string, svg::Rgb, svg::Rgba>;
    inline constexpr Color NoneColor = std::monostate{};

    enum class StrokeLineCap {
        BUTT,
        ROUND,
        SQUARE,
    };

    enum class StrokeLineJoin {
        ARCS,
        BEVEL,
        MITER,
        MITER_CLIP,
        ROUND,
    };

    inline std::ostream& operator<<(std::ostream& os, StrokeLineCap stroke_line_cap) {
        switch (stroke_line_cap) {
        case StrokeLineCap::BUTT:   return os << "butt";
        case StrokeLineCap::ROUND: return os << "round";
        case StrokeLineCap::SQUARE:  return os << "square";
        default:           return os << "Unknown";
        }
    }

    inline std::ostream& operator<<(std::ostream& os, StrokeLineJoin stroke_line_join) {
        switch (stroke_line_join) {
        case StrokeLineJoin::ARCS:   return os << "arcs";
        case StrokeLineJoin::BEVEL: return os << "bevel";
        case StrokeLineJoin::MITER:  return os << "miter";
        case StrokeLineJoin::MITER_CLIP: return os << "miter-clip";
        case StrokeLineJoin::ROUND:  return os << "round";
        default:           return os << "Unknown";
        }
    }

    struct OstreamColorPrinter {
        std::ostream& out;

        void operator()(std::monostate) const {
            out << "none";
        }
        void operator()(std::string color_name) const {
            out << color_name;
        }
        void operator()(Rgb color) const {
            out << "rgb(" << static_cast<int>(color.red) << ',' << static_cast<int>(color.green) << ',' 
                << static_cast<int>(color.blue) << ')';
        }
        void operator()(Rgba color) const {
            out << "rgba(" << static_cast<int>(color.red) << ',' << static_cast<int>(color.green) << ',' 
                << static_cast<int>(color.blue) << ',' << color.opacity << ')';
        }
    };

    inline std::ostream& operator<<(std::ostream& os, Color color) {
        std::ostringstream strm;
        std::visit(OstreamColorPrinter{ strm }, color);
        return os << strm.str();
    }

    template <typename Owner>
    class PathProps {
    public:
        Owner& SetFillColor(Color color) {
            fill_color_ = std::move(color);
            return AsOwner();
        }
        Owner& SetStrokeColor(Color color) {
            stroke_color_ = std::move(color);
            return AsOwner();
        }

        Owner& SetStrokeLineCap(StrokeLineCap slc) {
            stroke_line_cap_ = slc;
            return AsOwner();
        }

        Owner& SetStrokeLineJoin(StrokeLineJoin slj) {
            stroke_line_join_ = slj;
            return AsOwner();
        }

        Owner& SetStrokeWidth(double stroke_width) {
            stroke_width_ = stroke_width;
            return AsOwner();
        }

    protected:
        ~PathProps() = default;

        // Метод RenderAttrs выводит в поток общие для всех путей атрибуты fill и stroke
        void RenderAttrs(std::ostream& out) const {
            using namespace std::literals;

            if (fill_color_) {
                out << "fill=\""sv << *fill_color_ << "\""sv;
            }
            if (stroke_color_) {
                out << " stroke=\""sv << *stroke_color_ << "\""sv;
            }
            if (stroke_width_) {
                out << " stroke-width=\""sv << *stroke_width_ << "\""sv;
            }
            if (stroke_line_cap_) {
                out << " stroke-linecap=\""sv << *stroke_line_cap_ << "\""sv;
            }
            if (stroke_line_join_) {
                out << " stroke-linejoin=\""sv << *stroke_line_join_ << "\""sv;
            }
        }

    private:
        Owner& AsOwner() {
            // static_cast безопасно преобразует *this к Owner&,
            // если класс Owner — наследник PathProps
            return static_cast<Owner&>(*this);
        }

        std::optional<Color> fill_color_;
        std::optional<Color> stroke_color_;
        std::optional<StrokeLineCap> stroke_line_cap_;
        std::optional<StrokeLineJoin> stroke_line_join_;
        std::optional<double> stroke_width_;
    };

    struct Point {
        Point() = default;
        Point(double x, double y)
            : x(x)
            , y(y) {
        }
        double x = 0;
        double y = 0;
    };

    /*
     * Вспомогательная структура, хранящая контекст для вывода SVG-документа с отступами.
     * Хранит ссылку на поток вывода, текущее значение и шаг отступа при выводе элемента
     */
    struct RenderContext {
        RenderContext(std::ostream& out)
            : out(out) {
        }

        RenderContext(std::ostream& out, int indent_step, int indent = 0)
            : out(out)
            , indent_step(indent_step)
            , indent(indent) {
        }

        RenderContext Indented() const {
            return { out, indent_step, indent + indent_step };
        }

        void RenderIndent() const {
            for (int i = 0; i < indent; ++i) {
                out.put(' ');
            }
        }

        std::ostream& out;
        int indent_step = 0;
        int indent = 0;
    };

    /*
     * Абстрактный базовый класс Object служит для унифицированного хранения
     * конкретных тегов SVG-документа
     * Реализует паттерн "Шаблонный метод" для вывода содержимого тега
     */
    class Object {
    public:
        void Render(const RenderContext& context) const;

        virtual ~Object() = default;

    private:
        virtual void RenderObject(const RenderContext& context) const = 0;
    };

    //INTERFACE (kinda)
    class ObjectContainer {
    public:
        // Добавляет в svg-документ объект-наследник svg::Object
        virtual void AddPtr(std::unique_ptr<Object>&& obj) = 0;
        /*
         Метод Add добавляет в svg-документ любой объект-наследник svg::Object.
         Пример использования:
         Document doc;
         doc.Add(Circle().SetCenter({20, 30}).SetRadius(15));
        */
        template <typename T>
        void Add(T obj) {
            static_assert(std::is_base_of<Object, std::decay_t<T>>::value,
                "T must derive from Object");
            auto ptr = std::make_unique<std::decay_t<T>>(std::forward<T>(obj));
            AddPtr(std::move(ptr));
        }

        virtual ~ObjectContainer() = default;
    };

    //INTERFACE
    class Drawable {
    public:
        virtual void Draw(ObjectContainer& container) const = 0;
        virtual ~Drawable() = default;
    };

    /*
     * Класс Circle моделирует элемент <circle> для отображения круга
     * https://developer.mozilla.org/en-US/docs/Web/SVG/Element/circle
     */
    class Circle final : public Object, public PathProps<Circle> {
    public:
        Circle& SetCenter(Point center);
        Circle& SetRadius(double radius);

    private:
        void RenderObject(const RenderContext& context) const override;

        Point center_;
        double radius_ = 1.0;
    };

    /*
     * Класс Polyline моделирует элемент <polyline> для отображения ломаных линий
     * https://developer.mozilla.org/en-US/docs/Web/SVG/Element/polyline
     */
    class Polyline final : public Object, public PathProps<Polyline> {
    public:
        // Добавляет очередную вершину к ломаной линии
        Polyline& AddPoint(Point point);

        /*
         * Прочие методы и данные, необходимые для реализации элемента <polyline>
         */
    private:
        void RenderObject(const RenderContext& context) const override;

        std::vector<Point> coords_;
    };

    /*
     * Класс Text моделирует элемент <text> для отображения текста
     * https://developer.mozilla.org/en-US/docs/Web/SVG/Element/text
     */
    class Text final : public Object, public PathProps<Text> {
    public:
        // Задаёт координаты опорной точки (атрибуты x и y)
        Text& SetPosition(Point pos);

        // Задаёт смещение относительно опорной точки (атрибуты dx, dy)
        Text& SetOffset(Point offset);

        // Задаёт размеры шрифта (атрибут font-size)
        Text& SetFontSize(uint32_t size);

        // Задаёт название шрифта (атрибут font-family)
        Text& SetFontFamily(std::string font_family);

        // Задаёт толщину шрифта (атрибут font-weight)
        Text& SetFontWeight(std::string font_weight);

        // Задаёт текстовое содержимое объекта (отображается внутри тега text)
        Text& SetData(std::string data);

    private:
        void RenderObject(const RenderContext& context) const override;

        Point position_;
        Point offset_;
        uint32_t font_size_ = 1;
        std::string font_family_;
        std::string font_weight_;
        std::string data_;
    };

    class Document : public ObjectContainer {
    public:
        // Добавляет в svg-документ объект-наследник svg::Object
        void AddPtr(std::unique_ptr<Object>&& obj) override;

        // Выводит в ostream svg-представление документа
        void Render(std::ostream& out) const;

        // Прочие методы и данные, необходимые для реализации класса Document
    private:
        std::vector<std::unique_ptr<Object>> docs_;
    };

}  // namespace svg