// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "base/containers/fixed_flat_map.h"
#include "base/containers/map_util.h"
#include "base/strings/strcat.h"
#include "base/strings/string_util.h"
#include "shell/browser/api/electron_api_base_window.h"
#include "shell/browser/browser.h"
#include "shell/browser/javascript_environment.h"
#include "shell/browser/native_window.h"
#include "shell/browser/ui/certificate_trust.h"
#include "shell/browser/ui/file_dialog.h"
#include "shell/browser/ui/message_box.h"
#include "shell/common/gin_converters/callback_converter.h"
#include "shell/common/gin_converters/file_dialog_converter.h"
#include "shell/common/gin_converters/file_path_converter.h"
#include "shell/common/gin_converters/image_converter.h"
#include "shell/common/gin_converters/native_window_converter.h"
#include "shell/common/gin_converters/net_converter.h"
#include "shell/common/gin_converters/std_converter.h"
#include "shell/common/gin_helper/dictionary.h"
#include "shell/common/gin_helper/error_thrower.h"
#include "shell/common/gin_helper/promise.h"
#include "shell/common/node_includes.h"
#include "ui/gfx/image/image_skia.h"
#include "v8/include/v8-isolate.h"
#include "v8/include/v8-local-handle.h"
#include "v8/include/v8-promise.h"

namespace {

// dialog.showX([window, ]options): reads the optional leading BaseWindow and
// the options object. Throws when the app is not ready yet.
bool ReadWindowAndOptions(gin::Arguments* args,
                          electron::NativeWindow** window,
                          gin_helper::Dictionary* options,
                          bool* has_options) {
  v8::Isolate* isolate = args->isolate();
  gin_helper::ErrorThrower thrower(isolate);
  if (!electron::Browser::Get()->is_ready()) {
    thrower.ThrowError("dialog module can only be used after app is ready");
    return false;
  }
  *window = nullptr;
  *has_options = false;
  v8::Local<v8::Value> value;
  if (!args->GetNext(&value))
    return true;
  const bool is_window =
      value->IsObject() &&
      electron::api::BaseWindow::GetConstructorTemplate(isolate)->HasInstance(
          value);
  if (is_window) {
    electron::api::BaseWindow* base_window = nullptr;
    if (gin::ConvertFromV8(isolate, value, &base_window) && base_window)
      *window = base_window->window();
  }
  // (window, options), (falsy, options) or (options).
  if (is_window || !value->BooleanValue(isolate)) {
    if (!args->GetNext(&value))
      return true;
  }
  if (value->IsNullOrUndefined())
    return true;
  *has_options = true;
  if (!gin::ConvertFromV8(isolate, value, options))
    *options = gin_helper::Dictionary::CreateEmpty(isolate);
  return true;
}

// Reads options[key] as a string into |out|, leaving |out| when it is
// undefined; throws "<what> must be a string" for any other type.
bool ReadString(gin_helper::ErrorThrower thrower,
                gin_helper::Dictionary& options,
                std::string_view key,
                std::string_view what,
                std::string* out) {
  v8::Local<v8::Value> value;
  if (!options.Get(key, &value) || value->IsUndefined())
    return true;
  if (!value->IsString()) {
    thrower.ThrowTypeError(base::StrCat({what, " must be a string"}));
    return false;
  }
  *out = gin::V8ToString(thrower.isolate(), value);
  return true;
}

enum class FileDialogKind { kOpen, kSave };

bool ReadFileDialogSettings(gin::Arguments* args,
                            FileDialogKind kind,
                            file_dialog::DialogSettings* settings) {
  v8::Isolate* isolate = args->isolate();
  gin_helper::ErrorThrower thrower(isolate);
  gin_helper::Dictionary options;
  bool has_options = false;
  if (!ReadWindowAndOptions(args, &settings->parent_window, &options,
                            &has_options)) {
    return false;
  }
  if (!has_options) {
    settings->title = kind == FileDialogKind::kOpen ? "Open" : "Save";
    if (kind == FileDialogKind::kOpen)
      settings->properties = file_dialog::OPEN_DIALOG_OPEN_FILE;
    return true;
  }

  v8::Local<v8::Value> properties;
  bool has_properties =
      options.Get("properties", &properties) && !properties->IsUndefined();
  if (has_properties && !properties->IsArray()) {
    if (kind == FileDialogKind::kOpen) {
      thrower.ThrowTypeError("Properties must be an array");
      return false;
    }
    has_properties = false;
  }
  if (has_properties) {
    std::vector<v8::Local<v8::Value>> names;
    gin::ConvertFromV8(isolate, properties, &names);
    settings->properties =
        kind == FileDialogKind::kOpen
            ? file_dialog::OpenDialogPropertiesFromV8(isolate, names)
            : file_dialog::SaveDialogPropertiesFromV8(isolate, names);
  } else if (kind == FileDialogKind::kOpen) {
    settings->properties = file_dialog::OPEN_DIALOG_OPEN_FILE;
  }

  std::string default_path;
  if (!ReadString(thrower, options, "title", "Title", &settings->title) ||
      !ReadString(thrower, options, "buttonLabel", "Button label",
                  &settings->button_label) ||
      !ReadString(thrower, options, "defaultPath", "Default path",
                  &default_path) ||
      !ReadString(thrower, options, "message", "Message", &settings->message)) {
    return false;
  }
  if (kind == FileDialogKind::kSave &&
      !ReadString(thrower, options, "nameFieldLabel", "Name field label",
                  &settings->name_field_label)) {
    return false;
  }
  settings->default_path = base::FilePath::FromUTF8Unsafe(default_path);
  options.Get("filters", &settings->filters);
  options.Get("securityScopedBookmarks", &settings->security_scoped_bookmarks);
  if (kind == FileDialogKind::kSave)
    options.Get("showsTagField", &settings->shows_tag_field);
  return true;
}

void ShowOpenDialogSync(gin::Arguments* args) {
  file_dialog::DialogSettings settings;
  if (!ReadFileDialogSettings(args, FileDialogKind::kOpen, &settings))
    return;
  std::vector<base::FilePath> paths;
  if (file_dialog::ShowOpenDialogSync(settings, &paths))
    args->Return(paths);
}

v8::Local<v8::Value> ShowOpenDialog(gin::Arguments* args) {
  file_dialog::DialogSettings settings;
  if (!ReadFileDialogSettings(args, FileDialogKind::kOpen, &settings))
    return {};
  gin_helper::Promise<gin_helper::Dictionary> promise{args->isolate()};
  v8::Local<v8::Promise> handle = promise.GetHandle();
  file_dialog::ShowOpenDialog(settings, std::move(promise));
  return handle;
}

void ShowSaveDialogSync(gin::Arguments* args) {
  file_dialog::DialogSettings settings;
  if (!ReadFileDialogSettings(args, FileDialogKind::kSave, &settings))
    return;
  if (const auto path = file_dialog::ShowSaveDialogSync(settings))
    args->Return(*path);
}

v8::Local<v8::Value> ShowSaveDialog(gin::Arguments* args) {
  file_dialog::DialogSettings settings;
  if (!ReadFileDialogSettings(args, FileDialogKind::kSave, &settings))
    return {};
  gin_helper::Promise<gin_helper::Dictionary> promise{args->isolate()};
  v8::Local<v8::Promise> handle = promise.GetHandle();
  file_dialog::ShowSaveDialog(settings, std::move(promise));
  return handle;
}

// Access keys: macOS has none, so a single '&' is dropped and "&&" becomes
// "&"; Linux uses '_', so '_' is escaped as "__", "&&" becomes "&" and "&x"
// becomes "_x".
std::string NormalizeAccessKey(const std::string& label) {
#if BUILDFLAG(IS_MAC) || BUILDFLAG(IS_LINUX)
  std::string text = label;
#if BUILDFLAG(IS_LINUX)
  base::ReplaceSubstringsAfterOffset(&text, 0, "_", "__");
#endif
  std::string out;
  out.reserve(text.size());
  for (size_t i = 0; i < text.size(); ++i) {
    if (text[i] != '&') {
      out += text[i];
      continue;
    }
    const bool has_next =
        i + 1 < text.size() && text[i + 1] != '\n' && text[i + 1] != '\r';
    if (has_next && text[i + 1] == '&') {
      out += '&';
      ++i;
      continue;
    }
#if BUILDFLAG(IS_LINUX)
    out += '_';
    if (has_next) {
      out += text[i + 1];
      ++i;
    }
#endif
  }
  return out;
#else
  return label;
#endif
}

int g_next_message_box_id = 0;

struct MessageBoxCall {
  electron::MessageBoxSettings settings;
  // Set when options.signal was already aborted.
  bool aborted = false;
};

// |with_signal|: options.signal (an AbortSignal) can close the box; only the
// asynchronous showMessageBox() supports it.
bool ReadMessageBoxSettings(gin::Arguments* args,
                            bool with_signal,
                            MessageBoxCall* call) {
  v8::Isolate* isolate = args->isolate();
  gin_helper::ErrorThrower thrower(isolate);
  electron::MessageBoxSettings& settings = call->settings;
  settings.default_id = -1;
  settings.cancel_id = 0;
  gin_helper::Dictionary options;
  bool has_options = false;
  if (!ReadWindowAndOptions(args, &settings.parent_window, &options,
                            &has_options)) {
    return false;
  }
  if (!has_options)
    return true;

  static constexpr auto kTypes =
      base::MakeFixedFlatMap<std::string_view, electron::MessageBoxType>({
          {"none", electron::MessageBoxType::kNone},
          {"info", electron::MessageBoxType::kInformation},
          {"warning", electron::MessageBoxType::kWarning},
          {"error", electron::MessageBoxType::kError},
          {"question", electron::MessageBoxType::kQuestion},
      });
  v8::Local<v8::Value> type_value;
  if (options.Get("type", &type_value) && !type_value->IsUndefined()) {
    const auto* found =
        type_value->IsString()
            ? base::FindOrNull(kTypes, gin::V8ToString(isolate, type_value))
            : nullptr;
    if (!found) {
      thrower.ThrowTypeError("Invalid message box type");
      return false;
    }
    settings.type = *found;
  }

  v8::Local<v8::Value> buttons;
  if (options.Get("buttons", &buttons) && !buttons->IsUndefined()) {
    if (!buttons->IsArray() ||
        !gin::ConvertFromV8(isolate, buttons, &settings.buttons)) {
      thrower.ThrowTypeError("Buttons must be an array");
      return false;
    }
  }
  bool normalize_access_keys = false;
  options.Get("normalizeAccessKeys", &normalize_access_keys);
  if (normalize_access_keys) {
    for (std::string& button : settings.buttons)
      button = NormalizeAccessKey(button);
  }

  if (!ReadString(thrower, options, "title", "Title", &settings.title))
    return false;
  v8::Local<v8::Value> no_link;
  if (options.Get("noLink", &no_link) && !no_link->IsUndefined()) {
    if (!no_link->IsBoolean()) {
      thrower.ThrowTypeError("noLink must be a boolean");
      return false;
    }
    settings.no_link = no_link->BooleanValue(isolate);
  }
  if (!ReadString(thrower, options, "message", "Message", &settings.message) ||
      !ReadString(thrower, options, "detail", "Detail", &settings.detail) ||
      !ReadString(thrower, options, "checkboxLabel", "checkboxLabel",
                  &settings.checkbox_label)) {
    return false;
  }
  v8::Local<v8::Value> checkbox_checked;
  if (options.Get("checkboxChecked", &checkbox_checked))
    settings.checkbox_checked = checkbox_checked->BooleanValue(isolate);
  if (settings.checkbox_checked && settings.checkbox_label.empty()) {
    thrower.ThrowError(
        "checkboxChecked requires that checkboxLabel also be passed");
    return false;
  }
  options.Get("defaultId", &settings.default_id);
  options.Get("icon", &settings.icon);
  options.Get("textWidth", &settings.text_width);

  // The button chosen when the dialog is cancelled: "Cancel"/"No" if present,
  // otherwise the first button that is not the default one.
  v8::Local<v8::Value> cancel_id;
  if (options.Get("cancelId", &cancel_id) && !cancel_id->IsNullOrUndefined()) {
    gin::ConvertFromV8(isolate, cancel_id, &settings.cancel_id);
  } else {
    settings.cancel_id =
        settings.default_id == 0 && settings.buttons.size() > 1 ? 1 : 0;
    for (size_t i = 0; i < settings.buttons.size(); ++i) {
      const std::string text = base::ToLowerASCII(settings.buttons[i]);
      if (text == "cancel" || text == "no") {
        settings.cancel_id = i;
        break;
      }
    }
  }

  // options.signal: an AbortSignal that closes the message box.
  v8::Local<v8::Object> signal;
  if (with_signal && options.Get("signal", &signal)) {
    const int id = ++g_next_message_box_id;
    settings.id = id;
    bool aborted = false;
    gin::Dictionary(isolate, signal).Get("aborted", &aborted);
    if (aborted) {
      call->aborted = true;
      return true;
    }
    v8::Local<v8::Context> context = isolate->GetCurrentContext();
    v8::Local<v8::Value> add_event_listener;
    if (signal->Get(context, gin::StringToV8(isolate, "addEventListener"))
            .ToLocal(&add_event_listener) &&
        add_event_listener->IsFunction()) {
      v8::Local<v8::Value> argv[] = {
          gin::StringToV8(isolate, "abort"),
          gin::ConvertToV8(
              isolate, base::BindRepeating(&electron::CloseMessageBox, id))};
      std::ignore =
          add_event_listener.As<v8::Function>()->Call(context, signal, 2, argv);
    }
  }
  return true;
}

v8::Local<v8::Value> MessageBoxResult(v8::Isolate* isolate,
                                      int response,
                                      bool checkbox_checked) {
  auto result = gin_helper::Dictionary::CreateEmpty(isolate);
  result.Set("response", response);
  result.Set("checkboxChecked", checkbox_checked);
  return result.GetHandle();
}

v8::Local<v8::Value> ShowMessageBoxSync(gin::Arguments* args) {
  MessageBoxCall call;
  if (!ReadMessageBoxSettings(args, /*with_signal=*/false, &call))
    return {};
  return v8::Integer::New(args->isolate(),
                          electron::ShowMessageBoxSync(call.settings));
}

v8::Local<v8::Value> ShowMessageBox(gin::Arguments* args) {
  v8::Isolate* isolate = args->isolate();
  MessageBoxCall call;
  if (!ReadMessageBoxSettings(args, /*with_signal=*/true, &call))
    return {};
  gin_helper::Promise<v8::Local<v8::Value>> promise(isolate);
  v8::Local<v8::Promise> handle = promise.GetHandle();
  if (call.aborted) {
    promise.Resolve(MessageBoxResult(isolate, call.settings.cancel_id,
                                     call.settings.checkbox_checked));
    return handle;
  }
  electron::ShowMessageBox(
      call.settings, base::BindOnce(
                         [](gin_helper::Promise<v8::Local<v8::Value>> promise,
                            int response, bool checkbox_checked) {
                           v8::Isolate* isolate = promise.isolate();
                           v8::HandleScope handle_scope(isolate);
                           promise.Resolve(MessageBoxResult(isolate, response,
                                                            checkbox_checked));
                         },
                         std::move(promise)));
  return handle;
}

v8::Local<v8::Value> ShowCertificateTrustDialog(gin::Arguments* args) {
  v8::Isolate* isolate = args->isolate();
  gin_helper::ErrorThrower thrower(isolate);
  electron::NativeWindow* window = nullptr;
  v8::Local<v8::Value> value;
  args->GetNext(&value);
  const bool is_window =
      !value.IsEmpty() && value->IsObject() &&
      electron::api::BaseWindow::GetConstructorTemplate(isolate)->HasInstance(
          value);
  if (is_window)
    gin::ConvertFromV8(isolate, value, &window);
  // (window, options), (falsy, options) or (options).
  if (value.IsEmpty() || is_window || !value->BooleanValue(isolate)) {
    value = v8::Local<v8::Value>();
    args->GetNext(&value);
  }
  gin_helper::Dictionary options;
  if (value.IsEmpty() || !gin::ConvertFromV8(isolate, value, &options)) {
    thrower.ThrowTypeError("options must be an object");
    return {};
  }
  v8::Local<v8::Value> certificate_value;
  scoped_refptr<net::X509Certificate> certificate;
  if (!options.Get("certificate", &certificate_value) ||
      !certificate_value->IsObject()) {
    thrower.ThrowTypeError("certificate must be an object");
    return {};
  }
  std::string message;
  if (!ReadString(thrower, options, "message", "message", &message))
    return {};
  if (!gin::ConvertFromV8(isolate, certificate_value, &certificate)) {
    thrower.ThrowTypeError("certificate must be a valid certificate");
    return {};
  }
#if BUILDFLAG(IS_MAC) || BUILDFLAG(IS_WIN)
  return certificate_trust::ShowCertificateTrust(window, certificate, message);
#else
  return {};
#endif
}

void Initialize(v8::Local<v8::Object> exports,
                v8::Local<v8::Value> unused,
                v8::Local<v8::Context> context,
                void* priv) {
  v8::Isolate* const isolate = electron::JavascriptEnvironment::GetIsolate();
  gin_helper::Dictionary dict{isolate, exports};
  dict.SetMethod<&ShowMessageBoxSync>("showMessageBoxSync");
  dict.SetMethod<&ShowMessageBox>("showMessageBox");
  dict.SetMethod<&electron::ShowErrorBox>("showErrorBox");
  dict.SetMethod<&ShowOpenDialogSync>("showOpenDialogSync");
  dict.SetMethod<&ShowOpenDialog>("showOpenDialog");
  dict.SetMethod<&ShowSaveDialogSync>("showSaveDialogSync");
  dict.SetMethod<&ShowSaveDialog>("showSaveDialog");
  dict.SetMethod<&ShowCertificateTrustDialog>("showCertificateTrustDialog");
}

}  // namespace

NODE_LINKED_BINDING_CONTEXT_AWARE(electron_browser_dialog, Initialize)
