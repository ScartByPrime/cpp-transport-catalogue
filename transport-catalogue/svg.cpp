#include "svg.h"

#include <cmath>

namespace svg {

    using namespace std::literals;

    void Object::Render(const RenderContext& context) const {
        context.RenderIndent();

        // Делегируем вывод тега своим подклассам
        RenderObject(context);

        context.out << std::endl;
    }

    // ---------- Circle ------------------

    Circle& Circle::SetCenter(Point center) {
        center_ = center;
        return *this;
    }

    Circle& Circle::SetRadius(double radius) {
        radius_ = radius;
        return *this;
    }

    void Circle::RenderObject(const RenderContext& context) const {
        auto& out = context.out;
        out << "<circle cx=\""sv << center_.x << "\" cy=\""sv << center_.y << "\" "sv;
        out << "r=\""sv << radius_ << "\" "sv;
        RenderAttrs(out);
        out << "/>"sv;
    }

    // ---------- Polyline ------------------

    Polyline& Polyline::AddPoint(Point point) {
        coords_.push_back(point);
        return *this;
    }

    void Polyline::RenderObject(const RenderContext& context) const {
        auto& out = context.out;
        out << "<polyline points=\""sv;
        for (size_t i = 0; i < coords_.size(); ++i) {
            out << coords_[i].x << ',' << coords_[i].y;
            if (i < coords_.size() - 1) {
                out << ' ';
            }
        }
        out << "\" "sv;
        RenderAttrs(out);
        out << "/>"sv;
    }
    
    // ---------- Text ------------------

    Text& Text::SetPosition(Point pos) {
        position_ = pos;
        return *this;
    }

    Text& Text::SetOffset(Point offset) {
        offset_ = offset;
        return *this;
    }

    Text& Text::SetFontSize(uint32_t size) {
        font_size_ = size;
        return *this;
    }

    Text& Text::SetFontFamily(std::string font_family) {
        font_family_ = font_family;
        return *this;
    }

    Text& Text::SetFontWeight(std::string font_weight) {
        font_weight_ = font_weight;
        return *this;
    }

    Text& Text::SetData(std::string data) {
        data_ = data;
        return *this;
    }

    void Text::RenderObject(const RenderContext& context) const {
        auto& out = context.out;
        out << "<text ";
        RenderAttrs(out);
        out << " x=\""sv << position_.x << "\" "sv << "y=\""sv << position_.y << "\" "sv;
        out << "dx=\""sv << offset_.x << "\" "sv << "dy=\""sv<< offset_.y << "\" "sv;
        out << "font-size=\""sv << font_size_ << "\""sv;
        if (!font_family_.empty()) {
            out << " font-family=\""sv << font_family_ << "\" "sv;
        }
        if (!font_weight_.empty()) {
            out << "font-weight=\""sv << font_weight_ << "\""sv;
        }
        out << '>' << data_ << '<';
        out << "/text>"sv;
    }

    // ---------- Document ------------------

    void Document::AddPtr(std::unique_ptr<Object>&& obj) {
        docs_.push_back(std::move(obj));
    }

    void Document::Render(std::ostream& out) const {
        RenderContext ctx(out, 1);

        ctx.out << "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>"sv << '\n';
        ctx.out << "<svg xmlns=\"http://www.w3.org/2000/svg\" version=\"1.1\">"sv << '\n';

        RenderContext inner_ctx = ctx.Indented();
        for (size_t i = 0; i < docs_.size(); ++i) {
            inner_ctx.RenderIndent();
            docs_[i]->Render(inner_ctx);
        }
        ctx.out << "</svg>"sv << std::endl;
    }
}  // namespace svg