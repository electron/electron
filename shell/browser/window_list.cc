// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/window_list.h"

#include <algorithm>

#include "base/containers/to_vector.h"
#include "base/no_destructor.h"
#include "shell/browser/native_window.h"
#include "shell/browser/window_list_observer.h"

namespace {

template <typename T>
auto ConvertToWeakPtrVector(const std::vector<T*>& raw_ptrs) {
  return base::ToVector(raw_ptrs, [](T* t) { return t->GetWeakPtr(); });
}

}  // namespace

namespace electron {

// static
WindowList* WindowList::instance_ = nullptr;

// static
WindowList* WindowList::GetInstance() {
  if (!instance_)
    instance_ = new WindowList;
  return instance_;
}

// static
WindowList::WindowVector WindowList::GetWindows() {
  return GetInstance()->windows_;
}

// static
bool WindowList::IsEmpty() {
  return GetInstance()->windows_.empty();
}

// static
void WindowList::AddWindow(NativeWindow* window) {
  DCHECK(window);
  // Push |window| on the appropriate list instance.
  WindowVector& windows = GetInstance()->windows_;
  windows.push_back(window);
}

// static
void WindowList::RemoveWindow(NativeWindow* window) {
  WindowVector& windows = GetInstance()->windows_;
  std::erase(windows, window);

  if (windows.empty())
    GetObservers().Notify(&WindowListObserver::OnWindowAllClosed);
}

// static
void WindowList::WindowCloseCancelled(NativeWindow* window) {
  GetObservers().Notify(&WindowListObserver::OnWindowCloseCancelled, window);
}

// static
void WindowList::AddObserver(WindowListObserver* observer) {
  GetObservers().AddObserver(observer);
}

// static
void WindowList::RemoveObserver(WindowListObserver* observer) {
  GetObservers().RemoveObserver(observer);
}

// static
void WindowList::CloseAllWindows() {
  std::vector<base::WeakPtr<NativeWindow>> weak_windows =
      ConvertToWeakPtrVector(GetInstance()->windows_);
#if BUILDFLAG(IS_MAC)
  std::ranges::reverse(weak_windows);
#endif
  for (const auto& window : weak_windows) {
    if (window && !window->IsClosed())
      window->Close();
  }
}

// static
void WindowList::DestroyAllWindows() {
  std::vector<base::WeakPtr<NativeWindow>> weak_windows =
      ConvertToWeakPtrVector(GetInstance()->windows_);

  for (const auto& window : weak_windows) {
    if (window && !window->IsClosed())
      window->CloseImmediately();
  }
}

WindowList::WindowList() = default;

WindowList::~WindowList() = default;

// static
base::ObserverList<WindowListObserver>& WindowList::GetObservers() {
  static base::NoDestructor<base::ObserverList<WindowListObserver>> instance;
  return *instance;
}

}  // namespace electron
