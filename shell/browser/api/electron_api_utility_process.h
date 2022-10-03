// Copyright (c) 2022 Microsoft, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_API_ELECTRON_API_UTILITY_PROCESS_H_
#define ELECTRON_SHELL_BROWSER_API_ELECTRON_API_UTILITY_PROCESS_H_

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "base/containers/id_map.h"
#include "base/environment.h"
#include "base/memory/weak_ptr.h"
#include "base/process/process_handle.h"
#include "gin/wrappable.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "shell/browser/event_emitter_mixin.h"
#include "shell/common/gin_helper/pinnable.h"
#include "shell/services/node/public/mojom/node_service.mojom.h"
#include "v8/include/v8.h"

namespace gin {
class Arguments;
template <typename T>
class Handle;
}  // namespace gin

namespace base {
class Process;
}  // namespace base

namespace electron {

class PipeReaderBase;

namespace api {

class UtilityProcessWrapper
    : public gin::Wrappable<UtilityProcessWrapper>,
      public gin_helper::Pinnable<UtilityProcessWrapper>,
      public gin_helper::EventEmitterMixin<UtilityProcessWrapper> {
 public:
  enum class IOHandle : size_t { STDIN = 0, STDOUT = 1, STDERR = 2 };
  enum IOType { PIPE, INHERIT, IGNORE };
  ~UtilityProcessWrapper() override;
  static gin::Handle<UtilityProcessWrapper> Create(gin::Arguments* args);
  static raw_ptr<UtilityProcessWrapper> FromProcessId(base::ProcessId pid);

  void OnServiceProcessDisconnected(uint32_t error_code,
                                    const std::string& description);

  // gin::Wrappable
  static gin::WrapperInfo kWrapperInfo;
  gin::ObjectTemplateBuilder GetObjectTemplateBuilder(
      v8::Isolate* isolate) override;
  const char* GetTypeName() override;

  void ReceivePostMessage(blink::TransferableMessage message);
  void HandleStdioMessage(IOHandle handle, std::vector<uint8_t> message);
  void ShutdownStdioReader(IOHandle handle);
  void ResumeStdioReading(PipeReaderBase* pipe_io);
  void Shutdown(int exit_code);

 private:
  UtilityProcessWrapper(node::mojom::NodeServiceParamsPtr params,
                        std::u16string display_name,
                        std::map<IOHandle, IOType> stdio,
                        base::EnvironmentMap env_map,
                        base::FilePath current_working_directory,
                        bool use_plugin_helper);
  void OnServiceProcessLaunched(const base::Process& process);

  void PostMessage(gin::Arguments* args);
  bool Kill() const;
  v8::Local<v8::Value> GetOSProcessId(v8::Isolate* isolate) const;

  base::ProcessId pid_ = base::kNullProcessId;
  mojo::Remote<node::mojom::NodeService> node_service_remote_;
  std::unique_ptr<PipeReaderBase> stdout_reader_;
  std::unique_ptr<PipeReaderBase> stderr_reader_;
  base::WeakPtrFactory<UtilityProcessWrapper> weak_factory_{this};
};

}  // namespace api

}  // namespace electron

#endif  // ELECTRON_SHELL_BROWSER_API_ELECTRON_API_UTILITY_PROCESS_H_
