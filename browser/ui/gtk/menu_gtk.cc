// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "browser/ui/gtk/menu_gtk.h"

#include <map>

#include "base/bind.h"
#include "base/i18n/rtl.h"
#include "base/logging.h"
#include "base/message_loop/message_loop.h"
#include "base/stl_util.h"
#include "base/strings/utf_string_conversions.h"
#include "browser/ui/gtk/event_utils.h"
#include "browser/ui/gtk/gtk_custom_menu.h"
#include "browser/ui/gtk/gtk_custom_menu_item.h"
#include "browser/ui/gtk/gtk_util.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "ui/base/accelerators/menu_label_accelerator_util_linux.h"
#include "ui/base/accelerators/platform_accelerator_gtk.h"
#include "ui/base/models/button_menu_item_model.h"
#include "ui/base/models/menu_model.h"
#include "ui/base/window_open_disposition.h"
#include "ui/gfx/gtk_util.h"
#include "ui/gfx/image/image.h"

bool MenuGtk::block_activation_ = false;

namespace {

// Sets the ID of a menu item.
void SetMenuItemID(GtkWidget* menu_item, int menu_id) {
  DCHECK_GE(menu_id, 0);

  // Add 1 to the menu_id to avoid setting zero (null) to "menu-id".
  g_object_set_data(G_OBJECT(menu_item), "menu-id",
                    GINT_TO_POINTER(menu_id + 1));
}

// Gets the ID of a menu item.
// Returns true if the menu item has an ID.
bool GetMenuItemID(GtkWidget* menu_item, int* menu_id) {
  gpointer id_ptr = g_object_get_data(G_OBJECT(menu_item), "menu-id");
  if (id_ptr != NULL) {
    *menu_id = GPOINTER_TO_INT(id_ptr) - 1;
    return true;
  }

  return false;
}

ui::MenuModel* ModelForMenuItem(GtkMenuItem* menu_item) {
  return reinterpret_cast<ui::MenuModel*>(
      g_object_get_data(G_OBJECT(menu_item), "model"));
}

void SetUpButtonShowHandler(GtkWidget* button,
                            ui::ButtonMenuItemModel* model,
                            int index) {
  g_object_set_data(G_OBJECT(button), "button-model",
                    model);
  g_object_set_data(G_OBJECT(button), "button-model-id",
                    GINT_TO_POINTER(index));
}

void OnSubmenuShowButtonImage(GtkWidget* widget, GtkButton* button) {
  MenuGtk::Delegate* delegate = reinterpret_cast<MenuGtk::Delegate*>(
      g_object_get_data(G_OBJECT(button), "menu-gtk-delegate"));
  int icon_idr = GPOINTER_TO_INT(g_object_get_data(
      G_OBJECT(button), "button-image-idr"));

  GtkIconSet* icon_set = delegate->GetIconSetForId(icon_idr);
  if (icon_set) {
    gtk_button_set_image(
        button, gtk_image_new_from_icon_set(icon_set,
                                            GTK_ICON_SIZE_MENU));
  }
}

void SetupImageIcon(GtkWidget* button,
                    GtkWidget* menu,
                    int icon_idr,
                    MenuGtk::Delegate* menu_gtk_delegate) {
  g_object_set_data(G_OBJECT(button), "button-image-idr",
                    GINT_TO_POINTER(icon_idr));
  g_object_set_data(G_OBJECT(button), "menu-gtk-delegate",
                    menu_gtk_delegate);

  g_signal_connect(menu, "show", G_CALLBACK(OnSubmenuShowButtonImage), button);
}

// Popup menus may get squished if they open up too close to the bottom of the
// screen. This function takes the size of the screen, the size of the menu,
// an optional widget, the Y position of the mouse click, and adjusts the popup
// menu's Y position to make it fit if it's possible to do so.
// Returns the new Y position of the popup menu.
int CalculateMenuYPosition(const GdkRectangle* screen_rect,
                           const GtkRequisition* menu_req,
                           GtkWidget* widget, const int y) {
  CHECK(screen_rect);
  CHECK(menu_req);
  // If the menu would run off the bottom of the screen, and there is enough
  // screen space upwards to accommodate the menu, then pop upwards. If there
  // is a widget, then also move the anchor point to the top of the widget
  // rather than the bottom.
  const int screen_top = screen_rect->y;
  const int screen_bottom = screen_rect->y + screen_rect->height;
  const int menu_bottom = y + menu_req->height;
  int alternate_y = y - menu_req->height;
  if (widget) {
    GtkAllocation allocation;
    gtk_widget_get_allocation(widget, &allocation);
    alternate_y -= allocation.height;
  }
  if (menu_bottom >= screen_bottom && alternate_y >= screen_top)
    return alternate_y;
  return y;
}

}  // namespace

bool MenuGtk::Delegate::AlwaysShowIconForCmd(int command_id) const {
  return false;
}

GtkIconSet* MenuGtk::Delegate::GetIconSetForId(int idr) { return NULL; }

GtkWidget* MenuGtk::Delegate::GetDefaultImageForLabel(
    const std::string& label) {
  const char* stock = NULL;
  if (label == "New")
    stock = GTK_STOCK_NEW;
  else if (label == "Close")
    stock = GTK_STOCK_CLOSE;
  else if (label == "Save As")
    stock = GTK_STOCK_SAVE_AS;
  else if (label == "Save")
    stock = GTK_STOCK_SAVE;
  else if (label == "Copy")
    stock = GTK_STOCK_COPY;
  else if (label == "Cut")
    stock = GTK_STOCK_CUT;
  else if (label == "Paste")
    stock = GTK_STOCK_PASTE;
  else if (label == "Delete")
    stock = GTK_STOCK_DELETE;
  else if (label == "Undo")
    stock = GTK_STOCK_UNDO;
  else if (label == "Redo")
    stock = GTK_STOCK_REDO;
  else if (label == "Search" || label == "Find")
    stock = GTK_STOCK_FIND;
  else if (label == "Select All")
    stock = GTK_STOCK_SELECT_ALL;
  else if (label == "Clear")
    stock = GTK_STOCK_SELECT_ALL;
  else if (label == "Back")
    stock = GTK_STOCK_GO_BACK;
  else if (label == "Forward")
    stock = GTK_STOCK_GO_FORWARD;
  else if (label == "Reload" || label == "Refresh")
    stock = GTK_STOCK_REFRESH;
  else if (label == "Print")
    stock = GTK_STOCK_PRINT;
  else if (label == "About")
    stock = GTK_STOCK_ABOUT;
  else if (label == "Quit")
    stock = GTK_STOCK_QUIT;
  else if (label == "Help")
    stock = GTK_STOCK_HELP;

  return stock ? gtk_image_new_from_stock(stock, GTK_ICON_SIZE_MENU) : NULL;
}

GtkWidget* MenuGtk::Delegate::GetImageForCommandId(int command_id) const {
  return NULL;
}

MenuGtk::MenuGtk(MenuGtk::Delegate* delegate,
                 ui::MenuModel* model,
                 bool is_menubar)
    : delegate_(delegate),
      model_(model),
      is_menubar_(is_menubar),
      dummy_accel_group_(gtk_accel_group_new()),
      menu_(is_menubar ? gtk_menu_bar_new() : gtk_custom_menu_new()),
      weak_factory_(this) {
  DCHECK(model);
  g_object_ref_sink(menu_);
  ConnectSignalHandlers();
  BuildMenuFromModel();
}

MenuGtk::~MenuGtk() {
  Cancel();

  gtk_widget_destroy(menu_);
  g_object_unref(menu_);

  g_object_unref(dummy_accel_group_);
}

void MenuGtk::ConnectSignalHandlers() {
  // We connect afterwards because OnMenuShow calls SetMenuItemInfo, which may
  // take a long time or even start a nested message loop.
  g_signal_connect(menu_, "show", G_CALLBACK(OnMenuShowThunk), this);
  g_signal_connect(menu_, "hide", G_CALLBACK(OnMenuHiddenThunk), this);
  GtkWidget* toplevel_window = gtk_widget_get_toplevel(menu_);
  signal_.Connect(toplevel_window, "focus-out-event",
                  G_CALLBACK(OnMenuFocusOutThunk), this);
}

GtkWidget* MenuGtk::AppendMenuItemWithLabel(int command_id,
                                            const std::string& label) {
  std::string converted_label = ui::ConvertAcceleratorsFromWindowsStyle(label);
  GtkWidget* menu_item = BuildMenuItemWithLabel(converted_label, command_id);
  return AppendMenuItem(command_id, menu_item);
}

GtkWidget* MenuGtk::AppendMenuItemWithIcon(int command_id,
                                           const std::string& label,
                                           const gfx::Image& icon) {
  std::string converted_label = ui::ConvertAcceleratorsFromWindowsStyle(label);
  GtkWidget* menu_item = BuildMenuItemWithImage(converted_label, icon);
  return AppendMenuItem(command_id, menu_item);
}

GtkWidget* MenuGtk::AppendCheckMenuItemWithLabel(int command_id,
                                                 const std::string& label) {
  std::string converted_label = ui::ConvertAcceleratorsFromWindowsStyle(label);
  GtkWidget* menu_item =
      gtk_check_menu_item_new_with_mnemonic(converted_label.c_str());
  return AppendMenuItem(command_id, menu_item);
}

GtkWidget* MenuGtk::AppendSeparator() {
  GtkWidget* menu_item = gtk_separator_menu_item_new();
  gtk_widget_show(menu_item);
  gtk_menu_shell_append(GTK_MENU_SHELL(menu_), menu_item);
  return menu_item;
}

GtkWidget* MenuGtk::InsertSeparator(int position) {
  GtkWidget* menu_item = gtk_separator_menu_item_new();
  gtk_widget_show(menu_item);
  gtk_menu_shell_insert(GTK_MENU_SHELL(menu_), menu_item, position);
  return menu_item;
}

GtkWidget* MenuGtk::AppendMenuItem(int command_id, GtkWidget* menu_item) {
  if (delegate_ && delegate_->AlwaysShowIconForCmd(command_id) &&
      GTK_IS_IMAGE_MENU_ITEM(menu_item))
    gtk_util::SetAlwaysShowImage(menu_item);

  return AppendMenuItemToMenu(command_id, NULL, menu_item, menu_, true);
}

GtkWidget* MenuGtk::InsertMenuItem(int command_id, GtkWidget* menu_item,
                                   int position) {
  if (delegate_ && delegate_->AlwaysShowIconForCmd(command_id) &&
      GTK_IS_IMAGE_MENU_ITEM(menu_item))
    gtk_util::SetAlwaysShowImage(menu_item);

  return InsertMenuItemToMenu(command_id, NULL, menu_item, menu_, position,
      true);
}

GtkWidget* MenuGtk::AppendMenuItemToMenu(int index,
                                         ui::MenuModel* model,
                                         GtkWidget* menu_item,
                                         GtkWidget* menu,
                                         bool connect_to_activate) {
  int children_count = g_list_length(GTK_MENU_SHELL(menu)->children);
  return InsertMenuItemToMenu(index, model, menu_item, menu,
      children_count, connect_to_activate);
}

GtkWidget* MenuGtk::InsertMenuItemToMenu(int index,
                                         ui::MenuModel* model,
                                         GtkWidget* menu_item,
                                         GtkWidget* menu,
                                         int position,
                                         bool connect_to_activate) {
  SetMenuItemID(menu_item, index);

  // Native menu items do their own thing, so only selectively listen for the
  // activate signal.
  if (connect_to_activate) {
    g_signal_connect(menu_item, "activate",
                     G_CALLBACK(OnMenuItemActivatedThunk), this);
  }

  // AppendMenuItemToMenu is used both internally when we control menu creation
  // from a model (where the model can choose to hide certain menu items), and
  // with immediate commands which don't provide the option.
  if (model) {
    if (model->IsVisibleAt(index))
      gtk_widget_show(menu_item);
  } else {
    gtk_widget_show(menu_item);
  }
  gtk_menu_shell_insert(GTK_MENU_SHELL(menu), menu_item, position);
  return menu_item;
}

void MenuGtk::PopupForWidget(GtkWidget* widget, int button,
                             guint32 event_time) {
  gtk_menu_popup(GTK_MENU(menu_), NULL, NULL,
                 WidgetMenuPositionFunc,
                 widget,
                 button, event_time);
}

void MenuGtk::PopupAsContext(const gfx::Point& point, guint32 event_time) {
  // gtk_menu_popup doesn't like the "const" qualifier on point.
  gfx::Point nonconst_point(point);
  gtk_menu_popup(GTK_MENU(menu_), NULL, NULL,
                 PointMenuPositionFunc, &nonconst_point,
                 3, event_time);
}

void MenuGtk::PopupAsContextForStatusIcon(guint32 event_time, guint32 button,
                                          GtkStatusIcon* icon) {
  gtk_menu_popup(GTK_MENU(menu_), NULL, NULL, gtk_status_icon_position_menu,
                 icon, button, event_time);
}

void MenuGtk::PopupAsFromKeyEvent(GtkWidget* widget) {
  PopupForWidget(widget, 0, gtk_get_current_event_time());
  gtk_menu_shell_select_first(GTK_MENU_SHELL(menu_), FALSE);
}

void MenuGtk::Cancel() {
  if (!is_menubar_)
    gtk_menu_popdown(GTK_MENU(menu_));
}

void MenuGtk::UpdateMenu() {
  gtk_container_foreach(GTK_CONTAINER(menu_), SetMenuItemInfo, this);
}

GtkWidget* MenuGtk::BuildMenuItemWithImage(const std::string& label,
                                           GtkWidget* image) {
  GtkWidget* menu_item =
      gtk_image_menu_item_new_with_mnemonic(label.c_str());
  gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(menu_item), image);
  return menu_item;
}

GtkWidget* MenuGtk::BuildMenuItemWithImage(const std::string& label,
                                           const gfx::Image& icon) {
  GtkWidget* menu_item = BuildMenuItemWithImage(label,
      gtk_image_new_from_pixbuf(icon.ToGdkPixbuf()));
  return menu_item;
}

GtkWidget* MenuGtk::BuildMenuItemWithLabel(const std::string& label,
                                           int command_id) {
  GtkWidget* img =
      delegate_ ? delegate_->GetImageForCommandId(command_id) :
                  MenuGtk::Delegate::GetDefaultImageForLabel(label);
  return img ? BuildMenuItemWithImage(label, img) :
               gtk_menu_item_new_with_mnemonic(label.c_str());
}

void MenuGtk::BuildMenuFromModel() {
  BuildSubmenuFromModel(model_, menu_);
}

void MenuGtk::BuildSubmenuFromModel(ui::MenuModel* model, GtkWidget* menu) {
  std::map<int, GtkWidget*> radio_groups;
  GtkWidget* menu_item = NULL;
  for (int i = 0; i < model->GetItemCount(); ++i) {
    gfx::Image icon;
    std::string label = ui::ConvertAcceleratorsFromWindowsStyle(
        base::UTF16ToUTF8(model->GetLabelAt(i)));
    bool connect_to_activate = true;

    switch (model->GetTypeAt(i)) {
      case ui::MenuModel::TYPE_SEPARATOR:
        menu_item = gtk_separator_menu_item_new();
        break;

      case ui::MenuModel::TYPE_CHECK:
        menu_item = gtk_check_menu_item_new_with_mnemonic(label.c_str());
        break;

      case ui::MenuModel::TYPE_RADIO: {
        std::map<int, GtkWidget*>::iterator iter =
            radio_groups.find(model->GetGroupIdAt(i));

        if (iter == radio_groups.end()) {
          menu_item = gtk_radio_menu_item_new_with_mnemonic(
              NULL, label.c_str());
          radio_groups[model->GetGroupIdAt(i)] = menu_item;
        } else {
          menu_item = gtk_radio_menu_item_new_with_mnemonic_from_widget(
              GTK_RADIO_MENU_ITEM(iter->second), label.c_str());
        }
        break;
      }
      case ui::MenuModel::TYPE_BUTTON_ITEM: {
        ui::ButtonMenuItemModel* button_menu_item_model =
            model->GetButtonMenuItemAt(i);
        menu_item = BuildButtonMenuItem(button_menu_item_model, menu);
        connect_to_activate = false;
        break;
      }
      case ui::MenuModel::TYPE_SUBMENU:
      case ui::MenuModel::TYPE_COMMAND: {
        int command_id = model->GetCommandIdAt(i);
        if (model->GetIconAt(i, &icon))
          menu_item = BuildMenuItemWithImage(label, icon);
        else
          menu_item = BuildMenuItemWithLabel(label, command_id);
        if (delegate_ && delegate_->AlwaysShowIconForCmd(command_id) &&
            GTK_IS_IMAGE_MENU_ITEM(menu_item)) {
          gtk_util::SetAlwaysShowImage(menu_item);
        }
        break;
      }

      default:
        NOTREACHED();
    }

    if (model->GetTypeAt(i) == ui::MenuModel::TYPE_SUBMENU) {
      GtkWidget* submenu = gtk_menu_new();
      g_object_set_data(G_OBJECT(submenu), "menu-item", menu_item);
      ui::MenuModel* submenu_model = model->GetSubmenuModelAt(i);
      g_object_set_data(G_OBJECT(menu_item), "submenu-model", submenu_model);
      gtk_menu_item_set_submenu(GTK_MENU_ITEM(menu_item), submenu);
      // We will populate the submenu on demand when shown.
      g_signal_connect(submenu, "show", G_CALLBACK(OnSubMenuShowThunk), this);
      g_signal_connect(submenu, "hide", G_CALLBACK(OnSubMenuHiddenThunk), this);
      connect_to_activate = false;
    }

    ui::Accelerator accelerator;
    if (model->GetAcceleratorAt(i, &accelerator)) {
      gtk_widget_add_accelerator(menu_item,
                                 "activate",
                                 dummy_accel_group_,
                                 ui::GetGdkKeyCodeForAccelerator(accelerator),
                                 ui::GetGdkModifierForAccelerator(accelerator),
                                 GTK_ACCEL_VISIBLE);
    }

    g_object_set_data(G_OBJECT(menu_item), "model", model);
    AppendMenuItemToMenu(i, model, menu_item, menu, connect_to_activate);

    menu_item = NULL;
  }
}

GtkWidget* MenuGtk::BuildButtonMenuItem(ui::ButtonMenuItemModel* model,
                                        GtkWidget* menu) {
  GtkWidget* menu_item = gtk_custom_menu_item_new(
      ui::RemoveWindowsStyleAccelerators(
          base::UTF16ToUTF8(model->label())).c_str());

  // Set up the callback to the model for when it is clicked.
  g_object_set_data(G_OBJECT(menu_item), "button-model", model);
  g_signal_connect(menu_item, "button-pushed",
                   G_CALLBACK(OnMenuButtonPressedThunk), this);
  g_signal_connect(menu_item, "try-button-pushed",
                   G_CALLBACK(OnMenuTryButtonPressedThunk), this);

  GtkSizeGroup* group = NULL;
  for (int i = 0; i < model->GetItemCount(); ++i) {
    GtkWidget* button = NULL;

    switch (model->GetTypeAt(i)) {
      case ui::ButtonMenuItemModel::TYPE_SPACE: {
        gtk_custom_menu_item_add_space(GTK_CUSTOM_MENU_ITEM(menu_item));
        break;
      }
      case ui::ButtonMenuItemModel::TYPE_BUTTON: {
        button = gtk_custom_menu_item_add_button(
            GTK_CUSTOM_MENU_ITEM(menu_item),
            model->GetCommandIdAt(i));

        int icon_idr;
        if (model->GetIconAt(i, &icon_idr)) {
          SetupImageIcon(button, menu, icon_idr, delegate_);
        } else {
          gtk_button_set_label(
              GTK_BUTTON(button),
              ui::RemoveWindowsStyleAccelerators(
                  base::UTF16ToUTF8(model->GetLabelAt(i))).c_str());
        }

        SetUpButtonShowHandler(button, model, i);
        break;
      }
      case ui::ButtonMenuItemModel::TYPE_BUTTON_LABEL: {
        button = gtk_custom_menu_item_add_button_label(
            GTK_CUSTOM_MENU_ITEM(menu_item),
            model->GetCommandIdAt(i));
        gtk_button_set_label(
            GTK_BUTTON(button),
            ui::RemoveWindowsStyleAccelerators(
                base::UTF16ToUTF8(model->GetLabelAt(i))).c_str());
        SetUpButtonShowHandler(button, model, i);
        break;
      }
    }

    if (button && model->PartOfGroup(i)) {
      if (!group)
        group = gtk_size_group_new(GTK_SIZE_GROUP_HORIZONTAL);

      gtk_size_group_add_widget(group, button);
    }
  }

  if (group)
    g_object_unref(group);

  return menu_item;
}

void MenuGtk::OnMenuItemActivated(GtkWidget* menu_item) {
  if (block_activation_)
    return;

  ui::MenuModel* model = ModelForMenuItem(GTK_MENU_ITEM(menu_item));

  if (!model) {
    // There won't be a model for "native" submenus like the "Input Methods"
    // context menu. We don't need to handle activation messages for submenus
    // anyway, so we can just return here.
    DCHECK(gtk_menu_item_get_submenu(GTK_MENU_ITEM(menu_item)));
    return;
  }

  // The activate signal is sent to radio items as they get deselected;
  // ignore it in this case.
  if (GTK_IS_RADIO_MENU_ITEM(menu_item) &&
      !gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(menu_item))) {
    return;
  }

  int id;
  if (!GetMenuItemID(menu_item, &id))
    return;

  // The menu item can still be activated by hotkeys even if it is disabled.
  if (model->IsEnabledAt(id))
    ExecuteCommand(model, id);
}

void MenuGtk::OnMenuButtonPressed(GtkWidget* menu_item, int command_id) {
  ui::ButtonMenuItemModel* model =
      reinterpret_cast<ui::ButtonMenuItemModel*>(
          g_object_get_data(G_OBJECT(menu_item), "button-model"));
  if (model && model->IsCommandIdEnabled(command_id)) {
    if (delegate_)
      delegate_->CommandWillBeExecuted();

    model->ActivatedCommand(command_id);
  }
}

gboolean MenuGtk::OnMenuTryButtonPressed(GtkWidget* menu_item,
                                         int command_id) {
  gboolean pressed = FALSE;
  ui::ButtonMenuItemModel* model =
      reinterpret_cast<ui::ButtonMenuItemModel*>(
          g_object_get_data(G_OBJECT(menu_item), "button-model"));
  if (model &&
      model->IsCommandIdEnabled(command_id) &&
      !model->DoesCommandIdDismissMenu(command_id)) {
    if (delegate_)
      delegate_->CommandWillBeExecuted();

    model->ActivatedCommand(command_id);
    pressed = TRUE;
  }

  return pressed;
}

// static
void MenuGtk::WidgetMenuPositionFunc(GtkMenu* menu,
                                     int* x,
                                     int* y,
                                     gboolean* push_in,
                                     void* void_widget) {
  GtkWidget* widget = GTK_WIDGET(void_widget);
  GtkRequisition menu_req;

  gtk_widget_size_request(GTK_WIDGET(menu), &menu_req);

  gdk_window_get_origin(gtk_widget_get_window(widget), x, y);
  GdkScreen *screen = gtk_widget_get_screen(widget);
  gint monitor = gdk_screen_get_monitor_at_point(screen, *x, *y);

  GdkRectangle screen_rect;
  gdk_screen_get_monitor_geometry(screen, monitor,
                                  &screen_rect);

  GtkAllocation allocation;
  gtk_widget_get_allocation(widget, &allocation);

  if (!gtk_widget_get_has_window(widget)) {
    *x += allocation.x;
    *y += allocation.y;
  }
  *y += allocation.height;

  bool start_align =
    !!g_object_get_data(G_OBJECT(widget), "left-align-popup");
  if (base::i18n::IsRTL())
    start_align = !start_align;

  if (!start_align)
    *x += allocation.width - menu_req.width;

  *y = CalculateMenuYPosition(&screen_rect, &menu_req, widget, *y);

  *push_in = FALSE;
}

// static
void MenuGtk::PointMenuPositionFunc(GtkMenu* menu,
                                    int* x,
                                    int* y,
                                    gboolean* push_in,
                                    gpointer userdata) {
  *push_in = TRUE;

  gfx::Point* point = reinterpret_cast<gfx::Point*>(userdata);
  *x = point->x();
  *y = point->y();

  GtkRequisition menu_req;
  gtk_widget_size_request(GTK_WIDGET(menu), &menu_req);
  GdkScreen* screen;
  gdk_display_get_pointer(gdk_display_get_default(), &screen, NULL, NULL, NULL);
  gint monitor = gdk_screen_get_monitor_at_point(screen, *x, *y);

  GdkRectangle screen_rect;
  gdk_screen_get_monitor_geometry(screen, monitor, &screen_rect);

  *y = CalculateMenuYPosition(&screen_rect, &menu_req, NULL, *y);
}

void MenuGtk::ExecuteCommand(ui::MenuModel* model, int id) {
  if (delegate_)
    delegate_->CommandWillBeExecuted();

  GdkEvent* event = gtk_get_current_event();
  int event_flags = 0;

  if (event && event->type == GDK_BUTTON_RELEASE)
    event_flags = event_utils::EventFlagsFromGdkState(event->button.state);
  model->ActivatedAt(id, event_flags);

  if (event)
    gdk_event_free(event);
}

void MenuGtk::OnMenuShow(GtkWidget* widget) {
  model_->MenuWillShow();
  base::MessageLoop::current()->PostTask(
      FROM_HERE, base::Bind(&MenuGtk::UpdateMenu, weak_factory_.GetWeakPtr()));
}

void MenuGtk::OnMenuHidden(GtkWidget* widget) {
  if (delegate_)
    delegate_->StoppedShowing();
  model_->MenuClosed();
}

gboolean MenuGtk::OnMenuFocusOut(GtkWidget* widget, GdkEventFocus* event) {
  gtk_widget_hide(menu_);
  return TRUE;
}

void MenuGtk::OnSubMenuShow(GtkWidget* submenu) {
  GtkWidget* menu_item = static_cast<GtkWidget*>(
      g_object_get_data(G_OBJECT(submenu), "menu-item"));
  // TODO(mdm): Figure out why this can sometimes be NULL. See bug 131974.
  CHECK(menu_item);
  // Notify the submenu model that the menu will be shown.
  ui::MenuModel* submenu_model = static_cast<ui::MenuModel*>(
      g_object_get_data(G_OBJECT(menu_item), "submenu-model"));
  // We're extra cautious here, and bail out if the submenu model is NULL. In
  // some cases we clear it out from a parent menu; we shouldn't ever show the
  // menu after that, but we play it safe since we're dealing with wacky
  // injected libraries that toy with our menus. (See comments below.)
  if (!submenu_model)
    return;

  // If the submenu is already built, then return right away. This means we
  // recently showed this submenu, and have not yet processed the fact that it
  // was hidden before being shown again.
  if (g_object_get_data(G_OBJECT(submenu), "submenu-built"))
    return;
  g_object_set_data(G_OBJECT(submenu), "submenu-built", GINT_TO_POINTER(1));

  submenu_model->MenuWillShow();

  // Actually build the submenu and attach it to the parent menu item.
  BuildSubmenuFromModel(submenu_model, submenu);
  gtk_menu_item_set_submenu(GTK_MENU_ITEM(menu_item), submenu);

  // Update all the menu item info in the newly-generated menu.
  gtk_container_foreach(GTK_CONTAINER(submenu), SetMenuItemInfo, this);
}

void MenuGtk::OnSubMenuHidden(GtkWidget* submenu) {
  if (is_menubar_)
    return;

  // Increase the reference count of the old submenu, and schedule it to be
  // deleted later. We get this hide notification before we've processed menu
  // activations, so if we were to delete the submenu now, we might lose the
  // activation. This also lets us reuse the menu if it is shown again before
  // it gets deleted; in that case, OnSubMenuHiddenCallback() just decrements
  // the reference count again. Note that the delay is just an optimization; we
  // could use PostTask() and this would still work correctly.
  g_object_ref(G_OBJECT(submenu));
  base::MessageLoop::current()->PostDelayedTask(
      FROM_HERE,
      base::Bind(&MenuGtk::OnSubMenuHiddenCallback, submenu),
      base::TimeDelta::FromSeconds(2));
}

namespace {

// Remove all descendant submenu-model data pointers.
void RemoveSubMenuModels(GtkWidget* menu_item, void* unused) {
  if (!GTK_IS_MENU_ITEM(menu_item))
    return;
  g_object_steal_data(G_OBJECT(menu_item), "submenu-model");
  GtkWidget* submenu = gtk_menu_item_get_submenu(GTK_MENU_ITEM(menu_item));
  if (submenu)
    gtk_container_foreach(GTK_CONTAINER(submenu), RemoveSubMenuModels, NULL);
}

}  // namespace

// static
void MenuGtk::OnSubMenuHiddenCallback(GtkWidget* submenu) {
  if (!gtk_widget_get_visible(submenu)) {
    // Remove all the children of this menu, clearing out their submenu-model
    // pointers in case they have pending calls to OnSubMenuHiddenCallback().
    // (Normally that won't happen: we'd have hidden them first, and so they'd
    // have already been deleted. But in some cases [e.g. on Ubuntu 12.04],
    // GTK menu operations may be hooked to allow external applications to
    // mirror the menu structure, and the hooks may show and hide menus in
    // order to trigger exactly the kind of dynamic menu building we're doing.
    // The result is that we see show and hide events in strange orders.)
    GList* children = gtk_container_get_children(GTK_CONTAINER(submenu));
    for (GList* child = children; child; child = g_list_next(child)) {
      RemoveSubMenuModels(GTK_WIDGET(child->data), NULL);
      gtk_container_remove(GTK_CONTAINER(submenu), GTK_WIDGET(child->data));
    }
    g_list_free(children);

    // Clear out the bit that says the menu is built.
    // We'll rebuild it next time it is shown.
    g_object_steal_data(G_OBJECT(submenu), "submenu-built");

    // Notify the submenu model that the menu has been hidden. This may cause
    // it to delete descendant submenu models, which is why we cleared those
    // pointers out above.
    GtkWidget* menu_item = static_cast<GtkWidget*>(
        g_object_get_data(G_OBJECT(submenu), "menu-item"));
    // TODO(mdm): Figure out why this can sometimes be NULL. See bug 124110.
    CHECK(menu_item);
    ui::MenuModel* submenu_model = static_cast<ui::MenuModel*>(
        g_object_get_data(G_OBJECT(menu_item), "submenu-model"));
    if (submenu_model)
      submenu_model->MenuClosed();
  }

  // Remove the reference we grabbed in OnSubMenuHidden() above.
  g_object_unref(G_OBJECT(submenu));
}

// static
void MenuGtk::SetButtonItemInfo(GtkWidget* button, gpointer userdata) {
  ui::ButtonMenuItemModel* model =
      reinterpret_cast<ui::ButtonMenuItemModel*>(
          g_object_get_data(G_OBJECT(button), "button-model"));
  int index = GPOINTER_TO_INT(g_object_get_data(
      G_OBJECT(button), "button-model-id"));

  if (model->IsItemDynamicAt(index)) {
    std::string label = ui::ConvertAcceleratorsFromWindowsStyle(
        base::UTF16ToUTF8(model->GetLabelAt(index)));
    gtk_button_set_label(GTK_BUTTON(button), label.c_str());
  }

  gtk_widget_set_sensitive(GTK_WIDGET(button), model->IsEnabledAt(index));
}

// static
void MenuGtk::SetMenuItemInfo(GtkWidget* widget, gpointer userdata) {
  if (GTK_IS_SEPARATOR_MENU_ITEM(widget)) {
    // We need to explicitly handle this case because otherwise we'll ask the
    // menu delegate about something with an invalid id.
    return;
  }

  int id;
  if (!GetMenuItemID(widget, &id))
    return;

  ui::MenuModel* model = ModelForMenuItem(GTK_MENU_ITEM(widget));
  if (!model) {
    // If we're not providing the sub menu, then there's no model.  For
    // example, the IME submenu doesn't have a model.
    return;
  }

  if (GTK_IS_CHECK_MENU_ITEM(widget)) {
    GtkCheckMenuItem* item = GTK_CHECK_MENU_ITEM(widget);

    // gtk_check_menu_item_set_active() will send the activate signal. Touching
    // the underlying "active" property will also call the "activate" handler
    // for this menu item. So we prevent the "activate" handler from
    // being called while we set the checkbox.
    // Why not use one of the glib signal-blocking functions?  Because when we
    // toggle a radio button, it will deactivate one of the other radio buttons,
    // which we don't have a pointer to.
    // Wny not make this a member variable?  Because "menu" is a pointer to the
    // root of the MenuGtk and we want to disable *all* MenuGtks, including
    // submenus.
    block_activation_ = true;
    gtk_check_menu_item_set_active(item, model->IsItemCheckedAt(id));
    block_activation_ = false;
  }

  if (GTK_IS_CUSTOM_MENU_ITEM(widget)) {
    // Iterate across all the buttons to update their visible properties.
    gtk_custom_menu_item_foreach_button(GTK_CUSTOM_MENU_ITEM(widget),
                                        SetButtonItemInfo,
                                        userdata);
  }

  if (GTK_IS_MENU_ITEM(widget)) {
    gtk_widget_set_sensitive(widget, model->IsEnabledAt(id));

    if (model->IsVisibleAt(id)) {
      // Update the menu item label if it is dynamic.
      if (model->IsItemDynamicAt(id)) {
        std::string label = ui::ConvertAcceleratorsFromWindowsStyle(
            base::UTF16ToUTF8(model->GetLabelAt(id)));

        gtk_menu_item_set_label(GTK_MENU_ITEM(widget), label.c_str());
        if (GTK_IS_IMAGE_MENU_ITEM(widget)) {
          gfx::Image icon;
          if (model->GetIconAt(id, &icon)) {
            gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(widget),
                                          gtk_image_new_from_pixbuf(
                                              icon.ToGdkPixbuf()));
          } else {
            gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(widget), NULL);
          }
        }
      }

      gtk_widget_show(widget);
    } else {
      gtk_widget_hide(widget);
    }

    GtkWidget* submenu = gtk_menu_item_get_submenu(GTK_MENU_ITEM(widget));
    if (submenu) {
      gtk_container_foreach(GTK_CONTAINER(submenu), &SetMenuItemInfo,
                            userdata);
    }
  }
}
