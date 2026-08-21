#include <js_native_api.h>
#include <node_api.h>
#include <v8-cppgc.h>

#include <thread>

namespace {

napi_value Print(napi_env env, napi_callback_info info) {
  size_t argc = 1;
  napi_value args[1];
  napi_status status;

  status = napi_get_cb_info(env, info, &argc, args, NULL, NULL);
  if (status != napi_ok)
    return NULL;

  if (argc > 1) {
    napi_throw_error(env, NULL,
                     "Wrong number of arguments, expected single argument");
  }

  return args[0];
}

struct AsyncEcho {
  napi_async_work work = NULL;
  napi_ref value = NULL;
  napi_ref callback = NULL;
};

void ExecuteAsyncEcho(napi_env env, void* data) {}

void CompleteAsyncEcho(napi_env env, napi_status status, void* data) {
  AsyncEcho* echo = static_cast<AsyncEcho*>(data);
  napi_value box, value, callback, global;
  napi_get_reference_value(env, echo->value, &box);
  napi_get_named_property(env, box, "value", &value);
  napi_get_reference_value(env, echo->callback, &callback);
  napi_get_global(env, &global);
  napi_call_function(env, global, callback, 1, &value, NULL);
  napi_delete_reference(env, echo->value);
  napi_delete_reference(env, echo->callback);
  napi_delete_async_work(env, echo->work);
  delete echo;
}

// Calls back with its first argument once a round trip through the libuv
// threadpool completes.
napi_value PrintAsync(napi_env env, napi_callback_info info) {
  size_t argc = 2;
  napi_value args[2];
  if (napi_get_cb_info(env, info, &argc, args, NULL, NULL) != napi_ok ||
      argc != 2) {
    napi_throw_error(env, NULL, "Expected a value and a callback");
    return NULL;
  }

  AsyncEcho* echo = new AsyncEcho();
  napi_value name, box;
  napi_create_string_utf8(env, "echo", NAPI_AUTO_LENGTH, &name);
  napi_create_object(env, &box);
  napi_set_named_property(env, box, "value", args[0]);
  napi_create_reference(env, box, 1, &echo->value);
  napi_create_reference(env, args[1], 1, &echo->callback);
  napi_create_async_work(env, NULL, name, ExecuteAsyncEcho, CompleteAsyncEcho,
                         echo, &echo->work);
  napi_queue_async_work(env, echo->work);
  return NULL;
}

struct ThreadsafeEcho {
  napi_threadsafe_function function = NULL;
  napi_ref value = NULL;
  std::thread thread;
};

void CallThreadsafeEcho(napi_env env,
                        napi_value callback,
                        void* context,
                        void* data) {
  if (env == NULL)
    return;
  ThreadsafeEcho* echo = static_cast<ThreadsafeEcho*>(context);
  napi_value box, value, global;
  napi_get_reference_value(env, echo->value, &box);
  napi_get_named_property(env, box, "value", &value);
  napi_get_global(env, &global);
  napi_call_function(env, global, callback, 1, &value, NULL);
}

void FinalizeThreadsafeEcho(napi_env env, void* data, void* hint) {
  ThreadsafeEcho* echo = static_cast<ThreadsafeEcho*>(data);
  echo->thread.join();
  napi_delete_reference(env, echo->value);
  delete echo;
}

// Calls back with its first argument through a thread-safe function invoked
// from another thread.
napi_value PrintThreadsafe(napi_env env, napi_callback_info info) {
  size_t argc = 2;
  napi_value args[2];
  if (napi_get_cb_info(env, info, &argc, args, NULL, NULL) != napi_ok ||
      argc != 2) {
    napi_throw_error(env, NULL, "Expected a value and a callback");
    return NULL;
  }

  ThreadsafeEcho* echo = new ThreadsafeEcho();
  napi_value name, box;
  napi_create_string_utf8(env, "echo", NAPI_AUTO_LENGTH, &name);
  napi_create_object(env, &box);
  napi_set_named_property(env, box, "value", args[0]);
  napi_create_reference(env, box, 1, &echo->value);
  napi_create_threadsafe_function(env, args[1], NULL, name, 0, 1, echo,
                                  FinalizeThreadsafeEcho, echo,
                                  CallThreadsafeEcho, &echo->function);
  echo->thread = std::thread([function = echo->function] {
    napi_call_threadsafe_function(function, NULL, napi_tsfn_blocking);
    napi_release_threadsafe_function(function, napi_tsfn_release);
  });
  return NULL;
}

napi_value Init(napi_env env, napi_value exports) {
  napi_status status;
  napi_property_descriptor descriptors[] = {
      {"Print", NULL, Print, NULL, NULL, NULL, napi_default, NULL},
      {"PrintAsync", NULL, PrintAsync, NULL, NULL, NULL, napi_default, NULL},
      {"PrintThreadsafe", NULL, PrintThreadsafe, NULL, NULL, NULL, napi_default,
       NULL}};

  status = napi_define_properties(
      env, exports, sizeof(descriptors) / sizeof(*descriptors), descriptors);
  if (status != napi_ok)
    return NULL;

  return exports;
}

}  // namespace

NAPI_MODULE(NODE_GYP_MODULE_NAME, Init)
