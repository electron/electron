// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef SHELL_APP_ELECTRON_CONTENT_CLIENT_H_
#define SHELL_APP_ELECTRON_CONTENT_CLIENT_H_

#include <set>
#include <string>
#include <vector>

#include "base/files/file_path.h"
#include "content/public/common/content_client.h"

namespace electron {

class ElectronContentClient : public content::ContentClient {
 public:
  ElectronContentClient();
  ~ElectronContentClient() override;

 protected:
  // content::ContentClient:
  base::string16 GetLocalizedString(int message_id) override;
  base::StringPiece GetDataResource(int resource_id, ui::ScaleFactor) override;
  gfx::Image& GetNativeImageNamed(int resource_id) override;
  base::RefCountedMemory* GetDataResourceBytes(int resource_id) override;
  void AddAdditionalSchemes(Schemes* schemes) override;
  void AddPepperPlugins(
      std::vector<content::PepperPluginInfo>* plugins) override;
  void AddContentDecryptionModules(
      std::vector<content::CdmInfo>* cdms,
      std::vector<media::CdmHostFilePath>* cdm_host_file_paths) override;

 private:
  DISALLOW_COPY_AND_ASSIGN(ElectronContentClient);
};

}  // namespace electron

#endif  // SHELL_APP_ELECTRON_CONTENT_CLIENT_H_
