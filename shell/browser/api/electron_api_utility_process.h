// Copyright (c) 2022 Microsoft, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_API_ELECTRON_API_UTILITY_PROCESS_H_
#define ELECTRON_SHELL_BROWSER_API_ELECTRON_API_UTILITY_PROCESS_H_

#include <map>
#include <memory>
#include <string>

#include "base/callback_list.h"
#include "base/environment.h"
#include "base/process/process_handle.h"
#include "content/public/browser/service_process_host.h"
#include "gin/weak_cell.h"
#include "gin/wrappable.h"
#include "mojo/public/cpp/bindings/message.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "shell/browser/event_emitter_mixin.h"
#include "shell/browser/net/url_loader_network_observer.h"
#include "shell/common/gc_plugin.h"
#include "shell/common/gin_helper/self_keep_alive.h"
#include "shell/services/node/public/mojom/node_service.mojom.h"
#include "v8/include/v8-forward.h"

namespace gin {
class Arguments;
}  // namespace gin

namespace base {
class Process;
}  // namespace base

namespace mojo {
class Connector;
}  // namespace mojo

namespace electron::api {

class Session;

class UtilityProcessWrapper final
    : public gin::Wrappable<UtilityProcessWrapper>,
      public gin_helper::EventEmitterMixin<UtilityProcessWrapper>,
      private mojo::MessageReceiver,
      public node::mojom::NodeServiceClient,
      public content::ServiceProcessHost::Observer {
 public:
  enum class IOHandle : size_t { STDIN = 0, STDOUT = 1, STDERR = 2 };
  enum class IOType { IO_PIPE, IO_INHERIT, IO_IGNORE };

  UtilityProcessWrapper(node::mojom::NodeServiceParamsPtr params,
                        std::u16string display_name,
                        std::map<IOHandle, IOType> stdio,
                        base::EnvironmentMap env_map,
                        base::FilePath current_working_directory,
                        bool use_plugin_helper,
                        bool create_network_observer,
                        bool disclaim_responsibility,
                        Session* session);
  ~UtilityProcessWrapper() override;

  static UtilityProcessWrapper* Create(gin::Arguments* args);
  static UtilityProcessWrapper* FromProcessId(base::ProcessId pid);

  void Shutdown(uint32_t exit_code);

  bool has_session() const { return session_.Get(); }

  // gin::Wrappable
  static const gin::WrapperInfo kWrapperInfo;
  static const char* GetClassName() { return "UtilityProcess"; }
  void Trace(cppgc::Visitor*) const override;
  const gin::WrapperInfo* wrapper_info() const override;
  const char* GetHumanReadableName() const override;

 protected:
  gin::ObjectTemplateBuilder GetObjectTemplateBuilder(
      v8::Isolate* isolate) override;

 private:
  void OnServiceProcessLaunch(const base::Process& process);
  void CloseConnectorPort();

  void HandleTermination(uint32_t exit_code);

  void PostMessage(gin::Arguments* args);
  bool Kill();
  v8::Local<v8::Value> GetOSProcessId(v8::Isolate* isolate) const;

  // mojo::MessageReceiver
  bool Accept(mojo::Message* mojo_message) override;

  // node::mojom::NodeServiceClient
  void OnV8FatalError(const std::string& location,
                      const std::string& report) override;

  // content::ServiceProcessHost::Observer
  void OnServiceProcessTerminatedNormally(
      const content::ServiceProcessInfo& info) override;
  void OnServiceProcessCrashed(
      const content::ServiceProcessInfo& info) override;

  void OnServiceProcessDisconnected(uint32_t exit_code,
                                    const std::string& description);

  // Creates and sends a new URLLoaderFactory to the utility process.
  // Called after Network Service restart to update the factory.
  void CreateAndSendURLLoaderFactory(bool crashed);
  node::mojom::URLLoaderFactoryParamsPtr CreateURLLoaderFactoryParams();

  base::ProcessId pid_ = base::kNullProcessId;
#if BUILDFLAG(IS_WIN)
  // Non-owning handles, these will be closed when the
  // corresponding FD are closed via _close.
  HANDLE stdout_read_handle_;
  HANDLE stderr_read_handle_;
#endif
  int stdout_read_fd_ = -1;
  int stderr_read_fd_ = -1;
  bool connector_closed_ = false;
  bool terminated_ = false;
  bool killed_ = false;
  bool create_network_observer_ = false;
  std::unique_ptr<mojo::Connector> connector_;
  blink::MessagePortDescriptor host_port_;
  GC_PLUGIN_IGNORE(
      "Context tracking of receiver is not needed in the browser process.")
  mojo::Receiver<node::mojom::NodeServiceClient> receiver_{this};
  GC_PLUGIN_IGNORE(
      "Context tracking of remote is not needed in the browser process.")
  mojo::Remote<node::mojom::NodeService> node_service_remote_;
  cppgc::Member<Session> session_;
  std::optional<electron::URLLoaderNetworkObserver>
      url_loader_network_observer_;
  base::CallbackListSubscription network_service_gone_subscription_;
  gin_helper::SelfKeepAlive<UtilityProcessWrapper> keep_alive_{this};
  gin::WeakCellFactory<UtilityProcessWrapper> weak_factory_{this};
};

}  // namespace electron::api

#endif  // ELECTRON_SHELL_BROWSER_API_ELECTRON_API_UTILITY_PROCESS_H_
