// Copyright (c) 2016 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_UI_X_EVENT_DISABLER_H_
#define ATOM_BROWSER_UI_X_EVENT_DISABLER_H_

#include "base/macros.h"
#include "ui/events/event_rewriter.h"

namespace atom {

class EventDisabler : public ui::EventRewriter {
 public:
  EventDisabler();
  ~EventDisabler() override;

  // ui::EventRewriter:
  ui::EventRewriteStatus RewriteEvent(
      const ui::Event& event,
      std::unique_ptr<ui::Event>* rewritten_event) override;
  ui::EventRewriteStatus NextDispatchEvent(
      const ui::Event& last_event,
      std::unique_ptr<ui::Event>* new_event) override;

 private:
  DISALLOW_COPY_AND_ASSIGN(EventDisabler);
};

}  // namespace atom

#endif  // ATOM_BROWSER_UI_X_EVENT_DISABLER_H_
