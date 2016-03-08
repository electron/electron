// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/printing/printer_query.h"

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/message_loop/message_loop.h"
#include "base/threading/thread_restrictions.h"
#include "base/values.h"
#include "chrome/browser/printing/print_job_worker.h"

namespace printing {

PrinterQuery::PrinterQuery(int render_process_id, int render_view_id)
    : worker_(new PrintJobWorker(render_process_id, render_view_id, this)),
      is_print_dialog_box_shown_(false),
      cookie_(PrintSettings::NewCookie()),
      last_status_(PrintingContext::FAILED) {
  DCHECK(base::MessageLoopForIO::IsCurrent());
}

PrinterQuery::~PrinterQuery() {
  // The job should be finished (or at least canceled) when it is destroyed.
  DCHECK(!is_print_dialog_box_shown_);
  // If this fires, it is that this pending printer context has leaked.
  DCHECK(!worker_.get());
}

void PrinterQuery::GetSettingsDone(const PrintSettings& new_settings,
                                   PrintingContext::Result result) {
  is_print_dialog_box_shown_ = false;
  last_status_ = result;
  if (result != PrintingContext::FAILED) {
    settings_ = new_settings;
    cookie_ = PrintSettings::NewCookie();
  } else {
    // Failure.
    cookie_ = 0;
  }

  if (!callback_.is_null()) {
    // This may cause reentrancy like to call StopWorker().
    callback_.Run();
    callback_.Reset();
  }
}

PrintJobWorker* PrinterQuery::DetachWorker(PrintJobWorkerOwner* new_owner) {
  DCHECK(callback_.is_null());
  DCHECK(worker_.get());

  worker_->SetNewOwner(new_owner);
  return worker_.release();
}

const PrintSettings& PrinterQuery::settings() const {
  return settings_;
}

int PrinterQuery::cookie() const {
  return cookie_;
}

void PrinterQuery::GetSettings(
    GetSettingsAskParam ask_user_for_settings,
    int expected_page_count,
    bool has_selection,
    MarginType margin_type,
    bool is_scripted,
    const base::Closure& callback) {
  DCHECK(RunsTasksOnCurrentThread());
  DCHECK(!is_print_dialog_box_shown_ || !is_scripted);

  StartWorker(callback);

  // Real work is done in PrintJobWorker::GetSettings().
  is_print_dialog_box_shown_ =
      ask_user_for_settings == GetSettingsAskParam::ASK_USER;
  worker_->PostTask(FROM_HERE,
                    base::Bind(&PrintJobWorker::GetSettings,
                               base::Unretained(worker_.get()),
                               is_print_dialog_box_shown_,
                               expected_page_count,
                               has_selection,
                               margin_type,
                               is_scripted));
}

void PrinterQuery::SetSettings(scoped_ptr<base::DictionaryValue> new_settings,
                               const base::Closure& callback) {
  StartWorker(callback);

  worker_->PostTask(FROM_HERE,
                    base::Bind(&PrintJobWorker::SetSettings,
                               base::Unretained(worker_.get()),
                               base::Passed(&new_settings)));
}

void PrinterQuery::StartWorker(const base::Closure& callback) {
  DCHECK(callback_.is_null());
  DCHECK(worker_.get());

  // Lazily create the worker thread. There is one worker thread per print job.
  if (!worker_->IsRunning())
    worker_->Start();

  callback_ = callback;
}

void PrinterQuery::StopWorker() {
  if (worker_.get()) {
    // http://crbug.com/66082: We're blocking on the PrinterQuery's worker
    // thread.  It's not clear to me if this may result in blocking the current
    // thread for an unacceptable time.  We should probably fix it.
    base::ThreadRestrictions::ScopedAllowIO allow_io;
    worker_->Stop();
    worker_.reset();
  }
}

bool PrinterQuery::is_callback_pending() const {
  return !callback_.is_null();
}

bool PrinterQuery::is_valid() const {
  return worker_.get() != NULL;
}

}  // namespace printing
