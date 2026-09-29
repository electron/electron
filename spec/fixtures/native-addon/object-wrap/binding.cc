#include <node.h>
#include <node_object_wrap.h>
#include <uv.h>
#include <v8.h>

namespace {

int promise_rejection_count = 0;
int microtasks_completed_count = 0;
int interrupt_count = 0;
int use_counter_count = 0;
int import_meta_count = 0;
int uv_work_completed_count = 0;
int dynamic_import_count = 0;

void OnPromiseReject(v8::PromiseRejectMessage) {
  ++promise_rejection_count;
}

void OnMicrotasksCompleted(v8::Isolate*, void*) {
  ++microtasks_completed_count;
}

void OnInterrupt(v8::Isolate*, void*) {
  ++interrupt_count;
}

void OnUseCounter(v8::Isolate*, v8::Isolate::UseCounterFeature feature) {
  if (feature == v8::Isolate::kLegacyDateParser)
    ++use_counter_count;
}

void OnImportMeta(v8::Local<v8::Context>,
                  v8::Local<v8::Module>,
                  v8::Local<v8::Object>) {
  ++import_meta_count;
}

v8::MaybeLocal<v8::Promise> OnDynamicImport(v8::Local<v8::Context> context,
                                            v8::Local<v8::Data>,
                                            v8::Local<v8::Value>,
                                            v8::Local<v8::String>,
                                            v8::ModuleImportPhase,
                                            v8::Local<v8::FixedArray>) {
  ++dynamic_import_count;
  v8::Local<v8::Promise::Resolver> resolver =
      v8::Promise::Resolver::New(context).ToLocalChecked();
  resolver
      ->Reject(context, v8::Exception::Error(v8::String::NewFromUtf8Literal(
                            v8::Isolate::GetCurrent(),
                            "Expected dynamic import rejection")))
      .Check();
  return resolver->GetPromise();
}

void RegisterEmbedderCallbacks(
    const v8::FunctionCallbackInfo<v8::Value>& args) {
  v8::Isolate* isolate = args.GetIsolate();
  isolate->SetPromiseRejectCallback(OnPromiseReject);
  isolate->AddMicrotasksCompletedCallback(OnMicrotasksCompleted);
  isolate->RequestInterrupt(OnInterrupt, nullptr);
  if (args.Length() == 0 || args[0]->BooleanValue(isolate))
    isolate->SetUseCounterCallback(OnUseCounter);
  isolate->SetHostImportModuleWithPhaseDynamicallyCallback(OnDynamicImport);
  isolate->SetHostInitializeImportMetaObjectCallback(OnImportMeta);
}

v8::MaybeLocal<v8::Module> ResolveModule(v8::Local<v8::Context>,
                                         v8::Local<v8::String>,
                                         v8::Local<v8::FixedArray>,
                                         v8::Local<v8::Module>) {
  return {};
}

void RunImportMetaCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
  v8::Isolate* isolate = args.GetIsolate();
  v8::Local<v8::Context> context = isolate->GetCurrentContext();
  v8::ScriptOrigin origin(
      v8::String::NewFromUtf8Literal(isolate, "cfi-import-meta.mjs"), 0, 0,
      false, -1, {}, false, false, true);
  v8::ScriptCompiler::Source source(
      v8::String::NewFromUtf8Literal(isolate, "import.meta"), origin);
  v8::Local<v8::Module> module;
  if (!v8::ScriptCompiler::CompileModule(isolate, &source).ToLocal(&module) ||
      module->InstantiateModule(context, ResolveModule).IsNothing()) {
    return;
  }
  v8::Local<v8::Value> result;
  if (module->Evaluate(context).ToLocal(&result))
    args.GetReturnValue().Set(true);
}

void QueueUvWork(const v8::FunctionCallbackInfo<v8::Value>& args) {
  auto* request = new uv_work_t;
  int result = uv_queue_work(
      node::GetCurrentEventLoop(args.GetIsolate()), request, [](uv_work_t*) {},
      [](uv_work_t* request, int) {
        ++uv_work_completed_count;
        delete request;
      });
  if (result != 0) {
    delete request;
    args.GetIsolate()->ThrowException(
        v8::Exception::Error(v8::String::NewFromUtf8Literal(
            args.GetIsolate(), "Failed to queue libuv work")));
  }
}

void GetEmbedderCallbackCounts(
    const v8::FunctionCallbackInfo<v8::Value>& args) {
  v8::Isolate* isolate = args.GetIsolate();
  v8::Local<v8::Array> counts = v8::Array::New(isolate, 7);
  v8::Local<v8::Context> context = isolate->GetCurrentContext();
  counts->Set(context, 0, v8::Integer::New(isolate, promise_rejection_count))
      .Check();
  counts->Set(context, 1, v8::Integer::New(isolate, microtasks_completed_count))
      .Check();
  counts->Set(context, 2, v8::Integer::New(isolate, interrupt_count)).Check();
  counts->Set(context, 3, v8::Integer::New(isolate, use_counter_count)).Check();
  counts->Set(context, 4, v8::Integer::New(isolate, import_meta_count)).Check();
  counts->Set(context, 5, v8::Integer::New(isolate, uv_work_completed_count))
      .Check();
  counts->Set(context, 6, v8::Integer::New(isolate, dynamic_import_count))
      .Check();
  args.GetReturnValue().Set(counts);
}

// Minimal node::ObjectWrap subclass. Since Node.js 24.19.0 the ObjectWrap
// constructor and destructor register and remove an environment cleanup hook,
// so wrapping instances and letting them be garbage-collected exercises
// RemoveEnvironmentCleanupHook() from V8's weak callback, where no context is
// entered. See https://github.com/electron/electron/issues/53387.
class Wrapped final : public node::ObjectWrap {
 public:
  static void New(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    if (!args.IsConstructCall()) {
      isolate->ThrowException(v8::Exception::TypeError(
          v8::String::NewFromUtf8Literal(isolate, "Use new Wrapped()")));
      return;
    }
    Wrapped* wrapped = new Wrapped();
    wrapped->Wrap(args.This());
    args.GetReturnValue().Set(args.This());
  }
};

// Runs a full garbage collection while no context is entered, the situation
// in which V8 runs weak callbacks from platform tasks (idle-time GC, memory
// reducer). Requires the --expose-gc V8 flag, which the caller sets.
void CollectGarbageWithoutContext(
    const v8::FunctionCallbackInfo<v8::Value>& args) {
  v8::Isolate* isolate = args.GetIsolate();
  v8::Local<v8::Context> context = isolate->GetCurrentContext();
  context->Exit();
  isolate->RequestGarbageCollectionForTesting(
      v8::Isolate::kFullGarbageCollection);
  context->Enter();
}

void Initialize(v8::Local<v8::Object> exports,
                v8::Local<v8::Value> module,
                v8::Local<v8::Context> context) {
  v8::Isolate* isolate = v8::Isolate::GetCurrent();
  v8::Local<v8::FunctionTemplate> tpl =
      v8::FunctionTemplate::New(isolate, Wrapped::New);
  tpl->SetClassName(v8::String::NewFromUtf8Literal(isolate, "Wrapped"));
  tpl->InstanceTemplate()->SetInternalFieldCount(1);
  exports
      ->Set(context, v8::String::NewFromUtf8Literal(isolate, "Wrapped"),
            tpl->GetFunction(context).ToLocalChecked())
      .Check();
  exports
      ->Set(context,
            v8::String::NewFromUtf8Literal(isolate,
                                           "collectGarbageWithoutContext"),
            v8::Function::New(context, CollectGarbageWithoutContext)
                .ToLocalChecked())
      .Check();
  exports
      ->Set(
          context,
          v8::String::NewFromUtf8Literal(isolate, "registerEmbedderCallbacks"),
          v8::Function::New(context, RegisterEmbedderCallbacks)
              .ToLocalChecked())
      .Check();
  exports
      ->Set(context,
            v8::String::NewFromUtf8Literal(isolate, "embedderCallbackCounts"),
            v8::Function::New(context, GetEmbedderCallbackCounts)
                .ToLocalChecked())
      .Check();
  exports
      ->Set(context,
            v8::String::NewFromUtf8Literal(isolate, "runImportMetaCallback"),
            v8::Function::New(context, RunImportMetaCallback).ToLocalChecked())
      .Check();
  exports
      ->Set(context, v8::String::NewFromUtf8Literal(isolate, "queueUvWork"),
            v8::Function::New(context, QueueUvWork).ToLocalChecked())
      .Check();
}

}  // namespace

NODE_MODULE_INIT(/* exports, module, context */) {
  Initialize(exports, module, context);
}
