#include "browser/linux/inspectable_web_contents_view_linux.h"
#include <glib-object.h>
#include <gtk/gtk.h>

#include "base/strings/stringprintf.h"
#include "browser/browser_client.h"
#include "browser/inspectable_web_contents_impl.h"

#include "content/public/browser/web_contents_view.h"

namespace brightray {

namespace {

bool IsWidgetAncestryVisible(GtkWidget* widget) {
  GtkWidget* parent = widget;
  while (parent && gtk_widget_get_visible(parent))
    parent = gtk_widget_get_parent(parent);
  return !parent;
}

}

InspectableWebContentsView* CreateInspectableContentsView(
    InspectableWebContentsImpl* inspectable_web_contents) {
  return new InspectableWebContentsViewLinux(inspectable_web_contents);
}

InspectableWebContentsViewLinux::InspectableWebContentsViewLinux(
    InspectableWebContentsImpl* inspectable_web_contents)
    : inspectable_web_contents_(inspectable_web_contents),
      devtools_window_(NULL) {
}

InspectableWebContentsViewLinux::~InspectableWebContentsViewLinux() {
  if (devtools_window_) gtk_widget_destroy(devtools_window_);
}

#if 0  // some utility functions to debug GTK window hierarchies
static void dump_one(GtkWidget *wat, int indent) {
  GtkAllocation alloc;
  gtk_widget_get_allocation(wat, &alloc);
  fprintf(stderr, "%*s[%p] %s @%d,%d %dx%d",
    indent, "", wat,
    g_type_name_from_instance(reinterpret_cast<GTypeInstance*>(wat)),
    alloc.x, alloc.y, alloc.width, alloc.height);
  if (GTK_IS_WINDOW(wat)) {
    fprintf(stderr, " - \"%s\"", gtk_window_get_title(GTK_WINDOW(wat)));
  }
  fputc('\n', stderr);
}

static void dump_the_whole_tree(GtkWidget *wat, int indent) {
  if (!wat) {
    fprintf(stderr, "(nil)\n");
    return;
  }
  dump_one(wat, indent);
  GList *kids = gtk_container_get_children(GTK_CONTAINER(wat));
  for (GList *p = kids; p; p = p->next) {
    dump_the_whole_tree(GTK_WIDGET(p->data), indent+2);
  }
}

static void dump_parents(GtkWidget *wat) {
  fprintf(stderr, "Parents:\n");
  for (GtkWidget *p = gtk_widget_get_parent(wat);
       p;
       p = gtk_widget_get_parent(p)) {
    dump_one(p, 2);
  }
}
#endif

gfx::NativeView InspectableWebContentsViewLinux::GetNativeView() const {
  auto web_contents = inspectable_web_contents_->GetWebContents();
  return web_contents->GetView()->GetNativeView();
}


/* This code is a little bit hairy.
   The dev tools can be in any one of five places:
   1. Unassigned and invisible.  This is the default state until someone asks
      to 'inspect element' for the first time.  In this case, devtools->parent is
      NULL.
   2. In an onscreen window, visible.
   3. In the bottom half of a GtkVPaned.
   4. In the right half of a GtkHPaned.
   5. In an offscreen window, invisible.  This is where they go once they have
      been displayed and the user asks to "close" them.  They can't be put back
      into the unassigned state.
   ShowDevTools() and is responsible for transitioning from any one of these
   states to the three visible states, 2-4, as indicated by the contents of the
   'dockside_' variable.  The helper functions ShowDevToolsInWindow and
   ShowDevToolsInPane focus on transitioning to states 2 and 3+4, respectively.
   These helper functions are responsible for the entire transition, including
   cleaning up any extraneous containers from the old state.

   Hiding the dev tools is taken care of by CloseDevTools (from paned states
   3+4 to invisible state 5) or by the "delete-event" signal on the
   devtools_window_ (from window state 2 to 5).

   Remember that GTK does reference counting, so a view with no refs and no
   parent will be freed.  Views that have a ref but no parents will lose their
   dimensions.  So it's best to move the devtools view from place to place with
   gtk_widget_reparent whenever possible.  Unfortunately, one cannot reparent
   things into a GtkPaned, so fairly brittle use of g_object_[un]ref and
   gtk_container_remove happens.
*/

void InspectableWebContentsViewLinux::ShowDevTools() {
  auto devtools_web_contents =
      inspectable_web_contents()->devtools_web_contents();
  GtkWidget *devtools = devtools_web_contents->GetView()->GetNativeView();
  GtkWidget *parent = gtk_widget_get_parent(devtools);

  DLOG(INFO) << base::StringPrintf(
      "InspectableWebContentsViewLinux::ShowDevTools - " \
      "parent=%s@%p window=%p dockside=\"%s\"",
      g_type_name_from_instance(reinterpret_cast<GTypeInstance*>(parent)),
      parent,
      devtools_window_,
      dockside_.c_str());

  if (!parent || GTK_IS_PANED(parent)) {
    if (dockside_ == "undocked")    ShowDevToolsInWindow();
    else if (dockside_ == "bottom") ShowDevToolsInPane(true);
    else if (dockside_ == "right")  ShowDevToolsInPane(false);
  } else {
    DCHECK(parent == devtools_window_);
    if (dockside_ == "undocked")    gtk_widget_show_all(parent);
    else if (dockside_ == "bottom") ShowDevToolsInPane(true);
    else if (dockside_ == "right")  ShowDevToolsInPane(false);
  }
}

void InspectableWebContentsViewLinux::CloseDevTools() {
  auto devtools_web_contents =
      inspectable_web_contents()->devtools_web_contents();
  GtkWidget *devtools = devtools_web_contents->GetView()->GetNativeView();
  GtkWidget *parent = gtk_widget_get_parent(devtools);

  DLOG(INFO) << base::StringPrintf(
      "InspectableWebContentsViewLinux::CloseDevTools - " \
      "parent=%s@%p window=%p dockside=\"%s\"",
      g_type_name_from_instance(reinterpret_cast<GTypeInstance*>(parent)),
      parent,
      devtools_window_,
      dockside_.c_str());

  if (!parent) {
    return;  // Not visible -> nothing to do
  } else if (GTK_IS_PANED(parent)) {
    GtkWidget *browser = GetBrowserWindow();
    GtkWidget *view = GetNativeView();

    if (!devtools_window_) MakeDevToolsWindow();
    gtk_widget_reparent(devtools, devtools_window_);
    g_object_ref(parent);
    gtk_container_remove(GTK_CONTAINER(browser), parent);
    gtk_widget_reparent(view, browser);
    g_object_unref(parent);
  } else {
    DCHECK(parent == devtools_window_);
    gtk_widget_hide(parent);
  }
}

bool InspectableWebContentsViewLinux::IsDevToolsOpened() {
  auto devtools_web_contents =
      inspectable_web_contents()->devtools_web_contents();
  GtkWidget* devtools = devtools_web_contents->GetView()->GetNativeView();
  return IsWidgetAncestryVisible(devtools);
}

bool InspectableWebContentsViewLinux::SetDockSide(const std::string& side) {
  DLOG(INFO) <<
      "InspectableWebContentsViewLinux::SetDockSide: \"" << side << "\"";
  if (side != "undocked" && side != "bottom" && side != "right")
    return false;  // unsupported display location
  if (dockside_ == side)
    return true;  // no change from current location

  dockside_ = side;

  // If devtools already has a parent, then we're being asked to move it.
  auto devtools_web_contents =
      inspectable_web_contents()->devtools_web_contents();
  GtkWidget *devtools = devtools_web_contents->GetView()->GetNativeView();
  if (gtk_widget_get_parent(devtools)) {
    ShowDevTools();
  }

  return true;
}

void InspectableWebContentsViewLinux::ShowDevToolsInWindow() {
  auto devtools_web_contents =
      inspectable_web_contents()->devtools_web_contents();
  GtkWidget *devtools = devtools_web_contents->GetView()->GetNativeView();
  GtkWidget *parent = gtk_widget_get_parent(devtools);

  if (!devtools_window_)
    MakeDevToolsWindow();
  if (!parent) {
    gtk_container_add(GTK_CONTAINER(devtools_window_), devtools);
  } else if (parent != devtools_window_) {
    DCHECK(GTK_IS_PANED(parent));
    gtk_widget_reparent(devtools, devtools_window_);

    // Remove the pane.
    GtkWidget *view = GetNativeView();
    GtkWidget *browser = GetBrowserWindow();
    g_object_ref(view);
    gtk_container_remove(GTK_CONTAINER(parent), view);
    gtk_container_remove(GTK_CONTAINER(browser), parent);
    gtk_container_add(GTK_CONTAINER(browser), view);
    g_object_unref(view);
  }
  gtk_widget_show_all(devtools_window_);
}

void InspectableWebContentsViewLinux::MakeDevToolsWindow() {
  DCHECK(!devtools_window_);
  devtools_window_ = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title(GTK_WINDOW(devtools_window_), "Developer Tools");
  gtk_window_set_default_size(GTK_WINDOW(devtools_window_), 800, 600);
  g_signal_connect(GTK_OBJECT(devtools_window_),
                   "delete-event",
                   G_CALLBACK(gtk_widget_hide_on_delete),
                   this);
}

void InspectableWebContentsViewLinux::ShowDevToolsInPane(bool on_bottom) {
  auto devtools_web_contents =
      inspectable_web_contents()->devtools_web_contents();
  GtkWidget *devtools = devtools_web_contents->GetView()->GetNativeView();
  GtkWidget *parent = gtk_widget_get_parent(devtools);
  GtkWidget *pane = on_bottom ? gtk_vpaned_new() : gtk_hpaned_new();
  GtkWidget *view = GetNativeView();
  GtkWidget *browser = GetBrowserWindow();

  GtkAllocation alloc;
  gtk_widget_get_allocation(browser, &alloc);
  gtk_paned_set_position(GTK_PANED(pane),
                         on_bottom ? alloc.height * 2 / 3 : alloc.width / 2);
  if (!parent) {
    g_object_ref(view);
    gtk_container_remove(GTK_CONTAINER(browser), view);
    gtk_paned_add1(GTK_PANED(pane), view);
    gtk_paned_add2(GTK_PANED(pane), devtools);
    g_object_unref(view);
  } else if (GTK_IS_PANED(parent)) {
    g_object_ref(view);
    g_object_ref(devtools);
    gtk_container_remove(GTK_CONTAINER(parent), view);
    gtk_container_remove(GTK_CONTAINER(parent), devtools);
    gtk_paned_add1(GTK_PANED(pane), view);
    gtk_paned_add2(GTK_PANED(pane), devtools);
    g_object_unref(view);
    g_object_unref(devtools);
    gtk_container_remove(GTK_CONTAINER(browser), parent);
  } else {
    DCHECK(parent == devtools_window_);
    g_object_ref(view);
    gtk_container_remove(GTK_CONTAINER(devtools_window_), devtools);
    gtk_container_remove(GTK_CONTAINER(browser), view);
    gtk_paned_add1(GTK_PANED(pane), view);
    gtk_paned_add2(GTK_PANED(pane), devtools);
    g_object_unref(view);
    gtk_widget_hide(devtools_window_);
  }
  gtk_container_add(GTK_CONTAINER(browser), pane);
  gtk_widget_show_all(pane);
}

GtkWidget *InspectableWebContentsViewLinux::GetBrowserWindow() {
  GtkWidget *view = GetNativeView();
  GtkWidget *parent = gtk_widget_get_parent(view);
  GtkWidget *browser =
      GTK_IS_PANED(parent) ? gtk_widget_get_parent(parent) : parent;
  DCHECK(GTK_IS_WINDOW(browser));
  return browser;
}

}  // namespace brightray
