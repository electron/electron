// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_COMMON_CRASH_REPORTER_WIN_CRASH_SERVICE_H_
#define ATOM_COMMON_CRASH_REPORTER_WIN_CRASH_SERVICE_H_

#include <string>

#include "base/basictypes.h"
#include "base/files/file_path.h"
#include "base/synchronization/lock.h"

namespace google_breakpad {

class CrashReportSender;
class CrashGenerationServer;
class ClientInfo;

}

namespace breakpad {

// This class implements an out-of-process crash server. It uses breakpad's
// CrashGenerationServer and CrashReportSender to generate and then send the
// crash dumps. Internally, it uses OS specific pipe to allow applications to
// register for crash dumps and later on when a registered application crashes
// it will signal an event that causes this code to wake up and perform a
// crash dump on the signaling process. The dump is then stored on disk and
// possibly sent to the crash2 servers.
class CrashService {
 public:
  CrashService();
  ~CrashService();

  // Starts servicing crash dumps. Returns false if it failed. Do not use
  // other members in that case. |operating_dir| is where the CrashService
  // should store breakpad's checkpoint file. |dumps_path| is the directory
  // where the crash dumps should be stored.
  bool Initialize(const base::string16& application_name,
                  const base::FilePath& operating_dir,
                  const base::FilePath& dumps_path);

  // Command line switches:
  //
  // --max-reports=<number>
  // Allows to override the maximum number for reports per day. Normally
  // the crash dumps are never sent so if you want to send any you must
  // specify a positive number here.
  static const char kMaxReports[];
  // --no-window
  // Does not create a visible window on the desktop. The window does not have
  // any other functionality other than allowing the crash service to be
  // gracefully closed.
  static const char kNoWindow[];
  // --reporter=<string>
  // Allows to specify a custom string that appears on the detail crash report
  // page in the crash server. This should be a 25 chars or less string.
  // The default tag if not specified is 'crash svc'.
  static const char kReporterTag[];
  // --dumps-dir=<directory-path>
  // Override the directory to which crash dump files will be written.
  static const char kDumpsDir[];
  // --pipe-name=<string>
  // Override the name of the Windows named pipe on which we will
  // listen for crash dump request messages.
  static const char kPipeName[];
  // --reporter-url=<string>
  // Override the URL to which crash reports will be sent to.
  static const char kReporterURL[];

  // Returns number of crash dumps handled.
  int requests_handled() const {
    return requests_handled_;
  }
  // Returns number of crash clients registered.
  int clients_connected() const {
    return clients_connected_;
  }
  // Returns number of crash clients terminated.
  int clients_terminated() const {
    return clients_terminated_;
  }

  // Starts the processing loop. This function does not return unless the
  // user is logging off or the user closes the crash service window. The
  // return value is a good number to pass in ExitProcess().
  int ProcessingLoop();

 private:
  static void OnClientConnected(void* context,
                                const google_breakpad::ClientInfo* client_info);

  static void OnClientDumpRequest(
      void* context,
      const google_breakpad::ClientInfo* client_info,
      const std::wstring* file_path);

  static void OnClientExited(void* context,
                             const google_breakpad::ClientInfo* client_info);

  // This routine sends the crash dump to the server. It takes the sending_
  // lock when it is performing the send.
  static DWORD __stdcall AsyncSendDump(void* context);

  // Returns the security descriptor which access to low integrity processes
  // The caller is supposed to free the security descriptor by calling
  // LocalFree.
  PSECURITY_DESCRIPTOR GetSecurityDescriptorForLowIntegrity();

  google_breakpad::CrashGenerationServer* dumper_;
  google_breakpad::CrashReportSender* sender_;

  // the extra tag sent to the server with each dump.
  std::wstring reporter_tag_;

  // receiver URL of crash reports.
  std::wstring reporter_url_;

  // clients serviced statistics:
  int requests_handled_;
  int requests_sent_;
  volatile LONG clients_connected_;
  volatile LONG clients_terminated_;
  base::Lock sending_;

  DISALLOW_COPY_AND_ASSIGN(CrashService);
};

}  // namespace breakpad

#endif  // ATOM_COMMON_CRASH_REPORTER_WIN_CRASH_SERVICE_H_
