// Copyright (c) 2025 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/api/electron_api_msix_updater.h"

#include <sstream>
#include <string_view>
#include "base/environment.h"
#include "base/functional/bind.h"
#include "base/logging.h"
#include "base/strings/utf_string_conversions.h"
#include "base/task/single_thread_task_runner.h"
#include "base/task/task_traits.h"
#include "base/task/thread_pool.h"
#include "content/public/browser/browser_task_traits.h"
#include "content/public/browser/browser_thread.h"
#include "shell/browser/browser.h"
#include "shell/browser/javascript_environment.h"
#include "shell/browser/native_window.h"
#include "shell/browser/window_list.h"
#include "shell/common/gin_helper/dictionary.h"
#include "shell/common/gin_helper/error_thrower.h"
#include "shell/common/gin_helper/promise.h"
#include "shell/common/node_includes.h"

#if BUILDFLAG(IS_WIN)
#include <appmodel.h>
#include <roapi.h>
#include <windows.applicationmodel.h>
#include <windows.foundation.collections.h>
#include <windows.foundation.h>
#include <windows.foundation.metadata.h>
#include <windows.h>
#include <windows.management.deployment.h>
#include <winrt/base.h>
#include <winrt/windows.applicationmodel.core.h>
#include <winrt/windows.applicationmodel.h>
#include <winrt/windows.foundation.collections.h>
#include <winrt/windows.foundation.h>
#include <winrt/windows.foundation.metadata.h>
#include <winrt/windows.management.deployment.h>

#include "base/win/scoped_com_initializer.h"
#endif

namespace electron {

const bool debug_msix_updater =
    base::Environment::Create()->HasVar("ELECTRON_DEBUG_MSIX_UPDATER");

}  // namespace electron

namespace {

#if BUILDFLAG(IS_WIN)
// Helper function for debug logging
void DebugLog(std::string_view log_msg) {
  if (electron::debug_msix_updater)
    LOG(INFO) << std::string(log_msg);
}

// Check if the process has a package identity
bool HasPackageIdentity() {
  UINT32 length = 0;
  LONG rc = GetCurrentPackageFullName(&length, NULL);
  return rc != APPMODEL_ERROR_NO_PACKAGE;
}

// POD struct to hold MSIX update options
struct UpdateMsixOptions {
  bool defer_registration = false;
  bool developer_mode = false;
  bool force_shutdown = false;
  bool force_target_shutdown = false;
  bool force_update_from_any_version = false;
};

// POD struct to hold package registration options
struct RegisterPackageOptions {
  bool force_shutdown = false;
  bool force_target_shutdown = false;
  bool force_update_from_any_version = false;
};

// Performs MSIX update on IO thread
void DoUpdateMsix(const std::string& package_uri,
                  UpdateMsixOptions opts,
                  scoped_refptr<base::SingleThreadTaskRunner> reply_runner,
                  gin_helper::Promise<void> promise) {
  DebugLog("DoUpdateMsix: Starting");

  using winrt::Windows::Foundation::AsyncStatus;
  using winrt::Windows::Foundation::Uri;
  using winrt::Windows::Management::Deployment::AddPackageOptions;
  using winrt::Windows::Management::Deployment::DeploymentResult;
  using winrt::Windows::Management::Deployment::PackageManager;

  std::string error;
  std::wstring packageUriString =
      std::wstring(package_uri.begin(), package_uri.end());
  Uri uri{packageUriString};
  PackageManager packageManager;
  AddPackageOptions packageOptions;

  // Use the pre-parsed options
  packageOptions.DeferRegistrationWhenPackagesAreInUse(opts.defer_registration);
  packageOptions.DeveloperMode(opts.developer_mode);
  packageOptions.ForceAppShutdown(opts.force_shutdown);
  packageOptions.ForceTargetAppShutdown(opts.force_target_shutdown);
  packageOptions.ForceUpdateFromAnyVersion(opts.force_update_from_any_version);

  {
    std::ostringstream oss;
    oss << "Calling AddPackageByUriAsync... URI: " << package_uri;
    DebugLog(oss.str());
  }
  {
    std::ostringstream oss;
    oss << "Update options - deferRegistration: " << opts.defer_registration
        << ", developerMode: " << opts.developer_mode
        << ", forceShutdown: " << opts.force_shutdown
        << ", forceTargetShutdown: " << opts.force_target_shutdown
        << ", forceUpdateFromAnyVersion: "
        << opts.force_update_from_any_version;
    DebugLog(oss.str());
  }

  auto deploymentOperation =
      packageManager.AddPackageByUriAsync(uri, packageOptions);

  if (!deploymentOperation) {
    DebugLog("Deployment operation is null");
    error =
        "Deployment is NULL. See "
        "http://go.microsoft.com/fwlink/?LinkId=235160 for diagnosing.";
  } else {
    if (!opts.force_shutdown && !opts.force_target_shutdown) {
      DebugLog("Waiting for deployment...");
      deploymentOperation.get();
      DebugLog("Deployment finished.");

      if (deploymentOperation.Status() == AsyncStatus::Error) {
        auto deploymentResult{deploymentOperation.GetResults()};
        std::string errorText = winrt::to_string(deploymentResult.ErrorText());
        std::string errorCode =
            std::to_string(static_cast<int>(deploymentOperation.ErrorCode()));
        error = errorText + " (" + errorCode + ")";
        {
          std::ostringstream oss;
          oss << "Deployment failed: " << error;
          DebugLog(oss.str());
        }
      } else if (deploymentOperation.Status() == AsyncStatus::Canceled) {
        DebugLog("Deployment canceled");
        error = "Deployment canceled";
      } else if (deploymentOperation.Status() == AsyncStatus::Completed) {
        DebugLog("MSIX Deployment completed.");
      } else {
        error = "Deployment status unknown";
        DebugLog("Deployment status unknown");
      }
    } else {
      // At this point, we can not await the deployment because we require a
      // shutdown of the app to continue, so we do a fire and forget. When the
      // deployment process tries ot shutdown the app, the process waits for us
      // to finish here. But to finish we need to shutdow.  That leads to a 30s
      // dealock, till we forcefully get shutdown by the OS.
      DebugLog(
          "Deployment initiated. Force shutdown or target shutdown requested. "
          "Good bye!");
    }
  }

  // Post result back
  reply_runner->PostTask(
      FROM_HERE, base::BindOnce(
                     [](gin_helper::Promise<void> promise, std::string error) {
                       if (error.empty()) {
                         promise.Resolve();
                       } else {
                         promise.RejectWithErrorMessage(error);
                       }
                     },
                     std::move(promise), error));
}

// Performs package registration on IO thread
void DoRegisterPackage(const std::string& family_name,
                       RegisterPackageOptions opts,
                       scoped_refptr<base::SingleThreadTaskRunner> reply_runner,
                       gin_helper::Promise<void> promise) {
  DebugLog("DoRegisterPackage: Starting");

  using winrt::Windows::Foundation::AsyncStatus;
  using winrt::Windows::Foundation::Collections::IIterable;
  using winrt::Windows::Management::Deployment::DeploymentOptions;
  using winrt::Windows::Management::Deployment::PackageManager;

  std::string error;
  auto familyNameH = winrt::to_hstring(family_name);
  PackageManager packageManager;
  DeploymentOptions deploymentOptions = DeploymentOptions::None;

  // Use the pre-parsed options (no V8 access needed)
  if (opts.force_shutdown) {
    deploymentOptions |= DeploymentOptions::ForceApplicationShutdown;
  }
  if (opts.force_target_shutdown) {
    deploymentOptions |= DeploymentOptions::ForceTargetApplicationShutdown;
  }
  if (opts.force_update_from_any_version) {
    deploymentOptions |= DeploymentOptions::ForceUpdateFromAnyVersion;
  }

  // Create empty collections for dependency and optional packages
  IIterable<winrt::hstring> emptyDependencies{nullptr};
  IIterable<winrt::hstring> emptyOptional{nullptr};

  {
    std::ostringstream oss;
    oss << "Calling RegisterPackageByFamilyNameAsync... FamilyName: "
        << family_name;
    DebugLog(oss.str());
  }
  {
    std::ostringstream oss;
    oss << "Registration options - forceShutdown: " << opts.force_shutdown
        << ", forceTargetShutdown: " << opts.force_target_shutdown
        << ", forceUpdateFromAnyVersion: "
        << opts.force_update_from_any_version;
    DebugLog(oss.str());
  }

  auto deploymentOperation = packageManager.RegisterPackageByFamilyNameAsync(
      familyNameH, emptyDependencies, deploymentOptions, nullptr,
      emptyOptional);

  if (!deploymentOperation) {
    error =
        "Deployment is NULL. See "
        "http://go.microsoft.com/fwlink/?LinkId=235160 for diagnosing.";
  } else {
    if (!opts.force_shutdown && !opts.force_target_shutdown) {
      DebugLog("Waiting for registration...");
      deploymentOperation.get();
      DebugLog("Registration finished.");

      if (deploymentOperation.Status() == AsyncStatus::Error) {
        auto deploymentResult{deploymentOperation.GetResults()};
        std::string errorText = winrt::to_string(deploymentResult.ErrorText());
        std::string errorCode =
            std::to_string(static_cast<int>(deploymentOperation.ErrorCode()));
        error = errorText + " (" + errorCode + ")";
        {
          std::ostringstream oss;
          oss << "Registration failed: " << error;
          DebugLog(oss.str());
        }
      } else if (deploymentOperation.Status() == AsyncStatus::Canceled) {
        DebugLog("Registration canceled");
        error = "Registration canceled";
      } else if (deploymentOperation.Status() == AsyncStatus::Completed) {
        DebugLog("MSIX Registration completed.");
      } else {
        error = "Registration status unknown";
        DebugLog("Registration status unknown");
      }
    } else {
      // At this point, we can not await the registration because we require a
      // shutdown of the app to continue, so we do a fire and forget. When the
      // registration process tries ot shutdown the app, the process waits for
      // us to finish here. But to finish we need to shutdown. That leads to a
      // 30s dealock, till we forcefully get shutdown by the OS.
      DebugLog(
          "Registration initiated. Force shutdown or target shutdown "
          "requested. Good bye!");
    }
  }

  // Post result back to UI thread
  reply_runner->PostTask(
      FROM_HERE, base::BindOnce(
                     [](gin_helper::Promise<void> promise, std::string error) {
                       if (error.empty()) {
                         promise.Resolve();
                       } else {
                         promise.RejectWithErrorMessage(error);
                       }
                     },
                     std::move(promise), error));
}
#endif

// Update MSIX package
v8::Local<v8::Promise> UpdateMsix(const std::string& package_uri,
                                  gin_helper::Dictionary options) {
  v8::Isolate* isolate = electron::JavascriptEnvironment::GetIsolate();
  gin_helper::Promise<void> promise(isolate);
  v8::Local<v8::Promise> handle = promise.GetHandle();

#if BUILDFLAG(IS_WIN)
  if (!HasPackageIdentity()) {
    DebugLog("UpdateMsix: The process has no package identity");
    promise.RejectWithErrorMessage("The process has no package identity.");
    return handle;
  }

  // Parse options on UI thread (where V8 is available)
  UpdateMsixOptions opts;
  options.Get("deferRegistration", &opts.defer_registration);
  options.Get("developerMode", &opts.developer_mode);
  options.Get("forceShutdown", &opts.force_shutdown);
  options.Get("forceTargetShutdown", &opts.force_target_shutdown);
  options.Get("forceUpdateFromAnyVersion", &opts.force_update_from_any_version);

  {
    std::ostringstream oss;
    oss << "UpdateMsix called with URI: " << package_uri;
    DebugLog(oss.str());
  }

  // Post to IO thread
  content::GetIOThreadTaskRunner({})->PostTask(
      FROM_HERE,
      base::BindOnce(&DoUpdateMsix, package_uri, opts,
                     base::SingleThreadTaskRunner::GetCurrentDefault(),
                     std::move(promise)));
#else
  promise.RejectWithErrorMessage(
      "MSIX updates are only supported on Windows with identity.");
#endif

  return handle;
}

// Register MSIX package
v8::Local<v8::Promise> RegisterPackage(const std::string& family_name,
                                       gin_helper::Dictionary options) {
  v8::Isolate* isolate = electron::JavascriptEnvironment::GetIsolate();
  gin_helper::Promise<void> promise(isolate);
  v8::Local<v8::Promise> handle = promise.GetHandle();

#if BUILDFLAG(IS_WIN)
  if (!HasPackageIdentity()) {
    DebugLog("RegisterPackage: The process has no package identity");
    promise.RejectWithErrorMessage("The process has no package identity.");
    return handle;
  }

  // Parse options on UI thread (where V8 is available)
  RegisterPackageOptions opts;
  options.Get("forceShutdown", &opts.force_shutdown);
  options.Get("forceTargetShutdown", &opts.force_target_shutdown);
  options.Get("forceUpdateFromAnyVersion", &opts.force_update_from_any_version);

  {
    std::ostringstream oss;
    oss << "RegisterPackage called with family name: " << family_name;
    DebugLog(oss.str());
  }

  // Post to IO thread with POD options (no V8 objects)
  content::GetIOThreadTaskRunner({})->PostTask(
      FROM_HERE,
      base::BindOnce(&DoRegisterPackage, family_name, opts,
                     base::SingleThreadTaskRunner::GetCurrentDefault(),
                     std::move(promise)));
#else
  promise.RejectWithErrorMessage(
      "MSIX package registration is only supported on Windows.");
#endif

  return handle;
}

// Register application restart
// Only registers for update restarts (not crashes, hangs, or reboots)
bool RegisterRestartOnUpdate(const std::string& command_line) {
#if BUILDFLAG(IS_WIN)
  if (!HasPackageIdentity()) {
    DebugLog("Cannot restart: no package identity");
    return false;
  }

  const wchar_t* commandLine = nullptr;
  // Flags: RESTART_NO_CRASH | RESTART_NO_HANG | RESTART_NO_REBOOT
  // This means: only restart on updates (RESTART_NO_PATCH is NOT set)
  const DWORD dwFlags = 1 | 2 | 8;  // 11

  if (!command_line.empty()) {
    std::wstring commandLineW =
        std::wstring(command_line.begin(), command_line.end());
    commandLine = commandLineW.c_str();
  }

  HRESULT hr = RegisterApplicationRestart(commandLine, dwFlags);
  if (FAILED(hr)) {
    {
      std::ostringstream oss;
      oss << "RegisterApplicationRestart failed with error code: " << hr;
      DebugLog(oss.str());
    }
    return false;
  }
  {
    std::ostringstream oss;
    oss << "RegisterApplicationRestart succeeded"
        << (command_line.empty() ? "" : " with command line");
    DebugLog(oss.str());
  }
  return true;
#else
  return false;
#endif
}

// Get package information
v8::Local<v8::Value> GetPackageInfo() {
  v8::Isolate* isolate = electron::JavascriptEnvironment::GetIsolate();

#if BUILDFLAG(IS_WIN)
  // Check if running in a package
  if (!HasPackageIdentity()) {
    DebugLog("GetPackageInfo: The process has no package identity");
    gin_helper::ErrorThrower thrower(isolate);
    thrower.ThrowTypeError("The process has no package identity.");
    return v8::Null(isolate);
  }

  DebugLog("GetPackageInfo: Retrieving package information");

  gin_helper::Dictionary result(isolate, v8::Object::New(isolate));

  // Check API contract version (Windows 10 version 1703 or later)
  if (winrt::Windows::Foundation::Metadata::ApiInformation::
          IsApiContractPresent(L"Windows.Foundation.UniversalApiContract", 7)) {
    using winrt::Windows::ApplicationModel::Package;
    using winrt::Windows::ApplicationModel::PackageSignatureKind;
    Package package = Package::Current();

    // Get package ID and family name
    std::string packageId = winrt::to_string(package.Id().FullName());
    std::string familyName = winrt::to_string(package.Id().FamilyName());

    result.Set("id", packageId);
    result.Set("familyName", familyName);
    result.Set("developmentMode", package.IsDevelopmentMode());

    // Get package version
    auto packageVersion = package.Id().Version();
    std::string version = std::to_string(packageVersion.Major) + "." +
                          std::to_string(packageVersion.Minor) + "." +
                          std::to_string(packageVersion.Build) + "." +
                          std::to_string(packageVersion.Revision);
    result.Set("version", version);

    // Convert signature kind to string
    std::string signatureKind;
    switch (package.SignatureKind()) {
      case PackageSignatureKind::Developer:
        signatureKind = "developer";
        break;
      case PackageSignatureKind::Enterprise:
        signatureKind = "enterprise";
        break;
      case PackageSignatureKind::None:
        signatureKind = "none";
        break;
      case PackageSignatureKind::Store:
        signatureKind = "store";
        break;
      case PackageSignatureKind::System:
        signatureKind = "system";
        break;
      default:
        signatureKind = "none";
        break;
    }
    result.Set("signatureKind", signatureKind);

    // Get app installer info if available
    auto appInstallerInfo = package.GetAppInstallerInfo();
    if (appInstallerInfo != nullptr) {
      std::string uriStr = winrt::to_string(appInstallerInfo.Uri().ToString());
      result.Set("appInstallerUri", uriStr);
    }
  } else {
    // Windows version doesn't meet minimum API requirements
    result.Set("familyName", "");
    result.Set("id", "");
    result.Set("developmentMode", false);
    result.Set("signatureKind", "none");
    result.Set("version", "");
  }

  return result.GetHandle();
#else
  // Non-Windows platforms
  gin_helper::Dictionary result(isolate, v8::Object::New(isolate));
  result.Set("familyName", "");
  result.Set("id", "");
  result.Set("developmentMode", false);
  result.Set("signatureKind", "none");
  result.Set("version", "");
  return result.GetHandle();
#endif
}

void Initialize(v8::Local<v8::Object> exports,
                v8::Local<v8::Value> unused,
                v8::Local<v8::Context> context,
                void* priv) {
  v8::Isolate* const isolate = electron::JavascriptEnvironment::GetIsolate();
  gin_helper::Dictionary dict(isolate, exports);

  dict.SetMethod("updateMsix", base::BindRepeating(&UpdateMsix));
  dict.SetMethod("registerPackage", base::BindRepeating(&RegisterPackage));
  dict.SetMethod("registerRestartOnUpdate",
                 base::BindRepeating(&RegisterRestartOnUpdate));
  dict.SetMethod("getPackageInfo",
                 base::BindRepeating([]() { return GetPackageInfo(); }));
}

}  // namespace

NODE_LINKED_BINDING_CONTEXT_AWARE(electron_browser_msix_updater, Initialize)
