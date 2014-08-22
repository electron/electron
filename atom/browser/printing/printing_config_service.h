// Copyright (c) 2014 GitHub, Inc. All rights reserved.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_PRINTING_PRINTING_CONFIG_SERVICE_H_
#define ATOM_BROWSER_PRINTING_PRINTING_CONFIG_SERVICE_H_

#include "base/memory/weak_ptr.h"

template <typename T> struct DefaultSingletonTraits;
struct PrintHostMsg_ScriptedPrint_Params;
struct PrintMsg_PrintPages_Params;

namespace content {
class WebContents;
}

namespace printing {
class PrinterQuery;
}

namespace atom {

// This interface manages the config of printing.
class PrintingConfigService {
 public:
  static PrintingConfigService* GetInstance();

  typedef base::Callback<void(const PrintMsg_PrintPages_Params&)>
      PrintSettingsCallback;

  // Gets printing settings for query on UI thread, and then call the |callback|
  // on the IO thread with the result.
  void GetPrintSettings(content::WebContents* wc,
                        scoped_refptr<printing::PrinterQuery> printer_query,
                        bool ask_user_for_settings,
                        const PrintHostMsg_ScriptedPrint_Params& params,
                        PrintSettingsCallback callback);

 private:
  PrintingConfigService();
  virtual ~PrintingConfigService();

  // Called by content::PrinterQuery::GetSettings in GetPrintSettings.
  void OnGetSettings(scoped_refptr<printing::PrinterQuery> printer_query,
                     PrintSettingsCallback callback);
  void OnGetSettingsFailed(scoped_refptr<printing::PrinterQuery> printer_query,
                           PrintSettingsCallback callback);

 private:
  friend struct DefaultSingletonTraits<PrintingConfigService>;

  base::WeakPtrFactory<PrintingConfigService> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(PrintingConfigService);
};

}  // namespace atom

#endif  // ATOM_BROWSER_PRINTING_PRINTING_CONFIG_SERVICE_H_
