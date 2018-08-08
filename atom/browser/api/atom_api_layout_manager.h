// Copyright (c) 2018 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_API_ATOM_API_LAYOUT_MANAGER_H_
#define ATOM_BROWSER_API_ATOM_API_LAYOUT_MANAGER_H_

#include <memory>

#include "atom/browser/api/trackable_object.h"
#include "ui/views/layout/layout_manager.h"

namespace atom {

namespace api {

class LayoutManager : public mate::TrackableObject<LayoutManager> {
 public:
  static mate::WrappableBase* New(mate::Arguments* args);

  static void BuildPrototype(v8::Isolate* isolate,
                             v8::Local<v8::FunctionTemplate> prototype);

  // Take over the ownership of the LayoutManager, and leave weak ref here.
  std::unique_ptr<views::LayoutManager> TakeOver();

  views::LayoutManager* layout_manager() const { return layout_manager_; }

 protected:
  explicit LayoutManager(views::LayoutManager* layout_manager);
  ~LayoutManager() override;

 private:
  bool managed_by_us_ = true;
  views::LayoutManager* layout_manager_;

  DISALLOW_COPY_AND_ASSIGN(LayoutManager);
};

}  // namespace api

}  // namespace atom

#endif  // ATOM_BROWSER_API_ATOM_API_LAYOUT_MANAGER_H_
