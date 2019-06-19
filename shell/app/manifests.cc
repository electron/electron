// Copyright (c) 2019 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/app/manifests.h"

#include "base/no_destructor.h"
#include "electron/shell/common/api/api.mojom.h"
#include "printing/buildflags/buildflags.h"
#include "services/proxy_resolver/public/cpp/manifest.h"
#include "services/service_manager/public/cpp/manifest_builder.h"

#if BUILDFLAG(ENABLE_PRINTING)
#include "components/services/pdf_compositor/public/cpp/manifest.h"
#endif

#if BUILDFLAG(ENABLE_PRINT_PREVIEW)
#include "chrome/services/printing/public/cpp/manifest.h"
#endif

namespace {

// TODO(https://crbug.com/781334): Remove these helpers and just update the
// manifest definitions to be marked out-of-process. This is here only to avoid
// extra churn when transitioning away from content_packaged_services.
service_manager::Manifest MakeOutOfProcess(
    const service_manager::Manifest& manifest) {
  // cpplint.py emits a false positive [build/include_what_you_use]
  service_manager::Manifest copy(manifest);  // NOLINT
  copy.options.execution_mode =
      service_manager::Manifest::ExecutionMode::kOutOfProcessBuiltin;
  return copy;
}

}  // namespace

const service_manager::Manifest& GetElectronContentBrowserOverlayManifest() {
  static base::NoDestructor<service_manager::Manifest> manifest{
      service_manager::ManifestBuilder()
          .WithDisplayName("Electron (browser process)")
          .RequireCapability("device", "device:geolocation_control")
          .RequireCapability("proxy_resolver", "factory")
          .RequireCapability("chrome_printing", "converter")
          .RequireCapability("pdf_compositor", "compositor")
          .ExposeInterfaceFilterCapability_Deprecated(
              "navigation:frame", "renderer",
              service_manager::Manifest::InterfaceList<
                  electron::mojom::ElectronBrowser>())
          .Build()};
  return *manifest;
}

const std::vector<service_manager::Manifest>&
GetElectronBuiltinServiceManifests() {
  static base::NoDestructor<std::vector<service_manager::Manifest>> manifests{{
      MakeOutOfProcess(proxy_resolver::GetManifest()),
#if BUILDFLAG(ENABLE_PRINTING)
      MakeOutOfProcess(printing::GetPdfCompositorManifest()),
#endif
#if BUILDFLAG(ENABLE_PRINT_PREVIEW)
      MakeOutOfProcess(GetChromePrintingManifest()),
#endif
  }};
  return *manifests;
}
