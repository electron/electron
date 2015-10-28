// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_COMMON_WEB_CONTENTS_DELEGATE_H_
#define ATOM_BROWSER_COMMON_WEB_CONTENTS_DELEGATE_H_

#include <map>
#include <string>
#include <vector>

#include "brightray/browser/default_web_contents_delegate.h"
#include "brightray/browser/inspectable_web_contents_impl.h"
#include "brightray/browser/inspectable_web_contents_delegate.h"
#include "brightray/browser/inspectable_web_contents_view_delegate.h"

namespace atom {

class AtomJavaScriptDialogManager;
class NativeWindow;
class WebDialogHelper;

class CommonWebContentsDelegate
    : public brightray::DefaultWebContentsDelegate,
      public brightray::InspectableWebContentsDelegate,
      public brightray::InspectableWebContentsViewDelegate {
 public:
  CommonWebContentsDelegate();
  virtual ~CommonWebContentsDelegate();

  // Creates a InspectableWebContents object and takes onwership of
  // |web_contents|.
  void InitWithWebContents(content::WebContents* web_contents);

  // Set the window as owner window.
  void SetOwnerWindow(NativeWindow* owner_window);
  void SetOwnerWindow(content::WebContents* web_contents,
                      NativeWindow* owner_window);

  // Destroy the managed InspectableWebContents object.
  void DestroyWebContents();

  // Returns the WebContents managed by this delegate.
  content::WebContents* GetWebContents() const;

  // Returns the WebContents of devtools.
  content::WebContents* GetDevToolsWebContents() const;

  brightray::InspectableWebContents* managed_web_contents() const {
    return web_contents_.get();
  }

  NativeWindow* owner_window() const { return owner_window_.get(); }

  bool is_html_fullscreen() const { return html_fullscreen_; }

 protected:
  // content::WebContentsDelegate:
  content::WebContents* OpenURLFromTab(
      content::WebContents* source,
      const content::OpenURLParams& params) override;
  void RequestToLockMouse(content::WebContents* web_contents,
                          bool user_gesture,
                          bool last_unlocked_by_target) override;
  bool CanOverscrollContent() const override;
  content::JavaScriptDialogManager* GetJavaScriptDialogManager(
      content::WebContents* source) override;
  content::ColorChooser* OpenColorChooser(
      content::WebContents* web_contents,
      SkColor color,
      const std::vector<content::ColorSuggestion>& suggestions) override;
  void RunFileChooser(content::WebContents* web_contents,
                      const content::FileChooserParams& params) override;
  void EnumerateDirectory(content::WebContents* web_contents,
                          int request_id,
                          const base::FilePath& path) override;
  void EnterFullscreenModeForTab(content::WebContents* source,
                                 const GURL& origin) override;
  void ExitFullscreenModeForTab(content::WebContents* source) override;
  bool IsFullscreenForTabOrPending(
      const content::WebContents* source) const override;

  // brightray::InspectableWebContentsDelegate:
  void DevToolsSaveToFile(const std::string& url,
                          const std::string& content,
                          bool save_as) override;
  void DevToolsAppendToFile(const std::string& url,
                            const std::string& content) override;
  void DevToolsAddFileSystem(const base::FilePath& path) override;
  void DevToolsRemoveFileSystem(
      const base::FilePath& file_system_path) override;

  // brightray::InspectableWebContentsViewDelegate:
#if defined(TOOLKIT_VIEWS)
  gfx::ImageSkia GetDevToolsWindowIcon() override;
#endif
#if defined(USE_X11)
  void GetDevToolsWindowWMClass(
      std::string* name, std::string* class_name) override;
#endif

 private:
  // Callback for when DevToolsSaveToFile has completed.
  void OnDevToolsSaveToFile(const std::string& url);

  // Callback for when DevToolsAppendToFile has completed.
  void OnDevToolsAppendToFile(const std::string& url);

  // Set fullscreen mode triggered by html api.
  void SetHtmlApiFullscreen(bool enter_fullscreen);

  // The window that this WebContents belongs to.
  base::WeakPtr<NativeWindow> owner_window_;

  // Whether window is fullscreened by HTML5 api.
  bool html_fullscreen_;

  // Whether window is fullscreened by window api.
  bool native_fullscreen_;

  scoped_ptr<WebDialogHelper> web_dialog_helper_;
  scoped_ptr<AtomJavaScriptDialogManager> dialog_manager_;

  // The stored InspectableWebContents object.
  // Notice that web_contents_ must be placed after dialog_manager_, so we can
  // make sure web_contents_ is destroyed before dialog_manager_, otherwise a
  // crash would happen.
  scoped_ptr<brightray::InspectableWebContents> web_contents_;

  // Maps url to file path, used by the file requests sent from devtools.
  typedef std::map<std::string, base::FilePath> PathsMap;
  PathsMap saved_files_;

  // Maps file system id to file path, used by the file system requests
  // sent from devtools.
  typedef std::map<std::string, base::FilePath> WorkspaceMap;
  WorkspaceMap saved_paths_;

  DISALLOW_COPY_AND_ASSIGN(CommonWebContentsDelegate);
};

}  // namespace atom

#endif  // ATOM_BROWSER_COMMON_WEB_CONTENTS_DELEGATE_H_
