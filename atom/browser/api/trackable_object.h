// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_API_TRACKABLE_OBJECT_H_
#define ATOM_BROWSER_API_TRACKABLE_OBJECT_H_

#include <vector>

#include "atom/browser/api/event_emitter.h"
#include "atom/common/id_weak_map.h"
#include "base/bind.h"
#include "base/memory/scoped_ptr.h"
#include "base/memory/weak_ptr.h"
#include "native_mate/object_template_builder.h"

namespace base {
class SupportsUserData;
}

namespace mate {

// Users should use TrackableObject instead.
class TrackableObjectBase : public mate::EventEmitter {
 public:
  TrackableObjectBase();

  // The ID in weak map.
  int32_t weak_map_id() const { return weak_map_id_; }

  // Wrap TrackableObject into a class that SupportsUserData.
  void AttachAsUserData(base::SupportsUserData* wrapped);

  // Get the weak_map_id from SupportsUserData.
  static int32_t GetIDFromWrappedClass(base::SupportsUserData* wrapped);

 protected:
  ~TrackableObjectBase() override;

  // mate::Wrappable:
  void AfterInit(v8::Isolate* isolate) override;

  // Mark the JS object as destroyed.
  void MarkDestroyed();

  // Returns a closure that can destroy the native class.
  base::Closure GetDestroyClosure();

  // Register a callback that should be destroyed before JavaScript environment
  // gets destroyed.
  static base::Closure RegisterDestructionCallback(const base::Closure& c);

  int32_t weak_map_id_;
  base::SupportsUserData* wrapped_;

 private:
  void Destroy();

  base::Closure cleanup_;
  base::WeakPtrFactory<TrackableObjectBase> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(TrackableObjectBase);
};

// All instances of TrackableObject will be kept in a weak map and can be got
// from its ID.
template<typename T>
class TrackableObject : public TrackableObjectBase {
 public:
  // Finds out the TrackableObject from its ID in weak map.
  static T* FromWeakMapID(v8::Isolate* isolate, int32_t id) {
    if (!weak_map_)
      return nullptr;

    v8::MaybeLocal<v8::Object> object = weak_map_->Get(isolate, id);
    if (object.IsEmpty())
      return nullptr;

    T* self = nullptr;
    mate::ConvertFromV8(isolate, object.ToLocalChecked(), &self);
    return self;
  }

  // Finds out the TrackableObject from the class it wraps.
  static T* FromWrappedClass(v8::Isolate* isolate,
                             base::SupportsUserData* wrapped) {
    int32_t id = GetIDFromWrappedClass(wrapped);
    if (!id)
      return nullptr;
    return FromWeakMapID(isolate, id);
  }

  // Returns all objects in this class's weak map.
  static std::vector<v8::Local<v8::Object>> GetAll(v8::Isolate* isolate) {
    if (weak_map_)
      return weak_map_->Values(isolate);
    else
      return std::vector<v8::Local<v8::Object>>();
  }

  // Removes this instance from the weak map.
  void RemoveFromWeakMap() {
    if (weak_map_ && weak_map_->Has(weak_map_id()))
      weak_map_->Remove(weak_map_id());
  }

 protected:
  TrackableObject() {}
  ~TrackableObject() override {
    RemoveFromWeakMap();
  }

  void AfterInit(v8::Isolate* isolate) override {
    if (!weak_map_) {
      weak_map_.reset(new atom::IDWeakMap);
      RegisterDestructionCallback(
          base::Bind(&TrackableObject<T>::ReleaseAllWeakReferences));
    }
    weak_map_id_ = weak_map_->Add(isolate, GetWrapper(isolate));
    TrackableObjectBase::AfterInit(isolate);
  }

 private:
  // mate::Wrappable:
  mate::ObjectTemplateBuilder GetObjectTemplateBuilder(
      v8::Isolate* isolate) override {
    if (template_.IsEmpty()) {
      auto templ = v8::ObjectTemplate::New(isolate);
      T::BuildPrototype(isolate, templ);
      template_.Reset(isolate, templ);
    }

    return ObjectTemplateBuilder(
        isolate, v8::Local<v8::ObjectTemplate>::New(isolate, template_));
  }

  // Releases all weak references in weak map, called when app is terminating.
  static void ReleaseAllWeakReferences() {
    weak_map_.reset();
  }

  static v8::Persistent<v8::ObjectTemplate> template_;
  static scoped_ptr<atom::IDWeakMap> weak_map_;

  DISALLOW_COPY_AND_ASSIGN(TrackableObject);
};

template<typename T>
v8::Persistent<v8::ObjectTemplate> TrackableObject<T>::template_;

template<typename T>
scoped_ptr<atom::IDWeakMap> TrackableObject<T>::weak_map_;

}  // namespace mate

#endif  // ATOM_BROWSER_API_TRACKABLE_OBJECT_H_
