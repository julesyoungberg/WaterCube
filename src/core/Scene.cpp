#include "cinder/app/App.h"

#include "./Scene.h"

using namespace ci;
using namespace ci::app;
using namespace core;

Scene::Scene() {}

Scene::~Scene() {
    clear();
}

bool Scene::exists(const std::string& name) const {
    auto found = object_db_.find(name);
    return found != object_db_.end();
}

bool Scene::addObject(BaseObjectRef object, bool visible) {
    if (!object) {
        return false;
    }

    auto name = object->name();
    if (exists(name)) {
        return false;
    }

    object_db_[name] = object;
    object_list_.push_back(object);
    if (visible) {
        display_list_.insert(object);
    }

    return true;
}

BaseObjectRef Scene::getObject(const std::string& name) const {
    auto found = object_db_.find(name);
    if (found == object_db_.end()) {
        return BaseObjectRef(NULL);
    }
    return (*found).second;
}

BaseObjectRef Scene::getObjectFromIndex(unsigned int index) const {
    if (index >= object_list_.size()) {
        return BaseObjectRef(NULL);
    }
    return object_list_.at(index);
}

void Scene::update(double time) {
    // console() << "updating scene: " << object_list_.size() << " objects\n";
    for (const auto & o : object_list_) {
        o->update(time);
    }
}

void Scene::draw() {
    // console() << "drawing scene\n";
    for (const auto & o : display_list_) {
        o->draw();
    }
}

void Scene::reset() {
    for (const auto & o : object_list_) {
        o->reset();
    }
}

void Scene::clear() {
    display_list_.clear();
    object_list_.clear();
    object_db_.clear();
}

SceneRef Scene::create() {
    return std::make_shared<Scene>();
}
