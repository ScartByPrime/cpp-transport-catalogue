#include "json_builder.h"

using namespace std;
using namespace json;


KeyContext& BaseContext::Key(std::string&& key) {
    return builder_->Key(std::move(key));
}
Builder& BaseContext::EndDict() {
    return builder_->EndDict();
}
DictContext& BaseContext::StartDict() {
    return builder_->StartDict();
}
ArrayContext& BaseContext::StartArray() {
    return builder_->StartArray();
}
BaseContext& BaseContext::Value(Node&& node) {
    return builder_->Value(move(node.GetValue()));
}
Builder& BaseContext::EndArray() {
    return builder_->EndArray();
}
Node BaseContext::Build() {
    return builder_->Build();
}

DictContext& KeyContext::Value(Node&& node) {
    return builder_->DictValue(move(node.GetValue()));
}

ArrayContext& ArrayContext::Value(Node&& node) {
    return builder_->ArrayValue(move(node.GetValue()));
}

Builder::Builder()
    : base_ctx_(this), key_ctx_(this), dict_ctx_(this), array_ctx_(this) {
}

DictContext& Builder::StartDict() {
    if (!CtxAllowsValue()) {
        throw logic_error("Dict is not allowed by context");
    }
    if (root_.IsNull()) {
        root_ = Dict{};
        stack_.push_back(&root_);
    }
    else {
        if (stack_.back()->IsArray()) {
            stack_.push_back(&stack_.back()->AsArray().emplace_back(Dict{}));
        }
        else if (stack_.back()->IsString()) {
            stack_.pop_back();
            stack_.push_back(&(stack_.back()->AsDict()[tmp_key_.AsString()] = Dict{}));
        }
    }
    dict_ctx_ = DictContext(this);
    return dict_ctx_;
}

ArrayContext& Builder::StartArray() {
    if (!CtxAllowsValue()) {
        throw logic_error("Array is not allowed by context");
    }
    if (root_.IsNull()) {
        root_ = Array{};
        stack_.push_back(&root_);
    }
    else {
        if (stack_.back()->IsArray()) {
            stack_.push_back(&stack_.back()->AsArray().emplace_back(Array{}));
        }
        else if (stack_.back()->IsString()) {
            stack_.pop_back();
            stack_.push_back(&(stack_.back()->AsDict()[tmp_key_.AsString()] = Array{}));
        }
    }
    array_ctx_ = ArrayContext(this);
    return array_ctx_;
}

KeyContext& Builder::Key(string&& st) {
    if (!stack_.empty() && stack_.back()->IsDict()) {
        tmp_key_ = st;
        stack_.emplace_back(&tmp_key_);
    }
    else {
        throw logic_error("Key cannot be used outside of Dict");
    }
    key_ctx_ = KeyContext(this);
    return key_ctx_;
}

DictContext& Builder::DictValue(Node::Value&& val) {
    stack_.pop_back();
    stack_.back()->AsDict()[tmp_key_.AsString()].GetValue() = val;
    dict_ctx_ = DictContext(this);
    return dict_ctx_;
}

ArrayContext& Builder::ArrayValue(Node::Value&& val) {
    stack_.back()->AsArray().emplace_back(val);
    array_ctx_ = ArrayContext(this);
    return array_ctx_;
}

BaseContext& Builder::Value(Node::Value&& value) {
    if (!CtxAllowsValue()) {
        throw logic_error("Value is not allowed by context");
    }
    if (root_.IsNull()) {
        root_.GetValue() = value;
    }
    else {
        if (stack_.back()->IsArray()) {

            stack_.back()->AsArray().emplace_back(value);
        }
        else if (stack_.back()->IsString()) {
            stack_.pop_back();
            stack_.back()->AsDict()[tmp_key_.AsString()].GetValue() = value;
        }
    }
    base_ctx_ = BaseContext(this);
    return base_ctx_;
}

Builder& Builder::EndDict() {
    if (!stack_.empty() && stack_.back()->IsDict()) {
        stack_.pop_back();
    }
    else {
        throw logic_error("Dict cannot be closed");
    }
    return *this;
}

Builder& Builder::EndArray() {
    if (!stack_.empty() && stack_.back()->IsArray()) {
        stack_.pop_back();
    }
    else {
        throw logic_error("Array cannot be closed");
    }
    return *this;
}

Node Builder::Build() {
    return std::move(root_);
}

bool Builder::IsNodeFinished() const {
    if (root_.IsNull() || !stack_.empty()) {
        return false;
    }
    return true;
}

bool Builder::CtxAllowsValue() const {
    if (stack_.empty()) {
        if (root_.IsNull()) {
            return true;
        }
        else return false;
    }
    return (stack_.back()->IsArray() || stack_.back()->IsString());
}