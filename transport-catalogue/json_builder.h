#pragma once

#include "json.h"

namespace json {

    class Builder;
    class KeyContext;
    class DictContext;
    class ArrayContext;

    // interface
    class BaseContext {
    protected:
        Builder* builder_;
    public:
        explicit BaseContext(Builder* builder) : builder_(builder) {
        }
        KeyContext& Key(std::string&& key);
        virtual BaseContext& Value(Node&& node);
        DictContext& StartDict();
        ArrayContext& StartArray();
        Builder& EndDict();
        Builder& EndArray();
        Node Build();

        virtual ~BaseContext() = default;
    };

    class DictContext final : public BaseContext {
    public:
        using BaseContext::BaseContext;
        DictContext StartDict() = delete;
        ArrayContext StartArray() = delete;
        Builder& EndArray() = delete;
        Node Build() = delete;
    private:
        BaseContext& Value([[maybe_unused]] Node&& node) override {
            throw std::logic_error("Value() not allowed in DictContext");
        }
    };

    class KeyContext final : public BaseContext {
    public:
        using BaseContext::BaseContext;
        KeyContext Key(std::string&& key) = delete;
        Builder& EndDict() = delete;
        Builder& EndArray() = delete;
        Node Build() = delete;
        DictContext& Value(Node&& node) override;
    };

    class ArrayContext final : public BaseContext {
    public:
        using BaseContext::BaseContext;
        KeyContext& Key(std::string&& key) = delete;
        Builder& EndDict() = delete;
        Node Build() = delete;
        ArrayContext& Value(Node&& node) override;
    };

    class Builder {
    public:
        Builder();
        KeyContext& Key(std::string&& st);
        BaseContext& Value(Node::Value&& val);
        DictContext& DictValue(Node::Value&& val);
        ArrayContext& ArrayValue(Node::Value&& val);
        DictContext& StartDict();
        ArrayContext& StartArray();
        Builder& EndDict();
        Builder& EndArray();
        Node Build();

    private:
        bool CtxAllowsValue() const;
        bool IsNodeFinished() const;

        BaseContext base_ctx_;
        KeyContext key_ctx_;
        DictContext dict_ctx_;
        ArrayContext array_ctx_;

        Node root_;
        std::vector<Node*> stack_;
        Node tmp_key_{ std::string{} };
    };
} // json