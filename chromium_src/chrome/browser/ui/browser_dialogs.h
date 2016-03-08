// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_BROWSER_DIALOGS_H_
#define CHROME_BROWSER_UI_BROWSER_DIALOGS_H_

#include "base/callback.h"
#include "build/build_config.h"
#include "chrome/browser/ui/bookmarks/bookmark_editor.h"
#include "components/security_state/security_state_model.h"
#include "third_party/skia/include/core/SkColor.h"
#include "ui/gfx/native_widget_types.h"

class Browser;
class ContentSettingBubbleModel;
class GURL;
class LoginHandler;
class Profile;

namespace bookmarks {
class BookmarkBubbleObserver;
}

namespace content {
class BrowserContext;
class ColorChooser;
class WebContents;
}

namespace extensions {
class Extension;
}

namespace gfx {
class Point;
}

namespace net {
class AuthChallengeInfo;
class URLRequest;
}

namespace ui {
class WebDialogDelegate;
}

namespace chrome {

// Creates and shows an HTML dialog with the given delegate and context.
// The window is automatically destroyed when it is closed.
// Returns the created window.
//
// Make sure to use the returned window only when you know it is safe
// to do so, i.e. before OnDialogClosed() is called on the delegate.
gfx::NativeWindow ShowWebDialog(gfx::NativeView parent,
                                content::BrowserContext* context,
                                ui::WebDialogDelegate* delegate);

// Shows or hides the Task Manager. |browser| can be NULL when called from Ash.
void ShowTaskManager(Browser* browser);
void HideTaskManager();

#if !defined(OS_MACOSX)
// Shows the create web app shortcut dialog box.
void ShowCreateWebAppShortcutsDialog(gfx::NativeWindow parent_window,
                                     content::WebContents* web_contents);
#endif

// Shows the create chrome app shortcut dialog box.
// |close_callback| may be null.
void ShowCreateChromeAppShortcutsDialog(
    gfx::NativeWindow parent_window,
    Profile* profile,
    const extensions::Extension* app,
    const base::Callback<void(bool /* created */)>& close_callback);

// Shows a color chooser that reports to the given WebContents.
content::ColorChooser* ShowColorChooser(content::WebContents* web_contents,
                                        SkColor initial_color);

#if defined(OS_MACOSX)

// For Mac, returns true if Chrome should show an equivalent toolkit-views based
// dialog using one of the functions below, rather than showing a Cocoa dialog.
bool ToolkitViewsDialogsEnabled();

// Shows a Views website settings bubble at the given anchor point.
void ShowWebsiteSettingsBubbleViewsAtPoint(
    const gfx::Point& anchor_point,
    Profile* profile,
    content::WebContents* web_contents,
    const GURL& url,
    const security_state::SecurityStateModel::SecurityInfo& security_info);

// Show a Views bookmark bubble at the given point. This occurs when the
// bookmark star is clicked or "Bookmark This Page..." is selected from a menu
// or via a key equivalent.
void ShowBookmarkBubbleViewsAtPoint(const gfx::Point& anchor_point,
                                    gfx::NativeView parent,
                                    bookmarks::BookmarkBubbleObserver* observer,
                                    Browser* browser,
                                    const GURL& url,
                                    bool newly_bookmarked);

#endif  // OS_MACOSX

#if defined(TOOLKIT_VIEWS)

// Creates a toolkit-views based LoginHandler (e.g. HTTP-Auth dialog).
LoginHandler* CreateLoginHandlerViews(net::AuthChallengeInfo* auth_info,
                                      net::URLRequest* request);

// Shows the toolkit-views based BookmarkEditor.
void ShowBookmarkEditorViews(gfx::NativeWindow parent_window,
                             Profile* profile,
                             const BookmarkEditor::EditDetails& details,
                             BookmarkEditor::Configuration configuration);

#if defined(OS_MACOSX)

// This is a class so that it can be friended from ContentSettingBubbleContents,
// which allows it to call SetAnchorRect().
class ContentSettingBubbleViewsBridge {
 public:
  static void Show(gfx::NativeView parent_view,
                   ContentSettingBubbleModel* model,
                   content::WebContents* web_contents,
                   const gfx::Point& anchor);
 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(ContentSettingBubbleViewsBridge);
};

#endif  // OS_MACOSX

#endif  // TOOLKIT_VIEWS

}  // namespace chrome

#endif  // CHROME_BROWSER_UI_BROWSER_DIALOGS_H_
