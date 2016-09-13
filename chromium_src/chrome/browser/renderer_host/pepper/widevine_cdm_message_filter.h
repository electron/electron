// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_RENDERER_HOST_PEPPER_WIDEVINE_CDM_MESSAGE_FILTER_H_
#define CHROME_BROWSER_RENDERER_HOST_PEPPER_WIDEVINE_CDM_MESSAGE_FILTER_H_

#include "chrome/common/widevine_cdm_messages.h"
#include "content/public/browser/browser_message_filter.h"

namespace content {
class BrowserContext;
}

class WidevineCdmMessageFilter : public content::BrowserMessageFilter {
 public:
  explicit WidevineCdmMessageFilter(int render_process_id,
      content::BrowserContext* browser_context);
  bool OnMessageReceived(const IPC::Message& message) override;
  void OnDestruct() const override;

 private:
  friend class content::BrowserThread;
  friend class base::DeleteHelper<WidevineCdmMessageFilter>;

  virtual ~WidevineCdmMessageFilter();

  #if defined(ENABLE_PEPPER_CDMS)
  // Returns whether any internal plugin supporting |mime_type| is registered
  // and enabled. Does not determine whether the plugin can actually be
  // instantiated (e.g. whether it has all its dependencies).
  // When the returned *|is_available| is true, |additional_param_names| and
  // |additional_param_values| contain the name-value pairs, if any, specified
  // for the *first* non-disabled plugin found that is registered for
  // |mime_type|.
  void OnIsInternalPluginAvailableForMimeType(
      const std::string& mime_type,
      bool* is_available,
      std::vector<base::string16>* additional_param_names,
      std::vector<base::string16>* additional_param_values);
#endif

  int render_process_id_;
  content::BrowserContext* browser_context_;

  DISALLOW_COPY_AND_ASSIGN(WidevineCdmMessageFilter);
};

#endif  // CHROME_BROWSER_RENDERER_HOST_PEPPER_WIDEVINE_CDM_MESSAGE_FILTER_H_
