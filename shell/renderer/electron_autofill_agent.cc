// Copyright (c) 2017 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/renderer/electron_autofill_agent.h"

#include <utility>
#include <vector>

#include "content/public/renderer/render_frame.h"
#include "third_party/blink/public/common/associated_interfaces/associated_interface_provider.h"
#include "third_party/blink/public/common/input/web_keyboard_event.h"
#include "third_party/blink/public/platform/web_string.h"
#include "third_party/blink/public/web/web_document.h"
#include "third_party/blink/public/web/web_local_frame.h"
#include "third_party/blink/public/web/web_option_element.h"
#include "ui/events/keycodes/keyboard_codes.h"
#include "ui/gfx/geometry/rect_f.h"

namespace electron {

namespace {
const size_t kMaxDataLength = 1024;
const size_t kMaxListSize = 512;

void GetDataListSuggestions(const blink::WebInputElement& element,
                            std::vector<std::u16string>* values,
                            std::vector<std::u16string>* labels) {
  for (const auto& option : element.FilteredDataListOptions()) {
    values->push_back(option.Value().Utf16());
    if (option.Value() != option.Label())
      labels->push_back(option.Label().Utf16());
    else
      labels->push_back(std::u16string());
  }
}

void TrimStringVectorForIPC(std::vector<std::u16string>* strings) {
  // Limit the size of the vector.
  if (strings->size() > kMaxListSize)
    strings->resize(kMaxListSize);

  // Limit the size of the strings in the vector.
  for (auto& str : *strings) {
    if (str.length() > kMaxDataLength)
      str.resize(kMaxDataLength);
  }
}
}  // namespace

AutofillAgent::AutofillAgent(content::RenderFrame* frame,
                             blink::AssociatedInterfaceRegistry* registry)
    : content::RenderFrameObserver(frame) {
  render_frame()->GetWebFrame()->SetAutofillClient(this);
  registry->AddInterface<mojom::ElectronAutofillAgent>(base::BindRepeating(
      &AutofillAgent::BindReceiver, base::Unretained(this)));
}

AutofillAgent::~AutofillAgent() = default;

void AutofillAgent::BindReceiver(
    mojo::PendingAssociatedReceiver<mojom::ElectronAutofillAgent> receiver) {
  receiver_.Bind(std::move(receiver));
}

void AutofillAgent::OnDestruct() {
  delete this;
}

void AutofillAgent::DidChangeScrollOffset() {
  HidePopup();
}

void AutofillAgent::FocusedElementChanged(const blink::WebElement&) {
  focused_node_was_last_clicked_ = false;
  was_focused_before_now_ = false;
  HidePopup();
}

void AutofillAgent::TextFieldDidEndEditing(const blink::WebInputElement&) {
  HidePopup();
}

void AutofillAgent::TextFieldDidChange(
    const blink::WebFormControlElement& element) {
  if (!IsUserGesture() && !render_frame()->IsPasting())
    return;

  weak_ptr_factory_.InvalidateWeakPtrs();
  base::ThreadTaskRunnerHandle::Get()->PostTask(
      FROM_HERE, base::BindOnce(&AutofillAgent::TextFieldDidChangeImpl,
                                weak_ptr_factory_.GetWeakPtr(), element));
}

void AutofillAgent::TextFieldDidChangeImpl(
    const blink::WebFormControlElement& element) {
  ShowSuggestionsOptions options;
  options.requires_caret_at_end = true;
  ShowSuggestions(element, options);
}

void AutofillAgent::TextFieldDidReceiveKeyDown(
    const blink::WebInputElement& element,
    const blink::WebKeyboardEvent& event) {
  if (event.windows_key_code == ui::VKEY_DOWN ||
      event.windows_key_code == ui::VKEY_UP) {
    ShowSuggestionsOptions options;
    options.autofill_on_empty_values = true;
    options.requires_caret_at_end = true;
    ShowSuggestions(element, options);
  }
}

void AutofillAgent::OpenTextDataListChooser(
    const blink::WebInputElement& element) {
  ShowSuggestionsOptions options;
  options.autofill_on_empty_values = true;
  ShowSuggestions(element, options);
}

void AutofillAgent::DataListOptionsChanged(
    const blink::WebInputElement& element) {
  if (!element.Focused())
    return;

  ShowSuggestionsOptions options;
  options.requires_caret_at_end = true;
  ShowSuggestions(element, options);
}

AutofillAgent::ShowSuggestionsOptions::ShowSuggestionsOptions()
    : autofill_on_empty_values(false), requires_caret_at_end(false) {}

void AutofillAgent::ShowSuggestions(const blink::WebFormControlElement& element,
                                    const ShowSuggestionsOptions& options) {
  if (!element.IsEnabled() || element.IsReadOnly())
    return;
  const blink::WebInputElement input_element =
      element.DynamicTo<blink::WebInputElement>();
  if (!input_element.IsNull()) {
    if (!input_element.IsTextField())
      return;
  }

  blink::WebString value = element.EditingValue();
  if (value.length() > kMaxDataLength ||
      (!options.autofill_on_empty_values && value.IsEmpty()) ||
      (options.requires_caret_at_end &&
       (element.SelectionStart() != element.SelectionEnd() ||
        element.SelectionEnd() != static_cast<int>(value.length())))) {
    HidePopup();
    return;
  }

  std::vector<std::u16string> data_list_values;
  std::vector<std::u16string> data_list_labels;
  if (!input_element.IsNull()) {
    GetDataListSuggestions(input_element, &data_list_values, &data_list_labels);
    TrimStringVectorForIPC(&data_list_values);
    TrimStringVectorForIPC(&data_list_labels);
  }

  ShowPopup(element, data_list_values, data_list_labels);
}

void AutofillAgent::DidReceiveLeftMouseDownOrGestureTapInNode(
    const blink::WebNode& node) {
  focused_node_was_last_clicked_ = !node.IsNull() && node.Focused();
}

void AutofillAgent::DidCompleteFocusChangeInFrame() {
  DoFocusChangeComplete();
}

bool AutofillAgent::IsUserGesture() const {
  return render_frame()->GetWebFrame()->HasTransientUserActivation();
}

void AutofillAgent::HidePopup() {
  GetAutofillDriver()->HideAutofillPopup();
}

void AutofillAgent::ShowPopup(const blink::WebFormControlElement& element,
                              const std::vector<std::u16string>& values,
                              const std::vector<std::u16string>& labels) {
  gfx::RectF bounds = render_frame()->ElementBoundsInWindow(element);
  GetAutofillDriver()->ShowAutofillPopup(bounds, values, labels);
}

void AutofillAgent::AcceptDataListSuggestion(const std::u16string& suggestion) {
  auto element = render_frame()->GetWebFrame()->GetDocument().FocusedElement();
  if (element.IsFormControlElement()) {
    blink::WebInputElement input_element =
        element.DynamicTo<blink::WebInputElement>();
    if (!input_element.IsNull())
      input_element.SetAutofillValue(blink::WebString::FromUTF16(suggestion));
  }
}

void AutofillAgent::DoFocusChangeComplete() {
  auto element = render_frame()->GetWebFrame()->GetDocument().FocusedElement();
  if (element.IsNull() || !element.IsFormControlElement())
    return;

  if (focused_node_was_last_clicked_ && was_focused_before_now_) {
    ShowSuggestionsOptions options;
    options.autofill_on_empty_values = true;
    blink::WebInputElement input_element =
        element.DynamicTo<blink::WebInputElement>();
    if (!input_element.IsNull())
      ShowSuggestions(input_element, options);
  }

  was_focused_before_now_ = true;
  focused_node_was_last_clicked_ = false;
}

const mojo::AssociatedRemote<mojom::ElectronAutofillDriver>&
AutofillAgent::GetAutofillDriver() {
  if (!autofill_driver_) {
    render_frame()->GetRemoteAssociatedInterfaces()->GetInterface(
        &autofill_driver_);
  }

  return autofill_driver_;
}

}  // namespace electron
