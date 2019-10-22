// Copyright (c) 2019 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/app/manifests.h"

#include "base/no_destructor.h"
#include "electron/shell/common/api/api.mojom.h"
#include "printing/buildflags/buildflags.h"
#include "services/service_manager/public/cpp/manifest_builder.h"

#if defined(OS_MACOSX) && BUILDFLAG(ENABLE_BUILTIN_SPELLCHECKER)
#include "components/spellcheck/common/spellcheck_panel.mojom.h"  // nogncheck
#endif

const service_manager::Manifest& GetElectronContentBrowserOverlayManifest() {
  static base::NoDestructor<service_manager::Manifest> manifest{
      service_manager::ManifestBuilder()
          .WithDisplayName("Electron (browser process)")
          .RequireCapability("device", "device:geolocation_control")
          .RequireCapability("chrome_printing", "converter")
          .ExposeInterfaceFilterCapability_Deprecated(
              "navigation:frame", "renderer",
              service_manager::Manifest::InterfaceList<
                  electron::mojom::ElectronBrowser>())
          .Build()};
  return *manifest;
}

#if BUILDFLAG(ENABLE_BUILTIN_SPELLCHECKER)
const service_manager::Manifest& GetElectronContentRendererOverlayManifest() {
  static base::NoDestructor<service_manager::Manifest> manifest {
    service_manager::ManifestBuilder()
#if defined(OS_MACOSX)
        .ExposeInterfaceFilterCapability_Deprecated(
            "navigation:frame", "browser",
            service_manager::Manifest::InterfaceList<
                spellcheck::mojom::SpellCheckPanel>())
#endif
        .Build()
  };
  return *manifest;
}
#endif
