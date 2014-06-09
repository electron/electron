// Copyright (c) 2014 GitHub, Inc. All rights reserved.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_RENDERER_AUTOFILL_PAGE_CLICK_TRACKER_H_
#define ATOM_RENDERER_AUTOFILL_PAGE_CLICK_TRACKER_H_

#include <vector>

#include "base/basictypes.h"
#include "content/public/renderer/render_view_observer.h"
#include "third_party/WebKit/public/web/WebDOMEventListener.h"
#include "third_party/WebKit/public/web/WebNode.h"

// Prepare for upgrading to Chrome35.
namespace blink = WebKit;

namespace autofill {

class PageClickListener;

// This class is responsible for tracking clicks on elements in web pages and
// notifiying the associated listener when a node is clicked.
// Compared to a simple WebDOMEventListener, it offers the added capability of
// notifying the listeners of whether the clicked node was already focused
// before it was clicked.
//
// This is useful for password/form autofill where we want to trigger a
// suggestion popup when a text input is clicked.
// It only notifies of WebInputElement that are text inputs being clicked, but
// could easily be changed to report click on any type of WebNode.
//
// There is one PageClickTracker per RenderView.
class PageClickTracker : public content::RenderViewObserver,
                         public blink::WebDOMEventListener {
 public:
  // The |listener| will be notified when an element is clicked.  It must
  // outlive this class.
  PageClickTracker(content::RenderView* render_view,
                   PageClickListener* listener);
  virtual ~PageClickTracker();

 private:
  // RenderView::Observer implementation.
  virtual void DidFinishDocumentLoad(blink::WebFrame* frame) OVERRIDE;
  virtual void FrameDetached(blink::WebFrame* frame) OVERRIDE;
  virtual void DidHandleMouseEvent(const blink::WebMouseEvent& event) OVERRIDE;

  // blink::WebDOMEventListener implementation.
  virtual void handleEvent(const blink::WebDOMEvent& event);

  // Checks to see if a text field is losing focus and inform listeners if
  // it is.
  void HandleTextFieldMaybeLosingFocus(
      const blink::WebNode& newly_clicked_node);

  // The last node that was clicked and had focus.
  blink::WebNode last_node_clicked_;

  // Whether the last clicked node had focused before it was clicked.
  bool was_focused_;

  // The frames we are listening to for mouse events.
  std::vector<blink::WebFrame*> tracked_frames_;

  // The listener getting the actual notifications.
  PageClickListener* listener_;

  DISALLOW_COPY_AND_ASSIGN(PageClickTracker);
};

}  // namespace autofill

#endif  // ATOM_RENDERER_AUTOFILL_PAGE_CLICK_TRACKER_H_
