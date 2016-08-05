// Copyright (c) 2016 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/osr/osr_render_widget_host_view.h"

#import <Cocoa/Cocoa.h>

#include "atom/browser/native_window_mac.h"

#include "base/strings/utf_string_conversions.h"

ui::AcceleratedWidgetMac*
atom::OffScreenRenderWidgetHostView::GetAcceleratedWidgetMac() const {
  if (browser_compositor_)
    return browser_compositor_->accelerated_widget_mac();
  return nullptr;
}

void atom::OffScreenRenderWidgetHostView::SetActive(bool active) {
}

void atom::OffScreenRenderWidgetHostView::ShowDefinitionForSelection() {
}

bool atom::OffScreenRenderWidgetHostView::SupportsSpeech() const {
  return false;
}

void atom::OffScreenRenderWidgetHostView::SpeakSelection() {
}

bool atom::OffScreenRenderWidgetHostView::IsSpeaking() const {
  return false;
}

void atom::OffScreenRenderWidgetHostView::StopSpeaking() {
}

void atom::OffScreenRenderWidgetHostView::SelectionChanged(
    const base::string16& text,
    size_t offset,
    const gfx::Range& range) {
  if (range.is_empty() || text.empty()) {
    selected_text_.clear();
  } else {
    size_t pos = range.GetMin() - offset;
    size_t n = range.length();

    DCHECK(pos + n <= text.length()) << "The text can not fully cover range.";
    if (pos >= text.length()) {
      DCHECK(false) << "The text can not cover range.";
      return;
    }
    selected_text_ = base::UTF16ToUTF8(text.substr(pos, n));
  }

  RenderWidgetHostViewBase::SelectionChanged(text, offset, range);
}

void atom::OffScreenRenderWidgetHostView::CreatePlatformWidget() {
  browser_compositor_ = content::BrowserCompositorMac::Create();

  compositor_.reset(browser_compositor_->compositor());
  compositor_->SetRootLayer(root_layer_.get());
  browser_compositor_->accelerated_widget_mac()->SetNSView(
    static_cast<atom::NativeWindowMac*>(native_window_));
  browser_compositor_->compositor()->SetVisible(true);

  compositor_->SetLocksWillTimeOut(true);
  browser_compositor_->Unsuspend();
}

void atom::OffScreenRenderWidgetHostView::DestroyPlatformWidget() {
  ui::Compositor* compositor = compositor_.release();
  ALLOW_UNUSED_LOCAL(compositor);

  browser_compositor_->accelerated_widget_mac()->ResetNSView();
  browser_compositor_->compositor()->SetVisible(false);
  browser_compositor_->compositor()->SetScaleAndSize(1.0, gfx::Size(0, 0));
  browser_compositor_->compositor()->SetRootLayer(NULL);
  content::BrowserCompositorMac::Recycle(std::move(browser_compositor_));
}
