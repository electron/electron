// Copyright (c) 2014 GitHub, Inc. All rights reserved.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_RENDERER_AUTOFILL_PAGE_CLICK_LISTENER_H_
#define ATOM_RENDERER_AUTOFILL_PAGE_CLICK_LISTENER_H_

namespace WebKit {
class WebInputElement;
}

namespace autofill {

// Interface that should be implemented by classes interested in getting
// notifications for clicks on a page.
// Register on the PageListenerTracker object.
class PageClickListener {
 public:
  // Notification that |element| was clicked.
  // |was_focused| is true if |element| had focus BEFORE the click.
  // |is_focused| is true if |element| has focus AFTER the click was processed.
  virtual void InputElementClicked(const WebKit::WebInputElement& element,
                                   bool was_focused,
                                   bool is_focused) = 0;

  // If the previously focused element was an input field, listeners are
  // informed that the text field has lost its focus.
  virtual void InputElementLostFocus() = 0;

 protected:
  virtual ~PageClickListener() {}
};

}  // namespace autofill

#endif  // ATOM_RENDERER_AUTOFILL_PAGE_CLICK_LISTENER_H_
