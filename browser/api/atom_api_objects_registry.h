// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_API_ATOM_API_OBJECTS_REGISTRY_H_
#define ATOM_BROWSER_API_ATOM_API_OBJECTS_REGISTRY_H_

#include "base/id_map.h"
#include "base/basictypes.h"

namespace atom {

namespace api {

class RecordedObject;

class ObjectsRegistry {
 public:
  ObjectsRegistry();
  virtual ~ObjectsRegistry();

  int Add(RecordedObject* data) { return id_map_.Add(data); }
  void Remove(int id) { id_map_.Remove(id); }
  void Clear() { id_map_.Clear(); }
  RecordedObject* Lookup(int id) const { return id_map_.Lookup(id); }

 private:
  IDMap<RecordedObject> id_map_;

  DISALLOW_COPY_AND_ASSIGN(ObjectsRegistry);
};

}  // namespace api

}  // namespace atom

#endif  // ATOM_BROWSER_API_ATOM_API_OBJECTS_REGISTRY_H_
