#pragma once

#include <iostream>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include "./BaseObject.h"

namespace core {

typedef std::vector<BaseObjectRef> ObjectList;
typedef std::set<BaseObjectRef> DisplayList;
typedef std::map<std::string, BaseObjectRef> ObjectDB;

typedef std::shared_ptr<class Scene> SceneRef;

/**
 * Scene manager class
 */
class Scene {
public:
    Scene();
    ~Scene();

    int numObjects() { return int(object_list_.size()); }
    bool exists(const std::string& name) const;
    bool addObject(BaseObjectRef object, bool visible = true);
    BaseObjectRef getObject(const std::string& name) const;
    BaseObjectRef getObjectFromIndex(unsigned int index) const;

    void update(double time);
    void draw();
    void reset();
    void clear();

    static SceneRef create();

private:
    ObjectDB object_db_;
    ObjectList object_list_;
    DisplayList display_list_;
};

} // namespace core
