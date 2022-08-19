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
#include "gin/handle.h"
#include "gin/object_template_builder.h"
#include "gin/wrappable.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "net/server/http_connection.h"
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

namespace api {

namespace {

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
          LOG(ERROR) << "Connection terminated while reading from pipe";
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
      ShutdownReader();
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
  StdoutPipeReader(base::WeakPtr<UtilityProcessWrapper> utility_process_wrapper,
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
        FROM_HERE, base::BindOnce(&UtilityProcessWrapper::HandleMessage,
                                  utility_process_wrapper_,
                                  UtilityProcessWrapper::ReaderType::STDOUT,
                                  std::move(message)));
  }

  void ShutdownReader() override {
    content::GetUIThreadTaskRunner({})->PostTask(
        FROM_HERE, base::BindOnce(&UtilityProcessWrapper::ShutdownReader,
                                  utility_process_wrapper_,
                                  UtilityProcessWrapper::ReaderType::STDOUT));
  }

  base::WeakPtr<UtilityProcessWrapper> utility_process_wrapper_;
};

class StderrPipeReader : public PipeReaderBase {
 public:
  StderrPipeReader(base::WeakPtr<UtilityProcessWrapper> utility_process_wrapper,
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
        FROM_HERE, base::BindOnce(&UtilityProcessWrapper::HandleMessage,
                                  utility_process_wrapper_,
                                  UtilityProcessWrapper::ReaderType::STDERR,
                                  std::move(message)));
  }

  void ShutdownReader() override {
    content::GetUIThreadTaskRunner({})->PostTask(
        FROM_HERE, base::BindOnce(&UtilityProcessWrapper::ShutdownReader,
                                  utility_process_wrapper_,
                                  UtilityProcessWrapper::ReaderType::STDERR));
  }

  base::WeakPtr<UtilityProcessWrapper> utility_process_wrapper_;
};

}  // namespace

gin::WrapperInfo UtilityProcessWrapper::kWrapperInfo = {
    gin::kEmbedderNativeGin};

UtilityProcessWrapper::UtilityProcessWrapper(
    node::mojom::NodeServiceParamsPtr params,
    std::u16string display_name,
    std::vector<std::string> stdio,
    bool use_plugin_helper) {
  DCHECK(!node_service_remote_.is_bound());

#if BUILDFLAG(IS_WIN)
  base::win::ScopedHandle stdout_write(nullptr);
  base::win::ScopedHandle stderr_write(nullptr);
  if (stdio[1] == "pipe") {
    HANDLE read = nullptr;
    HANDLE write = nullptr;
    if (!::CreatePipe(&read, &write, nullptr, 0)) {
      LOG(ERROR) << "stdout pipe creation failed";
      return;
    }
    stdout_write.Set(write);
    stdout_reader_ =
        std::make_unique<StdoutPipeReader>(weak_factory_.GetWeakPtr(), read);
    if (!stdout_reader_->Start()) {
      LOG(ERROR) << "stdout output watcher thread failed to start.";
      PipeIOBase::Shutdown(std::move(stdout_reader_));
      CloseHandle(write);
      return;
    }
  } else if (stdio[1] == "ignore") {
    HANDLE handle = CreateFileW(
        L"NUL", FILE_GENERIC_WRITE | FILE_READ_ATTRIBUTES,
        FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, 0, nullptr);
    if (handle == INVALID_HANDLE_VALUE) {
      LOG(ERROR) << "stdout failed to create null handle";
      return;
    }
    stdout_write.Set(handle);
  }

  if (stdio[2] == "pipe") {
    HANDLE read = nullptr;
    HANDLE write = nullptr;
    if (!::CreatePipe(&read, &write, nullptr, 0)) {
      LOG(ERROR) << "stderr pipe creation failed";
      return;
    }
    stderr_write.Set(write);
    stderr_reader_ =
        std::make_unique<StderrPipeReader>(weak_factory_.GetWeakPtr(), read);
    if (!stderr_reader_->Start()) {
      LOG(ERROR) << "stderr output watcher thread failed to start.";
      PipeIOBase::Shutdown(std::move(stderr_reader_));
      CloseHandle(write);
      return;
    }
  } else if (stdio[2] == "ignore") {
    HANDLE handle =
        CreateFileW(L"NUL", FILE_GENERIC_WRITE | FILE_READ_ATTRIBUTES,
                    FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, 0, nullptr);
    if (handle == INVALID_HANDLE_VALUE) {
      LOG(ERROR) << "stderr failed to create null handle";
      return;
    }
    stderr_write.Set(handle);
  }
#elif BUILDFLAG(IS_POSIX)
  base::FileHandleMappingVector fds_to_remap;
  if (stdio[1] == "pipe") {
    int stdout_pipe_fd[2];
    if (HANDLE_EINTR(pipe(stdout_pipe_fd)) < 0) {
      LOG(ERROR) << "stdout pipe creation failed";
      return;
    }
    fds_to_remap.push_back(std::make_pair(stdout_pipe_fd[1], STDOUT_FILENO));
    stdout_reader_ = std::make_unique<StdoutPipeReader>(
        weak_factory_.GetWeakPtr(), stdout_pipe_fd[0]);
    if (!stdout_reader_->Start()) {
      LOG(ERROR) << "stdout output watcher thread failed to start.";
      PipeIOBase::Shutdown(std::move(stdout_reader_));
      IGNORE_EINTR(close(stdout_pipe_fd[1]));
      return;
    }
  } else if (stdio[1] == "ignore") {
    int devnull = open("/dev/null", O_WRONLY);
    if (devnull < 0) {
      LOG(ERROR) << "stdout ignore failed to open /dev/null";
      return;
    }
    fds_to_remap.push_back(std::make_pair(devnull, STDOUT_FILENO));
  }

  if (stdio[2] == "pipe") {
    int stderr_pipe_fd[2];
    if (HANDLE_EINTR(pipe(stderr_pipe_fd)) < 0) {
      LOG(ERROR) << "stderr pipe creation failed";
      return;
    }
    fds_to_remap.push_back(std::make_pair(stderr_pipe_fd[1], STDERR_FILENO));
    stderr_reader_ = std::make_unique<StderrPipeReader>(
        weak_factory_.GetWeakPtr(), stderr_pipe_fd[0]);
    if (!stderr_reader_->Start()) {
      LOG(ERROR) << "stderr output watcher thread failed to start.";
      PipeIOBase::Shutdown(std::move(stderr_reader_));
      IGNORE_EINTR(close(stderr_pipe_fd[1]));
      return;
    }
  } else if (stdio[2] == "ignore") {
    int devnull = open("/dev/null", O_WRONLY);
    if (devnull < 0) {
      LOG(ERROR) << "stderr ignore failed to open /dev/null";
      return;
    }
    fds_to_remap.push_back(std::make_pair(devnull, STDERR_FILENO));
  }
#endif

  mojo::PendingReceiver<node::mojom::NodeService> receiver =
      node_service_remote_.BindNewPipeAndPassReceiver();

  content::ServiceProcessHost::Launch(
      std::move(receiver),
      content::ServiceProcessHost::Options()
          .WithDisplayName(display_name.empty()
                               ? std::u16string(u"Node Service")
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

UtilityProcessWrapper::~UtilityProcessWrapper() {
  if (stdout_reader_.get())
    PipeIOBase::Shutdown(std::move(stdout_reader_));
  if (stderr_reader_.get())
    PipeIOBase::Shutdown(std::move(stderr_reader_));
};

void UtilityProcessWrapper::OnServiceProcessLaunched(
    const base::Process& process) {
  DCHECK(node_service_remote_.is_connected());
  pid_ = process.Pid();
  GetAllUtilityProcessWrappers().AddWithID(this, pid_);
  // Emit 'spawn' event
  Emit("spawn");
}

void UtilityProcessWrapper::OnServiceProcessDisconnected(
    uint32_t error_code,
    const std::string& description) {
  if (GetAllUtilityProcessWrappers().Lookup(pid_))
    GetAllUtilityProcessWrappers().Remove(pid_);
  node_service_remote_.reset();
  // Emit 'exit' event
  Emit("exit", 0 /* exit_code */);
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

int UtilityProcessWrapper::Kill(int signal) const {
  if (pid_ == base::kNullProcessId)
    return 0;
  return uv_kill(pid_, signal);
}

v8::Local<v8::Value> UtilityProcessWrapper::GetOSProcessId(
    v8::Isolate* isolate) const {
  if (pid_ == base::kNullProcessId)
    return v8::Undefined(isolate);
  return gin::ConvertToV8(isolate, pid_);
}

void UtilityProcessWrapper::HandleMessage(ReaderType type,
                                          std::vector<uint8_t> message) {
  v8::Isolate* isolate = JavascriptEnvironment::GetIsolate();
  v8::HandleScope handle_scope(isolate);
  auto array_buffer = v8::ArrayBuffer::New(isolate, message.size());
  auto backing_store = array_buffer->GetBackingStore();
  memcpy(backing_store->Data(), message.data(), message.size());
  if (type == ReaderType::STDOUT) {
    Emit("stdout", array_buffer,
         base::BindRepeating(&UtilityProcessWrapper::ResumeReading,
                             weak_factory_.GetWeakPtr(), stdout_reader_.get()));
  } else {
    Emit("stderr", array_buffer,
         base::BindRepeating(&UtilityProcessWrapper::ResumeReading,
                             weak_factory_.GetWeakPtr(), stderr_reader_.get()));
  }
}

void UtilityProcessWrapper::ResumeReading(PipeReaderBase* pipe_io) {
  if (!pipe_io->is_shutting_down()) {
    pipe_io->StartReadLoop();
  }
}

void UtilityProcessWrapper::ShutdownReader(ReaderType type) {
  if (type == ReaderType::STDOUT) {
    PipeIOBase::Shutdown(std::move(stdout_reader_));
  } else {
    PipeIOBase::Shutdown(std::move(stderr_reader_));
  }
}

// static
gin::Handle<UtilityProcessWrapper> UtilityProcessWrapper::Create(
    gin::Arguments* args) {
  gin_helper::Dictionary opts;
  if (!args->GetNext(&opts)) {
    args->ThrowTypeError("Expected a dictionary");
    return gin::Handle<UtilityProcessWrapper>();
  }
  node::mojom::NodeServiceParamsPtr params =
      node::mojom::NodeServiceParams::New();
  opts.Get("modulePath", &params->script);
  opts.Get("args", &params->args);
  std::map<std::string, std::string> env_map;
  if (opts.Get("env", &env_map)) {
    for (auto const& env : env_map) {
      params->environment.push_back(
          node::mojom::EnvironmentVariable::New(env.first, env.second));
    }
  }
  opts.Get("execArgv", &params->exec_args);
  std::u16string display_name;
  opts.Get("serviceName", &display_name);
  std::vector<std::string> stdio{"ignore", "pipe", "pipe"};
  opts.Get("stdio", &stdio);
  bool use_plugin_helper = false;
#if BUILDFLAG(IS_MAC)
  opts.Get("allowLoadingUnsignedLibraries", &use_plugin_helper);
#endif
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
