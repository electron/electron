// Copyright (c) 2017 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_RENDERER_ELECTRON_AUTOFILL_AGENT_H_
#define ELECTRON_SHELL_RENDERER_ELECTRON_AUTOFILL_AGENT_H_

#include <vector>

#include "base/memory/weak_ptr.h"
#include "content/public/renderer/render_frame_observer.h"
#include "mojo/public/cpp/bindings/associated_receiver.h"
#include "mojo/public/cpp/bindings/associated_remote.h"
#include "mojo/public/cpp/bindings/pending_associated_receiver.h"
#include "shell/common/api/api.mojom.h"
#include "third_party/blink/public/common/associated_interfaces/associated_interface_registry.h"
#include "third_party/blink/public/web/web_autofill_client.h"
#include "third_party/blink/public/web/web_element.h"
#include "third_party/blink/public/web/web_form_control_element.h"
#include "third_party/blink/public/web/web_input_element.h"

namespace electron {

class AutofillAgent : public content::RenderFrameObserver,
                      public blink::WebAutofillClient,
                      public mojom::ElectronAutofillAgent {
 public:
  explicit AutofillAgent(content::RenderFrame* frame,
                         blink::AssociatedInterfaceRegistry* registry);
  ~AutofillAgent() override;

  // disable copy
  AutofillAgent(const AutofillAgent&) = delete;
  AutofillAgent& operator=(const AutofillAgent&) = delete;

  void BindReceiver(
      mojo::PendingAssociatedReceiver<mojom::ElectronAutofillAgent> receiver);

  // content::RenderFrameObserver:
  void OnDestruct() override;

  void DidChangeScrollOffset() override;
  void FocusedElementChanged(const blink::WebElement&) override;
  void DidCompleteFocusChangeInFrame() override;
  void DidReceiveLeftMouseDownOrGestureTapInNode(
      const blink::WebNode&) override;

 private:
  struct ShowSuggestionsOptions {
    ShowSuggestionsOptions();
    bool autofill_on_empty_values;
    bool requires_caret_at_end;
  };

  // blink::WebAutofillClient:
  void TextFieldDidEndEditing(const blink::WebInputElement&) override;
  void TextFieldDidChange(const blink::WebFormControlElement&) override;
  void TextFieldDidChangeImpl(const blink::WebFormControlElement&);
  void TextFieldDidReceiveKeyDown(const blink::WebInputElement&,
                                  const blink::WebKeyboardEvent&) override;
  void OpenTextDataListChooser(const blink::WebInputElement&) override;
  void DataListOptionsChanged(const blink::WebInputElement&) override;

  // mojom::ElectronAutofillAgent
  void AcceptDataListSuggestion(const std::u16string& suggestion) override;

  bool IsUserGesture() const;
  void HidePopup();
  void ShowPopup(const blink::WebFormControlElement&,
                 const std::vector<std::u16string>&,
                 const std::vector<std::u16string>&);
  void ShowSuggestions(const blink::WebFormControlElement& element,
                       const ShowSuggestionsOptions& options);

  void DoFocusChangeComplete();

  const mojo::AssociatedRemote<mojom::ElectronAutofillDriver>&
  GetAutofillDriver();
  mojo::AssociatedRemote<mojom::ElectronAutofillDriver> autofill_driver_;

  // True when the last click was on the focused node.
  bool focused_node_was_last_clicked_ = false;

  // This is set to false when the focus changes, then set back to true soon
  // afterwards. This helps track whether an event happened after a node was
  // already focused, or if it caused the focus to change.
  bool was_focused_before_now_ = false;

  mojo::AssociatedReceiver<mojom::ElectronAutofillAgent> receiver_{this};

  base::WeakPtrFactory<AutofillAgent> weak_ptr_factory_{this};
};

}  // namespace electron

#endif  // ELECTRON_SHELL_RENDERER_ELECTRON_AUTOFILL_AGENT_H_
