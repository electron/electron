// Copyright (c) 2022 GitHub, Inc.

#include "shell/browser/ui/native_view.h"

#include "shell/browser/native_window.h"

namespace electron {

NativeView::NativeView() : view_(nullptr) {
  InitView();
}

NativeView::~NativeView() {
  DestroyView();
}

void NativeView::SetParent(NativeView* parent) {
  if (parent) {
    SetWindow(parent->window_);
  } else {
    SetWindow(nullptr);
  }
  parent_ = parent;
}

void NativeView::BecomeContentView(NativeWindow* window) {
  SetWindow(window);
  parent_ = nullptr;
}

void NativeView::SetWindow(NativeWindow* window) {
  window_ = window;
  SetWindowForChildren(window);
}

void NativeView::SetWindowForChildren(NativeWindow* window) {}

bool NativeView::IsContainer() const {
  return false;
}

void NativeView::DetachChildView(NativeView* view) {}

void NativeView::TriggerBeforeunloadEvents() {}

void NativeView::AddObserver(Observer* observer) {
  CHECK(observer);
  observers_.AddObserver(observer);
}

void NativeView::RemoveObserver(Observer* observer) {
  observers_.RemoveObserver(observer);
}

void NativeView::NotifyChildViewDetached(NativeView* view) {
  for (Observer& observer : observers_)
    observer.OnChildViewDetached(this, view);
}

void NativeView::NotifyViewIsDeleting() {
  for (Observer& observer : observers_)
    observer.OnViewIsDeleting(this);
}

}  // namespace electron
