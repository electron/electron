// Copyright (c) 2017 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_RENDERER_ATOM_AUTOFILL_AGENT_H_
#define ATOM_RENDERER_ATOM_AUTOFILL_AGENT_H_

#include <vector>

#include "base/memory/weak_ptr.h"
#include "content/public/renderer/render_frame_observer.h"
#include "content/public/renderer/render_view_observer.h"
#include "third_party/blink/public/web/web_autofill_client.h"
#include "third_party/blink/public/web/web_form_control_element.h"
#include "third_party/blink/public/web/web_input_element.h"
#include "third_party/blink/public/web/web_node.h"

namespace atom {

class AutofillAgent : public content::RenderFrameObserver,
                      public blink::WebAutofillClient {
 public:
  explicit AutofillAgent(content::RenderFrame* frame);
  ~AutofillAgent() override;

  // content::RenderFrameObserver:
  void OnDestruct() override;

  void DidChangeScrollOffset() override;
  void FocusedNodeChanged(const blink::WebNode&) override;
  void DidCompleteFocusChangeInFrame() override;
  void DidReceiveLeftMouseDownOrGestureTapInNode(
      const blink::WebNode&) override;

 private:
  struct ShowSuggestionsOptions {
    ShowSuggestionsOptions();
    bool autofill_on_empty_values;
    bool requires_caret_at_end;
  };

  bool OnMessageReceived(const IPC::Message& message) override;

  // blink::WebAutofillClient:
  void TextFieldDidEndEditing(const blink::WebInputElement&) override;
  void TextFieldDidChange(const blink::WebFormControlElement&) override;
  void TextFieldDidChangeImpl(const blink::WebFormControlElement&);
  void TextFieldDidReceiveKeyDown(const blink::WebInputElement&,
                                  const blink::WebKeyboardEvent&) override;
  void OpenTextDataListChooser(const blink::WebInputElement&) override;
  void DataListOptionsChanged(const blink::WebInputElement&) override;

  bool IsUserGesture() const;
  void HidePopup();
  void ShowPopup(const blink::WebFormControlElement&,
                 const std::vector<base::string16>&,
                 const std::vector<base::string16>&);
  void ShowSuggestions(const blink::WebFormControlElement& element,
                       const ShowSuggestionsOptions& options);
  void OnAcceptSuggestion(base::string16 suggestion);

  void DoFocusChangeComplete();

  // True when the last click was on the focused node.
  bool focused_node_was_last_clicked_ = false;

  // This is set to false when the focus changes, then set back to true soon
  // afterwards. This helps track whether an event happened after a node was
  // already focused, or if it caused the focus to change.
  bool was_focused_before_now_ = false;

  base::WeakPtrFactory<AutofillAgent> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(AutofillAgent);
};

}  // namespace atom

#endif  // ATOM_RENDERER_ATOM_AUTOFILL_AGENT_H_
