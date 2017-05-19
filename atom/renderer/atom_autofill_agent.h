// Copyright (c) 2017 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_RENDERER_ATOM_AUTOFILL_AGENT_H_
#define ATOM_RENDERER_ATOM_AUTOFILL_AGENT_H_

#include "base/memory/weak_ptr.h"
#include "content/public/renderer/render_frame_observer.h"
#include "third_party/WebKit/public/web/WebAutofillClient.h"
#include "third_party/WebKit/public/web/WebFormControlElement.h"
#include "third_party/WebKit/public/web/WebInputElement.h"
#include "third_party/WebKit/public/web/WebNode.h"

namespace atom {

class AutofillAgent : public content::RenderFrameObserver, 
                                public blink::WebAutofillClient {
 public:
  AutofillAgent(content::RenderFrame* frame);

  // content::RenderFrameObserver:
  void OnDestruct() override;
  
  void DidChangeScrollOffset() override;
  void FocusedNodeChanged(const blink::WebNode&) override;

 private:
  struct ShowSuggestionsOptions {
    ShowSuggestionsOptions();
    bool autofill_on_empty_values;
    bool requires_caret_at_end;
  };
   
  bool OnMessageReceived(const IPC::Message& message) override;
  void OnWebContentsRoutingIdReceived(int);

  // blink::WebAutofillClient:
  void textFieldDidEndEditing(const blink::WebInputElement&) override;
  void textFieldDidChange(const blink::WebFormControlElement&) override;
  void textFieldDidChangeImpl(const blink::WebFormControlElement&);
  void textFieldDidReceiveKeyDown(const blink::WebInputElement&,
                                  const blink::WebKeyboardEvent&) override;
  void openTextDataListChooser(const blink::WebInputElement&) override;
  void dataListOptionsChanged(const blink::WebInputElement&) override;
  
  bool IsUserGesture() const;
  void HidePopup();
  void ShowPopup(const blink::WebFormControlElement&,
                 const std::vector<base::string16>&,
                 const std::vector<base::string16>&);
  void ShowSuggestions(const blink::WebFormControlElement& element,
                       const ShowSuggestionsOptions& options);
  void OnAcceptSuggestion(base::string16 suggestion);

  content::RenderFrame* render_frame_;
  int web_contents_routing_id_;

  base::WeakPtrFactory<AutofillAgent> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(AutofillAgent);
};

}  // namespace atom

#endif  // ATOM_RENDERER_ATOM_AUTOFILL_AGENT_H_
