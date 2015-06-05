// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_COMMON_WEB_CONTENTS_DELEGATE_H_
#define ATOM_BROWSER_COMMON_WEB_CONTENTS_DELEGATE_H_

#include <map>

#include "brightray/browser/default_web_contents_delegate.h"
#include "brightray/browser/inspectable_web_contents_impl.h"
#include "brightray/browser/inspectable_web_contents_delegate.h"

namespace atom {

class AtomJavaScriptDialogManager;
class NativeWindow;

class CommonWebContentsDelegate
    : public brightray::DefaultWebContentsDelegate,
      public brightray::InspectableWebContentsDelegate {
 public:
  explicit CommonWebContentsDelegate(bool is_guest);
  virtual ~CommonWebContentsDelegate();

  // Create a InspectableWebContents object and takes onwership of
  // |web_contents|.
  void InitWithWebContents(content::WebContents* web_contents,
                           NativeWindow* owner_window);

  // Destroy the managed InspectableWebContents object.
  void DestroyWebContents();

  // Returns the WebContents managed by this delegate.
  content::WebContents* GetWebContents() const;

  // Returns the WebContents of devtools.
  content::WebContents* GetDevToolsWebContents() const;

  brightray::InspectableWebContents* inspectable_web_contents() const {
    return web_contents_.get();
  }

  bool is_guest() const { return is_guest_; };

 protected:
  // content::WebContentsDelegate:
  content::JavaScriptDialogManager* GetJavaScriptDialogManager(
      content::WebContents* source) override;

  // brightray::InspectableWebContentsDelegate:
  void DevToolsSaveToFile(const std::string& url,
                          const std::string& content,
                          bool save_as) override;
  void DevToolsAppendToFile(const std::string& url,
                            const std::string& content) override;
  void DevToolsAddFileSystem() override;
  void DevToolsRemoveFileSystem(const std::string& file_system_path) override;

 private:
  // Whether this is guest WebContents or NativeWindow.
  const bool is_guest_;

  // The window that this WebContents belongs to.
  NativeWindow* owner_window_;

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
