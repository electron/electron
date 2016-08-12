> A fork of Chromium's [gin library][chromium-gin-lib] that makes it easier to
> marshal types between C++ and JavaScript.

# Overview

`native-mate` was forked from `gin` so that it could be used in
[Electron][electron] without conflicting with Node's Environment. It has also
been extended to allow Electron to create classes in JavaScript.

With the help of Chromium's `base` library, `native-mate` makes writing JS
bindings very easy, and most of the intricate details of converting V8 types
to C++ types and back are taken care of auto-magically. In most cases there's
no need to use the raw V8 API to implement an API binding.

For example, here's an API binding that doesn't use `native-mate`:

```c++
// static
void Shell::OpenItem(const v8::FunctionCallbackInfo<v8::Value>& args) {
  base::FilePath file_path;
  if (!FromV8Arguments(args, &file_path))
    return node::ThrowTypeError("Bad argument");

  platform_util::OpenItem(file_path);
}

// static
void Shell::Initialize(v8::Handle<v8::Object> target) {
  NODE_SET_METHOD(target, "openItem", OpenItem);
}
```

And here's the same API binding using `native-mate`:

```c++
void Initialize(v8::Handle<v8::Object> exports) {
  mate::Dictionary dict(v8::Isolate::GetCurrent(), exports);
  dict.SetMethod("openItem", &platform_util::OpenItem);
}
```

# Code Structure

* `converter.h` - Templatized JS<->C++ conversion routines for many common C++
  types. You can define your own by specializing `Converter`.
* `function_template.h` - Create JavaScript functions that dispatch to any C++
  function, member function pointer, or `base::Callback`.
* `object_template_builder.h` - A handy utility for creation of `v8::ObjectTemplate`.
* `wrappable.h` - Base class for C++ classes that want to be owned by the V8 GC.
  Wrappable objects are automatically deleted when GC discovers that nothing in
  the V8 heap refers to them. This is also an easy way to expose C++ objects to
  JavaScript.


[chromium-gin-lib]: https://code.google.com/p/chromium/codesearch#chromium/src/gin/README&sq=package:chromium
[electron]: http://electron.atom.io/
