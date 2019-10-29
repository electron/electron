// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef SHELL_BROWSER_COMMON_WEB_CONTENTS_DELEGATE_H_
#define SHELL_BROWSER_COMMON_WEB_CONTENTS_DELEGATE_H_

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "base/memory/weak_ptr.h"
#include "chrome/browser/devtools/devtools_file_system_indexer.h"
#include "content/public/browser/web_contents_delegate.h"
#include "electron/buildflags/buildflags.h"
#include "shell/browser/ui/inspectable_web_contents_delegate.h"
#include "shell/browser/ui/inspectable_web_contents_impl.h"
#include "shell/browser/ui/inspectable_web_contents_view_delegate.h"

namespace base {
class SequencedTaskRunner;
}

namespace electron {

class AtomBrowserContext;
class NativeWindow;
class WebDialogHelper;

#if BUILDFLAG(ENABLE_OSR)
class OffScreenWebContentsView;
#endif

class CommonWebContentsDelegate : public content::WebContentsDelegate,
                                  public InspectableWebContentsDelegate,
                                  public InspectableWebContentsViewDelegate {
 public:
  CommonWebContentsDelegate();
  ~CommonWebContentsDelegate() override;

  // Creates a InspectableWebContents object and takes onwership of
  // |web_contents|.
  void InitWithWebContents(content::WebContents* web_contents,
                           AtomBrowserContext* browser_context,
                           bool is_guest);

  // Set the window as owner window.
  void SetOwnerWindow(NativeWindow* owner_window);
  void SetOwnerWindow(content::WebContents* web_contents,
                      NativeWindow* owner_window);

  // Returns the WebContents managed by this delegate.
  content::WebContents* GetWebContents() const;

  // Returns the WebContents of devtools.
  content::WebContents* GetDevToolsWebContents() const;

  InspectableWebContents* managed_web_contents() const {
    return web_contents_.get();
  }

  NativeWindow* owner_window() const { return owner_window_.get(); }

  bool is_html_fullscreen() const { return html_fullscreen_; }

  void set_fullscreen_frame(content::RenderFrameHost* rfh) {
    fullscreen_frame_ = rfh;
  }

 protected:
#if BUILDFLAG(ENABLE_OSR)
  virtual OffScreenWebContentsView* GetOffScreenWebContentsView() const;
#endif

  // content::WebContentsDelegate:
  content::WebContents* OpenURLFromTab(
      content::WebContents* source,
      const content::OpenURLParams& params) override;
  bool CanOverscrollContent() override;
  content::ColorChooser* OpenColorChooser(
      content::WebContents* web_contents,
      SkColor color,
      const std::vector<blink::mojom::ColorSuggestionPtr>& suggestions)
      override;
  void RunFileChooser(content::RenderFrameHost* render_frame_host,
                      std::unique_ptr<content::FileSelectListener> listener,
                      const blink::mojom::FileChooserParams& params) override;
  void EnumerateDirectory(content::WebContents* web_contents,
                          std::unique_ptr<content::FileSelectListener> listener,
                          const base::FilePath& path) override;
  void EnterFullscreenModeForTab(
      content::WebContents* source,
      const GURL& origin,
      const blink::mojom::FullscreenOptions& options) override;
  void ExitFullscreenModeForTab(content::WebContents* source) override;
  bool IsFullscreenForTabOrPending(const content::WebContents* source) override;
  blink::SecurityStyle GetSecurityStyle(
      content::WebContents* web_contents,
      content::SecurityStyleExplanations* explanations) override;
  bool TakeFocus(content::WebContents* source, bool reverse) override;
  bool HandleKeyboardEvent(
      content::WebContents* source,
      const content::NativeWebKeyboardEvent& event) override;
  content::PictureInPictureResult EnterPictureInPicture(
      content::WebContents* web_contents,
      const viz::SurfaceId&,
      const gfx::Size& natural_size) override;
  void ExitPictureInPicture() override;

  // InspectableWebContentsDelegate:
  void DevToolsSaveToFile(const std::string& url,
                          const std::string& content,
                          bool save_as) override;
  void DevToolsAppendToFile(const std::string& url,
                            const std::string& content) override;
  void DevToolsRequestFileSystems() override;
  void DevToolsAddFileSystem(const std::string& type,
                             const base::FilePath& file_system_path) override;
  void DevToolsRemoveFileSystem(
      const base::FilePath& file_system_path) override;
  void DevToolsIndexPath(int request_id,
                         const std::string& file_system_path,
                         const std::string& excluded_folders_message) override;
  void DevToolsStopIndexing(int request_id) override;
  void DevToolsSearchInPath(int request_id,
                            const std::string& file_system_path,
                            const std::string& query) override;

  // InspectableWebContentsViewDelegate:
#if defined(TOOLKIT_VIEWS) && !defined(OS_MACOSX)
  gfx::ImageSkia GetDevToolsWindowIcon() override;
#endif
#if defined(USE_X11)
  void GetDevToolsWindowWMClass(std::string* name,
                                std::string* class_name) override;
#endif

  // Destroy the managed InspectableWebContents object.
  void ResetManagedWebContents(bool async);

 private:
  // DevTools index event callbacks.
  void OnDevToolsIndexingWorkCalculated(int request_id,
                                        const std::string& file_system_path,
                                        int total_work);
  void OnDevToolsIndexingWorked(int request_id,
                                const std::string& file_system_path,
                                int worked);
  void OnDevToolsIndexingDone(int request_id,
                              const std::string& file_system_path);
  void OnDevToolsSearchCompleted(int request_id,
                                 const std::string& file_system_path,
                                 const std::vector<std::string>& file_paths);

  // Set fullscreen mode triggered by html api.
  void SetHtmlApiFullscreen(bool enter_fullscreen);

  // The window that this WebContents belongs to.
  base::WeakPtr<NativeWindow> owner_window_;

  bool offscreen_ = false;

  // Whether window is fullscreened by HTML5 api.
  bool html_fullscreen_ = false;

  // Whether window is fullscreened by window api.
  bool native_fullscreen_ = false;

  // UI related helper classes.
  std::unique_ptr<WebDialogHelper> web_dialog_helper_;

  scoped_refptr<DevToolsFileSystemIndexer> devtools_file_system_indexer_;

  // Make sure BrowserContext is alwasys destroyed after WebContents.
  scoped_refptr<AtomBrowserContext> browser_context_;

  // The stored InspectableWebContents object.
  // Notice that web_contents_ must be placed after dialog_manager_, so we can
  // make sure web_contents_ is destroyed before dialog_manager_, otherwise a
  // crash would happen.
  std::unique_ptr<InspectableWebContents> web_contents_;

  // Maps url to file path, used by the file requests sent from devtools.
  typedef std::map<std::string, base::FilePath> PathsMap;
  PathsMap saved_files_;

  // Map id to index job, used for file system indexing requests from devtools.
  typedef std::
      map<int, scoped_refptr<DevToolsFileSystemIndexer::FileSystemIndexingJob>>
          DevToolsIndexingJobsMap;
  DevToolsIndexingJobsMap devtools_indexing_jobs_;

  scoped_refptr<base::SequencedTaskRunner> file_task_runner_;

  // Stores the frame thats currently in fullscreen, nullptr if there is none.
  content::RenderFrameHost* fullscreen_frame_ = nullptr;

  base::WeakPtrFactory<CommonWebContentsDelegate> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(CommonWebContentsDelegate);
};

}  // namespace electron

#endif  // SHELL_BROWSER_COMMON_WEB_CONTENTS_DELEGATE_H_
