// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "browser/native_window.h"

#include <algorithm>
#include <string>

#include "base/utf_string_conversions.h"
#include "base/values.h"
#include "brightray/browser/inspectable_web_contents.h"
#include "brightray/browser/inspectable_web_contents_view.h"
#include "browser/api/atom_browser_bindings.h"
#include "browser/atom_browser_context.h"
#include "browser/atom_browser_main_parts.h"
#include "browser/atom_javascript_dialog_manager.h"
#include "content/public/browser/navigation_entry.h"
#include "content/public/browser/notification_details.h"
#include "content/public/browser/notification_source.h"
#include "content/public/browser/notification_types.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/render_view_host.h"
#include "common/api/api_messages.h"
#include "common/options_switches.h"
#include "ipc/ipc_message_macros.h"
#include "ui/gfx/point.h"
#include "ui/gfx/rect.h"
#include "ui/gfx/size.h"

using content::NavigationEntry;

namespace atom {

// static
std::vector<NativeWindow*> NativeWindow::windows_;

NativeWindow::NativeWindow(content::WebContents* web_contents,
                           base::DictionaryValue* options)
    : content::WebContentsObserver(web_contents),
      inspectable_web_contents_(
          brightray::InspectableWebContents::Create(web_contents)) {
  web_contents->SetDelegate(this);

  windows_.push_back(this);

  // Get notified of title updated message.
  registrar_.Add(this, content::NOTIFICATION_WEB_CONTENTS_TITLE_UPDATED,
      content::Source<content::WebContents>(web_contents));
}

NativeWindow::~NativeWindow() {
  windows_.erase(std::remove(windows_.begin(), windows_.end(), this),
                 windows_.end());
}

// static
NativeWindow* NativeWindow::Create(base::DictionaryValue* options) {
  content::WebContents::CreateParams create_params(AtomBrowserContext::Get());
  return Create(content::WebContents::Create(create_params), options);
}

// static
NativeWindow* NativeWindow::FromProcessIDAndRoutingID(int process_id,
                                                      int routing_id) {
  // Stupid iterating.
  for (auto window : windows_) {
    content::WebContents* web_contents = window->GetWebContents();
    int window_process_id = web_contents->GetRenderProcessHost()->GetID();
    int window_routing_id = web_contents->GetRoutingID();
    if (window_routing_id == routing_id && window_process_id == process_id)
      return window;
  }

  return NULL;
}

void NativeWindow::InitFromOptions(base::DictionaryValue* options) {
  // Setup window from options.
  int x, y;
  std::string position;
  if (options->GetInteger(switches::kX, &x) &&
      options->GetInteger(switches::kY, &y)) {
    int width, height;
    options->GetInteger(switches::kWidth, &width);
    options->GetInteger(switches::kHeight, &height);
    Move(gfx::Rect(x, y, width, height));
  } else if (options->GetString(switches::kPosition, &position)) {
    SetPosition(position);
  }
  int min_height, min_width;
  if (options->GetInteger(switches::kMinHeight, &min_height) &&
      options->GetInteger(switches::kMinWidth, &min_width)) {
    SetMinimumSize(gfx::Size(min_width, min_height));
  }
  int max_height, max_width;
  if (options->GetInteger(switches::kMaxHeight, &max_height) &&
      options->GetInteger(switches::kMaxWidth, &max_width)) {
    SetMaximumSize(gfx::Size(max_width, max_height));
  }
  bool resizable;
  if (options->GetBoolean(switches::kResizable, &resizable)) {
    SetResizable(resizable);
  }
  bool top;
  if (options->GetBoolean(switches::kAlwaysOnTop, &top) && top) {
    SetAlwaysOnTop(true);
  }
  bool fullscreen;
  if (options->GetBoolean(switches::kFullscreen, &fullscreen) && fullscreen) {
    SetFullscreen(true);
  }
  bool kiosk;
  if (options->GetBoolean(switches::kKiosk, &kiosk) && kiosk) {
    SetKiosk(kiosk);
  }
  std::string title("Atom Shell");
  options->GetString(switches::kTitle, &title);
  SetTitle(title);

  // Then show it.
  bool show = true;
  options->GetBoolean(switches::kShow, &show);
  if (show)
    Show();
}

void NativeWindow::ShowDevTools() {
  inspectable_web_contents()->ShowDevTools();
}

void NativeWindow::CloseDevTools() {
  inspectable_web_contents()->GetView()->CloseDevTools();
}

void NativeWindow::CloseWebContents() {
  bool prevent_default = false;
  FOR_EACH_OBSERVER(NativeWindowObserver,
                    observers_,
                    WillCloseWindow(&prevent_default));
  if (prevent_default)
    return;

  content::WebContents* web_contents(GetWebContents());

  if (web_contents->NeedToFireBeforeUnload())
    web_contents->GetRenderViewHost()->FirePageBeforeUnload(false);
  else
    web_contents->Close();
}

content::WebContents* NativeWindow::GetWebContents() const {
  return inspectable_web_contents_->GetWebContents();
}

// Window opened by window.open.
void NativeWindow::WebContentsCreated(
    content::WebContents* source_contents,
    int64 source_frame_id,
    const string16& frame_name,
    const GURL& target_url,
    content::WebContents* new_contents) {
  LOG(WARNING) << "Please use node-style Window API to create window, "
                  "using window.open has very strict constrains.";

  scoped_ptr<base::DictionaryValue> options(new base::DictionaryValue);
  options->SetInteger(switches::kWidth, 800);
  options->SetInteger(switches::kHeight, 600);

  NativeWindow* window = Create(new_contents, options.get());
  window->InitFromOptions(options.get());
}

content::JavaScriptDialogManager* NativeWindow::GetJavaScriptDialogManager() {
  if (!dialog_manager_)
    dialog_manager_.reset(new AtomJavaScriptDialogManager);

  return dialog_manager_.get();
}

void NativeWindow::CloseContents(content::WebContents* source) {
  // When the web contents is gone, close the window immediately, but the
  // memory will not be freed until you call delete.
  // In this way, it would be safe to manage windows via smart pointers. If you
  // want to free memory when the window is closed, you can do deleting by
  // overriding WillCloseWindow method in the observer.
  CloseImmediately();

  FOR_EACH_OBSERVER(NativeWindowObserver, observers_, OnWindowClosed());
}

bool NativeWindow::OnMessageReceived(const IPC::Message& message) {
  bool handled = true;
  IPC_BEGIN_MESSAGE_MAP(NativeWindow, message)
    IPC_MESSAGE_HANDLER(AtomViewHostMsg_Message, OnRendererMessage)
    IPC_MESSAGE_HANDLER(AtomViewHostMsg_Message_Sync, OnRendererMessageSync)
    IPC_MESSAGE_UNHANDLED(handled = false)
  IPC_END_MESSAGE_MAP()

  return handled;
}

void NativeWindow::Observe(int type,
                           const content::NotificationSource& source,
                           const content::NotificationDetails& details) {
  if (type == content::NOTIFICATION_WEB_CONTENTS_TITLE_UPDATED) {
    std::pair<NavigationEntry*, bool>* title =
        content::Details<std::pair<NavigationEntry*, bool>>(details).ptr();

    if (title->first) {
      bool prevent_default = false;
      std::string text = UTF16ToUTF8(title->first->GetTitle());
      FOR_EACH_OBSERVER(NativeWindowObserver,
                        observers_,
                        OnPageTitleUpdated(&prevent_default, text));

      if (!prevent_default)
        SetTitle(text);
    }
  }
}

void NativeWindow::OnRendererMessage(const std::string& channel,
                                     const base::ListValue& args) {
  AtomBrowserMainParts::Get()->atom_bindings()->OnRendererMessage(
      GetWebContents()->GetRenderProcessHost()->GetID(),
      GetWebContents()->GetRoutingID(),
      channel,
      args);
}

void NativeWindow::OnRendererMessageSync(const std::string& channel,
                                         const base::ListValue& args,
                                         base::DictionaryValue* result) {
  AtomBrowserMainParts::Get()->atom_bindings()->OnRendererMessageSync(
      GetWebContents()->GetRenderProcessHost()->GetID(),
      GetWebContents()->GetRoutingID(),
      channel,
      args,
      result);
}

}  // namespace atom
