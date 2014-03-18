// Copyright (c) 2014 GitHub, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "atom/browser/native_window_gtk.h"

#include <string>
#include <vector>

#include "base/values.h"
#include "chrome/browser/ui/gtk/gtk_window_util.h"
#include "atom/common/draggable_region.h"
#include "atom/common/options_switches.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_contents_view.h"
#include "content/public/common/renderer_preferences.h"
#include "ui/base/accelerators/platform_accelerator_gtk.h"
#include "ui/base/models/simple_menu_model.h"
#include "ui/base/x/x11_util.h"
#include "ui/gfx/gtk_util.h"
#include "ui/gfx/rect.h"
#include "ui/gfx/skia_utils_gtk.h"

namespace atom {

namespace {

// Dividing GTK's cursor blink cycle time (in milliseconds) by this value yields
// an appropriate value for content::RendererPreferences::caret_blink_interval.
// This matches the logic in the WebKit GTK port.
const double kGtkCursorBlinkCycleFactor = 2000.0;

}  // namespace

NativeWindowGtk::NativeWindowGtk(content::WebContents* web_contents,
                                 base::DictionaryValue* options)
    : NativeWindow(web_contents, options),
      window_(GTK_WINDOW(gtk_window_new(GTK_WINDOW_TOPLEVEL))),
      vbox_(gtk_vbox_new(FALSE, 0)),
      state_(GDK_WINDOW_STATE_WITHDRAWN),
      is_always_on_top_(false),
      suppress_window_raise_(false),
      frame_cursor_(NULL) {
  gtk_container_add(GTK_CONTAINER(window_), vbox_);
  gtk_container_add(GTK_CONTAINER(vbox_),
                    GetWebContents()->GetView()->GetNativeView());

  int width = 800, height = 600;
  options->GetInteger(switches::kWidth, &width);
  options->GetInteger(switches::kHeight, &height);
  gtk_window_set_default_size(window_, width, height);

  if (!icon_.IsEmpty())
    gtk_window_set_icon(window_, icon_.ToGdkPixbuf());

  // In some (older) versions of compiz, raising top-level windows when they
  // are partially off-screen causes them to get snapped back on screen, not
  // always even on the current virtual desktop.  If we are running under
  // compiz, suppress such raises, as they are not necessary in compiz anyway.
  if (ui::GuessWindowManager() == ui::WM_COMPIZ)
    suppress_window_raise_ = true;

  g_signal_connect(window_, "delete-event",
                   G_CALLBACK(OnWindowDeleteEventThunk), this);
  g_signal_connect(window_, "focus-out-event",
                   G_CALLBACK(OnFocusOutThunk), this);
  g_signal_connect(window_, "key-press-event",
                   G_CALLBACK(OnKeyPressThunk), this);

  if (!has_frame_) {
    gtk_window_set_decorated(window_, false);

    g_signal_connect(window_, "motion-notify-event",
                     G_CALLBACK(OnMouseMoveEventThunk), this);
    g_signal_connect(window_, "button-press-event",
                     G_CALLBACK(OnButtonPressThunk), this);
  }

  SetWebKitColorStyle();
}

NativeWindowGtk::~NativeWindowGtk() {
  if (window_)
    gtk_widget_destroy(GTK_WIDGET(window_));
}

void NativeWindowGtk::Close() {
  CloseWebContents();
}

void NativeWindowGtk::CloseImmediately() {
  gtk_widget_destroy(GTK_WIDGET(window_));
  window_ = NULL;
}

void NativeWindowGtk::Move(const gfx::Rect& pos) {
  gtk_window_move(window_, pos.x(), pos.y());
  gtk_window_resize(window_, pos.width(), pos.height());
}

void NativeWindowGtk::Focus(bool focus) {
  if (!IsVisible())
    return;

  if (focus)
    gtk_window_present(window_);
  else
    gdk_window_lower(gtk_widget_get_window(GTK_WIDGET(window_)));
}

bool NativeWindowGtk::IsFocused() {
  return gtk_window_is_active(window_);
}

void NativeWindowGtk::Show() {
  gtk_widget_show_all(GTK_WIDGET(window_));
}

void NativeWindowGtk::Hide() {
  gtk_widget_hide(GTK_WIDGET(window_));
}

bool NativeWindowGtk::IsVisible() {
  return gtk_widget_get_visible(GTK_WIDGET(window_));
}

void NativeWindowGtk::Maximize() {
  gtk_window_maximize(window_);
}

void NativeWindowGtk::Unmaximize() {
  gtk_window_unmaximize(window_);
}

void NativeWindowGtk::Minimize() {
  gtk_window_iconify(window_);
}

void NativeWindowGtk::Restore() {
  gtk_window_present(window_);
}

void NativeWindowGtk::SetFullscreen(bool fullscreen) {
  if (fullscreen)
    gtk_window_fullscreen(window_);
  else
    gtk_window_unfullscreen(window_);
}

bool NativeWindowGtk::IsFullscreen() {
  return state_ & GDK_WINDOW_STATE_FULLSCREEN;
}

void NativeWindowGtk::SetSize(const gfx::Size& size) {
  gtk_window_resize(window_, size.width(), size.height());
}

gfx::Size NativeWindowGtk::GetSize() {
  GdkWindow* gdk_window = gtk_widget_get_window(GTK_WIDGET(window_));

  GdkRectangle frame_extents;
  gdk_window_get_frame_extents(gdk_window, &frame_extents);

  return gfx::Size(frame_extents.width, frame_extents.height);
}

void NativeWindowGtk::SetMinimumSize(const gfx::Size& size) {
  minimum_size_ = size;

  GdkGeometry geometry = { 0 };
  geometry.min_width = size.width();
  geometry.min_height = size.height();
  int hints = GDK_HINT_POS | GDK_HINT_MIN_SIZE;
  gtk_window_set_geometry_hints(
      window_, GTK_WIDGET(window_), &geometry, (GdkWindowHints)hints);
}

gfx::Size NativeWindowGtk::GetMinimumSize() {
  return minimum_size_;
}

void NativeWindowGtk::SetMaximumSize(const gfx::Size& size) {
  maximum_size_ = size;

  GdkGeometry geometry = { 0 };
  geometry.max_width = size.width();
  geometry.max_height = size.height();
  int hints = GDK_HINT_POS | GDK_HINT_MAX_SIZE;
  gtk_window_set_geometry_hints(
      window_, GTK_WIDGET(window_), &geometry, (GdkWindowHints)hints);
}

gfx::Size NativeWindowGtk::GetMaximumSize() {
  return maximum_size_;
}

void NativeWindowGtk::SetResizable(bool resizable) {
  // Should request widget size after setting unresizable, otherwise the
  // window will shrink to a very small size.
  if (!IsResizable()) {
    gint width, height;
    gtk_window_get_size(window_, &width, &height);
    gtk_widget_set_size_request(GTK_WIDGET(window_), width, height);
  }

  gtk_window_set_resizable(window_, resizable);
}

bool NativeWindowGtk::IsResizable() {
  return gtk_window_get_resizable(window_);
}

void NativeWindowGtk::SetAlwaysOnTop(bool top) {
  is_always_on_top_ = top;
  gtk_window_set_keep_above(window_, top ? TRUE : FALSE);
}

bool NativeWindowGtk::IsAlwaysOnTop() {
  return is_always_on_top_;
}

void NativeWindowGtk::Center() {
  gtk_window_set_position(window_, GTK_WIN_POS_CENTER);
}

void NativeWindowGtk::SetPosition(const gfx::Point& position) {
  gtk_window_move(window_, position.x(), position.y());
}

gfx::Point NativeWindowGtk::GetPosition() {
  GdkWindow* gdk_window = gtk_widget_get_window(GTK_WIDGET(window_));

  GdkRectangle frame_extents;
  gdk_window_get_frame_extents(gdk_window, &frame_extents);

  return gfx::Point(frame_extents.x, frame_extents.y);
}

void NativeWindowGtk::SetTitle(const std::string& title) {
  gtk_window_set_title(window_, title.c_str());
}

std::string NativeWindowGtk::GetTitle() {
  return gtk_window_get_title(window_);
}

void NativeWindowGtk::FlashFrame(bool flash) {
  gtk_window_set_urgency_hint(window_, flash);
}

void NativeWindowGtk::SetKiosk(bool kiosk) {
  SetFullscreen(kiosk);
}

bool NativeWindowGtk::IsKiosk() {
  return IsFullscreen();
}

gfx::NativeWindow NativeWindowGtk::GetNativeWindow() {
  return window_;
}

void NativeWindowGtk::SetMenu(ui::MenuModel* menu_model) {
  menu_.reset(new ::MenuGtk(this, menu_model, true));
  gtk_box_pack_start(GTK_BOX(vbox_), menu_->widget(), FALSE, FALSE, 0);
  gtk_box_reorder_child(GTK_BOX(vbox_), menu_->widget(), 0);
  RegisterAccelerators();
}

void NativeWindowGtk::UpdateDraggableRegions(
    const std::vector<DraggableRegion>& regions) {
  // Draggable region is not supported for non-frameless window.
  if (has_frame_)
    return;

  draggable_region_.reset(new SkRegion);

  // By default, the whole window is non-draggable. We need to explicitly
  // include those draggable regions.
  for (std::vector<DraggableRegion>::const_iterator iter =
           regions.begin();
       iter != regions.end(); ++iter) {
    const DraggableRegion& region = *iter;
    draggable_region_->op(
        region.bounds.x(),
        region.bounds.y(),
        region.bounds.right(),
        region.bounds.bottom(),
        region.draggable ? SkRegion::kUnion_Op : SkRegion::kDifference_Op);
  }
}

void NativeWindowGtk::RegisterAccelerators() {
  DCHECK(menu_);
  accelerator_table_.clear();
  accelerator_util::GenerateAcceleratorTable(&accelerator_table_,
                                             menu_->model());
}

void NativeWindowGtk::SetWebKitColorStyle() {
  content::RendererPreferences* prefs =
      GetWebContents()->GetMutableRendererPrefs();
  GtkStyle* frame_style = gtk_rc_get_style(GTK_WIDGET(window_));
  prefs->focus_ring_color =
      gfx::GdkColorToSkColor(frame_style->bg[GTK_STATE_SELECTED]);
  prefs->thumb_active_color = SkColorSetRGB(244, 244, 244);
  prefs->thumb_inactive_color = SkColorSetRGB(234, 234, 234);
  prefs->track_color = SkColorSetRGB(211, 211, 211);

  GtkWidget* url_entry = gtk_entry_new();
  GtkStyle* entry_style = gtk_rc_get_style(url_entry);
  prefs->active_selection_bg_color =
      gfx::GdkColorToSkColor(entry_style->base[GTK_STATE_SELECTED]);
  prefs->active_selection_fg_color =
      gfx::GdkColorToSkColor(entry_style->text[GTK_STATE_SELECTED]);
  prefs->inactive_selection_bg_color =
      gfx::GdkColorToSkColor(entry_style->base[GTK_STATE_ACTIVE]);
  prefs->inactive_selection_fg_color =
      gfx::GdkColorToSkColor(entry_style->text[GTK_STATE_ACTIVE]);
  gtk_widget_destroy(url_entry);

  const base::TimeDelta cursor_blink_time = gfx::GetCursorBlinkCycle();
  prefs->caret_blink_interval =
      cursor_blink_time.InMilliseconds() ?
      cursor_blink_time.InMilliseconds() / kGtkCursorBlinkCycleFactor :
      0;
}

bool NativeWindowGtk::IsMaximized() const {
  return state_ & GDK_WINDOW_STATE_MAXIMIZED;
}

bool NativeWindowGtk::GetWindowEdge(int x, int y, GdkWindowEdge* edge) {
  if (has_frame_)
    return false;

  if (IsMaximized() || IsFullscreen())
    return false;

  return gtk_window_util::GetWindowEdge(GetSize(), 0, x, y, edge);
}

gboolean NativeWindowGtk::OnWindowDeleteEvent(GtkWidget* widget,
                                              GdkEvent* event) {
  Close();
  return TRUE;
}

gboolean NativeWindowGtk::OnFocusOut(GtkWidget* window, GdkEventFocus*) {
  NotifyWindowBlur();
  return FALSE;
}

gboolean NativeWindowGtk::OnWindowState(GtkWidget* window,
                                        GdkEventWindowState* event) {
  state_ = event->new_window_state;
  return FALSE;
}

gboolean NativeWindowGtk::OnMouseMoveEvent(GtkWidget* widget,
                                           GdkEventMotion* event) {
  if (has_frame_) {
    // Reset the cursor.
    if (frame_cursor_) {
      frame_cursor_ = NULL;
      gdk_window_set_cursor(gtk_widget_get_window(GTK_WIDGET(window_)), NULL);
    }
    return FALSE;
  }

  if (!IsResizable())
    return FALSE;

  // Update the cursor if we're on the custom frame border.
  GdkWindowEdge edge;
  bool has_hit_edge = GetWindowEdge(static_cast<int>(event->x),
                                    static_cast<int>(event->y), &edge);
  GdkCursorType new_cursor = GDK_LAST_CURSOR;
  if (has_hit_edge)
    new_cursor = gtk_window_util::GdkWindowEdgeToGdkCursorType(edge);

  GdkCursorType last_cursor = GDK_LAST_CURSOR;
  if (frame_cursor_)
    last_cursor = frame_cursor_->type;

  if (last_cursor != new_cursor) {
    frame_cursor_ = has_hit_edge ? gfx::GetCursor(new_cursor) : NULL;
    gdk_window_set_cursor(gtk_widget_get_window(GTK_WIDGET(window_)),
                          frame_cursor_);
  }
  return FALSE;
}

gboolean NativeWindowGtk::OnButtonPress(GtkWidget* widget,
                                        GdkEventButton* event) {
  DCHECK(!has_frame_);
  // Make the button press coordinate relative to the browser window.
  int win_x, win_y;
  GdkWindow* gdk_window = gtk_widget_get_window(GTK_WIDGET(window_));
  gdk_window_get_origin(gdk_window, &win_x, &win_y);

  GdkWindowEdge edge;
  gfx::Point point(static_cast<int>(event->x_root - win_x),
                   static_cast<int>(event->y_root - win_y));
  bool has_hit_edge = IsResizable() &&
                      GetWindowEdge(point.x(), point.y(), &edge);
  bool has_hit_titlebar =
      draggable_region_ && draggable_region_->contains(event->x, event->y);

  if (event->button == 1) {
    if (GDK_BUTTON_PRESS == event->type) {
      // Raise the window after a click on either the titlebar or the border to
      // match the behavior of most window managers, unless that behavior has
      // been suppressed.
      if ((has_hit_titlebar || has_hit_edge) && !suppress_window_raise_)
        gdk_window_raise(GTK_WIDGET(widget)->window);

      if (has_hit_edge) {
        gtk_window_begin_resize_drag(window_, edge, event->button,
                                     static_cast<gint>(event->x_root),
                                     static_cast<gint>(event->y_root),
                                     event->time);
        return TRUE;
      } else if (has_hit_titlebar) {
        GdkRectangle window_bounds = {0};
        gdk_window_get_frame_extents(gdk_window, &window_bounds);
        gfx::Rect bounds(window_bounds.x, window_bounds.y,
                         window_bounds.width, window_bounds.height);
        return gtk_window_util::HandleTitleBarLeftMousePress(
            window_, bounds, event);
      }
    } else if (GDK_2BUTTON_PRESS == event->type) {
      if (has_hit_titlebar && IsResizable()) {
        // Maximize/restore on double click.
        if (IsMaximized())
          gtk_window_unmaximize(window_);
        else
          gtk_window_maximize(window_);
        return TRUE;
      }
    }
  } else if (event->button == 2) {
    if (has_hit_titlebar || has_hit_edge)
      gdk_window_lower(gdk_window);
    return TRUE;
  }

  return FALSE;
}

gboolean NativeWindowGtk::OnKeyPress(GtkWidget* widget, GdkEventKey* event) {
  ui::Accelerator accelerator = ui::AcceleratorForGdkKeyCodeAndModifier(
      event->keyval, static_cast<GdkModifierType>(event->state));
  return accelerator_util::TriggerAcceleratorTableCommand(
      &accelerator_table_, accelerator) ? TRUE: FALSE;
}

// static
NativeWindow* NativeWindow::Create(content::WebContents* web_contents,
                                   base::DictionaryValue* options) {
  return new NativeWindowGtk(web_contents, options);
}

}  // namespace atom
