// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/ui/views/global_menu_bar_x11.h"

#include <X11/Xlib.h>

// This conflicts with mate::Converter,
#undef True
#undef False
// and V8.
#undef None

#include <dlfcn.h>
#include <glib-object.h>

#include "atom/browser/native_window_views.h"
#include "base/logging.h"
#include "base/strings/stringprintf.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/ui/views/frame/global_menu_bar_registrar_x11.h"
#include "ui/aura/window.h"
#include "ui/aura/window_tree_host.h"
#include "ui/base/accelerators/menu_label_accelerator_util_linux.h"
#include "ui/base/models/menu_model.h"
#include "ui/events/keycodes/keyboard_code_conversion_x.h"

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

typedef struct _DbusmenuServer      DbusmenuServer;
typedef DbusmenuServer* (*dbusmenu_server_new_func)(const char* object);
typedef void (*dbusmenu_server_set_root_func)(DbusmenuServer* self,
                                              DbusmenuMenuitem* root);

namespace atom {

namespace {

// Retrieved functions from libdbusmenu-glib.

// DbusmenuMenuItem methods:
dbusmenu_menuitem_new_func menuitem_new = NULL;
dbusmenu_menuitem_new_with_id_func menuitem_new_with_id = NULL;
dbusmenu_menuitem_get_id_func menuitem_get_id = NULL;
dbusmenu_menuitem_get_children_func menuitem_get_children = NULL;
dbusmenu_menuitem_get_children_func menuitem_take_children = NULL;
dbusmenu_menuitem_child_append_func menuitem_child_append = NULL;
dbusmenu_menuitem_property_set_func menuitem_property_set = NULL;
dbusmenu_menuitem_property_set_variant_func menuitem_property_set_variant =
    NULL;
dbusmenu_menuitem_property_set_bool_func menuitem_property_set_bool = NULL;
dbusmenu_menuitem_property_set_int_func menuitem_property_set_int = NULL;

// DbusmenuServer methods:
dbusmenu_server_new_func server_new = NULL;
dbusmenu_server_set_root_func server_set_root = NULL;

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

ui::MenuModel* ModelForMenuItem(DbusmenuMenuitem* item) {
  return reinterpret_cast<ui::MenuModel*>(
      g_object_get_data(G_OBJECT(item), "model"));
}

bool GetMenuItemID(DbusmenuMenuitem* item, int *id) {
  gpointer id_ptr = g_object_get_data(G_OBJECT(item), "menu-id");
  if (id_ptr != NULL) {
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

}  // namespace

GlobalMenuBarX11::GlobalMenuBarX11(NativeWindowViews* window)
    : window_(window),
      xid_(window_->GetNativeWindow()->GetHost()->GetAcceleratedWidget()),
      server_(NULL) {
  EnsureMethodsLoaded();
  if (server_new)
    InitServer(xid_);

  GlobalMenuBarRegistrarX11::GetInstance()->OnWindowMapped(xid_);
}

GlobalMenuBarX11::~GlobalMenuBarX11() {
  if (IsServerStarted())
    g_object_unref(server_);

  GlobalMenuBarRegistrarX11::GetInstance()->OnWindowUnmapped(xid_);
}

// static
std::string GlobalMenuBarX11::GetPathForWindow(gfx::AcceleratedWidget xid) {
  return base::StringPrintf("/com/canonical/menu/%lX", xid);
}

void GlobalMenuBarX11::SetMenu(ui::MenuModel* menu_model) {
  if (!IsServerStarted())
    return;

  DbusmenuMenuitem* root_item = menuitem_new();
  menuitem_property_set(root_item, kPropertyLabel, "Root");
  menuitem_property_set_bool(root_item, kPropertyVisible, true);
  BuildMenuFromModel(menu_model, root_item);

  server_set_root(server_, root_item);
  g_object_unref(root_item);
}

bool GlobalMenuBarX11::IsServerStarted() const {
  return server_;
}

void GlobalMenuBarX11::InitServer(gfx::AcceleratedWidget xid) {
  std::string path = GetPathForWindow(xid);
  server_ = server_new(path.c_str());
}

void GlobalMenuBarX11::BuildMenuFromModel(ui::MenuModel* model,
                                          DbusmenuMenuitem* parent) {
  for (int i = 0; i < model->GetItemCount(); ++i) {
    DbusmenuMenuitem* item = menuitem_new();
    menuitem_property_set_bool(item, kPropertyVisible, model->IsVisibleAt(i));

    ui::MenuModel::ItemType type = model->GetTypeAt(i);
    if (type == ui::MenuModel::TYPE_SEPARATOR) {
      menuitem_property_set(item, kPropertyType, kTypeSeparator);
    } else {
      std::string label = ui::ConvertAcceleratorsFromWindowsStyle(
          base::UTF16ToUTF8(model->GetLabelAt(i)));
      menuitem_property_set(item, kPropertyLabel, label.c_str());
      menuitem_property_set_bool(item, kPropertyEnabled, model->IsEnabledAt(i));

      g_object_set_data(G_OBJECT(item), "model", model);
      SetMenuItemID(item, i);

      if (type == ui::MenuModel::TYPE_SUBMENU) {
        menuitem_property_set(item, kPropertyChildrenDisplay, kDisplaySubmenu);
        g_signal_connect(item, "about-to-show",
                         G_CALLBACK(OnSubMenuShowThunk), this);
      } else {
        ui::Accelerator accelerator;
        if (model->GetAcceleratorAt(i, &accelerator))
          RegisterAccelerator(item, accelerator);

        g_signal_connect(item, "item-activated",
                         G_CALLBACK(OnItemActivatedThunk), this);

        if (type == ui::MenuModel::TYPE_CHECK ||
            type == ui::MenuModel::TYPE_RADIO) {
          menuitem_property_set(item, kPropertyToggleType,
              type == ui::MenuModel::TYPE_CHECK ? kToggleCheck : kToggleRadio);
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

  char* name = XKeysymToString(XKeysymForWindowsKeyCode(
      accelerator.key_code(), false));
  if (!name) {
    NOTIMPLEMENTED();
    return;
  }
  g_variant_builder_add(&builder, "s", name);

  GVariant* inside_array = g_variant_builder_end(&builder);
  g_variant_builder_init(&builder, G_VARIANT_TYPE_ARRAY);
  g_variant_builder_add_value(&builder, inside_array);
  GVariant* outside_array = g_variant_builder_end(&builder);

  menuitem_property_set_variant(item, kPropertyShortcut, outside_array);
}

void GlobalMenuBarX11::OnItemActivated(DbusmenuMenuitem* item,
                                       unsigned int timestamp) {
  int id;
  ui::MenuModel* model = ModelForMenuItem(item);
  if (model && GetMenuItemID(item, &id))
    model->ActivatedAt(id, 0);
}

void GlobalMenuBarX11::OnSubMenuShow(DbusmenuMenuitem* item) {
  int id;
  ui::MenuModel* model = ModelForMenuItem(item);
  if (!model || !GetMenuItemID(item, &id))
    return;

  // Clear children.
  GList *children = menuitem_take_children(item);
  g_list_foreach(children, reinterpret_cast<GFunc>(g_object_unref), NULL);
  g_list_free(children);

  // Build children.
  BuildMenuFromModel(model->GetSubmenuModelAt(id), item);
}

}  // namespace atom
