# Electron Quick Quiz

## What does a v8:Isolate correspond to?

Isolate corresponds to a physical thread. Isolate : physical thread in Blink = 1 : 1. The main thread has its own Isolate. A worker thread has its own Isolate.

## What does a v8:Context correspond to?

Context corresponds to a global object (In case of a Frame, it's a window object of the Frame). Since each frame has its own window object, there are multiple Contexts in a renderer process. When you call V8 APIs, you have to make sure that you're in a correct context. Otherwise, v8::Isolate::GetCurrentContext() will return a wrong context and in the worst case it will end up leaking objects and causing security issues.

## What is the difference between a static and shared library in C++?

Static libraries, while reusable in multiple programs, are locked into a program at compile time. Dynamic, or shared libraries on the other hand, exist as separate files outside of the executable file.

## What is the difference between a strong and weak reference? Explain. Give an example.

Weak reference. In computer programming, a weak reference is a reference that does not protect the referenced object from collection by a garbage collector, unlike a strong reference.

http://elliot.land/post/strong-vs-weak-references

## What is libchromiumcontent?

Shared library build of Chromium’s Content module

## Why is libchromiumcontent useful in Electron? How is it used?

In Electron, the Debug version is linked with the shared_library version of libchromiumcontent, because it is small to download and takes little time when linking the final executable. And the Release version of Electron is linked with the static_library version of libchromiumcontent, so the compiler can generate full symbols which are important for debugging, and the linker can do much better optimization since it knows which object files are needed and which are not.

## What are v8 handles? What is the common common v8 handle?

V8 uses handles to point to V8 objects. The most common handle is v8::Local<>, which is used to point to V8 objects from a machine stack. v8::Local<> must be used after allocating v8::HandleScope on the machine stack. v8::Local<> should not be used outside the machine stack:

```c++
void function() {
  v8::HandleScope scope;
  v8::Local<v8::Object> object = ...;  // This is correct.
}
  
class SomeObject : public GarbageCollected<SomeObject> {
  v8::Local<v8::Object> object_;  // This is wrong.
};
```