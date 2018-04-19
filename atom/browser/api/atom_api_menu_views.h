// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_API_ATOM_API_MENU_VIEWS_H_
#define ATOM_BROWSER_API_ATOM_API_MENU_VIEWS_H_

#include <map>

#include "atom/browser/api/atom_api_menu.h"
#include "base/memory/weak_ptr.h"
#include "ui/display/screen.h"
#include "ui/views/controls/menu/menu_runner.h"

namespace atom {

namespace api {

class MenuViews : public Menu {
 public:
  MenuViews(v8::Isolate* isolate, v8::Local<v8::Object> wrapper);
  ~MenuViews() override;

 protected:
  void PopupAt(TopLevelWindow* window,
               int x,
               int y,
               int positioning_item,
               const base::Closure& callback) override;
  void ClosePopupAt(int32_t window_id) override;

 private:
  void OnClosed(int32_t window_id, base::Closure callback);

  // window ID -> open context menu
  std::map<int32_t, std::unique_ptr<views::MenuRunner>> menu_runners_;

  base::WeakPtrFactory<MenuViews> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(MenuViews);
};

}  // namespace api

}  // namespace atom

#endif  // ATOM_BROWSER_API_ATOM_API_MENU_VIEWS_H_
