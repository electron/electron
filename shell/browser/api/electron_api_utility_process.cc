// Copyright (c) 2022 Microsoft, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/api/electron_api_utility_process.h"

#include <map>
#include <utility>

#include "base/bind.h"
#include "base/files/file_util.h"
#include "base/memory/ref_counted_memory.h"
#include "base/no_destructor.h"
#include "base/process/kill.h"
#include "base/process/launch.h"
#include "base/process/process.h"
#include "base/synchronization/atomic_flag.h"
#include "base/task/sequenced_task_runner.h"
#include "base/task/thread_pool.h"
#include "base/threading/thread.h"
#include "content/public/browser/browser_task_traits.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/service_process_host.h"
#include "content/public/common/child_process_host.h"
#include "content/public/common/result_codes.h"
#include "gin/handle.h"
#include "gin/object_template_builder.h"
#include "gin/wrappable.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "net/server/http_connection.h"  // nogncheck TODO(deepak1556): Remove dependency on this header.
#include "shell/browser/api/message_port.h"
#include "shell/browser/javascript_environment.h"
#include "shell/common/gin_converters/callback_converter.h"
#include "shell/common/gin_converters/file_path_converter.h"
#include "shell/common/gin_helper/dictionary.h"
#include "shell/common/gin_helper/object_template_builder.h"
#include "shell/common/node_includes.h"
#include "shell/common/v8_value_serializer.h"

#if BUILDFLAG(IS_POSIX)
#include "base/posix/eintr_wrapper.h"
#endif

#if BUILDFLAG(IS_WIN)
#include "base/win/windows_types.h"
#endif

namespace electron {

base::IDMap<api::UtilityProcessWrapper*, base::ProcessId>&
GetAllUtilityProcessWrappers() {
  static base::NoDestructor<
      base::IDMap<api::UtilityProcessWrapper*, base::ProcessId>>
      s_all_utility_process_wrappers;
  return *s_all_utility_process_wrappers;
}

// Pipe reader logic is derived from content::DevToolsPipeHandler.
class PipeIOBase {
 public:
  explicit PipeIOBase(const char* thread_name)
      : thread_(new base::Thread(thread_name)) {}

  virtual ~PipeIOBase() = default;
  virtual void StartReadLoop() {}

  bool Start() {
    base::Thread::Options options;
    options.message_pump_type = base::MessagePumpType::IO;
    if (!thread_->StartWithOptions(std::move(options)))
      return false;
    StartReadLoop();
    return true;
  }

  bool is_shutting_down() { return shutting_down_.IsSet(); }

  static void Shutdown(std::unique_ptr<PipeIOBase> pipe_io) {
    if (!pipe_io)
      return;
    auto thread = std::move(pipe_io->thread_);
    pipe_io->shutting_down_.Set();
    pipe_io->ClosePipe();
    // Post self destruction on the custom thread if it's running.
    if (thread->task_runner()) {
      thread->task_runner()->DeleteSoon(FROM_HERE, std::move(pipe_io));
    } else {
      pipe_io.reset();
    }
    // Post background task that would join and destroy the thread.
    base::ThreadPool::CreateSequencedTaskRunner(
        {base::MayBlock(), base::TaskShutdownBehavior::CONTINUE_ON_SHUTDOWN,
         base::WithBaseSyncPrimitives(), base::TaskPriority::BEST_EFFORT})
        ->DeleteSoon(FROM_HERE, std::move(thread));
  }

 protected:
  virtual void ClosePipe() = 0;

  std::unique_ptr<base::Thread> thread_;
  base::AtomicFlag shutting_down_;
};

class PipeReaderBase : public PipeIOBase {
 public:
  PipeReaderBase(const char* thread_name,
#if BUILDFLAG(IS_WIN)
                 HANDLE read_handle
#else
                 int read_fd
#endif
                 )
      : PipeIOBase(thread_name) {
#if BUILDFLAG(IS_WIN)
    read_handle_ = read_handle;
#else
    read_fd_ = read_fd;
#endif
    read_buffer_ = new net::HttpConnection::ReadIOBuffer();
    read_buffer_->set_max_buffer_size(4096);
  }

  void StartReadLoop() override {
    thread_->task_runner()->PostTask(
        FROM_HERE,
        base::BindOnce(&PipeReaderBase::ReadInternal, base::Unretained(this)));
  }

 protected:
  void ClosePipe() override {
// Concurrently discard the pipe handles to successfully join threads.
#if BUILDFLAG(IS_WIN)
    // Cancel pending synchronous read.
    CancelIoEx(read_handle_, nullptr);
    CloseHandle(read_handle_);
#else
    shutdown(read_fd_, SHUT_RDWR);
#endif
  }

  size_t ReadBytes(void* buffer, size_t size) {
    size_t bytes_read = 0;
    while (bytes_read < size) {
#if BUILDFLAG(IS_WIN)
      DWORD size_read = 0;
      bool had_error =
          !ReadFile(read_handle_, static_cast<char*>(buffer) + bytes_read,
                    size - bytes_read, &size_read, nullptr);
#else
      int size_read = read(read_fd_, static_cast<char*>(buffer) + bytes_read,
                           size - bytes_read);
      if (size_read < 0 && errno == EINTR)
        continue;
      bool had_error = size_read <= 0;
#endif
      if (had_error) {
        if (!shutting_down_.IsSet()) {
          ShutdownReader();
        }
        return 0;
      }
      bytes_read += size_read;
      break;
    }
    return bytes_read;
  }

  virtual void HandleMessage(std::vector<uint8_t> message) = 0;
  virtual void ShutdownReader() = 0;

 private:
  void ReadInternal() {
    if (read_buffer_->RemainingCapacity() == 0 &&
        !read_buffer_->IncreaseCapacity()) {
      LOG(ERROR) << "Connection closed, not enough capacity";
      ShutdownReader();
      return;
    }
    size_t bytes_read =
        ReadBytes(read_buffer_->data(), read_buffer_->RemainingCapacity());
    if (!bytes_read) {
      return;
    }
    read_buffer_->DidRead(bytes_read);
    HandleMessage(std::vector<uint8_t>(
        read_buffer_->StartOfBuffer(),
        read_buffer_->StartOfBuffer() + read_buffer_->GetSize()));
    read_buffer_->DidConsume(bytes_read);
  }

  scoped_refptr<net::HttpConnection::ReadIOBuffer> read_buffer_;
#if BUILDFLAG(IS_WIN)
  HANDLE read_handle_;
#else
  int read_fd_;
#endif
};

class StdoutPipeReader : public PipeReaderBase {
 public:
  StdoutPipeReader(
      base::WeakPtr<api::UtilityProcessWrapper> utility_process_wrapper,
#if BUILDFLAG(IS_WIN)
      HANDLE read_handle)
      : PipeReaderBase("UtilityProcessStdoutReadThread", read_handle),
#else
      int read_fd)
      : PipeReaderBase("UtilityProcessStdoutReadThread", read_fd),
#endif
        utility_process_wrapper_(std::move(utility_process_wrapper)) {
  }

 private:
  void HandleMessage(std::vector<uint8_t> message) override {
    content::GetUIThreadTaskRunner({})->PostTask(
        FROM_HERE,
        base::BindOnce(&api::UtilityProcessWrapper::HandleStdioMessage,
                       utility_process_wrapper_,
                       api::UtilityProcessWrapper::IOHandle::STDOUT,
                       std::move(message)));
  }

  void ShutdownReader() override {
    content::GetUIThreadTaskRunner({})->PostTask(
        FROM_HERE,
        base::BindOnce(&api::UtilityProcessWrapper::ShutdownStdioReader,
                       utility_process_wrapper_,
                       api::UtilityProcessWrapper::IOHandle::STDOUT));
  }

  base::WeakPtr<api::UtilityProcessWrapper> utility_process_wrapper_;
};

class StderrPipeReader : public PipeReaderBase {
 public:
  StderrPipeReader(
      base::WeakPtr<api::UtilityProcessWrapper> utility_process_wrapper,
#if BUILDFLAG(IS_WIN)
      HANDLE read_handle)
      : PipeReaderBase("UtilityProcessStderrReadThread", read_handle),
#else
      int read_fd)
      : PipeReaderBase("UtilityProcessStderrReadThread", read_fd),
#endif
        utility_process_wrapper_(std::move(utility_process_wrapper)) {
  }

 private:
  void HandleMessage(std::vector<uint8_t> message) override {
    content::GetUIThreadTaskRunner({})->PostTask(
        FROM_HERE,
        base::BindOnce(&api::UtilityProcessWrapper::HandleStdioMessage,
                       utility_process_wrapper_,
                       api::UtilityProcessWrapper::IOHandle::STDERR,
                       std::move(message)));
  }

  void ShutdownReader() override {
    content::GetUIThreadTaskRunner({})->PostTask(
        FROM_HERE,
        base::BindOnce(&api::UtilityProcessWrapper::ShutdownStdioReader,
                       utility_process_wrapper_,
                       api::UtilityProcessWrapper::IOHandle::STDERR));
  }

  base::WeakPtr<api::UtilityProcessWrapper> utility_process_wrapper_;
};

namespace api {

gin::WrapperInfo UtilityProcessWrapper::kWrapperInfo = {
    gin::kEmbedderNativeGin};

UtilityProcessWrapper::UtilityProcessWrapper(
    node::mojom::NodeServiceParamsPtr params,
    std::u16string display_name,
    std::map<IOHandle, IOType> stdio,
    bool use_plugin_helper) {
#if BUILDFLAG(IS_WIN)
  base::win::ScopedHandle stdout_write(nullptr);
  base::win::ScopedHandle stderr_write(nullptr);
#elif BUILDFLAG(IS_POSIX)
  base::FileHandleMappingVector fds_to_remap;
#endif
  for (const auto& [io_handle, io_type] : stdio) {
    if (io_type == IOType::PIPE) {
#if BUILDFLAG(IS_WIN)
      HANDLE read = nullptr;
      HANDLE write = nullptr;
      if (!::CreatePipe(&read, &write, nullptr, 0)) {
        PLOG(ERROR) << "pipe creation failed";
        return;
      }
      if (io_handle == IOHandle::STDOUT) {
        stdout_write.Set(write);
        stdout_reader_ = std::make_unique<StdoutPipeReader>(
            weak_factory_.GetWeakPtr(), read);
        if (!stdout_reader_->Start()) {
          PLOG(ERROR) << "stdout output watcher thread failed to start.";
          PipeIOBase::Shutdown(std::move(stdout_reader_));
          CloseHandle(write);
          return;
        }
      } else if (io_handle == IOHandle::STDERR) {
        stderr_write.Set(write);
        stderr_reader_ = std::make_unique<StderrPipeReader>(
            weak_factory_.GetWeakPtr(), read);
        if (!stderr_reader_->Start()) {
          PLOG(ERROR) << "stderr output watcher thread failed to start.";
          PipeIOBase::Shutdown(std::move(stderr_reader_));
          CloseHandle(write);
          return;
        }
      }
#elif BUILDFLAG(IS_POSIX)
      int pipe_fd[2];
      if (HANDLE_EINTR(pipe(pipe_fd)) < 0) {
        PLOG(ERROR) << "pipe creation failed";
        return;
      }
      if (io_handle == IOHandle::STDOUT) {
        fds_to_remap.push_back(std::make_pair(pipe_fd[1], STDOUT_FILENO));
        stdout_reader_ = std::make_unique<StdoutPipeReader>(
            weak_factory_.GetWeakPtr(), pipe_fd[0]);
        if (!stdout_reader_->Start()) {
          PLOG(ERROR) << "stdout output watcher thread failed to start.";
          PipeIOBase::Shutdown(std::move(stdout_reader_));
          IGNORE_EINTR(close(pipe_fd[1]));
          return;
        }
      } else if (io_handle == IOHandle::STDERR) {
        fds_to_remap.push_back(std::make_pair(pipe_fd[1], STDERR_FILENO));
        stderr_reader_ = std::make_unique<StderrPipeReader>(
            weak_factory_.GetWeakPtr(), pipe_fd[0]);
        if (!stderr_reader_->Start()) {
          PLOG(ERROR) << "stderr output watcher thread failed to start.";
          PipeIOBase::Shutdown(std::move(stderr_reader_));
          IGNORE_EINTR(close(pipe_fd[1]));
          return;
        }
      }
#endif
    } else if (io_type == IOType::IGNORE) {
#if BUILDFLAG(IS_WIN)
      HANDLE handle =
          CreateFileW(L"NUL", FILE_GENERIC_WRITE | FILE_READ_ATTRIBUTES,
                      FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr,
                      OPEN_EXISTING, 0, nullptr);
      if (handle == INVALID_HANDLE_VALUE) {
        PLOG(ERROR) << "Failed to create null handle";
        return;
      }
      if (io_handle == IOHandle::STDOUT) {
        stdout_write.Set(handle);
      } else if (io_handle == IOHandle::STDERR) {
        stderr_write.Set(handle);
      }
#elif BUILDFLAG(IS_POSIX)
      int devnull = open("/dev/null", O_WRONLY);
      if (devnull < 0) {
        PLOG(ERROR) << "failed to open /dev/null";
        return;
      }
      if (io_handle == IOHandle::STDOUT) {
        fds_to_remap.push_back(std::make_pair(devnull, STDOUT_FILENO));
      } else if (io_handle == IOHandle::STDERR) {
        fds_to_remap.push_back(std::make_pair(devnull, STDERR_FILENO));
      }
#endif
    }
  }

  mojo::PendingReceiver<node::mojom::NodeService> receiver =
      node_service_remote_.BindNewPipeAndPassReceiver();

  content::ServiceProcessHost::Launch(
      std::move(receiver),
      content::ServiceProcessHost::Options()
          .WithDisplayName(display_name.empty()
                               ? std::u16string(u"Node Utility Process")
                               : display_name)
          .WithExtraCommandLineSwitches(params->exec_args)
#if BUILDFLAG(IS_WIN)
          .WithStdoutHandle(std::move(stdout_write))
          .WithStderrHandle(std::move(stderr_write))
#elif BUILDFLAG(IS_POSIX)
          .WithAdditionalFds(std::move(fds_to_remap))
#endif
#if BUILDFLAG(IS_MAC)
          .WithChildFlags(use_plugin_helper
                              ? content::ChildProcessHost::CHILD_PLUGIN
                              : content::ChildProcessHost::CHILD_NORMAL)
#endif
          .WithProcessCallback(
              base::BindOnce(&UtilityProcessWrapper::OnServiceProcessLaunched,
                             weak_factory_.GetWeakPtr()))
          .Pass());
  node_service_remote_.set_disconnect_with_reason_handler(
      base::BindOnce(&UtilityProcessWrapper::OnServiceProcessDisconnected,
                     weak_factory_.GetWeakPtr()));
  node_service_remote_->Initialize(std::move(params));
}

UtilityProcessWrapper::~UtilityProcessWrapper() = default;

void UtilityProcessWrapper::OnServiceProcessLaunched(
    const base::Process& process) {
  DCHECK(node_service_remote_.is_connected());
  pid_ = process.Pid();
  GetAllUtilityProcessWrappers().AddWithID(this, pid_);
  // Emit 'spawn' event
  EmitWithoutCustomEvent("spawn");
}

void UtilityProcessWrapper::OnServiceProcessDisconnected(
    uint32_t error_code,
    const std::string& description) {
  Shutdown(0 /* exit_code */);
}

void UtilityProcessWrapper::Shutdown(int exit_code) {
  if (pid_ != base::kNullProcessId)
    GetAllUtilityProcessWrappers().Remove(pid_);
  node_service_remote_.reset();
  // Shutdown output readers before exit event..
  if (stdout_reader_.get())
    PipeIOBase::Shutdown(std::move(stdout_reader_));
  if (stderr_reader_.get())
    PipeIOBase::Shutdown(std::move(stderr_reader_));
  // Emit 'exit' event
  EmitWithoutCustomEvent("exit", exit_code);
  Unpin();
}

void UtilityProcessWrapper::PostMessage(gin::Arguments* args) {
  if (!node_service_remote_.is_connected())
    return;

  blink::TransferableMessage transferable_message;
  v8::Local<v8::Value> message_value;
  if (args->GetNext(&message_value)) {
    if (!electron::SerializeV8Value(args->isolate(), message_value,
                                    &transferable_message)) {
      // SerializeV8Value sets an exception.
      return;
    }
  }

  v8::Local<v8::Value> transferables;
  std::vector<gin::Handle<MessagePort>> wrapped_ports;
  if (args->GetNext(&transferables)) {
    if (!gin::ConvertFromV8(args->isolate(), transferables, &wrapped_ports)) {
      args->isolate()->ThrowException(v8::Exception::Error(
          gin::StringToV8(args->isolate(), "Invalid value for transfer")));
      return;
    }
  }

  bool threw_exception = false;
  transferable_message.ports = MessagePort::DisentanglePorts(
      args->isolate(), wrapped_ports, &threw_exception);
  if (threw_exception)
    return;

  node_service_remote_->ReceivePostMessage(std::move(transferable_message));
}

bool UtilityProcessWrapper::Kill() const {
  if (pid_ == base::kNullProcessId)
    return 0;
  base::Process process = base::Process::Open(pid_);
  bool result = process.Terminate(content::RESULT_CODE_NORMAL_EXIT, false);
  // Refs https://bugs.chromium.org/p/chromium/issues/detail?id=818244
  // Currently utility process is not sandboxed which
  // means Zygote is not used on linux, refs
  // content::UtilitySandboxedProcessLauncherDelegate::GetZygote.
  // If sandbox feature is enabled for the utility process, then the
  // process reap should be signaled through the zygote via
  // content::ZygoteCommunication::EnsureProcessTerminated.
  base::EnsureProcessTerminated(std::move(process));
  return result;
}

v8::Local<v8::Value> UtilityProcessWrapper::GetOSProcessId(
    v8::Isolate* isolate) const {
  if (pid_ == base::kNullProcessId)
    return v8::Undefined(isolate);
  return gin::ConvertToV8(isolate, pid_);
}

void UtilityProcessWrapper::ReceivePostMessage(
    blink::TransferableMessage message) {
  v8::Isolate* isolate = JavascriptEnvironment::GetIsolate();
  v8::HandleScope handle_scope(isolate);
  v8::Local<v8::Value> message_value =
      electron::DeserializeV8Value(isolate, message);
  EmitWithoutCustomEvent("message", message_value);
}

void UtilityProcessWrapper::HandleStdioMessage(IOHandle handle,
                                               std::vector<uint8_t> message) {
  v8::Isolate* isolate = JavascriptEnvironment::GetIsolate();
  v8::HandleScope handle_scope(isolate);
  auto array_buffer = v8::ArrayBuffer::New(isolate, message.size());
  auto backing_store = array_buffer->GetBackingStore();
  memcpy(backing_store->Data(), message.data(), message.size());
  if (handle == IOHandle::STDOUT) {
    Emit("stdout", array_buffer,
         base::BindRepeating(&UtilityProcessWrapper::ResumeStdioReading,
                             weak_factory_.GetWeakPtr(), stdout_reader_.get()));
  } else {
    Emit("stderr", array_buffer,
         base::BindRepeating(&UtilityProcessWrapper::ResumeStdioReading,
                             weak_factory_.GetWeakPtr(), stderr_reader_.get()));
  }
}

void UtilityProcessWrapper::ResumeStdioReading(PipeReaderBase* pipe_io) {
  if (!pipe_io->is_shutting_down()) {
    pipe_io->StartReadLoop();
  }
}

void UtilityProcessWrapper::ShutdownStdioReader(IOHandle handle) {
  if (handle == IOHandle::STDOUT) {
    PipeIOBase::Shutdown(std::move(stdout_reader_));
  } else {
    PipeIOBase::Shutdown(std::move(stderr_reader_));
  }
}

// static
raw_ptr<UtilityProcessWrapper> UtilityProcessWrapper::FromProcessId(
    base::ProcessId pid) {
  auto* utility_process_wrapper = GetAllUtilityProcessWrappers().Lookup(pid);
  return !!utility_process_wrapper ? utility_process_wrapper : nullptr;
}

// static
gin::Handle<UtilityProcessWrapper> UtilityProcessWrapper::Create(
    gin::Arguments* args) {
  gin_helper::Dictionary dict;
  if (!args->GetNext(&dict)) {
    args->ThrowTypeError("Options must be an object.");
    return gin::Handle<UtilityProcessWrapper>();
  }

  std::u16string display_name;
  bool use_plugin_helper = false;
  std::map<IOHandle, IOType> stdio;
  node::mojom::NodeServiceParamsPtr params =
      node::mojom::NodeServiceParams::New();
  dict.Get("modulePath", &params->script);
  if (dict.Has("args") && !dict.Get("args", &params->args)) {
    args->ThrowTypeError("Invalid value for args");
    return gin::Handle<UtilityProcessWrapper>();
  }

  gin_helper::Dictionary opts;
  if (dict.Get("options", &opts)) {
    std::map<std::string, std::string> env_map;
    if (opts.Has("env")) {
      if (opts.Get("env", &env_map)) {
        for (auto const& env : env_map) {
          params->environment.push_back(
              node::mojom::EnvironmentVariable::New(env.first, env.second));
        }
      } else {
        args->ThrowTypeError("Invalid value for env");
        return gin::Handle<UtilityProcessWrapper>();
      }
    }

    if (opts.Has("execArgv") && !opts.Get("execArgv", &params->exec_args)) {
      args->ThrowTypeError("Invalid value for execArgv");
      return gin::Handle<UtilityProcessWrapper>();
    }

    opts.Get("serviceName", &display_name);

    std::vector<std::string> stdio_arr{"ignore", "inherit", "inherit"};
    opts.Get("stdio", &stdio_arr);
    for (size_t i = 0; i < 3; i++) {
      IOType type;
      if (stdio_arr[i] == "ignore")
        type = IOType::IGNORE;
      else if (stdio_arr[i] == "inherit")
        type = IOType::INHERIT;
      else if (stdio_arr[i] == "pipe")
        type = IOType::PIPE;

      stdio.emplace(static_cast<IOHandle>(i), type);
    }

#if BUILDFLAG(IS_MAC)
    opts.Get("allowLoadingUnsignedLibraries", &use_plugin_helper);
#endif
  }
  auto handle = gin::CreateHandle(
      args->isolate(),
      new UtilityProcessWrapper(std::move(params), display_name,
                                std::move(stdio), use_plugin_helper));
  handle->Pin(args->isolate());
  return handle;
}

// static
gin::ObjectTemplateBuilder UtilityProcessWrapper::GetObjectTemplateBuilder(
    v8::Isolate* isolate) {
  return gin_helper::EventEmitterMixin<
             UtilityProcessWrapper>::GetObjectTemplateBuilder(isolate)
      .SetMethod("postMessage", &UtilityProcessWrapper::PostMessage)
      .SetMethod("kill", &UtilityProcessWrapper::Kill)
      .SetProperty("pid", &UtilityProcessWrapper::GetOSProcessId);
}

const char* UtilityProcessWrapper::GetTypeName() {
  return "UtilityProcessWrapper";
}

}  // namespace api

}  // namespace electron

namespace {

void Initialize(v8::Local<v8::Object> exports,
                v8::Local<v8::Value> unused,
                v8::Local<v8::Context> context,
                void* priv) {
  v8::Isolate* isolate = context->GetIsolate();
  gin_helper::Dictionary dict(isolate, exports);
  dict.SetMethod("createProcessWrapper",
                 &electron::api::UtilityProcessWrapper::Create);
}

}  // namespace

NODE_LINKED_MODULE_CONTEXT_AWARE(electron_browser_utility_process, Initialize)
