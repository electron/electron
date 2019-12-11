// Copyright (c) 2018 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef SHELL_BROWSER_API_VIEWS_ATOM_API_LAYOUT_MANAGER_H_
#define SHELL_BROWSER_API_VIEWS_ATOM_API_LAYOUT_MANAGER_H_

#include <memory>

#include "shell/common/gin_helper/trackable_object.h"
#include "ui/views/layout/layout_manager.h"

namespace electron {

namespace api {

class LayoutManager : public gin_helper::TrackableObject<LayoutManager> {
 public:
  static gin_helper::WrappableBase* New(gin_helper::Arguments* args);

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

}  // namespace electron

#endif  // SHELL_BROWSER_API_VIEWS_ATOM_API_LAYOUT_MANAGER_H_
