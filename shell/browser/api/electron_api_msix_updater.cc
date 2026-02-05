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
#include "content/public/browser/browser_task_traits.h"
#include "content/public/browser/browser_thread.h"
#include "shell/browser/javascript_environment.h"
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
#include <wrl.h>

#include "base/win/core_winrt_util.h"
#include "base/win/scoped_hstring.h"
#endif

namespace electron {

const bool debug_msix_updater =
    base::Environment::Create()->HasVar("ELECTRON_DEBUG_MSIX_UPDATER");

}  // namespace electron

namespace {

#if BUILDFLAG(IS_WIN)

// Type aliases for cleaner code
using ABI::Windows::ApplicationModel::IAppInstallerInfo;
using ABI::Windows::ApplicationModel::IPackage;
using ABI::Windows::ApplicationModel::IPackage2;
using ABI::Windows::ApplicationModel::IPackage4;
using ABI::Windows::ApplicationModel::IPackage6;
using ABI::Windows::ApplicationModel::IPackageId;
using ABI::Windows::ApplicationModel::IPackageStatics;
using ABI::Windows::ApplicationModel::PackageSignatureKind;
using ABI::Windows::ApplicationModel::PackageSignatureKind_Developer;
using ABI::Windows::ApplicationModel::PackageSignatureKind_Enterprise;
using ABI::Windows::ApplicationModel::PackageSignatureKind_None;
using ABI::Windows::ApplicationModel::PackageSignatureKind_Store;
using ABI::Windows::ApplicationModel::PackageSignatureKind_System;
using ABI::Windows::Foundation::AsyncStatus;
using ABI::Windows::Foundation::IAsyncInfo;
using ABI::Windows::Foundation::IUriRuntimeClass;
using ABI::Windows::Foundation::IUriRuntimeClassFactory;
using ABI::Windows::Foundation::Metadata::IApiInformationStatics;
using ABI::Windows::Management::Deployment::DeploymentOptions;
using ABI::Windows::Management::Deployment::
    DeploymentOptions_ForceApplicationShutdown;
using ABI::Windows::Management::Deployment::
    DeploymentOptions_ForceTargetApplicationShutdown;
using ABI::Windows::Management::Deployment::
    DeploymentOptions_ForceUpdateFromAnyVersion;
using ABI::Windows::Management::Deployment::DeploymentOptions_None;
using ABI::Windows::Management::Deployment::IAddPackageOptions;
using ABI::Windows::Management::Deployment::IDeploymentResult;
using ABI::Windows::Management::Deployment::IPackageManager;
using ABI::Windows::Management::Deployment::IPackageManager5;
using ABI::Windows::Management::Deployment::IPackageManager9;
using Microsoft::WRL::Callback;
using Microsoft::WRL::ComPtr;

// Type alias for deployment async operation
// AddPackageByUriAsync returns IAsyncOperationWithProgress<DeploymentResult*,
// DeploymentProgress>
using DeploymentAsyncOp = ABI::Windows::Foundation::IAsyncOperationWithProgress<
    ABI::Windows::Management::Deployment::DeploymentResult*,
    ABI::Windows::Management::Deployment::DeploymentProgress>;
using DeploymentCompletedHandler =
    ABI::Windows::Foundation::IAsyncOperationWithProgressCompletedHandler<
        ABI::Windows::Management::Deployment::DeploymentResult*,
        ABI::Windows::Management::Deployment::DeploymentProgress>;

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

// Helper: Create PackageManager using RoActivateInstance
//
// Note on COM interface versioning: In COM/WinRT, each interface version
// (IPackageManager, IPackageManager5, IPackageManager9, etc.) is a separate
// interface that must be queried independently. Unlike C++ inheritance,
// IPackageManager9 does NOT inherit methods from IPackageManager5 or the base
// IPackageManager. Each version only contains the methods that were newly
// added in that version. To call methods from different versions, you must
// QueryInterface (or ComPtr::As) for each specific interface version needed.
HRESULT CreatePackageManager(ComPtr<IPackageManager>* package_manager) {
  base::win::ScopedHString class_id = base::win::ScopedHString::Create(
      RuntimeClass_Windows_Management_Deployment_PackageManager);
  if (!class_id.is_valid()) {
    return E_FAIL;
  }

  ComPtr<IInspectable> inspectable;
  HRESULT hr = base::win::RoActivateInstance(class_id.get(), &inspectable);
  if (FAILED(hr)) {
    return hr;
  }

  return inspectable.As(package_manager);
}

// Helper: Create URI using IUriRuntimeClassFactory
HRESULT CreateUri(const std::wstring& uri_string,
                  ComPtr<IUriRuntimeClass>* uri) {
  ComPtr<IUriRuntimeClassFactory> uri_factory;
  HRESULT hr =
      base::win::GetActivationFactory<IUriRuntimeClassFactory,
                                      RuntimeClass_Windows_Foundation_Uri>(
          &uri_factory);
  if (FAILED(hr)) {
    return hr;
  }

  base::win::ScopedHString uri_hstring =
      base::win::ScopedHString::Create(uri_string);
  if (!uri_hstring.is_valid()) {
    return E_FAIL;
  }

  return uri_factory->CreateUri(uri_hstring.get(), uri->GetAddressOf());
}

// Helper: Create and configure AddPackageOptions
HRESULT CreateAddPackageOptions(const UpdateMsixOptions& opts,
                                ComPtr<IAddPackageOptions>* package_options) {
  base::win::ScopedHString class_id = base::win::ScopedHString::Create(
      RuntimeClass_Windows_Management_Deployment_AddPackageOptions);
  if (!class_id.is_valid()) {
    return E_FAIL;
  }

  ComPtr<IInspectable> inspectable;
  HRESULT hr = base::win::RoActivateInstance(class_id.get(), &inspectable);
  if (FAILED(hr)) {
    return hr;
  }

  hr = inspectable.As(package_options);
  if (FAILED(hr)) {
    return hr;
  }

  // Configure options using ABI interface methods
  (*package_options)
      ->put_DeferRegistrationWhenPackagesAreInUse(opts.defer_registration);
  (*package_options)->put_DeveloperMode(opts.developer_mode);
  (*package_options)->put_ForceAppShutdown(opts.force_shutdown);
  (*package_options)->put_ForceTargetAppShutdown(opts.force_target_shutdown);
  (*package_options)
      ->put_ForceUpdateFromAnyVersion(opts.force_update_from_any_version);

  return S_OK;
}

// Helper: Check if API contract is present
HRESULT CheckApiContractPresent(UINT16 version, boolean* is_present) {
  ComPtr<IApiInformationStatics> api_info;
  HRESULT hr = base::win::GetActivationFactory<
      IApiInformationStatics,
      RuntimeClass_Windows_Foundation_Metadata_ApiInformation>(&api_info);
  if (FAILED(hr)) {
    return hr;
  }

  base::win::ScopedHString contract_name = base::win::ScopedHString::Create(
      L"Windows.Foundation.UniversalApiContract");
  if (!contract_name.is_valid()) {
    return E_FAIL;
  }

  return api_info->IsApiContractPresentByMajor(contract_name.get(), version,
                                               is_present);
}

// Helper: Get current package using IPackageStatics
HRESULT GetCurrentPackage(ComPtr<IPackage>* package) {
  ComPtr<IPackageStatics> package_statics;
  HRESULT hr = base::win::GetActivationFactory<
      IPackageStatics, RuntimeClass_Windows_ApplicationModel_Package>(
      &package_statics);
  if (FAILED(hr)) {
    return hr;
  }

  return package_statics->get_Current(package->GetAddressOf());
}

// Structure to hold callback data for async operations
struct DeploymentCallbackData {
  scoped_refptr<base::SingleThreadTaskRunner> reply_runner;
  gin_helper::Promise<void> promise;
  bool fire_and_forget;
  ComPtr<DeploymentAsyncOp> async_op;  // Keep async_op alive
  std::string operation_name;  // "Deployment" or "Registration" for logs
};

// Handler for deployment/registration completion
void OnDeploymentCompleted(std::unique_ptr<DeploymentCallbackData> data,
                           DeploymentAsyncOp* async_op,
                           AsyncStatus status) {
  std::string error;
  const std::string& op_name = data->operation_name;

  if (data->fire_and_forget) {
    std::ostringstream oss;
    oss << op_name
        << " initiated. Force shutdown or target shutdown requested. "
           "Good bye!";
    DebugLog(oss.str());
    // Don't wait for result in fire-and-forget mode
    data->reply_runner->PostTask(
        FROM_HERE,
        base::BindOnce(
            [](gin_helper::Promise<void> promise) { promise.Resolve(); },
            std::move(data->promise)));
    return;
  }

  if (status == AsyncStatus::Error) {
    ComPtr<IDeploymentResult> result;
    HRESULT hr = async_op->GetResults(&result);
    if (SUCCEEDED(hr) && result) {
      HSTRING error_text_hstring;
      hr = result->get_ErrorText(&error_text_hstring);
      if (SUCCEEDED(hr)) {
        base::win::ScopedHString scoped_error(error_text_hstring);
        error = scoped_error.GetAsUTF8();
      }

      ComPtr<IAsyncInfo> async_info;
      hr = async_op->QueryInterface(IID_PPV_ARGS(&async_info));
      if (SUCCEEDED(hr)) {
        HRESULT error_code;
        hr = async_info->get_ErrorCode(&error_code);
        if (SUCCEEDED(hr)) {
          error += " (" + std::to_string(static_cast<int>(error_code)) + ")";
        }
      }
    }
    if (error.empty()) {
      error = op_name + " failed with unknown error";
    }
    {
      std::ostringstream oss;
      oss << op_name << " failed: " << error;
      DebugLog(oss.str());
    }
  } else if (status == AsyncStatus::Canceled) {
    std::ostringstream oss;
    oss << op_name << " canceled";
    DebugLog(oss.str());
    error = op_name + " canceled";
  } else if (status == AsyncStatus::Completed) {
    std::ostringstream oss;
    oss << "MSIX " << op_name << " completed.";
    DebugLog(oss.str());
  } else {
    error = op_name + " status unknown";
    std::ostringstream oss;
    oss << op_name << " status unknown";
    DebugLog(oss.str());
  }

  // Post result back to UI thread
  data->reply_runner->PostTask(
      FROM_HERE, base::BindOnce(
                     [](gin_helper::Promise<void> promise, std::string error) {
                       if (error.empty()) {
                         promise.Resolve();
                       } else {
                         promise.RejectWithErrorMessage(error);
                       }
                     },
                     std::move(data->promise), std::move(error)));
}

// Performs MSIX update on IO thread
void DoUpdateMsix(const std::string& package_uri,
                  UpdateMsixOptions opts,
                  scoped_refptr<base::SingleThreadTaskRunner> reply_runner,
                  gin_helper::Promise<void> promise) {
  DebugLog("DoUpdateMsix: Starting");
  std::string error;

  // Create PackageManager
  ComPtr<IPackageManager> package_manager;
  HRESULT hr = CreatePackageManager(&package_manager);
  if (FAILED(hr)) {
    error = "Failed to create PackageManager";
    reply_runner->PostTask(
        FROM_HERE,
        base::BindOnce(
            [](gin_helper::Promise<void> promise, std::string error) {
              promise.RejectWithErrorMessage(error);
            },
            std::move(promise), std::move(error)));
    return;
  }

  // Get IPackageManager9 for AddPackageByUriAsync
  ComPtr<IPackageManager9> package_manager9;
  hr = package_manager.As(&package_manager9);
  if (FAILED(hr)) {
    error = "Failed to get IPackageManager9 interface";
    reply_runner->PostTask(
        FROM_HERE,
        base::BindOnce(
            [](gin_helper::Promise<void> promise, std::string error) {
              promise.RejectWithErrorMessage(error);
            },
            std::move(promise), std::move(error)));
    return;
  }

  // Create URI
  std::wstring uri_wstring = base::UTF8ToWide(package_uri);
  ComPtr<IUriRuntimeClass> uri;
  hr = CreateUri(uri_wstring, &uri);
  if (FAILED(hr)) {
    error = "Failed to create URI";
    reply_runner->PostTask(
        FROM_HERE,
        base::BindOnce(
            [](gin_helper::Promise<void> promise, std::string error) {
              promise.RejectWithErrorMessage(error);
            },
            std::move(promise), std::move(error)));
    return;
  }

  // Create AddPackageOptions
  ComPtr<IAddPackageOptions> package_options;
  hr = CreateAddPackageOptions(opts, &package_options);
  if (FAILED(hr)) {
    error = "Failed to create AddPackageOptions";
    reply_runner->PostTask(
        FROM_HERE,
        base::BindOnce(
            [](gin_helper::Promise<void> promise, std::string error) {
              promise.RejectWithErrorMessage(error);
            },
            std::move(promise), std::move(error)));
    return;
  }

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

  // Start async operation
  ComPtr<DeploymentAsyncOp> async_op;
  hr = package_manager9->AddPackageByUriAsync(uri.Get(), package_options.Get(),
                                              &async_op);
  if (FAILED(hr) || !async_op) {
    DebugLog("AddPackageByUriAsync failed or returned null");
    error =
        "Deployment is NULL. See "
        "http://go.microsoft.com/fwlink/?LinkId=235160 for diagnosing.";
    reply_runner->PostTask(
        FROM_HERE,
        base::BindOnce(
            [](gin_helper::Promise<void> promise, std::string error) {
              promise.RejectWithErrorMessage(error);
            },
            std::move(promise), std::move(error)));
    return;
  }

  // Set up callback data
  auto callback_data = std::make_unique<DeploymentCallbackData>();
  callback_data->reply_runner = reply_runner;
  callback_data->promise = std::move(promise);
  callback_data->fire_and_forget =
      opts.force_shutdown || opts.force_target_shutdown;
  callback_data->async_op = async_op;  // Keep async_op alive
  callback_data->operation_name = "Deployment";

  // Register completion handler
  DeploymentCallbackData* raw_data = callback_data.get();
  hr = async_op->put_Completed(
      Callback<DeploymentCompletedHandler>([data = std::move(callback_data)](
                                               DeploymentAsyncOp* op,
                                               AsyncStatus status) mutable {
        OnDeploymentCompleted(std::move(data), op, status);
        return S_OK;
      }).Get());

  if (FAILED(hr)) {
    DebugLog("Failed to register completion handler");
    raw_data->reply_runner->PostTask(
        FROM_HERE, base::BindOnce(
                       [](gin_helper::Promise<void> promise) {
                         promise.RejectWithErrorMessage(
                             "Failed to register completion handler");
                       },
                       std::move(raw_data->promise)));
  }
}

// Performs package registration on IO thread
void DoRegisterPackage(const std::string& family_name,
                       RegisterPackageOptions opts,
                       scoped_refptr<base::SingleThreadTaskRunner> reply_runner,
                       gin_helper::Promise<void> promise) {
  DebugLog("DoRegisterPackage: Starting");
  std::string error;

  // Create PackageManager
  ComPtr<IPackageManager> package_manager;
  HRESULT hr = CreatePackageManager(&package_manager);
  if (FAILED(hr)) {
    error = "Failed to create PackageManager";
    reply_runner->PostTask(
        FROM_HERE,
        base::BindOnce(
            [](gin_helper::Promise<void> promise, std::string error) {
              promise.RejectWithErrorMessage(error);
            },
            std::move(promise), std::move(error)));
    return;
  }

  // Get IPackageManager5 for RegisterPackageByFamilyNameAsync
  ComPtr<IPackageManager5> package_manager5;
  hr = package_manager.As(&package_manager5);
  if (FAILED(hr)) {
    error = "Failed to get IPackageManager5 interface";
    reply_runner->PostTask(
        FROM_HERE,
        base::BindOnce(
            [](gin_helper::Promise<void> promise, std::string error) {
              promise.RejectWithErrorMessage(error);
            },
            std::move(promise), std::move(error)));
    return;
  }

  // Build DeploymentOptions flags
  DeploymentOptions deployment_options = DeploymentOptions_None;
  if (opts.force_shutdown) {
    deployment_options = static_cast<DeploymentOptions>(
        deployment_options | DeploymentOptions_ForceApplicationShutdown);
  }
  if (opts.force_target_shutdown) {
    deployment_options = static_cast<DeploymentOptions>(
        deployment_options | DeploymentOptions_ForceTargetApplicationShutdown);
  }
  if (opts.force_update_from_any_version) {
    deployment_options = static_cast<DeploymentOptions>(
        deployment_options | DeploymentOptions_ForceUpdateFromAnyVersion);
  }

  // Create HSTRING for family name
  base::win::ScopedHString family_name_hstring =
      base::win::ScopedHString::Create(family_name);
  if (!family_name_hstring.is_valid()) {
    error = "Failed to create family name string";
    reply_runner->PostTask(
        FROM_HERE,
        base::BindOnce(
            [](gin_helper::Promise<void> promise, std::string error) {
              promise.RejectWithErrorMessage(error);
            },
            std::move(promise), std::move(error)));
    return;
  }

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

  // RegisterPackageByFamilyNameAndOptionalPackagesAsync (ABI name)
  ComPtr<DeploymentAsyncOp> async_op;
  hr = package_manager5->RegisterPackageByFamilyNameAndOptionalPackagesAsync(
      family_name_hstring.get(),
      nullptr,  // dependencyPackageFamilyNames
      deployment_options,
      nullptr,  // appDataVolume
      nullptr,  // optionalPackageFamilyNames
      &async_op);

  if (FAILED(hr) || !async_op) {
    error =
        "Deployment is NULL. See "
        "http://go.microsoft.com/fwlink/?LinkId=235160 for diagnosing.";
    reply_runner->PostTask(
        FROM_HERE,
        base::BindOnce(
            [](gin_helper::Promise<void> promise, std::string error) {
              promise.RejectWithErrorMessage(error);
            },
            std::move(promise), std::move(error)));
    return;
  }

  // Set up callback data
  auto callback_data = std::make_unique<DeploymentCallbackData>();
  callback_data->reply_runner = reply_runner;
  callback_data->promise = std::move(promise);
  callback_data->fire_and_forget =
      opts.force_shutdown || opts.force_target_shutdown;
  callback_data->async_op = async_op;  // Keep async_op alive
  callback_data->operation_name = "Registration";

  // Register completion handler
  DeploymentCallbackData* raw_data = callback_data.get();
  hr = async_op->put_Completed(
      Callback<DeploymentCompletedHandler>([data = std::move(callback_data)](
                                               DeploymentAsyncOp* op,
                                               AsyncStatus status) mutable {
        OnDeploymentCompleted(std::move(data), op, status);
        return S_OK;
      }).Get());

  if (FAILED(hr)) {
    DebugLog("Failed to register completion handler");
    raw_data->reply_runner->PostTask(
        FROM_HERE, base::BindOnce(
                       [](gin_helper::Promise<void> promise) {
                         promise.RejectWithErrorMessage(
                             "Failed to register completion handler");
                       },
                       std::move(raw_data->promise)));
  }
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

  // Check for required API contract (IPackageManager9 requires v10)
  boolean is_api_present = FALSE;
  if (FAILED(CheckApiContractPresent(10, &is_api_present)) || !is_api_present) {
    DebugLog("UpdateMsix: Required Windows API contract not present");
    promise.RejectWithErrorMessage(
        "This Windows version does not support MSIX updates via this API. "
        "Windows 10 version 2004 or later is required.");
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

  // Check for required API contract (IPackageManager5 requires v3)
  boolean is_api_present = FALSE;
  if (FAILED(CheckApiContractPresent(3, &is_api_present)) || !is_api_present) {
    DebugLog("RegisterPackage: Required Windows API contract not present");
    promise.RejectWithErrorMessage(
        "This Windows version does not support package registration via this "
        "API. Windows 10 version 1607 or later is required.");
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

  // Flags: RESTART_NO_CRASH | RESTART_NO_HANG | RESTART_NO_REBOOT
  // This means: only restart on updates (RESTART_NO_PATCH is NOT set)
  const DWORD dwFlags = 1 | 2 | 8;  // 11

  // Convert command line to wide string (keep in scope for API call)
  std::wstring command_line_wide;
  const wchar_t* command_line_ptr = nullptr;
  if (!command_line.empty()) {
    command_line_wide = base::UTF8ToWide(command_line);
    command_line_ptr = command_line_wide.c_str();
  }

  HRESULT hr = RegisterApplicationRestart(command_line_ptr, dwFlags);
  if (FAILED(hr)) {
    std::ostringstream oss;
    oss << "RegisterApplicationRestart failed with error code: " << hr;
    DebugLog(oss.str());
    return false;
  }

  std::ostringstream oss;
  oss << "RegisterApplicationRestart succeeded"
      << (command_line.empty() ? "" : " with command line");
  DebugLog(oss.str());
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
  boolean is_present = FALSE;
  HRESULT hr = CheckApiContractPresent(7, &is_present);
  if (SUCCEEDED(hr) && is_present) {
    ComPtr<IPackage> package;
    hr = GetCurrentPackage(&package);
    if (SUCCEEDED(hr) && package) {
      // Query all needed package interface versions upfront.
      // Note: Like IPackageManager, each IPackage version (IPackage2,
      // IPackage4, IPackage6) is a separate COM interface. IPackage6 does NOT
      // inherit methods from earlier versions. We must query each version
      // separately to access its specific methods:
      //   - IPackage2: get_IsDevelopmentMode
      //   - IPackage4: get_SignatureKind
      //   - IPackage6: GetAppInstallerInfo
      ComPtr<IPackage2> package2;
      ComPtr<IPackage4> package4;
      ComPtr<IPackage6> package6;
      package.As(&package2);
      package.As(&package4);
      package.As(&package6);

      // Get package ID (from base IPackage)
      ComPtr<IPackageId> package_id;
      hr = package->get_Id(&package_id);
      if (SUCCEEDED(hr) && package_id) {
        // Get FullName
        HSTRING full_name;
        hr = package_id->get_FullName(&full_name);
        if (SUCCEEDED(hr)) {
          base::win::ScopedHString scoped_name(full_name);
          result.Set("id", scoped_name.GetAsUTF8());
        }

        // Get FamilyName
        HSTRING family_name;
        hr = package_id->get_FamilyName(&family_name);
        if (SUCCEEDED(hr)) {
          base::win::ScopedHString scoped_name(family_name);
          result.Set("familyName", scoped_name.GetAsUTF8());
        }

        // Get Version
        ABI::Windows::ApplicationModel::PackageVersion pkg_version;
        hr = package_id->get_Version(&pkg_version);
        if (SUCCEEDED(hr)) {
          std::string version = std::to_string(pkg_version.Major) + "." +
                                std::to_string(pkg_version.Minor) + "." +
                                std::to_string(pkg_version.Build) + "." +
                                std::to_string(pkg_version.Revision);
          result.Set("version", version);
        }
      }

      // Get IsDevelopmentMode (from IPackage2)
      if (package2) {
        boolean is_dev_mode = FALSE;
        hr = package2->get_IsDevelopmentMode(&is_dev_mode);
        result.Set("developmentMode", SUCCEEDED(hr) && is_dev_mode != FALSE);
      } else {
        result.Set("developmentMode", false);
      }

      // Get SignatureKind (from IPackage4)
      if (package4) {
        PackageSignatureKind sig_kind;
        hr = package4->get_SignatureKind(&sig_kind);
        if (SUCCEEDED(hr)) {
          std::string signature_kind;
          switch (sig_kind) {
            case PackageSignatureKind_Developer:
              signature_kind = "developer";
              break;
            case PackageSignatureKind_Enterprise:
              signature_kind = "enterprise";
              break;
            case PackageSignatureKind_None:
              signature_kind = "none";
              break;
            case PackageSignatureKind_Store:
              signature_kind = "store";
              break;
            case PackageSignatureKind_System:
              signature_kind = "system";
              break;
            default:
              signature_kind = "none";
              break;
          }
          result.Set("signatureKind", signature_kind);
        } else {
          result.Set("signatureKind", "none");
        }
      } else {
        result.Set("signatureKind", "none");
      }

      // Get AppInstallerInfo (from IPackage6)
      if (package6) {
        ComPtr<IAppInstallerInfo> app_installer_info;
        hr = package6->GetAppInstallerInfo(&app_installer_info);
        if (SUCCEEDED(hr) && app_installer_info) {
          ComPtr<IUriRuntimeClass> uri;
          hr = app_installer_info->get_Uri(&uri);
          if (SUCCEEDED(hr) && uri) {
            HSTRING uri_string;
            hr = uri->get_AbsoluteUri(&uri_string);
            if (SUCCEEDED(hr)) {
              base::win::ScopedHString scoped_uri(uri_string);
              result.Set("appInstallerUri", scoped_uri.GetAsUTF8());
            }
          }
        }
      }
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
