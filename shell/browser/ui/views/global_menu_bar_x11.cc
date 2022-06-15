// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/ui/views/global_menu_bar_x11.h"

#include <dlfcn.h>
#include <glib-object.h>

#include "base/logging.h"
#include "base/strings/stringprintf.h"
#include "base/strings/utf_string_conversions.h"
#include "shell/browser/native_window_views.h"
#include "shell/browser/ui/electron_menu_model.h"
#include "shell/browser/ui/views/global_menu_bar_registrar_x11.h"
#include "ui/aura/window.h"
#include "ui/aura/window_tree_host.h"
#include "ui/base/accelerators/menu_label_accelerator_util_linux.h"
#include "ui/events/keycodes/keyboard_code_conversion_x.h"
#include "ui/events/keycodes/keysym_to_unicode.h"
#include "ui/gfx/x/connection.h"
#include "ui/gfx/x/event.h"
#include "ui/gfx/x/keysyms/keysyms.h"
#include "ui/gfx/x/xproto.h"

// libdbusmenu-glib types
typedef struct _DbusmenuMenuitem DbusmenuMenuitem;
typedef DbusmenuMenuitem* (*dbusmenu_menuitem_new_func)();
typedef DbusmenuMenuitem* (*dbusmenu_menuitem_new_with_id_func)(int id);

typedef int (*dbusmenu_menuitem_get_id_func)(DbusmenuMenuitem* item);
typedef GList* (*dbusmenu_menuitem_get_children_func)(DbusmenuMenuitem* item);
typedef DbusmenuMenuitem* (*dbusmenu_menuitem_child_append_func)(
    DbusmenuMenuitem* parent,
    DbusmenuMenuitem* child);
typedef DbusmenuMenuitem* (*dbusmenu_menuitem_property_set_func)(
    DbusmenuMenuitem* item,
    const char* property,
    const char* value);
typedef DbusmenuMenuitem* (*dbusmenu_menuitem_property_set_variant_func)(
    DbusmenuMenuitem* item,
    const char* property,
    GVariant* value);
typedef DbusmenuMenuitem* (*dbusmenu_menuitem_property_set_bool_func)(
    DbusmenuMenuitem* item,
    const char* property,
    bool value);
typedef DbusmenuMenuitem* (*dbusmenu_menuitem_property_set_int_func)(
    DbusmenuMenuitem* item,
    const char* property,
    int value);

typedef struct _DbusmenuServer DbusmenuServer;
typedef DbusmenuServer* (*dbusmenu_server_new_func)(const char* object);
typedef void (*dbusmenu_server_set_root_func)(DbusmenuServer* self,
                                              DbusmenuMenuitem* root);

namespace electron {

namespace {

// Retrieved functions from libdbusmenu-glib.

// DbusmenuMenuItem methods:
dbusmenu_menuitem_new_func menuitem_new = nullptr;
dbusmenu_menuitem_new_with_id_func menuitem_new_with_id = nullptr;
dbusmenu_menuitem_get_id_func menuitem_get_id = nullptr;
dbusmenu_menuitem_get_children_func menuitem_get_children = nullptr;
dbusmenu_menuitem_get_children_func menuitem_take_children = nullptr;
dbusmenu_menuitem_child_append_func menuitem_child_append = nullptr;
dbusmenu_menuitem_property_set_func menuitem_property_set = nullptr;
dbusmenu_menuitem_property_set_variant_func menuitem_property_set_variant =
    nullptr;
dbusmenu_menuitem_property_set_bool_func menuitem_property_set_bool = nullptr;
dbusmenu_menuitem_property_set_int_func menuitem_property_set_int = nullptr;

// DbusmenuServer methods:
dbusmenu_server_new_func server_new = nullptr;
dbusmenu_server_set_root_func server_set_root = nullptr;

// Properties that we set on menu items:
const char kPropertyEnabled[] = "enabled";
const char kPropertyLabel[] = "label";
const char kPropertyShortcut[] = "shortcut";
const char kPropertyType[] = "type";
const char kPropertyToggleType[] = "toggle-type";
const char kPropertyToggleState[] = "toggle-state";
const char kPropertyVisible[] = "visible";
const char kPropertyChildrenDisplay[] = "children-display";

const char kToggleCheck[] = "checkmark";
const char kToggleRadio[] = "radio";
const char kTypeSeparator[] = "separator";
const char kDisplaySubmenu[] = "submenu";

void EnsureMethodsLoaded() {
  static bool attempted_load = false;
  if (attempted_load)
    return;
  attempted_load = true;

  void* dbusmenu_lib = dlopen("libdbusmenu-glib.so", RTLD_LAZY);
  if (!dbusmenu_lib)
    dbusmenu_lib = dlopen("libdbusmenu-glib.so.4", RTLD_LAZY);
  if (!dbusmenu_lib)
    return;

  // DbusmenuMenuItem methods.
  menuitem_new = reinterpret_cast<dbusmenu_menuitem_new_func>(
      dlsym(dbusmenu_lib, "dbusmenu_menuitem_new"));
  menuitem_new_with_id = reinterpret_cast<dbusmenu_menuitem_new_with_id_func>(
      dlsym(dbusmenu_lib, "dbusmenu_menuitem_new_with_id"));
  menuitem_get_id = reinterpret_cast<dbusmenu_menuitem_get_id_func>(
      dlsym(dbusmenu_lib, "dbusmenu_menuitem_get_id"));
  menuitem_get_children = reinterpret_cast<dbusmenu_menuitem_get_children_func>(
      dlsym(dbusmenu_lib, "dbusmenu_menuitem_get_children"));
  menuitem_take_children =
      reinterpret_cast<dbusmenu_menuitem_get_children_func>(
          dlsym(dbusmenu_lib, "dbusmenu_menuitem_take_children"));
  menuitem_child_append = reinterpret_cast<dbusmenu_menuitem_child_append_func>(
      dlsym(dbusmenu_lib, "dbusmenu_menuitem_child_append"));
  menuitem_property_set = reinterpret_cast<dbusmenu_menuitem_property_set_func>(
      dlsym(dbusmenu_lib, "dbusmenu_menuitem_property_set"));
  menuitem_property_set_variant =
      reinterpret_cast<dbusmenu_menuitem_property_set_variant_func>(
          dlsym(dbusmenu_lib, "dbusmenu_menuitem_property_set_variant"));
  menuitem_property_set_bool =
      reinterpret_cast<dbusmenu_menuitem_property_set_bool_func>(
          dlsym(dbusmenu_lib, "dbusmenu_menuitem_property_set_bool"));
  menuitem_property_set_int =
      reinterpret_cast<dbusmenu_menuitem_property_set_int_func>(
          dlsym(dbusmenu_lib, "dbusmenu_menuitem_property_set_int"));

  // DbusmenuServer methods.
  server_new = reinterpret_cast<dbusmenu_server_new_func>(
      dlsym(dbusmenu_lib, "dbusmenu_server_new"));
  server_set_root = reinterpret_cast<dbusmenu_server_set_root_func>(
      dlsym(dbusmenu_lib, "dbusmenu_server_set_root"));
}

ElectronMenuModel* ModelForMenuItem(DbusmenuMenuitem* item) {
  return reinterpret_cast<ElectronMenuModel*>(
      g_object_get_data(G_OBJECT(item), "model"));
}

bool GetMenuItemID(DbusmenuMenuitem* item, int* id) {
  gpointer id_ptr = g_object_get_data(G_OBJECT(item), "menu-id");
  if (id_ptr != nullptr) {
    *id = GPOINTER_TO_INT(id_ptr) - 1;
    return true;
  }

  return false;
}

void SetMenuItemID(DbusmenuMenuitem* item, int id) {
  DCHECK_GE(id, 0);

  // Add 1 to the menu_id to avoid setting zero (null) to "menu-id".
  g_object_set_data(G_OBJECT(item), "menu-id", GINT_TO_POINTER(id + 1));
}

std::string GetMenuModelStatus(ElectronMenuModel* model) {
  std::string ret;
  for (int i = 0; i < model->GetItemCount(); ++i) {
    int status = model->GetTypeAt(i) | (model->IsVisibleAt(i) << 3) |
                 (model->IsEnabledAt(i) << 4) |
                 (model->IsItemCheckedAt(i) << 5);
    ret += base::StringPrintf(
        "%s-%X\n", base::UTF16ToUTF8(model->GetLabelAt(i)).c_str(), status);
  }
  return ret;
}

}  // namespace

GlobalMenuBarX11::GlobalMenuBarX11(NativeWindowViews* window)
    : window_(window),
      xwindow_(static_cast<x11::Window>(
          window_->GetNativeWindow()->GetHost()->GetAcceleratedWidget())) {
  EnsureMethodsLoaded();
  if (server_new)
    InitServer(xwindow_);

  GlobalMenuBarRegistrarX11::GetInstance()->OnWindowMapped(xwindow_);
}

GlobalMenuBarX11::~GlobalMenuBarX11() {
  if (IsServerStarted())
    g_object_unref(server_);

  GlobalMenuBarRegistrarX11::GetInstance()->OnWindowUnmapped(xwindow_);
}

// static
std::string GlobalMenuBarX11::GetPathForWindow(x11::Window window) {
  return base::StringPrintf("/com/canonical/menu/%X", window);
}

void GlobalMenuBarX11::SetMenu(ElectronMenuModel* menu_model) {
  if (!IsServerStarted())
    return;

  DbusmenuMenuitem* root_item = menuitem_new();
  menuitem_property_set(root_item, kPropertyLabel, "Root");
  menuitem_property_set_bool(root_item, kPropertyVisible, true);
  if (menu_model != nullptr) {
    BuildMenuFromModel(menu_model, root_item);
  }

  server_set_root(server_, root_item);
  g_object_unref(root_item);
}

bool GlobalMenuBarX11::IsServerStarted() const {
  return server_;
}

void GlobalMenuBarX11::InitServer(x11::Window window) {
  std::string path = GetPathForWindow(window);
  server_ = server_new(path.c_str());
}

void GlobalMenuBarX11::OnWindowMapped() {
  GlobalMenuBarRegistrarX11::GetInstance()->OnWindowMapped(xwindow_);
}

void GlobalMenuBarX11::OnWindowUnmapped() {
  GlobalMenuBarRegistrarX11::GetInstance()->OnWindowUnmapped(xwindow_);
}

void GlobalMenuBarX11::BuildMenuFromModel(ElectronMenuModel* model,
                                          DbusmenuMenuitem* parent) {
  for (int i = 0; i < model->GetItemCount(); ++i) {
    DbusmenuMenuitem* item = menuitem_new();
    menuitem_property_set_bool(item, kPropertyVisible, model->IsVisibleAt(i));

    ElectronMenuModel::ItemType type = model->GetTypeAt(i);
    if (type == ElectronMenuModel::TYPE_SEPARATOR) {
      menuitem_property_set(item, kPropertyType, kTypeSeparator);
    } else {
      std::string label = ui::ConvertAcceleratorsFromWindowsStyle(
          base::UTF16ToUTF8(model->GetLabelAt(i)));
      menuitem_property_set(item, kPropertyLabel, label.c_str());
      menuitem_property_set_bool(item, kPropertyEnabled, model->IsEnabledAt(i));

      g_object_set_data(G_OBJECT(item), "model", model);
      SetMenuItemID(item, i);

      if (type == ElectronMenuModel::TYPE_SUBMENU) {
        menuitem_property_set(item, kPropertyChildrenDisplay, kDisplaySubmenu);
        g_signal_connect(item, "about-to-show", G_CALLBACK(OnSubMenuShowThunk),
                         this);
      } else {
        ui::Accelerator accelerator;
        if (model->GetAcceleratorAtWithParams(i, true, &accelerator))
          RegisterAccelerator(item, accelerator);

        g_signal_connect(item, "item-activated",
                         G_CALLBACK(OnItemActivatedThunk), this);

        if (type == ElectronMenuModel::TYPE_CHECK ||
            type == ElectronMenuModel::TYPE_RADIO) {
          menuitem_property_set(item, kPropertyToggleType,
                                type == ElectronMenuModel::TYPE_CHECK
                                    ? kToggleCheck
                                    : kToggleRadio);
          menuitem_property_set_int(item, kPropertyToggleState,
                                    model->IsItemCheckedAt(i));
        }
      }
    }

    menuitem_child_append(parent, item);
    g_object_unref(item);
  }
}

void GlobalMenuBarX11::RegisterAccelerator(DbusmenuMenuitem* item,
                                           const ui::Accelerator& accelerator) {
  // A translation of libdbusmenu-gtk's menuitem_property_set_shortcut()
  // translated from GDK types to ui::Accelerator types.
  GVariantBuilder builder;
  g_variant_builder_init(&builder, G_VARIANT_TYPE_ARRAY);

  if (accelerator.IsCtrlDown())
    g_variant_builder_add(&builder, "s", "Control");
  if (accelerator.IsAltDown())
    g_variant_builder_add(&builder, "s", "Alt");
  if (accelerator.IsShiftDown())
    g_variant_builder_add(&builder, "s", "Shift");

  uint16_t keysym = ui::GetUnicodeCharacterFromXKeySym(
      XKeysymForWindowsKeyCode(accelerator.key_code(), false));
  if (!keysym) {
    NOTIMPLEMENTED();
    return;
  }
  std::string name = base::UTF16ToUTF8(std::u16string(1, keysym));
  g_variant_builder_add(&builder, "s", name.c_str());

  GVariant* inside_array = g_variant_builder_end(&builder);
  g_variant_builder_init(&builder, G_VARIANT_TYPE_ARRAY);
  g_variant_builder_add_value(&builder, inside_array);
  GVariant* outside_array = g_variant_builder_end(&builder);

  menuitem_property_set_variant(item, kPropertyShortcut, outside_array);
}

void GlobalMenuBarX11::OnItemActivated(DbusmenuMenuitem* item,
                                       unsigned int timestamp) {
  int id;
  ElectronMenuModel* model = ModelForMenuItem(item);
  if (model && GetMenuItemID(item, &id))
    model->ActivatedAt(id, 0);
}

void GlobalMenuBarX11::OnSubMenuShow(DbusmenuMenuitem* item) {
  int id;
  ElectronMenuModel* model = ModelForMenuItem(item);
  if (!model || !GetMenuItemID(item, &id))
    return;

  // Do not update menu if the submenu has not been changed.
  std::string status = GetMenuModelStatus(model);
  char* old = static_cast<char*>(g_object_get_data(G_OBJECT(item), "status"));
  if (old && status == old)
    return;

  // Save the new status.
  g_object_set_data_full(G_OBJECT(item), "status", g_strdup(status.c_str()),
                         g_free);

  // Clear children.
  GList* children = menuitem_take_children(item);
  g_list_foreach(children, reinterpret_cast<GFunc>(g_object_unref), nullptr);
  g_list_free(children);

  // Build children.
  BuildMenuFromModel(model->GetSubmenuModelAt(id), item);
}

}  // namespace electron
