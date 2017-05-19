// Copyright (c) 2017 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/renderer/atom_autofill_agent.h"

#include <vector>

#include "atom/common/api/api_messages.h"
#include "content/public/renderer/render_frame.h"
#include "content/public/renderer/render_view.h"
#include "third_party/WebKit/public/platform/WebKeyboardEvent.h"
#include "third_party/WebKit/public/platform/WebString.h"
#include "third_party/WebKit/public/web/WebDocument.h"
#include "third_party/WebKit/public/web/WebLocalFrame.h"
#include "third_party/WebKit/public/web/WebOptionElement.h"
#include "third_party/WebKit/public/web/WebUserGestureIndicator.h"
#include "ui/events/keycodes/keyboard_codes.h"
#include "ui/gfx/geometry/rect_f.h"

namespace atom {

namespace {
const size_t kMaxDataLength = 1024;
const size_t kMaxListSize = 512;

void GetDataListSuggestions(const blink::WebInputElement& element,
    std::vector<base::string16>* values,
    std::vector<base::string16>* labels) {
  for (const auto& option : element.filteredDataListOptions()) {
    values->push_back(option.value().utf16());
    if (option.value() != option.label())
      labels->push_back(option.label().utf16());
    else
      labels->push_back(base::string16());
  }
}

void TrimStringVectorForIPC(std::vector<base::string16>* strings) {
  // Limit the size of the vector.
  if (strings->size() > kMaxListSize)
    strings->resize(kMaxListSize);

  // Limit the size of the strings in the vector.
  for (size_t i = 0; i < strings->size(); ++i) {
    if ((*strings)[i].length() > kMaxDataLength)
      (*strings)[i].resize(kMaxDataLength);
  }
}
}  // namespace

AutofillAgent::AutofillAgent(
    content::RenderFrame* frame)
  : content::RenderFrameObserver(frame),
    render_frame_(frame),
    weak_ptr_factory_(this) {
  render_frame_->GetWebFrame()->setAutofillClient(this);
}

void AutofillAgent::OnDestruct() {
  delete this;
}

void AutofillAgent::DidChangeScrollOffset() {
  HidePopup();
}

void AutofillAgent::FocusedNodeChanged(const blink::WebNode&) {
  HidePopup();
}

void AutofillAgent::textFieldDidEndEditing(
    const blink::WebInputElement&) {
  HidePopup();
}

void AutofillAgent::textFieldDidChange(
    const blink::WebFormControlElement& element) {
  if (!IsUserGesture() && !render_frame()->IsPasting())
    return;

  weak_ptr_factory_.InvalidateWeakPtrs();
  base::ThreadTaskRunnerHandle::Get()->PostTask(
      FROM_HERE, base::Bind(&AutofillAgent::textFieldDidChangeImpl,
                            weak_ptr_factory_.GetWeakPtr(), element));
}

void AutofillAgent::textFieldDidChangeImpl(
    const blink::WebFormControlElement& element) {
  ShowSuggestionsOptions options;
  options.requires_caret_at_end = true;
  ShowSuggestions(element, options);
}

void AutofillAgent::textFieldDidReceiveKeyDown(
    const blink::WebInputElement& element,
    const blink::WebKeyboardEvent& event) {
  if (event.windowsKeyCode == ui::VKEY_DOWN ||
      event.windowsKeyCode == ui::VKEY_UP) {
    ShowSuggestionsOptions options;
    options.autofill_on_empty_values = true;
    options.requires_caret_at_end = true;
    ShowSuggestions(element, options);
  }
}

void AutofillAgent::openTextDataListChooser(
    const blink::WebInputElement& element) {
  ShowSuggestionsOptions options;
  options.autofill_on_empty_values = true;
  ShowSuggestions(element, options);
}

void AutofillAgent::dataListOptionsChanged(
    const blink::WebInputElement& element) {
  if (!element.focused())
    return;

  ShowSuggestionsOptions options;
  options.requires_caret_at_end = true;
  ShowSuggestions(element, options);
}

AutofillAgent::ShowSuggestionsOptions::ShowSuggestionsOptions()
    : autofill_on_empty_values(false),
      requires_caret_at_end(false) {
}

void AutofillAgent::ShowSuggestions(
    const blink::WebFormControlElement& element,
    const ShowSuggestionsOptions& options) {
  if (!element.isEnabled() || element.isReadOnly())
    return;
  if (!element.suggestedValue().isEmpty())
    return;
  const blink::WebInputElement* input_element = toWebInputElement(&element);
  if (input_element) {
    if (!input_element->isTextField())
      return;
  }

  blink::WebString value = element.editingValue();
  if (value.length() > kMaxDataLength ||
      (!options.autofill_on_empty_values && value.isEmpty()) ||
      (options.requires_caret_at_end &&
       (element.selectionStart() != element.selectionEnd() ||
        element.selectionEnd() != static_cast<int>(value.length())))) {
    HidePopup();
    return;
  }

  std::vector<base::string16> data_list_values;
  std::vector<base::string16> data_list_labels;
  if (input_element) {
    GetDataListSuggestions(
      *input_element, &data_list_values, &data_list_labels);
    TrimStringVectorForIPC(&data_list_values);
    TrimStringVectorForIPC(&data_list_labels);
  }

  ShowPopup(element, data_list_values, data_list_labels);
}

void AutofillAgent::OnAcceptSuggestion(base::string16 suggestion) {
  auto element = render_frame_->GetWebFrame()->document().focusedElement();
  if (element.isFormControlElement()) {
    toWebInputElement(&element)->setAutofillValue(
      blink::WebString::fromUTF16(suggestion));
  }
}

bool AutofillAgent::OnMessageReceived(const IPC::Message& message) {
  bool handled = true;
  IPC_BEGIN_MESSAGE_MAP(AutofillAgent, message)
    IPC_MESSAGE_HANDLER(AtomAutofillViewHostMsg_RoutingId,
      OnWebContentsRoutingIdReceived)
    IPC_MESSAGE_HANDLER(AtomAutofillViewMsg_AcceptSuggestion,
      OnAcceptSuggestion)
    IPC_MESSAGE_UNHANDLED(handled = false)
  IPC_END_MESSAGE_MAP()

  return handled;
}

void AutofillAgent::OnWebContentsRoutingIdReceived(int id) {
  web_contents_routing_id_ = id;
}

bool AutofillAgent::IsUserGesture() const {
  return blink::WebUserGestureIndicator::isProcessingUserGesture();
}

void AutofillAgent::HidePopup() {
  Send(new AtomAutofillViewMsg_HidePopup(web_contents_routing_id_));
}

void AutofillAgent::ShowPopup(
    const blink::WebFormControlElement& element,
    const std::vector<base::string16>& values,
    const std::vector<base::string16>& labels) {
  gfx::RectF bounds =
    render_frame_->GetRenderView()->ElementBoundsInWindow(element);
  Send(new AtomAutofillViewMsg_ShowPopup(
    web_contents_routing_id_, routing_id(), bounds, values, labels));
}

}  // namespace atom
