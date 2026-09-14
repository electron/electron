#include <node.h>
#include <node_object_wrap.h>
#include <v8.h>

namespace {

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
}

}  // namespace

NODE_MODULE_INIT(/* exports, module, context */) {
  Initialize(exports, module, context);
}
