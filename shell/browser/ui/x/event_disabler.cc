// Copyright (c) 2016 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/ui/x/event_disabler.h"

#include <memory>

namespace electron {

EventDisabler::EventDisabler() = default;

EventDisabler::~EventDisabler() = default;

ui::EventRewriteStatus EventDisabler::RewriteEvent(
    const ui::Event& event,
    std::unique_ptr<ui::Event>* rewritten_event) {
  return ui::EVENT_REWRITE_DISCARD;
}

ui::EventRewriteStatus EventDisabler::NextDispatchEvent(
    const ui::Event& last_event,
    std::unique_ptr<ui::Event>* new_event) {
  return ui::EVENT_REWRITE_CONTINUE;
}

}  // namespace electron
