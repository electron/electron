// Copyright (c) 2014 GitHub, Inc. All rights reserved.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/renderer/autofill/page_click_tracker.h"

#include "atom/renderer/autofill/page_click_listener.h"
#include "content/public/renderer/render_view.h"
#include "third_party/WebKit/public/platform/WebString.h"
#include "third_party/WebKit/public/web/WebDOMMouseEvent.h"
#include "third_party/WebKit/public/web/WebDocument.h"
#include "third_party/WebKit/public/web/WebFrame.h"
#include "third_party/WebKit/public/web/WebInputElement.h"
#include "third_party/WebKit/public/web/WebInputEvent.h"
#include "third_party/WebKit/public/web/WebView.h"

using blink::WebDOMEvent;
using blink::WebDOMMouseEvent;
using blink::WebElement;
using blink::WebFormControlElement;
using blink::WebFrame;
using blink::WebInputElement;
using blink::WebInputEvent;
using blink::WebMouseEvent;
using blink::WebNode;
using blink::WebString;
using blink::WebView;

namespace autofill {

namespace {

// All text fields, including password fields, should be extracted.
bool IsTextInput(const WebInputElement* element) {
  return element && element->isTextField();
}

// Casts |node| to a WebInputElement.
// Returns an empty (isNull()) WebInputElement if |node| is not a text field.
const WebInputElement GetTextWebInputElement(const WebNode& node) {
  if (!node.isElementNode())
    return WebInputElement();
  const WebElement element = node.toConst<WebElement>();
  if (!element.hasHTMLTagName("input"))
    return WebInputElement();
  const WebInputElement* input = blink::toWebInputElement(&element);
  if (!IsTextInput(input))
    return WebInputElement();
  return *input;
}

// Checks to see if a text field was the previously selected node and is now
// losing its focus.
bool DidSelectedTextFieldLoseFocus(const WebNode& newly_clicked_node) {
  blink::WebNode focused_node = newly_clicked_node.document().focusedNode();

  if (focused_node.isNull() || GetTextWebInputElement(focused_node).isNull())
    return false;

  return focused_node != newly_clicked_node;
}

}  // namespace

PageClickTracker::PageClickTracker(content::RenderView* render_view,
                                   PageClickListener* listener)
    : content::RenderViewObserver(render_view),
      was_focused_(false),
      listener_(listener) {
}

PageClickTracker::~PageClickTracker() {
  // Note that even though RenderView calls FrameDetached when notified that
  // a frame was closed, it might not always get that notification from WebKit
  // for all frames.
  // By the time we get here, the frame could have been destroyed so we cannot
  // unregister listeners in frames remaining in tracked_frames_ as they might
  // be invalid.
}

void PageClickTracker::DidHandleMouseEvent(const WebMouseEvent& event) {
  if (event.type != WebInputEvent::MouseDown ||
      last_node_clicked_.isNull()) {
    return;
  }

  // We are only interested in text field clicks.
  const WebInputElement input_element =
      GetTextWebInputElement(last_node_clicked_);
  if (input_element.isNull())
    return;

  bool is_focused = (last_node_clicked_ == render_view()->GetFocusedNode());
  listener_->InputElementClicked(input_element, was_focused_, is_focused);
}

void PageClickTracker::DidFinishDocumentLoad(blink::WebFrame* frame) {
  tracked_frames_.push_back(frame);
  frame->document().addEventListener("mousedown", this, false);
}

void PageClickTracker::FrameDetached(blink::WebFrame* frame) {
  std::vector<blink::WebFrame*>::iterator iter =
      std::find(tracked_frames_.begin(), tracked_frames_.end(), frame);
  if (iter == tracked_frames_.end()) {
    // Some frames might never load contents so we may not have a listener on
    // them.  Calling removeEventListener() on them would trigger an assert, so
    // we need to keep track of which frames we are listening to.
    return;
  }
  tracked_frames_.erase(iter);
}

void PageClickTracker::handleEvent(const WebDOMEvent& event) {
  last_node_clicked_.reset();

  if (!event.isMouseEvent())
    return;

  const WebDOMMouseEvent mouse_event = event.toConst<WebDOMMouseEvent>();
  DCHECK(mouse_event.buttonDown());
  if (mouse_event.button() != 0)
    return;  // We are only interested in left clicks.

  // Remember which node has focus before the click is processed.
  // We'll get a notification once the mouse event has been processed
  // (DidHandleMouseEvent), we'll notify the listener at that point.
  WebNode node = mouse_event.target();
  if (node.isNull())
    // Node may be null if the target was an SVG instance element from a <use>
    // tree and the tree has been rebuilt due to an earlier event.
    return;

  HandleTextFieldMaybeLosingFocus(node);

  // We are only interested in text field clicks.
  if (GetTextWebInputElement(node).isNull())
    return;

  last_node_clicked_ = node;
  was_focused_ = (node.document().focusedNode() == last_node_clicked_);
}

void PageClickTracker::HandleTextFieldMaybeLosingFocus(
    const WebNode& newly_clicked_node) {
  if (DidSelectedTextFieldLoseFocus(newly_clicked_node))
    listener_->InputElementLostFocus();
}

}  // namespace autofill
