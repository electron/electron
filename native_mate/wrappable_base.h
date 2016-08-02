#ifndef NATIVE_MATE_WRAPPABLE_BASE_H_
#define NATIVE_MATE_WRAPPABLE_BASE_H_

#include "native_mate/compat.h"

namespace mate {

namespace internal {
struct Destroyable;
}

// Wrappable is a base class for C++ objects that have corresponding v8 wrapper
// objects. To retain a Wrappable object on the stack, use a gin::Handle.
//
// USAGE:
// // my_class.h
// class MyClass : Wrappable<MyClass> {
//  public:
//   ...
// };
//
// Subclasses should also typically have private constructors and expose a
// static Create function that returns a mate::Handle. Forcing creators through
// this static Create function will enforce that clients actually create a
// wrapper for the object. If clients fail to create a wrapper for a wrappable
// object, the object will leak because we use the weak callback from the
// wrapper as the signal to delete the wrapped object.
class WrappableBase {
 public:
  WrappableBase();
  virtual ~WrappableBase();

  // Retrieve the v8 wrapper object cooresponding to this object.
  v8::Local<v8::Object> GetWrapper();

  // Returns the Isolate this object is created in.
  v8::Isolate* isolate() const { return isolate_; }

 protected:
  // Called after the "_init" method gets called in JavaScript.
  virtual void AfterInit(v8::Isolate* isolate) {}

  // Bind the C++ class to the JS wrapper.
  // This method should only be called by classes using Constructor.
  virtual void InitWith(v8::Isolate* isolate, v8::Local<v8::Object> wrapper);

 private:
  friend struct internal::Destroyable;

  static void FirstWeakCallback(
      const v8::WeakCallbackInfo<WrappableBase>& data);
  static void SecondWeakCallback(
      const v8::WeakCallbackInfo<WrappableBase>& data);

  v8::Isolate* isolate_;
  v8::Global<v8::Object> wrapper_;  // Weak

  DISALLOW_COPY_AND_ASSIGN(WrappableBase);
};

}  // namespace mate

#endif  // NATIVE_MATE_WRAPPABLE_BASE_H_
