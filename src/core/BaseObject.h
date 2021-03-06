#pragma once

#include <memory>
#include <string>

namespace core {

typedef std::shared_ptr<class BaseObject> BaseObjectRef;

/**
 * Base Class for all scene objects to inherit from
 */
class BaseObject {
public:
    BaseObject(const std::string& name) : name_(name) {}
    virtual ~BaseObject() {}

    std::string name() { return name_; }
    BaseObject& name(std::string& name) {
        name_ = name;
        return *this;
    }

    virtual void update(double time) {}
    virtual void draw() {}
    virtual void reset() {}

    static BaseObjectRef create(const std::string& name) {
        return std::make_shared<BaseObject>(name);
    }

    std::string name_;
};

} // namespace core
