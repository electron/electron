// Copyright (c) 2026 Anthropic, PBC.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/api/electron_api_ipc_dispatch.h"

#include <initializer_list>
#include <string_view>
#include <utility>

#include "base/memory/stack_allocated.h"
#include "base/no_destructor.h"
#include "base/strings/strcat.h"
#include "base/strings/string_number_conversions.h"
#include "base/trace_event/trace_event.h"
#include "content/public/browser/frame_tree_node_id.h"
#include "gin/converter.h"
#include "gin/dictionary.h"
#include "shell/browser/api/electron_api_service_worker_context.h"
#include "shell/browser/api/electron_api_session.h"
#include "shell/browser/api/electron_api_web_contents.h"
#include "shell/browser/api/electron_api_web_frame_main.h"
#include "shell/browser/javascript_environment.h"
#include "shell/common/gin_helper/dictionary.h"
#include "shell/common/node_includes.h"

namespace electron::ipc_dispatch {

namespace {

// The JS objects dispatch needs, handed over once by
// lib/browser/ipc-dispatch.ts, plus the prototype synchronous events get for
// `returnValue`.
struct Registry {
  v8::Global<v8::Object> ipc_main;
  v8::Global<v8::Object> ipc_main_internal;
  v8::Global<v8::Function> message_port_main;  // class MessagePortMain
  // Accessor pair installed as event.returnValue on synchronous events.
  v8::Global<v8::Function> return_value_getter;
  v8::Global<v8::Function> return_value_setter;
};

Registry& GetRegistry() {
  static base::NoDestructor<Registry> registry;
  return *registry;
}

v8::Local<v8::String> Str(v8::Isolate* isolate, std::string_view s) {
  return gin::StringToSymbol(isolate, s);
}

v8::Local<v8::Value> GetProperty(v8::Local<v8::Context> context,
                                 v8::Local<v8::Object> object,
                                 std::string_view name) {
  v8::Local<v8::Value> value;
  if (object->Get(context, Str(JavascriptEnvironment::GetIsolate(), name))
          .ToLocal(&value))
    return value;
  return v8::Undefined(JavascriptEnvironment::GetIsolate());
}

// object[method](...argv); empty if the property is not callable or the call
// threw.
v8::MaybeLocal<v8::Value> CallMethod(v8::Local<v8::Context> context,
                                     v8::Local<v8::Object> object,
                                     std::string_view method,
                                     int argc,
                                     v8::Local<v8::Value>* argv) {
  v8::Local<v8::Value> fn = GetProperty(context, object, method);
  if (!fn->IsFunction())
    return {};
  return fn.As<v8::Function>()->Call(context, object, argc, argv);
}

// console[level](...argv) in the main process.
void Console(v8::Local<v8::Context> context,
             std::string_view level,
             std::initializer_list<v8::Local<v8::Value>> args) {
  v8::LocalVector<v8::Value> argv(JavascriptEnvironment::GetIsolate(), args);
  v8::Local<v8::Value> console =
      GetProperty(context, context->Global(), "console");
  if (console->IsObject()) {
    std::ignore = CallMethod(context, console.As<v8::Object>(), level,
                             argv.size(), argv.data());
  }
}

// target.emit(...argv). Returns emit()'s result (true if there were
// listeners); empty if a listener threw.
v8::MaybeLocal<v8::Value> Emit(v8::Local<v8::Context> context,
                               v8::Local<v8::Object> target,
                               v8::LocalVector<v8::Value>& argv) {
  v8::Local<v8::Value> emit = GetProperty(context, target, "emit");
  if (!emit->IsFunction())
    return v8::Undefined(JavascriptEnvironment::GetIsolate());
  return emit.As<v8::Function>()->Call(context, target, argv.size(),
                                       argv.data());
}

// The `ipc` emitter of |holder| (a WebContents, WebFrameMain or
// ServiceWorkerMain wrapper), if it evaluates to an object.
v8::MaybeLocal<v8::Object> IpcOf(v8::Local<v8::Context> context,
                                 v8::Local<v8::Object> holder) {
  v8::Local<v8::Value> ipc = GetProperty(context, holder, "ipc");
  if (ipc->IsObject())
    return ipc.As<v8::Object>();
  return {};
}

// argv = [channel, event, ...args], or [event, ...args] without |channel|.
void BuildArgv(v8::Local<v8::Context> context,
               const std::string* channel,
               v8::Local<v8::Object> event,
               v8::Local<v8::Value> args,
               v8::LocalVector<v8::Value>* argv) {
  v8::Isolate* isolate = JavascriptEnvironment::GetIsolate();
  if (channel)
    argv->push_back(gin::StringToV8(isolate, *channel));
  argv->push_back(event);
  if (args->IsArray()) {
    v8::Local<v8::Array> array = args.As<v8::Array>();
    const uint32_t length = array->Length();
    argv->reserve(2 + length);
    for (uint32_t i = 0; i < length; ++i) {
      v8::Local<v8::Value> item;
      if (!array->Get(context, i).ToLocal(&item))
        item = v8::Undefined(isolate);
      argv->push_back(item);
    }
  }
}

// ---- event decorations ------------------------------------------------

// event._replyChannel.sendReply(value)
void SendReply(v8::Local<v8::Context> context,
               v8::Local<v8::Object> event,
               v8::Local<v8::Value> value) {
  v8::Local<v8::Value> channel = GetProperty(context, event, "_replyChannel");
  if (channel->IsObject()) {
    std::ignore =
        CallMethod(context, channel.As<v8::Object>(), "sendReply", 1, &value);
  }
}

void ReturnValueGetter(const v8::FunctionCallbackInfo<v8::Value>& info) {
  info.GetReturnValue().SetUndefined();
}

void ReturnValueSetter(const v8::FunctionCallbackInfo<v8::Value>& info) {
  v8::Isolate* isolate = info.GetIsolate();
  if (info.Length() < 1 || !info.This()->IsObject())
    return;
  SendReply(isolate->GetCurrentContext(), info.This().As<v8::Object>(),
            info[0]);
}

// event.returnValue = x replies to a synchronous message: an own,
// non-enumerable accessor, as the JS dispatcher defined it.
void AddReturnValue(v8::Local<v8::Context> context,
                    v8::Local<v8::Object> event) {
  v8::Isolate* isolate = JavascriptEnvironment::GetIsolate();
  Registry& registry = GetRegistry();
  if (registry.return_value_getter.IsEmpty()) {
    registry.return_value_getter.Reset(
        isolate,
        v8::Function::New(context, ReturnValueGetter).ToLocalChecked());
    registry.return_value_setter.Reset(
        isolate,
        v8::Function::New(context, ReturnValueSetter).ToLocalChecked());
  }
  event->SetAccessorProperty(
      Str(isolate, "returnValue"), registry.return_value_getter.Get(isolate),
      registry.return_value_setter.Get(isolate), v8::DontEnum);
}

// event.reply(channel, ...args) =>
//     event.sender.sendToFrame([event.processId, event.frameId], channel,
//                              ...args)
void ReplyImpl(const v8::FunctionCallbackInfo<v8::Value>& info) {
  v8::Isolate* isolate = info.GetIsolate();
  v8::Local<v8::Context> context = isolate->GetCurrentContext();
  if (!info.Data()->IsObject())
    return;
  v8::Local<v8::Object> event = info.Data().As<v8::Object>();
  v8::Local<v8::Value> sender = GetProperty(context, event, "sender");
  if (!sender->IsObject())
    return;
  v8::Local<v8::Value> frame_parts[] = {
      GetProperty(context, event, "processId"),
      GetProperty(context, event, "frameId")};
  v8::LocalVector<v8::Value> argv(isolate);
  argv.reserve(info.Length() + 1);
  argv.push_back(v8::Array::New(isolate, frame_parts, 2));
  for (int i = 0; i < info.Length(); ++i)
    argv.push_back(info[i]);
  std::ignore = CallMethod(context, sender.As<v8::Object>(), "sendToFrame",
                           argv.size(), argv.data());
}

// An own data property (so it enumerates and spreads like the closure the JS
// dispatcher used to assign) whose bound function is only created on read.
void ReplyGetter(v8::Local<v8::Name> name,
                 const v8::PropertyCallbackInfo<v8::Value>& info) {
  v8::Isolate* isolate = info.GetIsolate();
  v8::Local<v8::Context> context = isolate->GetCurrentContext();
  v8::Local<v8::Function> reply;
  if (v8::Function::New(context, ReplyImpl, info.Holder(), 0,
                        v8::ConstructorBehavior::kThrow)
          .ToLocal(&reply)) {
    info.GetReturnValue().Set(reply);
  }
}

void AddReply(v8::Local<v8::Context> context, v8::Local<v8::Object> event) {
  std::ignore = event->SetLazyDataProperty(
      context, Str(JavascriptEnvironment::GetIsolate(), "reply"), ReplyGetter);
}

// event.serviceWorker => event.session.serviceWorkers
//                            .getWorkerFromVersionID(event.versionId)
void ServiceWorkerGetter(v8::Local<v8::Name> name,
                         const v8::PropertyCallbackInfo<v8::Value>& info) {
  v8::Isolate* isolate = info.GetIsolate();
  v8::Local<v8::Context> context = isolate->GetCurrentContext();
  v8::Local<v8::Object> event = info.Holder();
  v8::Local<v8::Value> session = GetProperty(context, event, "session");
  if (!session->IsObject())
    return;
  v8::Local<v8::Value> workers =
      GetProperty(context, session.As<v8::Object>(), "serviceWorkers");
  if (!workers->IsObject())
    return;
  v8::Local<v8::Value> version_id = GetProperty(context, event, "versionId");
  v8::Local<v8::Value> worker;
  if (CallMethod(context, workers.As<v8::Object>(), "getWorkerFromVersionID", 1,
                 &version_id)
          .ToLocal(&worker)) {
    info.GetReturnValue().Set(worker);
  }
}

void AddServiceWorkerProperty(v8::Local<v8::Context> context,
                              v8::Local<v8::Object> event) {
  // Re-evaluated on each access, as the accessor the JS dispatcher defined.
  std::ignore = event->SetNativeDataProperty(
      context, Str(JavascriptEnvironment::GetIsolate(), "serviceWorker"),
      ServiceWorkerGetter, nullptr, v8::Local<v8::Value>(), v8::DontEnum);
}

// event.ports = ports.map((p) => new MessagePortMain(p))
bool AddPorts(v8::Local<v8::Context> context,
              v8::Local<v8::Object> event,
              const v8::LocalVector<v8::Value>& ports) {
  v8::Isolate* isolate = JavascriptEnvironment::GetIsolate();
  v8::Local<v8::Function> ctor = GetRegistry().message_port_main.Get(isolate);
  v8::LocalVector<v8::Value> wrapped(isolate);
  wrapped.reserve(ports.size());
  for (v8::Local<v8::Value> port : ports) {
    v8::Local<v8::Object> instance;
    if (!ctor->NewInstance(context, 1, &port).ToLocal(&instance))
      return false;
    wrapped.push_back(instance);
  }
  return event
      ->Set(context, Str(isolate, "ports"),
            v8::Array::New(isolate, wrapped.data(), wrapped.size()))
      .FromMaybe(false);
}

// ---- invoke ------------------------------------------------------------

// Data carried to the promise reactions of an invoke handler's result.
v8::Local<v8::Object> MakeInvokeReplyData(v8::Local<v8::Context> context,
                                          v8::Local<v8::Object> event,
                                          v8::Local<v8::Value> channel) {
  v8::Isolate* isolate = JavascriptEnvironment::GetIsolate();
  v8::Local<v8::Object> data = v8::Object::New(isolate);
  std::ignore = data->Set(context, Str(isolate, "event"), event);
  std::ignore = data->Set(context, Str(isolate, "channel"), channel);
  return data;
}

void ReplyWithError(v8::Local<v8::Context> context,
                    v8::Local<v8::Object> event,
                    v8::Local<v8::Value> channel,
                    v8::Local<v8::Value> error) {
  v8::Isolate* isolate = JavascriptEnvironment::GetIsolate();
  std::string channel_name;
  gin::ConvertFromV8(isolate, channel, &channel_name);
  Console(
      context, "error",
      {gin::StringToV8(isolate, base::StrCat({"Error occurred in handler for '",
                                              channel_name, "':"})),
       error});
  // error.toString(), which unlike abstract ToString also works for a Symbol;
  // anything that still throws is reported as a plain "Error".
  v8::Local<v8::Value> message;
  {
    v8::TryCatch try_catch(isolate);
    v8::Local<v8::Value> string;
    if (error->IsObject() &&
        CallMethod(context, error.As<v8::Object>(), "toString", 0, nullptr)
            .ToLocal(&string) &&
        string->IsString()) {
      message = string;
    } else if (!try_catch.HasCaught() && !error->IsObject() &&
               error->ToDetailString(context).ToLocal(&string)) {
      message = string;
    } else {
      message = Str(isolate, "Error");
    }
    if (try_catch.HasCaught() && !try_catch.HasTerminated())
      try_catch.Reset();
  }
  v8::Local<v8::Object> reply = v8::Object::New(isolate);
  std::ignore = reply->Set(context, Str(isolate, "error"), message);
  SendReply(context, event, reply);
}

// Replies {result}, or {error} if the result cannot be serialised (the
// "An object could not be cloned" case), as the JS try/catch did.
void ReplyWithResult(v8::Local<v8::Context> context,
                     v8::Local<v8::Object> event,
                     v8::Local<v8::Value> channel,
                     v8::Local<v8::Value> result) {
  v8::Isolate* isolate = JavascriptEnvironment::GetIsolate();
  v8::Local<v8::Object> reply = v8::Object::New(isolate);
  std::ignore = reply->Set(context, Str(isolate, "result"), result);
  v8::TryCatch try_catch(isolate);
  SendReply(context, event, reply);
  if (try_catch.HasCaught() && !try_catch.HasTerminated()) {
    v8::Local<v8::Value> error = try_catch.Exception();
    try_catch.Reset();
    ReplyWithError(context, event, channel, error);
  }
}

void OnInvokeFulfilled(const v8::FunctionCallbackInfo<v8::Value>& info) {
  v8::Local<v8::Context> context = info.GetIsolate()->GetCurrentContext();
  v8::Local<v8::Object> data = info.Data().As<v8::Object>();
  ReplyWithResult(context, GetProperty(context, data, "event").As<v8::Object>(),
                  GetProperty(context, data, "channel"), info[0]);
}

void OnInvokeRejected(const v8::FunctionCallbackInfo<v8::Value>& info) {
  v8::Local<v8::Context> context = info.GetIsolate()->GetCurrentContext();
  v8::Local<v8::Object> data = info.Data().As<v8::Object>();
  ReplyWithError(context, GetProperty(context, data, "event").As<v8::Object>(),
                 GetProperty(context, data, "channel"), info[0]);
}

// The first of |targets| whose _invokeHandlers map has |channel|.
v8::Local<v8::Value> FindInvokeHandler(v8::Local<v8::Context> context,
                                       v8::LocalVector<v8::Object>& targets,
                                       v8::Local<v8::Value> channel) {
  for (v8::Local<v8::Object> target : targets) {
    v8::Local<v8::Value> handlers =
        GetProperty(context, target, "_invokeHandlers");
    if (!handlers->IsMap())
      continue;
    v8::Local<v8::Value> handler;
    if (handlers.As<v8::Map>()->Get(context, channel).ToLocal(&handler) &&
        handler->IsFunction()) {
      return handler;
    }
  }
  return v8::Undefined(JavascriptEnvironment::GetIsolate());
}

// Calls the handler and replies with {result} once its (possibly promised)
// value settles, or {error} if there is none or it throws/rejects.
void RunInvokeHandler(v8::Local<v8::Context> context,
                      v8::LocalVector<v8::Object>& targets,
                      v8::Local<v8::Object> event,
                      const std::string& channel,
                      v8::Local<v8::Value> args) {
  v8::Isolate* isolate = JavascriptEnvironment::GetIsolate();
  v8::Local<v8::Value> channel_value = gin::StringToV8(isolate, channel);
  v8::Local<v8::Value> handler =
      FindInvokeHandler(context, targets, channel_value);
  if (!handler->IsFunction()) {
    ReplyWithError(context, event, channel_value,
                   v8::Exception::Error(gin::StringToV8(
                       isolate, base::StrCat({"No handler registered for '",
                                              channel, "'"}))));
    return;
  }

  // handler(event, ...args)
  v8::LocalVector<v8::Value> argv(isolate);
  BuildArgv(context, nullptr, event, args, &argv);
  v8::TryCatch try_catch(isolate);
  v8::Local<v8::Value> result;
  if (!handler.As<v8::Function>()
           ->Call(context, v8::Undefined(isolate), argv.size(), argv.data())
           .ToLocal(&result)) {
    if (try_catch.HasCaught() && !try_catch.HasTerminated()) {
      v8::Local<v8::Value> error = try_catch.Exception();
      try_catch.Reset();
      ReplyWithError(context, event, channel_value, error);
    }
    return;
  }

  // Promise.resolve(result).then(reply, replyWithError)
  v8::Local<v8::Promise::Resolver> resolver;
  if (!v8::Promise::Resolver::New(context).ToLocal(&resolver) ||
      resolver->Resolve(context, result).IsNothing()) {
    return;
  }
  v8::Local<v8::Object> data =
      MakeInvokeReplyData(context, event, channel_value);
  v8::Local<v8::Function> on_fulfilled, on_rejected;
  if (!v8::Function::New(context, OnInvokeFulfilled, data, 1,
                         v8::ConstructorBehavior::kThrow)
           .ToLocal(&on_fulfilled) ||
      !v8::Function::New(context, OnInvokeRejected, data, 1,
                         v8::ConstructorBehavior::kThrow)
           .ToLocal(&on_rejected)) {
    return;
  }
  std::ignore =
      resolver->GetPromise()->Then(context, on_fulfilled, on_rejected);
}

// ---- observers -----------------------------------------------------------

// The private '-ipc-*' Session events the JS dispatcher used to be driven by
// are still emitted, after delivery, when something listens for them (IPC
// inspectors such as Devtron do). Decided from the emitter's listener table
// without calling into JS.
void NotifySessionObservers(v8::Local<v8::Context> context,
                            v8::Local<v8::Value> session,
                            const char* event_name,
                            std::initializer_list<v8::Local<v8::Value>> args) {
  if (!session->IsObject())
    return;
  v8::Isolate* isolate = JavascriptEnvironment::GetIsolate();
  v8::LocalVector<v8::Value> argv(isolate, args);
  v8::Local<v8::Object> session_object = session.As<v8::Object>();
  v8::Local<v8::String> name = Str(isolate, event_name);
  v8::Local<v8::Value> events = GetProperty(context, session_object, "_events");
  if (!events->IsObject() || !events.As<v8::Object>()
                                  ->HasOwnProperty(context, name)
                                  .FromMaybe(false)) {
    return;
  }
  argv.insert(argv.begin(), name);
  std::ignore = Emit(context, session_object, argv);
}

// ---- dispatch --------------------------------------------------------------

// One node callback scope around a whole dispatch: pending microtasks and
// ticks run when it closes, once, as they did when a single JS listener did
// the fan-out.
class DispatchScope {
  STACK_ALLOCATED();

 public:
  DispatchScope(v8::Isolate* isolate, v8::Local<v8::Object> resource)
      : context_scope_(resource->GetCreationContextChecked(isolate)),
        callback_scope_(isolate, resource, node::async_context{0, 0}),
        microtasks_scope_(resource->GetCreationContextChecked(isolate),
                          v8::MicrotasksScope::kRunMicrotasks) {}

  static bool Possible(v8::Isolate* isolate, v8::Local<v8::Object> resource) {
    v8::Local<v8::Context> context;
    return resource->GetCreationContext(isolate).ToLocal(&context) &&
           node::Environment::GetCurrent(context) != nullptr;
  }

 private:
  v8::Context::Scope context_scope_;
  node::CallbackScope callback_scope_;
  v8::MicrotasksScope microtasks_scope_;
};

// The emitters a non-internal frame IPC is delivered to after the
// WebContents itself, in dispatch order: the sending frame's ipc (looked up
// by frame tree node so a frame that is mid-swap still receives it), the
// WebContents' ipc, then ipcMain.
void CollectFrameTargets(v8::Local<v8::Context> context,
                         v8::Local<v8::Object> sender,
                         int frame_tree_node_id,
                         v8::LocalVector<v8::Object>* out) {
  v8::Isolate* isolate = JavascriptEnvironment::GetIsolate();
  if (frame_tree_node_id) {
    if (auto* frame = api::WebFrameMain::FromFrameTreeNodeId(
            content::FrameTreeNodeId(frame_tree_node_id))) {
      v8::Local<v8::Object> wrapper;
      v8::Local<v8::Object> ipc;
      if (frame->GetWrapper(isolate).ToLocal(&wrapper) &&
          IpcOf(context, wrapper).ToLocal(&ipc)) {
        out->push_back(ipc);
      }
    }
  }
  v8::Local<v8::Object> wc_ipc;
  if (IpcOf(context, sender).ToLocal(&wc_ipc))
    out->push_back(wc_ipc);
  out->push_back(GetRegistry().ipc_main.Get(isolate));
}

// The worker's ServiceWorkerMain.ipc, if the worker (still) exists.
void CollectServiceWorkerTargets(v8::Local<v8::Context> context,
                                 api::Session* session,
                                 int64_t version_id,
                                 v8::LocalVector<v8::Object>* out) {
  v8::Isolate* isolate = JavascriptEnvironment::GetIsolate();
  api::ServiceWorkerContext* workers = session->ServiceWorkerContext();
  if (!workers)
    return;
  v8::Local<v8::Value> worker =
      workers->GetWorkerFromVersionIDIfExists(isolate, version_id);
  v8::Local<v8::Object> ipc;
  if (!worker.IsEmpty() && worker->IsObject() &&
      IpcOf(context, worker.As<v8::Object>()).ToLocal(&ipc)) {
    out->push_back(ipc);
  }
}

}  // namespace

bool IsReady() {
  const Registry& registry = GetRegistry();
  return !registry.ipc_main.IsEmpty() &&
         !registry.ipc_main_internal.IsEmpty() &&
         !registry.message_port_main.IsEmpty();
}

// ---- frames ---------------------------------------------------------------

void Message(v8::Isolate* isolate,
             api::WebContents* sender,
             v8::Local<v8::Object> event,
             bool internal,
             int frame_tree_node_id,
             const std::string& channel,
             v8::Local<v8::Value> args,
             bool sync) {
  TRACE_EVENT1("electron",
               sync ? "IpcDispatcher::MessageSync" : "IpcDispatcher::Message",
               "channel", channel);
  v8::HandleScope handle_scope(isolate);
  // |sender| may be destroyed by a listener below (webContents.destroy() on
  // a <webview> guest deletes it synchronously); only V8 handles are used
  // once JS has run.
  v8::Local<v8::Object> sender_wrapper;
  if (!sender->GetWrapper(isolate).ToLocal(&sender_wrapper) ||
      !DispatchScope::Possible(isolate, sender_wrapper)) {
    return;
  }
  v8::Local<v8::Value> session = sender->Session(isolate);
  const int32_t sender_id = sender->ID();
  DispatchScope dispatch_scope(isolate, sender_wrapper);
  v8::Local<v8::Context> context = isolate->GetCurrentContext();
  Registry& registry = GetRegistry();

  if (sync)
    AddReturnValue(context, event);
  if (!internal)
    AddReply(context, event);

  v8::LocalVector<v8::Value> argv(isolate);
  BuildArgv(context, &channel, event, args, &argv);
  const char* session_event = sync ? "-ipc-message-sync" : "-ipc-message";

  if (internal) {
    if (Emit(context, registry.ipc_main_internal.Get(isolate), argv).IsEmpty())
      return;
    NotifySessionObservers(context, session, session_event,
                           {event, argv[0], args});
    return;
  }

  bool handled = false;
  // webContents.emit('ipc-message[-sync]', event, channel, ...args)
  {
    v8::LocalVector<v8::Value> wc_argv(isolate);
    wc_argv.reserve(argv.size() + 1);
    wc_argv.push_back(Str(isolate, sync ? "ipc-message-sync" : "ipc-message"));
    wc_argv.push_back(event);
    wc_argv.push_back(argv[0]);
    wc_argv.insert(wc_argv.end(), argv.begin() + 2, argv.end());
    v8::Local<v8::Value> result;
    if (!Emit(context, sender_wrapper, wc_argv).ToLocal(&result))
      return;  // a listener threw; leave the exception to propagate
    handled = result->IsTrue();
  }
  // Looked up after the WebContents listeners ran, as before, so an ipc
  // emitter one of them creates (e.g. event.senderFrame.ipc) still receives
  // this message.
  v8::LocalVector<v8::Object> targets(isolate);
  CollectFrameTargets(context, sender_wrapper, frame_tree_node_id, &targets);
  for (v8::Local<v8::Object> target : targets) {
    v8::Local<v8::Value> result;
    if (!Emit(context, target, argv).ToLocal(&result))
      return;
    handled = handled || result->IsTrue();
  }

  if (sync && !handled) {
    v8::Local<v8::String> warning = gin::StringToV8(
        isolate, base::StrCat({"WebContents #", base::NumberToString(sender_id),
                               " called ipcRenderer.sendSync() with '", channel,
                               "' channel without listeners."}));
    Console(context, "warn", {warning});
  }
  NotifySessionObservers(context, session, session_event,
                         {event, argv[0], args});
}

void Invoke(v8::Isolate* isolate,
            api::WebContents* sender,
            v8::Local<v8::Object> event,
            bool internal,
            int frame_tree_node_id,
            const std::string& channel,
            v8::Local<v8::Value> args) {
  TRACE_EVENT1("electron", "IpcDispatcher::Invoke", "channel", channel);
  v8::HandleScope handle_scope(isolate);
  v8::Local<v8::Object> sender_wrapper;
  if (!sender->GetWrapper(isolate).ToLocal(&sender_wrapper) ||
      !DispatchScope::Possible(isolate, sender_wrapper)) {
    return;
  }
  v8::Local<v8::Value> session = sender->Session(isolate);
  DispatchScope dispatch_scope(isolate, sender_wrapper);
  v8::Local<v8::Context> context = isolate->GetCurrentContext();
  Registry& registry = GetRegistry();

  v8::LocalVector<v8::Object> targets(isolate);
  if (internal) {
    targets.push_back(registry.ipc_main_internal.Get(isolate));
  } else {
    CollectFrameTargets(context, sender_wrapper, frame_tree_node_id, &targets);
  }
  RunInvokeHandler(context, targets, event, channel, args);
  NotifySessionObservers(context, session, "-ipc-invoke",
                         {event, gin::StringToV8(isolate, channel), args});
}

void PostMessage(v8::Isolate* isolate,
                 api::WebContents* sender,
                 v8::Local<v8::Object> event,
                 int frame_tree_node_id,
                 const std::string& channel,
                 v8::Local<v8::Value> message,
                 v8::LocalVector<v8::Value> ports) {
  TRACE_EVENT1("electron", "IpcDispatcher::ReceivePostMessage", "channel",
               channel);
  v8::HandleScope handle_scope(isolate);
  v8::Local<v8::Object> sender_wrapper;
  if (!sender->GetWrapper(isolate).ToLocal(&sender_wrapper) ||
      !DispatchScope::Possible(isolate, sender_wrapper)) {
    return;
  }
  v8::Local<v8::Value> session = sender->Session(isolate);
  DispatchScope dispatch_scope(isolate, sender_wrapper);
  v8::Local<v8::Context> context = isolate->GetCurrentContext();

  v8::Local<v8::Value> native_ports =
      v8::Array::New(isolate, ports.data(), ports.size());
  if (!AddPorts(context, event, ports))
    return;
  v8::LocalVector<v8::Object> targets(isolate);
  CollectFrameTargets(context, sender_wrapper, frame_tree_node_id, &targets);
  v8::LocalVector<v8::Value> argv(
      isolate, {gin::StringToV8(isolate, channel), event, message});
  for (v8::Local<v8::Object> target : targets) {
    if (Emit(context, target, argv).IsEmpty())
      return;
  }
  NotifySessionObservers(context, session, "-ipc-ports",
                         {event, argv[0], message, native_ports});
}

void MessageHost(v8::Isolate* isolate,
                 api::WebContents* sender,
                 v8::Local<v8::Object> event,
                 const std::string& channel,
                 v8::Local<v8::Value> args) {
  TRACE_EVENT1("electron", "IpcDispatcher::MessageHost", "channel", channel);
  v8::HandleScope handle_scope(isolate);
  v8::Local<v8::Object> sender_wrapper;
  if (!sender->GetWrapper(isolate).ToLocal(&sender_wrapper) ||
      !DispatchScope::Possible(isolate, sender_wrapper)) {
    return;
  }
  v8::Local<v8::Value> session = sender->Session(isolate);
  DispatchScope dispatch_scope(isolate, sender_wrapper);
  v8::Local<v8::Context> context = isolate->GetCurrentContext();
  v8::Local<v8::Value> channel_value = gin::StringToV8(isolate, channel);
  v8::LocalVector<v8::Value> argv(
      isolate, {Str(isolate, "-ipc-message-host"), event, channel_value, args});
  if (Emit(context, sender_wrapper, argv).IsEmpty())
    return;
  NotifySessionObservers(context, session, "-ipc-message-host",
                         {event, channel_value, args});
}

// ---- service workers -------------------------------------------------------

void ServiceWorkerMessage(v8::Isolate* isolate,
                          api::Session* session,
                          int64_t version_id,
                          v8::Local<v8::Object> event,
                          bool internal,
                          const std::string& channel,
                          v8::Local<v8::Value> args,
                          bool sync) {
  TRACE_EVENT1("electron",
               sync ? "IpcDispatcher::MessageSync" : "IpcDispatcher::Message",
               "channel", channel);
  v8::HandleScope handle_scope(isolate);
  v8::Local<v8::Object> session_wrapper;
  if (!session->GetWrapper(isolate).ToLocal(&session_wrapper) ||
      !DispatchScope::Possible(isolate, session_wrapper)) {
    return;
  }
  DispatchScope dispatch_scope(isolate, session_wrapper);
  v8::Local<v8::Context> context = isolate->GetCurrentContext();
  Registry& registry = GetRegistry();

  if (sync)
    AddReturnValue(context, event);
  v8::LocalVector<v8::Object> targets(isolate);
  if (internal) {
    targets.push_back(registry.ipc_main_internal.Get(isolate));
  } else {
    AddServiceWorkerProperty(context, event);
    CollectServiceWorkerTargets(context, session, version_id, &targets);
  }
  v8::LocalVector<v8::Value> argv(isolate);
  BuildArgv(context, &channel, event, args, &argv);
  for (v8::Local<v8::Object> target : targets) {
    if (Emit(context, target, argv).IsEmpty())
      return;
  }
  NotifySessionObservers(context, session_wrapper,
                         sync ? "-ipc-message-sync" : "-ipc-message",
                         {event, argv[0], args});
}

void ServiceWorkerInvoke(v8::Isolate* isolate,
                         api::Session* session,
                         int64_t version_id,
                         v8::Local<v8::Object> event,
                         bool internal,
                         const std::string& channel,
                         v8::Local<v8::Value> args) {
  TRACE_EVENT1("electron", "IpcDispatcher::Invoke", "channel", channel);
  v8::HandleScope handle_scope(isolate);
  v8::Local<v8::Object> session_wrapper;
  if (!session->GetWrapper(isolate).ToLocal(&session_wrapper) ||
      !DispatchScope::Possible(isolate, session_wrapper)) {
    return;
  }
  DispatchScope dispatch_scope(isolate, session_wrapper);
  v8::Local<v8::Context> context = isolate->GetCurrentContext();
  Registry& registry = GetRegistry();

  v8::LocalVector<v8::Object> targets(isolate);
  if (internal) {
    targets.push_back(registry.ipc_main_internal.Get(isolate));
  } else {
    AddServiceWorkerProperty(context, event);
    CollectServiceWorkerTargets(context, session, version_id, &targets);
  }
  RunInvokeHandler(context, targets, event, channel, args);
  NotifySessionObservers(context, session_wrapper, "-ipc-invoke",
                         {event, gin::StringToV8(isolate, channel), args});
}

void ServiceWorkerPostMessage(v8::Isolate* isolate,
                              api::Session* session,
                              int64_t version_id,
                              v8::Local<v8::Object> event,
                              const std::string& channel,
                              v8::Local<v8::Value> message,
                              v8::LocalVector<v8::Value> ports) {
  TRACE_EVENT1("electron", "IpcDispatcher::ReceivePostMessage", "channel",
               channel);
  v8::HandleScope handle_scope(isolate);
  v8::Local<v8::Object> session_wrapper;
  if (!session->GetWrapper(isolate).ToLocal(&session_wrapper) ||
      !DispatchScope::Possible(isolate, session_wrapper)) {
    return;
  }
  DispatchScope dispatch_scope(isolate, session_wrapper);
  v8::Local<v8::Context> context = isolate->GetCurrentContext();

  v8::Local<v8::Value> native_ports =
      v8::Array::New(isolate, ports.data(), ports.size());
  if (!AddPorts(context, event, ports))
    return;
  AddServiceWorkerProperty(context, event);
  v8::LocalVector<v8::Object> targets(isolate);
  CollectServiceWorkerTargets(context, session, version_id, &targets);
  v8::LocalVector<v8::Value> argv(
      isolate, {gin::StringToV8(isolate, channel), event, message});
  for (v8::Local<v8::Object> target : targets) {
    if (Emit(context, target, argv).IsEmpty())
      return;
  }
  NotifySessionObservers(context, session_wrapper, "-ipc-ports",
                         {event, argv[0], message, native_ports});
}

}  // namespace electron::ipc_dispatch

namespace {

void Setup(v8::Isolate* isolate, const gin_helper::Dictionary& options) {
  auto& registry = electron::ipc_dispatch::GetRegistry();
  v8::Local<v8::Object> object;
  v8::Local<v8::Function> function;
  if (options.Get("ipcMain", &object))
    registry.ipc_main.Reset(isolate, object);
  if (options.Get("ipcMainInternal", &object))
    registry.ipc_main_internal.Reset(isolate, object);
  if (options.Get("MessagePortMain", &function))
    registry.message_port_main.Reset(isolate, function);
}

void Initialize(v8::Local<v8::Object> exports,
                v8::Local<v8::Value> unused,
                v8::Local<v8::Context> context,
                void* priv) {
  v8::Isolate* const isolate = electron::JavascriptEnvironment::GetIsolate();
  gin_helper::Dictionary dict{isolate, exports};
  dict.SetMethod("setup", &Setup);
}

}  // namespace

NODE_LINKED_BINDING_CONTEXT_AWARE(electron_browser_ipc_dispatch, Initialize)
