#pragma once

#include <memory>
#include <string>

namespace core {

    typedef std::shared_ptr<class BaseObject> BaseObjectRef;

    class BaseObject {
    public:
        BaseObject(const std::string& name): name_(name) {}
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

    private:
        std::string name_;

    };

};
