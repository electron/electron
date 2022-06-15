// Copyright (c) 2016 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_UI_X_EVENT_DISABLER_H_
#define ELECTRON_SHELL_BROWSER_UI_X_EVENT_DISABLER_H_

#include <memory>

#include "ui/events/event_rewriter.h"

namespace electron {

class EventDisabler : public ui::EventRewriter {
 public:
  EventDisabler();
  ~EventDisabler() override;

  // disable copy
  EventDisabler(const EventDisabler&) = delete;
  EventDisabler& operator=(const EventDisabler&) = delete;

  // ui::EventRewriter:
  ui::EventRewriteStatus RewriteEvent(
      const ui::Event& event,
      std::unique_ptr<ui::Event>* rewritten_event) override;
  ui::EventRewriteStatus NextDispatchEvent(
      const ui::Event& last_event,
      std::unique_ptr<ui::Event>* new_event) override;
};

}  // namespace electron

#endif  // ELECTRON_SHELL_BROWSER_UI_X_EVENT_DISABLER_H_
