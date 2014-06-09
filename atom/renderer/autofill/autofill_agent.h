// Copyright (c) 2014 GitHub, Inc. All rights reserved.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_RENDERER_AUTOFILL_AUTOFILL_AGENT_H_
#define ATOM_RENDERER_AUTOFILL_AUTOFILL_AGENT_H_

#include "atom/renderer/autofill/page_click_listener.h"
#include "content/public/renderer/render_view_observer.h"
#include "third_party/WebKit/public/web/WebAutofillClient.h"

namespace autofill {

// AutofillAgent deals with Autofill related communications between WebKit and
// the browser. There is one AutofillAgent per RenderView.
class AutofillAgent : public content::RenderViewObserver,
                      public PageClickListener,
                      public WebKit::WebAutofillClient {
 public:
  AutofillAgent(content::RenderView* render_view);
  virtual ~AutofillAgent();

 private:
  enum AutofillAction {
    AUTOFILL_NONE,     // No state set.
    AUTOFILL_FILL,     // Fill the Autofill form data.
    AUTOFILL_PREVIEW,  // Preview the Autofill form data.
  };

  // RenderView::Observer:
  virtual void DidFinishDocumentLoad(WebKit::WebFrame* frame) OVERRIDE;
  virtual void DidCommitProvisionalLoad(WebKit::WebFrame* frame,
                                        bool is_new_navigation) OVERRIDE;
  virtual void FrameDetached(WebKit::WebFrame* frame) OVERRIDE;
  virtual void WillSubmitForm(WebKit::WebFrame* frame,
                              const WebKit::WebFormElement& form) OVERRIDE;
  virtual void ZoomLevelChanged() OVERRIDE;
  virtual void DidChangeScrollOffset(WebKit::WebFrame* frame) OVERRIDE;
  virtual void FocusedNodeChanged(const WebKit::WebNode& node) OVERRIDE;
  virtual void OrientationChangeEvent(int orientation) OVERRIDE;

  // PageClickListener:
  virtual void InputElementClicked(const WebKit::WebInputElement& element,
                                   bool was_focused,
                                   bool is_focused) OVERRIDE;
  virtual void InputElementLostFocus() OVERRIDE;

  // WebKit::WebAutofillClient:
  virtual void didClearAutofillSelection(const WebKit::WebNode& node) OVERRIDE;
  virtual void textFieldDidEndEditing(
      const WebKit::WebInputElement& element) OVERRIDE;
  virtual void textFieldDidChange(
      const WebKit::WebInputElement& element) OVERRIDE;
  virtual void textFieldDidReceiveKeyDown(
      const WebKit::WebInputElement& element,
      const WebKit::WebKeyboardEvent& event) OVERRIDE;
  virtual void didRequestAutocomplete(
      WebKit::WebFrame* frame,
      const WebKit::WebFormElement& form) OVERRIDE;
  virtual void setIgnoreTextChanges(bool ignore) OVERRIDE;
  virtual void didAssociateFormControls(
      const WebKit::WebVector<WebKit::WebNode>& nodes) OVERRIDE;

  DISALLOW_COPY_AND_ASSIGN(AutofillAgent);
};

}  // namespace autofill

#endif  // ATOM_RENDERER_AUTOFILL_AUTOFILL_AGENT_H_
