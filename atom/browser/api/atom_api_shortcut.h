// Copyright (c) 2014 GitHub, Inc. All rights reserved.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_API_ATOM_API_SHORTCUT_H_
#define ATOM_BROWSER_API_ATOM_API_SHORTCUT_H_

#include <string>
#include <vector>

#include "atom/browser/api/event_emitter.h"
#include "chrome/browser/ui/shortcut/global_shortcut_listener.h"
#include "ui/base/accelerators/accelerator.h"

namespace mate {

class Dictionary;

}

namespace atom {

class Shortcut;

namespace api {

class Menu;

class Shortcut : public mate::EventEmitter,
                 public GlobalShortcutListener::Observer {
 public:
  static mate::Wrappable* Create(const std::string& key);

  static void BuildPrototype(v8::Isolate* isolate,
                             v8::Handle<v8::ObjectTemplate> prototype);

 protected:
  explicit Shortcut(const std::string& key);
  virtual ~Shortcut();

  const ui::Accelerator& GetAccelerator() const {
    return accelerator_;
  }

  void OnActive() ;
  void OnFailed(const std::string& error_msg) ;

  // GlobalShortcutListener::Observer implementation.
  virtual void OnKeyPressed(const ui::Accelerator& accelerator) OVERRIDE;

  void SetKey(const std::string& key);
  void Register();
  void Unregister();
  bool IsRegistered();

 private:
  bool is_key_valid_;
  ui::Accelerator accelerator_;

  DISALLOW_COPY_AND_ASSIGN(Shortcut);
};

}  // namespace api

}  // namespace atom

#endif  // ATOM_BROWSER_API_ATOM_API_SHORTCUT_H_
