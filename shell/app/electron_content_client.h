// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_APP_ELECTRON_CONTENT_CLIENT_H_
#define ELECTRON_SHELL_APP_ELECTRON_CONTENT_CLIENT_H_

#include <vector>

#include "base/files/file_path.h"
#include "content/public/common/content_client.h"

namespace electron {

class ElectronContentClient : public content::ContentClient {
 public:
  ElectronContentClient();
  ~ElectronContentClient() override;

  // disable copy
  ElectronContentClient(const ElectronContentClient&) = delete;
  ElectronContentClient& operator=(const ElectronContentClient&) = delete;

 protected:
  // content::ContentClient:
  std::u16string GetLocalizedString(int message_id) override;
  base::StringPiece GetDataResource(int resource_id,
                                    ui::ResourceScaleFactor) override;
  gfx::Image& GetNativeImageNamed(int resource_id) override;
  base::RefCountedMemory* GetDataResourceBytes(int resource_id) override;
  void AddAdditionalSchemes(Schemes* schemes) override;
  void AddPepperPlugins(
      std::vector<content::PepperPluginInfo>* plugins) override;
  void AddContentDecryptionModules(
      std::vector<content::CdmInfo>* cdms,
      std::vector<media::CdmHostFilePath>* cdm_host_file_paths) override;
};

}  // namespace electron

#endif  // ELECTRON_SHELL_APP_ELECTRON_CONTENT_CLIENT_H_
