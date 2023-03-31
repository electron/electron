#include <napi.h>
#include <node_buffer.h>

using namespace Napi;

namespace {

const size_t testLength = 1024 * 1024 * sizeof(uint8_t);
int finalizeCount = 0;

void InitData(uint8_t* data, size_t length) {
  for (size_t i = 0; i < length; i++) {
    data[i] = static_cast<uint8_t>(i);
  }
}

bool VerifyData(uint8_t* data, size_t length) {
  for (size_t i = 0; i < length; i++) {
    if (data[i] != static_cast<uint8_t>(i)) {
      return false;
    }
  }
  return true;
}

Value CreateExternalBufferWithFinalize(const CallbackInfo& info) {
  v8::Isolate* isolate = v8::Isolate::GetCurrent();
  EscapableHandleScope scope(info.Env());

  uint8_t* data = new uint8_t[testLength];

  v8::MaybeLocal<v8::Object> maybe_buffer = node::Buffer::New(
      isolate, reinterpret_cast<char*>(data), testLength,
      [](char* finalizeData, void* /* hint */) {
        delete[] reinterpret_cast<uint8_t*>(finalizeData);
        finalizeCount++;
      },
      nullptr);

  v8::Local<v8::Object> buffer;
  if (!maybe_buffer.ToLocal(&buffer)) {
    Error::New(info.Env(), "A buffer could not be allocated.")
        .ThrowAsJavaScriptException();
    return scope.Escape(Value());
  }

  InitData(data, testLength);
  Value buffer_val = Value(info.Env(), reinterpret_cast<napi_value>(*buffer));
  return scope.Escape(buffer_val);
}

void CheckBuffer(const CallbackInfo& info) {
  v8::Isolate* isolate = v8::Isolate::GetCurrent();
  HandleScope scope(info.Env());

  v8::Local<v8::Value> local;
  napi_value val = info[0];
  memcpy(static_cast<void*>(&local), &val, sizeof(val));

  if (!local->IsArrayBufferView()) {
    Error::New(info.Env(), "A buffer was expected.")
        .ThrowAsJavaScriptException();
    return;
  }

  v8::Local<v8::ArrayBuffer> buffer = local.As<v8::ArrayBufferView>()->Buffer();

  if (buffer->ByteLength() != testLength) {
    Error::New(info.Env(), "Incorrect buffer length.")
        .ThrowAsJavaScriptException();
    return;
  }

  if (!VerifyData(static_cast<uint8_t*>(buffer->Data()), testLength)) {
    Error::New(info.Env(), "Incorrect buffer data.")
        .ThrowAsJavaScriptException();
    return;
  }
}

Value GetFinalizeCount(const CallbackInfo& info) {
  return Number::New(info.Env(), finalizeCount);
}

}  // end anonymous namespace

Object InitArrayBuffer(Env env) {
  Object exports = Object::New(env);

  exports["createExternalBufferWithFinalize"] =
      Function::New(env, CreateExternalBufferWithFinalize);
  exports["checkBuffer"] = Function::New(env, CheckBuffer);
  exports["getFinalizeCount"] = Function::New(env, GetFinalizeCount);

  return exports;
}

Object Init(Env env, Object exports) {
  exports.Set("arraybuffer", InitArrayBuffer(env));
  return exports;
}

NODE_API_MODULE(addon, Init)