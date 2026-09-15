// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_API_ELECTRON_API_MENU_H_
#define ELECTRON_SHELL_BROWSER_API_ELECTRON_API_MENU_H_

#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "base/functional/function_ref.h"
#include "gin/wrappable.h"
#include "shell/browser/event_emitter_mixin.h"
#include "shell/browser/ui/electron_menu_model.h"
#include "shell/common/gin_helper/constructible.h"
#include "shell/common/gin_helper/dictionary.h"
#include "shell/common/gin_helper/self_keep_alive.h"
#include "ui/base/models/image_model.h"
#include "ui/base/mojom/menu_source_type.mojom-forward.h"
#include "v8/include/cppgc/garbage-collected.h"
#include "v8/include/cppgc/member.h"
#include "v8/include/v8-traced-handle.h"

#if BUILDFLAG(IS_MAC)
#include "shell/common/gin_converters/file_path_converter.h"
#include "shell/common/gin_converters/gurl_converter.h"
#include "shell/common/gin_converters/std_converter.h"
#endif

namespace gin {
class Arguments;
}  // namespace gin

namespace gin_helper {
class ErrorThrower;
}  // namespace gin_helper

namespace electron::api {

class BaseWindow;
class MenuItem;
class WebFrameMain;

class Menu : public gin::Wrappable<Menu>,
             public gin_helper::EventEmitterMixin<Menu>,
             public gin_helper::Constructible<Menu>,
             public ElectronMenuModel::Delegate,
             private ElectronMenuModel::Observer {
 public:
  static Menu* New(gin::Arguments* args);

  // Make public for cppgc::MakeGarbageCollected.
  explicit Menu(gin::Arguments* args);
  ~Menu() override;

  // disable copy
  Menu(const Menu&) = delete;
  Menu& operator=(const Menu&) = delete;

  // gin::Wrappable
  static const gin::WrapperInfo kWrapperInfo;
  const gin::WrapperInfo* wrapper_info() const override;
  const char* GetHumanReadableName() const override;
  void Trace(cppgc::Visitor*) const override;

  // gin_helper::Constructible
  static void FillObjectTemplate(v8::Isolate*, v8::Local<v8::ObjectTemplate>);
  static void FillInstanceTemplate(v8::Isolate*, v8::Local<v8::ObjectTemplate>);
  static const char* GetClassName() { return "Menu"; }

#if BUILDFLAG(IS_MAC)
  // Set the global menubar.
  static void SetApplicationMenu(Menu* menu);

  // Fake sending an action from the application menu.
  static void SendActionToFirstResponder(const std::string& action);
#endif

  ElectronMenuModel* model() const { return model_.get(); }

  // Throws on |thrower| and returns an empty handle for an invalid template.
  static v8::Local<v8::Value> BuildFromTemplate(
      gin_helper::ErrorThrower thrower,
      v8::Local<v8::Value> tmpl);

  // |args| is only present when constructed from JS.
  static Menu* Create(v8::Isolate* isolate, gin::Arguments* args = nullptr);

  // The items in order; an item may be in more than one menu.
  class Entry final : public cppgc::GarbageCollected<Entry> {
   public:
    Entry(MenuItem* item, Entry* next);
    ~Entry();
    void Trace(cppgc::Visitor* visitor) const;
    cppgc::Member<MenuItem> item;
    cppgc::Member<Entry> next;
  };
  size_t GetItemCount() const;
  MenuItem* ItemAt(size_t index) const;
  void ForEachInRadioGroup(int group_id,
                           base::FunctionRef<void(MenuItem*)> fn) const;

  // ElectronMenuModel::Delegate:
  void ActivatedAt(size_t index, int event_flags) override;
  void MenuWillShow() override;
  // 0 <= pos <= count. Throws on |thrower|, if given, for an invalid item.
  void InsertItem(v8::Isolate* isolate,
                  int pos,
                  MenuItem* item,
                  gin_helper::ErrorThrower* thrower = nullptr);
  void AppendItem(v8::Isolate* isolate, MenuItem* item);
#if BUILDFLAG(IS_MAC)
  v8::Local<v8::Value> GetUserAcceleratorAt(int command_id) const;
#endif

  // JS API.
  void Insert(gin_helper::ErrorThrower thrower,
              int index,
              v8::Local<v8::Value> item);
  void Append(gin_helper::ErrorThrower thrower, v8::Local<v8::Value> item);
  v8::Local<v8::Value> Items(v8::Isolate* isolate);
  v8::Local<v8::Value> GetMenuItemById(gin::Arguments* args);
  v8::Local<v8::Value> FindItemById(v8::Isolate* isolate,
                                    v8::Local<v8::Value> id);
  v8::Local<v8::Value> Popup(gin::Arguments* args);
  void ClosePopup(gin::Arguments* args);
  int GetIndexOfCommandId(int command_id) const;
  void ActivateForTesting(int command_id);
  void MenuWillShowForTesting();
  static void SetApplicationMenuFromJS(gin::Arguments* args);
  static v8::Local<v8::Value> GetApplicationMenu(v8::Isolate* isolate);
  static bool ApplicationMenuWasSet();

 protected:
  // Remove this instance as an observer from the model. Called by derived
  // class destructors to ensure observer is removed before platform-specific
  // cleanup that may trigger model callbacks.
  void RemoveModelObserver();
  // Returns a new callback which keeps references of the JS wrapper until the
  // passed |callback| is called.
  base::OnceClosure BindSelfToClosure(base::OnceClosure callback);

#if BUILDFLAG(IS_MAC)
  virtual void SimulateSubmenuCloseSequenceForTesting();
#endif

  virtual void PopupAt(BaseWindow* window,
                       std::optional<WebFrameMain*> frame,
                       int x,
                       int y,
                       int positioning_item,
                       ui::mojom::MenuSourceType source_type,
                       base::OnceClosure callback) = 0;
  virtual void ClosePopupAt(int32_t window_id) = 0;
  virtual std::u16string GetAcceleratorTextAtForTesting(int index) const;

  // Observable:
  void OnMenuWillClose() override;
  void OnMenuWillShow() override;

 private:
  int GenerateGroupId(int pos);

  std::unique_ptr<ElectronMenuModel> model_;
  cppgc::Member<Menu> parent_;
  // Owns the items model_ refers to.
  cppgc::Member<Entry> first_entry_;
  cppgc::Member<Entry> last_entry_;
  // menu.items, rebuilt after the item list changes.
  v8::TracedReference<v8::Array> items_;

  // Keep active menus alive even if they've been replaced.
  gin_helper::SelfKeepAlive<Menu> keep_alive_{nullptr};
};

}  // namespace electron::api

namespace gin {

#if BUILDFLAG(IS_MAC)
template <>
struct Converter<electron::ElectronMenuModel::SharingItem> {
  static bool FromV8(v8::Isolate* isolate,
                     v8::Local<v8::Value> val,
                     electron::ElectronMenuModel::SharingItem* out) {
    gin_helper::Dictionary dict;
    if (!ConvertFromV8(isolate, val, &dict))
      return false;
    dict.GetOptional("texts", &(out->texts));
    dict.GetOptional("filePaths", &(out->file_paths));
    dict.GetOptional("urls", &(out->urls));
    return true;
  }
};

template <>
struct Converter<electron::ElectronMenuModel::Badge> {
  static bool FromV8(v8::Isolate* isolate,
                     v8::Local<v8::Value> val,
                     electron::ElectronMenuModel::Badge* out) {
    gin_helper::Dictionary dict;
    if (!ConvertFromV8(isolate, val, &dict))
      return false;
    out->type = "none";
    dict.Get("type", &(out->type));
    dict.GetOptional("count", &(out->count));
    dict.GetOptional("content", &(out->content));
    return true;
  }
};
#endif

template <>
struct Converter<electron::ElectronMenuModel*> {
  static bool FromV8(v8::Isolate* isolate,
                     v8::Local<v8::Value> val,
                     electron::ElectronMenuModel** out) {
    // null would be transferred to nullptr.
    if (val->IsNull()) {
      *out = nullptr;
      return true;
    }

    electron::api::Menu* menu;
    if (!Converter<electron::api::Menu*>::FromV8(isolate, val, &menu))
      return false;
    *out = menu->model();
    return true;
  }
};

}  // namespace gin

#endif  // ELECTRON_SHELL_BROWSER_API_ELECTRON_API_MENU_H_
