// Copyright (c) 2019 Slack Technologies, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_EXTENSIONS_API_TABS_TABS_API_H_
#define ELECTRON_SHELL_BROWSER_EXTENSIONS_API_TABS_TABS_API_H_

#include <string>

#include "base/memory/raw_ptr.h"
#include "extensions/browser/api/execute_code_function.h"
#include "extensions/browser/extension_function.h"
#include "extensions/common/extension_resource.h"

class GURL;

namespace extensions {

// Implement API call tabs.executeScript and tabs.insertCSS.
class ExecuteCodeInTabFunction : public ExecuteCodeFunction {
 public:
  ExecuteCodeInTabFunction();

 protected:
  ~ExecuteCodeInTabFunction() override;

  // Initializes |execute_tab_id_| and |details_|.
  InitResult Init() override;
  bool CanExecuteScriptOnPage(std::string* error) override;
  ScriptExecutor* GetScriptExecutor(std::string* error) override;
  bool IsWebView() const override;
  int GetRootFrameId() const override;
  const GURL& GetWebViewSrc() const override;

 private:
  // Id of tab which executes code.
  int execute_tab_id_;
};

class TabsExecuteScriptFunction : public ExecuteCodeInTabFunction {
 protected:
  bool ShouldInsertCSS() const override;
  bool ShouldRemoveCSS() const override;

 private:
  ~TabsExecuteScriptFunction() override = default;

  DECLARE_EXTENSION_FUNCTION("tabs.executeScript", TABS_EXECUTESCRIPT)
};

class TabsReloadFunction : public ExtensionFunction {
  ~TabsReloadFunction() override = default;

  ResponseAction Run() override;

  DECLARE_EXTENSION_FUNCTION("tabs.reload", TABS_RELOAD)
};

class TabsQueryFunction : public ExtensionFunction {
  ~TabsQueryFunction() override = default;

  ResponseAction Run() override;

  DECLARE_EXTENSION_FUNCTION("tabs.query", TABS_QUERY)
};

class TabsGetFunction : public ExtensionFunction {
  ~TabsGetFunction() override = default;

  ResponseAction Run() override;

  DECLARE_EXTENSION_FUNCTION("tabs.get", TABS_GET)
};

class TabsSetZoomFunction : public ExtensionFunction {
 private:
  ~TabsSetZoomFunction() override = default;

  ResponseAction Run() override;

  DECLARE_EXTENSION_FUNCTION("tabs.setZoom", TABS_SETZOOM)
};

class TabsGetZoomFunction : public ExtensionFunction {
 private:
  ~TabsGetZoomFunction() override = default;

  ResponseAction Run() override;

  DECLARE_EXTENSION_FUNCTION("tabs.getZoom", TABS_GETZOOM)
};

class TabsSetZoomSettingsFunction : public ExtensionFunction {
 private:
  ~TabsSetZoomSettingsFunction() override = default;

  ResponseAction Run() override;

  DECLARE_EXTENSION_FUNCTION("tabs.setZoomSettings", TABS_SETZOOMSETTINGS)
};

class TabsGetZoomSettingsFunction : public ExtensionFunction {
 private:
  ~TabsGetZoomSettingsFunction() override = default;

  ResponseAction Run() override;

  DECLARE_EXTENSION_FUNCTION("tabs.getZoomSettings", TABS_GETZOOMSETTINGS)
};

class TabsUpdateFunction : public ExtensionFunction {
 public:
  TabsUpdateFunction();

 protected:
  ~TabsUpdateFunction() override = default;
  bool UpdateURL(const std::string& url, int tab_id, std::string* error);
  ResponseValue GetResult();

  raw_ptr<content::WebContents> web_contents_;

 private:
  ResponseAction Run() override;

  DECLARE_EXTENSION_FUNCTION("tabs.update", TABS_UPDATE)
};
}  // namespace extensions

#endif  // ELECTRON_SHELL_BROWSER_EXTENSIONS_API_TABS_TABS_API_H_
