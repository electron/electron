// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "base/bind.h"
#include "base/logging.h"
#include "content/public/renderer/render_frame.h"
#include "shell/common/gin_helper/dictionary.h"
#include "shell/common/node_includes.h"
#include "third_party/blink/renderer/bindings/core/v8/v8_file.h"
#include "third_party/blink/renderer/bindings/modules/v8/v8_file_system_directory_handle.h"
#include "third_party/blink/renderer/bindings/modules/v8/v8_file_system_file_handle.h"
#include "third_party/blink/renderer/platform/bindings/v8_dom_wrapper.h"
#include "third_party/blink/public/mojom/file_system_access/file_system_access_transfer_token.mojom.h"
#include "third_party/blink/public/mojom/file_system_access/file_system_access_transfer_token.mojom-blink.h"
#include "third_party/blink/renderer/platform/bindings/wrapper_type_info.h"
#include "third_party/blink/renderer/core/fileapi/file.h"
#include "third_party/blink/renderer/modules/file_system_access/file_system_handle.h"
#include "shell/common/api/api.mojom.h"
#include "third_party/blink/public/common/associated_interfaces/associated_interface_provider.h"
#include "third_party/blink/public/platform/cross_variant_mojo_util.h"

namespace electron {
content::RenderFrame* GetRenderFrame(v8::Local<v8::Object> value);
}

namespace {

v8::Local<v8::Value> GetPath(v8::Isolate* isolate, v8::Local<v8::Value> value) {
  std::map<std::string, std::string> keys;
  if (!value->IsObject()) {
    keys["is object"] = "false";
    return gin::ConvertToV8(isolate, keys);
  }
  const auto& object = value->ToObject(isolate->GetCurrentContext()).ToLocalChecked();
  keys["internal field count"] = std::to_string(object->InternalFieldCount());
  keys["constructor name"] = *v8::String::Utf8Value(isolate, object->GetConstructorName());
  const auto& names = object->GetPropertyNames(isolate->GetCurrentContext()).ToLocalChecked();
  for (uint32_t i=0;i<names->Length();i++) {
    keys["name "+std::to_string(i)] = *v8::String::Utf8Value(isolate, names->Get(isolate->GetCurrentContext(), i).ToLocalChecked());
  }
  keys["IsApiWrapper"] = object->IsApiWrapper() ? "true" : "false";
  keys["IsUndetectable"] = object->IsUndetectable() ? "true" : "false";
  auto path = object->Get(isolate->GetCurrentContext(), v8::String::NewFromUtf8(isolate, "kind").ToLocalChecked());
  if (!path.IsEmpty()) {
    keys["path"] = *v8::String::Utf8Value(isolate, path.ToLocalChecked());
  }
  if (blink::V8DOMWrapper::IsWrapper(isolate, object)) {
    keys["is wrapper"] = "yes";
    auto *wrappable = blink::ToScriptWrappable(object);
    const auto* wrapper_type_info = wrappable->GetWrapperTypeInfo();
    keys["is FileSystemFileHandle"] = wrapper_type_info == blink::V8FileSystemFileHandle::GetWrapperTypeInfo() ? "true" : "false";
    keys["is FileSystemDirectoryHandle"] = wrapper_type_info == blink::V8FileSystemDirectoryHandle::GetWrapperTypeInfo() ? "true" : "false";
    keys["is File"] = wrapper_type_info == blink::V8File::GetWrapperTypeInfo() ? "true" : "false";
    if (wrapper_type_info == blink::V8File::GetWrapperTypeInfo()) {
      auto* file = wrappable->ToImpl<blink::File>();
      keys["file path"] = file->path().Utf8();//FIXME: does not work for FileSystemFileHandle.getFile()
    }
    else if (wrapper_type_info == blink::V8FileSystemFileHandle::GetWrapperTypeInfo() || wrapper_type_info == blink::V8FileSystemDirectoryHandle::GetWrapperTypeInfo()) {
      auto* file = wrappable->ToImpl<blink::FileSystemHandle>();

      auto* render_frame = electron::GetRenderFrame(isolate->GetCurrentContext()->Global());
      CHECK(render_frame);
      auto* frame = render_frame->GetWebFrame();
      CHECK(frame);

      VLOG(1)<<("_-^-_-^-_-^-_-^-_-^- going to call ResolveTransferToken ");

      mojo::AssociatedRemote<electron::mojom::ElectronWebContentsUtility>
          web_contents_utility_remote;
      render_frame->GetRemoteAssociatedInterfaces()->GetInterface(
          &web_contents_utility_remote);
      web_contents_utility_remote->ResolveFileSystemAccessTransferToken(blink::ToCrossVariantMojoType(file->Transfer()), base::BindOnce([](const std::string& path) {
        VLOG(1)<<("_-^-_-^-_-^-_-^-_-^- got path:::: "+path);
      }));


      keys["file path"] = "now what? " + std::to_string(uintptr_t(file));
    }
    // wrappable->ToImpl<FileSystemHandle>()
    keys["interface_name"] = wrapper_type_info->interface_name ? std::string(wrapper_type_info->interface_name) : "nullptr";
  } else {
    keys["is wrapper"] = "no";
  }
  // ToWrapperTypeInfo(object)->GetWrapperInfo(&keys);
  return gin::ConvertToV8(isolate, keys);
}

void Initialize(v8::Local<v8::Object> exports,
                v8::Local<v8::Value> unused,
                v8::Local<v8::Context> context,
                void* priv) {
  gin_helper::Dictionary dict(context->GetIsolate(), exports);
  dict.SetMethod("getPath", &GetPath);
}

}  // namespace

NODE_LINKED_MODULE_CONTEXT_AWARE(electron_renderer_files, Initialize)
