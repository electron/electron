// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// On Linux, when the user tries to launch a second copy of chrome, we check
// for a socket in the user's profile directory.  If the socket file is open we
// send a message to the first chrome browser process with the current
// directory and second process command line flags.  The second process then
// exits.
//
// Because many networked filesystem implementations do not support unix domain
// sockets, we create the socket in a temporary directory and create a symlink
// in the profile. This temporary directory is no longer bound to the profile,
// and may disappear across a reboot or login to a separate session. To bind
// them, we store a unique cookie in the profile directory, which must also be
// present in the remote directory to connect. The cookie is checked both before
// and after the connection. /tmp is sticky, and different Chrome sessions use
// different cookies. Thus, a matching cookie before and after means the
// connection was to a directory with a valid cookie.
//
// We also have a lock file, which is a symlink to a non-existent destination.
// The destination is a string containing the hostname and process id of
// chrome's browser process, eg. "SingletonLock -> example.com-9156".  When the
// first copy of chrome exits it will delete the lock file on shutdown, so that
// a different instance on a different host may then use the profile directory.
//
// If writing to the socket fails, the hostname in the lock is checked to see if
// another instance is running a different host using a shared filesystem (nfs,
// etc.) If the hostname differs an error is displayed and the second process
// exits.  Otherwise the first process (if any) is killed and the second process
// starts as normal.
//
// When the second process sends the current directory and command line flags to
// the first process, it waits for an ACK message back from the first process
// for a certain time. If there is no ACK message back in time, then the first
// process will be considered as hung for some reason. The second process then
// retrieves the process id from the symbol link and kills it by sending
// SIGKILL. Then the second process starts as normal.

#include "chrome/browser/process_singleton.h"

#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>

#include <cstring>
#include <memory>
#include <set>
#include <string>

#include <stddef.h>

#include "atom/browser/browser.h"
#include "atom/common/atom_command_line.h"

#include "base/base_paths.h"
#include "base/bind.h"
#include "base/command_line.h"
#include "base/files/file_descriptor_watcher_posix.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/location.h"
#include "base/logging.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/message_loop/message_loop.h"
#include "base/metrics/histogram_macros.h"
#include "base/path_service.h"
#include "base/posix/eintr_wrapper.h"
#include "base/posix/safe_strerror.h"
#include "base/rand_util.h"
#include "base/sequenced_task_runner_helpers.h"
#include "base/single_thread_task_runner.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "base/strings/sys_string_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "base/threading/platform_thread.h"
#include "base/threading/thread_restrictions.h"
#include "base/threading/thread_task_runner_handle.h"
#include "base/time/time.h"
#include "base/timer/timer.h"
#include "build/build_config.h"
#include "content/public/browser/browser_thread.h"
#include "net/base/network_interfaces.h"
#include "ui/base/l10n/l10n_util.h"

#if defined(TOOLKIT_VIEWS) && defined(OS_LINUX) && !defined(OS_CHROMEOS)
#include "ui/views/linux_ui/linux_ui.h"
#endif

using content::BrowserThread;

namespace {

// Timeout for the current browser process to respond. 20 seconds should be
// enough.
const int kTimeoutInSeconds = 20;
// Number of retries to notify the browser. 20 retries over 20 seconds = 1 try
// per second.
const int kRetryAttempts = 20;
static bool g_disable_prompt;
const char kStartToken[] = "START";
const char kACKToken[] = "ACK";
const char kShutdownToken[] = "SHUTDOWN";
const char kTokenDelimiter = '\0';
const int kMaxMessageLength = 32 * 1024;
const int kMaxACKMessageLength = arraysize(kShutdownToken) - 1;

const char kLockDelimiter = '-';

const base::FilePath::CharType kSingletonCookieFilename[] =
    FILE_PATH_LITERAL("SingletonCookie");

const base::FilePath::CharType kSingletonLockFilename[] =
    FILE_PATH_LITERAL("SingletonLock");
const base::FilePath::CharType kSingletonSocketFilename[] =
    FILE_PATH_LITERAL("SS");

// Set the close-on-exec bit on a file descriptor.
// Returns 0 on success, -1 on failure.
int SetCloseOnExec(int fd) {
  int flags = fcntl(fd, F_GETFD, 0);
  if (-1 == flags)
    return flags;
  if (flags & FD_CLOEXEC)
    return 0;
  return fcntl(fd, F_SETFD, flags | FD_CLOEXEC);
}

// Close a socket and check return value.
void CloseSocket(int fd) {
  int rv = IGNORE_EINTR(close(fd));
  DCHECK_EQ(0, rv) << "Error closing socket: " << base::safe_strerror(errno);
}

// Write a message to a socket fd.
bool WriteToSocket(int fd, const char* message, size_t length) {
  DCHECK(message);
  DCHECK(length);
  size_t bytes_written = 0;
  do {
    ssize_t rv = HANDLE_EINTR(
        write(fd, message + bytes_written, length - bytes_written));
    if (rv < 0) {
      if (errno == EAGAIN || errno == EWOULDBLOCK) {
        // The socket shouldn't block, we're sending so little data.  Just give
        // up here, since NotifyOtherProcess() doesn't have an asynchronous api.
        LOG(ERROR) << "ProcessSingleton would block on write(), so it gave up.";
        return false;
      }
      PLOG(ERROR) << "write() failed";
      return false;
    }
    bytes_written += rv;
  } while (bytes_written < length);

  return true;
}

struct timeval TimeDeltaToTimeVal(const base::TimeDelta& delta) {
  struct timeval result;
  result.tv_sec = delta.InSeconds();
  result.tv_usec = delta.InMicroseconds() % base::Time::kMicrosecondsPerSecond;
  return result;
}

// Wait a socket for read for a certain timeout.
// Returns -1 if error occurred, 0 if timeout reached, > 0 if the socket is
// ready for read.
int WaitSocketForRead(int fd, const base::TimeDelta& timeout) {
  fd_set read_fds;
  struct timeval tv = TimeDeltaToTimeVal(timeout);

  FD_ZERO(&read_fds);
  FD_SET(fd, &read_fds);

  return HANDLE_EINTR(select(fd + 1, &read_fds, NULL, NULL, &tv));
}

// Read a message from a socket fd, with an optional timeout.
// If |timeout| <= 0 then read immediately.
// Return number of bytes actually read, or -1 on error.
ssize_t ReadFromSocket(int fd,
                       char* buf,
                       size_t bufsize,
                       const base::TimeDelta& timeout) {
  if (timeout > base::TimeDelta()) {
    int rv = WaitSocketForRead(fd, timeout);
    if (rv <= 0)
      return rv;
  }

  size_t bytes_read = 0;
  do {
    ssize_t rv = HANDLE_EINTR(read(fd, buf + bytes_read, bufsize - bytes_read));
    if (rv < 0) {
      if (errno != EAGAIN && errno != EWOULDBLOCK) {
        PLOG(ERROR) << "read() failed";
        return rv;
      } else {
        // It would block, so we just return what has been read.
        return bytes_read;
      }
    } else if (!rv) {
      // No more data to read.
      return bytes_read;
    } else {
      bytes_read += rv;
    }
  } while (bytes_read < bufsize);

  return bytes_read;
}

// Set up a sockaddr appropriate for messaging.
void SetupSockAddr(const std::string& path, struct sockaddr_un* addr) {
  addr->sun_family = AF_UNIX;
  CHECK(path.length() < arraysize(addr->sun_path))
      << "Socket path too long: " << path;
  base::strlcpy(addr->sun_path, path.c_str(), arraysize(addr->sun_path));
}

// Set up a socket appropriate for messaging.
int SetupSocketOnly() {
  int sock = socket(PF_UNIX, SOCK_STREAM, 0);
  PCHECK(sock >= 0) << "socket() failed";

  DCHECK(base::SetNonBlocking(sock)) << "Failed to make non-blocking socket.";
  int rv = SetCloseOnExec(sock);
  DCHECK_EQ(0, rv) << "Failed to set CLOEXEC on socket.";

  return sock;
}

// Set up a socket and sockaddr appropriate for messaging.
void SetupSocket(const std::string& path, int* sock, struct sockaddr_un* addr) {
  *sock = SetupSocketOnly();
  SetupSockAddr(path, addr);
}

// Read a symbolic link, return empty string if given path is not a symbol link.
base::FilePath ReadLink(const base::FilePath& path) {
  base::FilePath target;
  if (!base::ReadSymbolicLink(path, &target)) {
    // The only errno that should occur is ENOENT.
    if (errno != 0 && errno != ENOENT)
      PLOG(ERROR) << "readlink(" << path.value() << ") failed";
  }
  return target;
}

// Unlink a path. Return true on success.
bool UnlinkPath(const base::FilePath& path) {
  int rv = unlink(path.value().c_str());
  if (rv < 0 && errno != ENOENT)
    PLOG(ERROR) << "Failed to unlink " << path.value();

  return rv == 0;
}

// Create a symlink. Returns true on success.
bool SymlinkPath(const base::FilePath& target, const base::FilePath& path) {
  if (!base::CreateSymbolicLink(target, path)) {
    // Double check the value in case symlink suceeded but we got an incorrect
    // failure due to NFS packet loss & retry.
    int saved_errno = errno;
    if (ReadLink(path) != target) {
      // If we failed to create the lock, most likely another instance won the
      // startup race.
      errno = saved_errno;
      PLOG(ERROR) << "Failed to create " << path.value();
      return false;
    }
  }
  return true;
}

// Extract the hostname and pid from the lock symlink.
// Returns true if the lock existed.
bool ParseLockPath(const base::FilePath& path,
                   std::string* hostname,
                   int* pid) {
  std::string real_path = ReadLink(path).value();
  if (real_path.empty())
    return false;

  std::string::size_type pos = real_path.rfind(kLockDelimiter);

  // If the path is not a symbolic link, or doesn't contain what we expect,
  // bail.
  if (pos == std::string::npos) {
    *hostname = "";
    *pid = -1;
    return true;
  }

  *hostname = real_path.substr(0, pos);

  const std::string& pid_str = real_path.substr(pos + 1);
  if (!base::StringToInt(pid_str, pid))
    *pid = -1;

  return true;
}

// Returns true if the user opted to unlock the profile.
bool DisplayProfileInUseError(const base::FilePath& lock_path,
                              const std::string& hostname,
                              int pid) {
  return true;
}

bool IsChromeProcess(pid_t pid) {
  base::FilePath other_chrome_path(base::GetProcessExecutablePath(pid));

  auto* command_line = base::CommandLine::ForCurrentProcess();
  base::FilePath exec_path(command_line->GetProgram());
  base::PathService::Get(base::FILE_EXE, &exec_path);

  return (!other_chrome_path.empty() &&
          other_chrome_path.BaseName() == exec_path.BaseName());
}

// A helper class to hold onto a socket.
class ScopedSocket {
 public:
  ScopedSocket() : fd_(-1) { Reset(); }
  ~ScopedSocket() { Close(); }
  int fd() { return fd_; }
  void Reset() {
    Close();
    fd_ = SetupSocketOnly();
  }
  void Close() {
    if (fd_ >= 0)
      CloseSocket(fd_);
    fd_ = -1;
  }

 private:
  int fd_;
};

// Returns a random string for uniquifying profile connections.
std::string GenerateCookie() {
  return base::NumberToString(base::RandUint64());
}

bool CheckCookie(const base::FilePath& path, const base::FilePath& cookie) {
  return (cookie == ReadLink(path));
}

bool IsAppSandboxed() {
#if defined(OS_MACOSX)
  // NB: There is no sane API for this, we have to just guess by
  // reading tea leaves
  base::FilePath home_dir;
  if (!base::PathService::Get(base::DIR_HOME, &home_dir)) {
    return false;
  }

  return home_dir.value().find("Library/Containers") != std::string::npos;
#else
  return false;
#endif  // defined(OS_MACOSX)
}

bool ConnectSocket(ScopedSocket* socket,
                   const base::FilePath& socket_path,
                   const base::FilePath& cookie_path) {
  base::FilePath socket_target;
  if (base::ReadSymbolicLink(socket_path, &socket_target)) {
    // It's a symlink. Read the cookie.
    base::FilePath cookie = ReadLink(cookie_path);
    if (cookie.empty())
      return false;
    base::FilePath remote_cookie =
        socket_target.DirName().Append(kSingletonCookieFilename);
    // Verify the cookie before connecting.
    if (!CheckCookie(remote_cookie, cookie))
      return false;
    // Now we know the directory was (at that point) created by the profile
    // owner. Try to connect.
    sockaddr_un addr;
    SetupSockAddr(socket_target.value(), &addr);
    int ret = HANDLE_EINTR(connect(
        socket->fd(), reinterpret_cast<sockaddr*>(&addr), sizeof(addr)));
    if (ret != 0)
      return false;
    // Check the cookie again. We only link in /tmp, which is sticky, so, if the
    // directory is still correct, it must have been correct in-between when we
    // connected. POSIX, sadly, lacks a connectat().
    if (!CheckCookie(remote_cookie, cookie)) {
      socket->Reset();
      return false;
    }
    // Success!
    return true;
  } else if (errno == EINVAL) {
    // It exists, but is not a symlink (or some other error we detect
    // later). Just connect to it directly; this is an older version of Chrome.
    sockaddr_un addr;
    SetupSockAddr(socket_path.value(), &addr);
    int ret = HANDLE_EINTR(connect(
        socket->fd(), reinterpret_cast<sockaddr*>(&addr), sizeof(addr)));
    return (ret == 0);
  } else {
    // File is missing, or other error.
    if (errno != ENOENT)
      PLOG(ERROR) << "readlink failed";
    return false;
  }
}

#if defined(OS_MACOSX)
bool ReplaceOldSingletonLock(const base::FilePath& symlink_content,
                             const base::FilePath& lock_path) {
  // Try taking an flock(2) on the file. Failure means the lock is taken so we
  // should quit.
  base::ScopedFD lock_fd(HANDLE_EINTR(
      open(lock_path.value().c_str(), O_RDWR | O_CREAT | O_SYMLINK, 0644)));
  if (!lock_fd.is_valid()) {
    PLOG(ERROR) << "Could not open singleton lock";
    return false;
  }

  int rc = HANDLE_EINTR(flock(lock_fd.get(), LOCK_EX | LOCK_NB));
  if (rc == -1) {
    if (errno == EWOULDBLOCK) {
      LOG(ERROR) << "Singleton lock held by old process.";
    } else {
      PLOG(ERROR) << "Error locking singleton lock";
    }
    return false;
  }

  // Successfully taking the lock means we can replace it with the a new symlink
  // lock. We never flock() the lock file from now on. I.e. we assume that an
  // old version of Chrome will not run with the same user data dir after this
  // version has run.
  if (!base::DeleteFile(lock_path, false)) {
    PLOG(ERROR) << "Could not delete old singleton lock.";
    return false;
  }

  return SymlinkPath(symlink_content, lock_path);
}
#endif  // defined(OS_MACOSX)

}  // namespace

///////////////////////////////////////////////////////////////////////////////
// ProcessSingleton::LinuxWatcher
// A helper class for a Linux specific implementation of the process singleton.
// This class sets up a listener on the singleton socket and handles parsing
// messages that come in on the singleton socket.
class ProcessSingleton::LinuxWatcher
    : public base::RefCountedThreadSafe<ProcessSingleton::LinuxWatcher,
                                        BrowserThread::DeleteOnIOThread> {
 public:
  // A helper class to read message from an established socket.
  class SocketReader {
   public:
    SocketReader(ProcessSingleton::LinuxWatcher* parent,
                 scoped_refptr<base::SingleThreadTaskRunner> ui_task_runner,
                 int fd)
        : parent_(parent),
          ui_task_runner_(ui_task_runner),
          fd_(fd),
          bytes_read_(0) {
      DCHECK_CURRENTLY_ON(BrowserThread::IO);
      // Wait for reads.
      fd_watch_controller_ = base::FileDescriptorWatcher::WatchReadable(
          fd, base::Bind(&SocketReader::OnSocketCanReadWithoutBlocking,
                         base::Unretained(this)));
      // If we haven't completed in a reasonable amount of time, give up.
      timer_.Start(FROM_HERE, base::TimeDelta::FromSeconds(kTimeoutInSeconds),
                   this, &SocketReader::CleanupAndDeleteSelf);
    }

    ~SocketReader() { CloseSocket(fd_); }

    // Finish handling the incoming message by optionally sending back an ACK
    // message and removing this SocketReader.
    void FinishWithACK(const char* message, size_t length);

   private:
    void OnSocketCanReadWithoutBlocking();

    void CleanupAndDeleteSelf() {
      DCHECK_CURRENTLY_ON(BrowserThread::IO);

      parent_->RemoveSocketReader(this);
      // We're deleted beyond this point.
    }

    // Controls watching |fd_|.
    std::unique_ptr<base::FileDescriptorWatcher::Controller>
        fd_watch_controller_;

    // The ProcessSingleton::LinuxWatcher that owns us.
    ProcessSingleton::LinuxWatcher* const parent_;

    // A reference to the UI task runner.
    scoped_refptr<base::SingleThreadTaskRunner> ui_task_runner_;

    // The file descriptor we're reading.
    const int fd_;

    // Store the message in this buffer.
    char buf_[kMaxMessageLength];

    // Tracks the number of bytes we've read in case we're getting partial
    // reads.
    size_t bytes_read_;

    base::OneShotTimer timer_;

    DISALLOW_COPY_AND_ASSIGN(SocketReader);
  };

  // We expect to only be constructed on the UI thread.
  explicit LinuxWatcher(ProcessSingleton* parent)
      : ui_task_runner_(base::ThreadTaskRunnerHandle::Get()), parent_(parent) {}

  // Start listening for connections on the socket.  This method should be
  // called from the IO thread.
  void StartListening(int socket);

  // This method determines if we should use the same process and if we should,
  // opens a new browser tab.  This runs on the UI thread.
  // |reader| is for sending back ACK message.
  void HandleMessage(const std::string& current_dir,
                     const std::vector<std::string>& argv,
                     SocketReader* reader);

 private:
  friend struct BrowserThread::DeleteOnThread<BrowserThread::IO>;
  friend class base::DeleteHelper<ProcessSingleton::LinuxWatcher>;

  ~LinuxWatcher() { DCHECK_CURRENTLY_ON(BrowserThread::IO); }

  void OnSocketCanReadWithoutBlocking(int socket);

  // Removes and deletes the SocketReader.
  void RemoveSocketReader(SocketReader* reader);

  std::unique_ptr<base::FileDescriptorWatcher::Controller> socket_watcher_;

  // A reference to the UI message loop (i.e., the message loop we were
  // constructed on).
  scoped_refptr<base::SingleThreadTaskRunner> ui_task_runner_;

  // The ProcessSingleton that owns us.
  ProcessSingleton* const parent_;

  std::set<std::unique_ptr<SocketReader>> readers_;

  DISALLOW_COPY_AND_ASSIGN(LinuxWatcher);
};

void ProcessSingleton::LinuxWatcher::OnSocketCanReadWithoutBlocking(
    int socket) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  // Accepting incoming client.
  sockaddr_un from;
  socklen_t from_len = sizeof(from);
  int connection_socket = HANDLE_EINTR(
      accept(socket, reinterpret_cast<sockaddr*>(&from), &from_len));
  if (-1 == connection_socket) {
    PLOG(ERROR) << "accept() failed";
    return;
  }
  DCHECK(base::SetNonBlocking(connection_socket))
      << "Failed to make non-blocking socket.";
  readers_.insert(
      std::make_unique<SocketReader>(this, ui_task_runner_, connection_socket));
}

void ProcessSingleton::LinuxWatcher::StartListening(int socket) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  // Watch for client connections on this socket.
  socket_watcher_ = base::FileDescriptorWatcher::WatchReadable(
      socket, base::Bind(&LinuxWatcher::OnSocketCanReadWithoutBlocking,
                         base::Unretained(this), socket));
}

void ProcessSingleton::LinuxWatcher::HandleMessage(
    const std::string& current_dir,
    const std::vector<std::string>& argv,
    SocketReader* reader) {
  DCHECK(ui_task_runner_->BelongsToCurrentThread());
  DCHECK(reader);

  if (parent_->notification_callback_.Run(argv, base::FilePath(current_dir))) {
    // Send back "ACK" message to prevent the client process from starting up.
    reader->FinishWithACK(kACKToken, arraysize(kACKToken) - 1);
  } else {
    LOG(WARNING) << "Not handling interprocess notification as browser"
                    " is shutting down";
    // Send back "SHUTDOWN" message, so that the client process can start up
    // without killing this process.
    reader->FinishWithACK(kShutdownToken, arraysize(kShutdownToken) - 1);
    return;
  }
}

void ProcessSingleton::LinuxWatcher::RemoveSocketReader(SocketReader* reader) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  DCHECK(reader);
  auto it = std::find_if(readers_.begin(), readers_.end(),
                         [reader](const std::unique_ptr<SocketReader>& ptr) {
                           return ptr.get() == reader;
                         });
  readers_.erase(it);
}

///////////////////////////////////////////////////////////////////////////////
// ProcessSingleton::LinuxWatcher::SocketReader
//

void ProcessSingleton::LinuxWatcher::SocketReader::
    OnSocketCanReadWithoutBlocking() {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  while (bytes_read_ < sizeof(buf_)) {
    ssize_t rv =
        HANDLE_EINTR(read(fd_, buf_ + bytes_read_, sizeof(buf_) - bytes_read_));
    if (rv < 0) {
      if (errno != EAGAIN && errno != EWOULDBLOCK) {
        PLOG(ERROR) << "read() failed";
        CloseSocket(fd_);
        return;
      } else {
        // It would block, so we just return and continue to watch for the next
        // opportunity to read.
        return;
      }
    } else if (!rv) {
      // No more data to read.  It's time to process the message.
      break;
    } else {
      bytes_read_ += rv;
    }
  }

  // Validate the message.  The shortest message is kStartToken\0x\0x
  const size_t kMinMessageLength = arraysize(kStartToken) + 4;
  if (bytes_read_ < kMinMessageLength) {
    buf_[bytes_read_] = 0;
    LOG(ERROR) << "Invalid socket message (wrong length):" << buf_;
    CleanupAndDeleteSelf();
    return;
  }

  std::string str(buf_, bytes_read_);
  std::vector<std::string> tokens =
      base::SplitString(str, std::string(1, kTokenDelimiter),
                        base::TRIM_WHITESPACE, base::SPLIT_WANT_ALL);

  if (tokens.size() < 3 || tokens[0] != kStartToken) {
    LOG(ERROR) << "Wrong message format: " << str;
    CleanupAndDeleteSelf();
    return;
  }

  // Stop the expiration timer to prevent this SocketReader object from being
  // terminated unexpectly.
  timer_.Stop();

  std::string current_dir = tokens[1];
  // Remove the first two tokens.  The remaining tokens should be the command
  // line argv array.
  tokens.erase(tokens.begin());
  tokens.erase(tokens.begin());

  // Return to the UI thread to handle opening a new browser tab.
  ui_task_runner_->PostTask(
      FROM_HERE, base::Bind(&ProcessSingleton::LinuxWatcher::HandleMessage,
                            parent_, current_dir, tokens, this));
  fd_watch_controller_.reset();

  // LinuxWatcher::HandleMessage() is in charge of destroying this SocketReader
  // object by invoking SocketReader::FinishWithACK().
}

void ProcessSingleton::LinuxWatcher::SocketReader::FinishWithACK(
    const char* message,
    size_t length) {
  if (message && length) {
    // Not necessary to care about the return value.
    WriteToSocket(fd_, message, length);
  }

  if (shutdown(fd_, SHUT_WR) < 0)
    PLOG(ERROR) << "shutdown() failed";

  BrowserThread::PostTask(
      BrowserThread::IO, FROM_HERE,
      base::Bind(&ProcessSingleton::LinuxWatcher::RemoveSocketReader, parent_,
                 this));
  // We will be deleted once the posted RemoveSocketReader task runs.
}

///////////////////////////////////////////////////////////////////////////////
// ProcessSingleton
//
ProcessSingleton::ProcessSingleton(
    const base::FilePath& user_data_dir,
    const NotificationCallback& notification_callback)
    : notification_callback_(notification_callback),
      current_pid_(base::GetCurrentProcId()) {
  // The user_data_dir may have not been created yet.
  base::ThreadRestrictions::ScopedAllowIO allow_io;
  base::CreateDirectoryAndGetError(user_data_dir, nullptr);

  socket_path_ = user_data_dir.Append(kSingletonSocketFilename);
  lock_path_ = user_data_dir.Append(kSingletonLockFilename);
  cookie_path_ = user_data_dir.Append(kSingletonCookieFilename);

  kill_callback_ =
      base::Bind(&ProcessSingleton::KillProcess, base::Unretained(this));
}

ProcessSingleton::~ProcessSingleton() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  // Manually free resources with IO explicitly allowed.
  base::ThreadRestrictions::ScopedAllowIO allow_io;
  watcher_ = nullptr;
  ignore_result(socket_dir_.Delete());
}

ProcessSingleton::NotifyResult ProcessSingleton::NotifyOtherProcess() {
  return NotifyOtherProcessWithTimeout(
      *base::CommandLine::ForCurrentProcess(), kRetryAttempts,
      base::TimeDelta::FromSeconds(kTimeoutInSeconds), true);
}

ProcessSingleton::NotifyResult ProcessSingleton::NotifyOtherProcessWithTimeout(
    const base::CommandLine& cmd_line,
    int retry_attempts,
    const base::TimeDelta& timeout,
    bool kill_unresponsive) {
  DCHECK_GE(retry_attempts, 0);
  DCHECK_GE(timeout.InMicroseconds(), 0);

  base::TimeDelta sleep_interval = timeout / retry_attempts;

  ScopedSocket socket;
  for (int retries = 0; retries <= retry_attempts; ++retries) {
    // Try to connect to the socket.
    if (ConnectSocket(&socket, socket_path_, cookie_path_))
      break;

    // If we're in a race with another process, they may be in Create() and have
    // created the lock but not attached to the socket.  So we check if the
    // process with the pid from the lockfile is currently running and is a
    // chrome browser.  If so, we loop and try again for |timeout|.

    std::string hostname;
    int pid;
    if (!ParseLockPath(lock_path_, &hostname, &pid)) {
      // No lockfile exists.
      return PROCESS_NONE;
    }

    if (hostname.empty()) {
      // Invalid lockfile.
      UnlinkPath(lock_path_);
      return PROCESS_NONE;
    }

    if (hostname != net::GetHostName() && !IsChromeProcess(pid)) {
      // Locked by process on another host. If the user selected to unlock
      // the profile, try to continue; otherwise quit.
      if (DisplayProfileInUseError(lock_path_, hostname, pid)) {
        UnlinkPath(lock_path_);
        return PROCESS_NONE;
      }
      return PROFILE_IN_USE;
    }

    if (!IsChromeProcess(pid)) {
      // Orphaned lockfile (no process with pid, or non-chrome process.)
      UnlinkPath(lock_path_);
      return PROCESS_NONE;
    }

    if (IsSameChromeInstance(pid)) {
      // Orphaned lockfile (pid is part of same chrome instance we are, even
      // though we haven't tried to create a lockfile yet).
      UnlinkPath(lock_path_);
      return PROCESS_NONE;
    }

    if (retries == retry_attempts) {
      // Retries failed.  Kill the unresponsive chrome process and continue.
      if (!kill_unresponsive || !KillProcessByLockPath())
        return PROFILE_IN_USE;
      return PROCESS_NONE;
    }

    base::PlatformThread::Sleep(sleep_interval);
  }

  timeval socket_timeout = TimeDeltaToTimeVal(timeout);
  setsockopt(socket.fd(), SOL_SOCKET, SO_SNDTIMEO, &socket_timeout,
             sizeof(socket_timeout));

  // Found another process, prepare our command line
  // format is "START\0<current dir>\0<argv[0]>\0...\0<argv[n]>".
  std::string to_send(kStartToken);
  to_send.push_back(kTokenDelimiter);

  base::FilePath current_dir;
  if (!base::PathService::Get(base::DIR_CURRENT, &current_dir))
    return PROCESS_NONE;
  to_send.append(current_dir.value());

  const std::vector<std::string>& argv = atom::AtomCommandLine::argv();
  for (std::vector<std::string>::const_iterator it = argv.begin();
       it != argv.end(); ++it) {
    to_send.push_back(kTokenDelimiter);
    to_send.append(*it);
  }

  // Send the message
  if (!WriteToSocket(socket.fd(), to_send.data(), to_send.length())) {
    // Try to kill the other process, because it might have been dead.
    if (!kill_unresponsive || !KillProcessByLockPath())
      return PROFILE_IN_USE;
    return PROCESS_NONE;
  }

  if (shutdown(socket.fd(), SHUT_WR) < 0)
    PLOG(ERROR) << "shutdown() failed";

  // Read ACK message from the other process. It might be blocked for a certain
  // timeout, to make sure the other process has enough time to return ACK.
  char buf[kMaxACKMessageLength + 1];
  ssize_t len = ReadFromSocket(socket.fd(), buf, kMaxACKMessageLength, timeout);

  // Failed to read ACK, the other process might have been frozen.
  if (len <= 0) {
    if (!kill_unresponsive || !KillProcessByLockPath())
      return PROFILE_IN_USE;
    return PROCESS_NONE;
  }

  buf[len] = '\0';
  if (strncmp(buf, kShutdownToken, arraysize(kShutdownToken) - 1) == 0) {
    // The other process is shutting down, it's safe to start a new process.
    return PROCESS_NONE;
  } else if (strncmp(buf, kACKToken, arraysize(kACKToken) - 1) == 0) {
#if defined(TOOLKIT_VIEWS) && defined(OS_LINUX) && !defined(OS_CHROMEOS)
    // Likely NULL in unit tests.
    views::LinuxUI* linux_ui = views::LinuxUI::instance();
    if (linux_ui)
      linux_ui->NotifyWindowManagerStartupComplete();
#endif

    // Assume the other process is handling the request.
    return PROCESS_NOTIFIED;
  }

  NOTREACHED() << "The other process returned unknown message: " << buf;
  return PROCESS_NOTIFIED;
}

ProcessSingleton::NotifyResult ProcessSingleton::NotifyOtherProcessOrCreate() {
  return NotifyOtherProcessWithTimeoutOrCreate(
      *base::CommandLine::ForCurrentProcess(), kRetryAttempts,
      base::TimeDelta::FromSeconds(kTimeoutInSeconds));
}

void ProcessSingleton::StartListeningOnSocket() {
  watcher_ = new LinuxWatcher(this);
  BrowserThread::PostTask(
      BrowserThread::IO, FROM_HERE,
      base::Bind(&ProcessSingleton::LinuxWatcher::StartListening, watcher_,
                 sock_));
}

void ProcessSingleton::OnBrowserReady() {
  if (listen_on_ready_) {
    StartListeningOnSocket();
    listen_on_ready_ = false;
  }
}

ProcessSingleton::NotifyResult
ProcessSingleton::NotifyOtherProcessWithTimeoutOrCreate(
    const base::CommandLine& command_line,
    int retry_attempts,
    const base::TimeDelta& timeout) {
  const base::TimeTicks begin_ticks = base::TimeTicks::Now();
  NotifyResult result = NotifyOtherProcessWithTimeout(
      command_line, retry_attempts, timeout, true);
  if (result != PROCESS_NONE) {
    if (result == PROCESS_NOTIFIED) {
      UMA_HISTOGRAM_MEDIUM_TIMES("Chrome.ProcessSingleton.TimeToNotify",
                                 base::TimeTicks::Now() - begin_ticks);
    } else {
      UMA_HISTOGRAM_MEDIUM_TIMES("Chrome.ProcessSingleton.TimeToFailure",
                                 base::TimeTicks::Now() - begin_ticks);
    }
    return result;
  }

  if (Create()) {
    UMA_HISTOGRAM_MEDIUM_TIMES("Chrome.ProcessSingleton.TimeToCreate",
                               base::TimeTicks::Now() - begin_ticks);
    return PROCESS_NONE;
  }

  // If the Create() failed, try again to notify. (It could be that another
  // instance was starting at the same time and managed to grab the lock before
  // we did.)
  // This time, we don't want to kill anything if we aren't successful, since we
  // aren't going to try to take over the lock ourselves.
  result = NotifyOtherProcessWithTimeout(command_line, retry_attempts, timeout,
                                         false);

  if (result == PROCESS_NOTIFIED) {
    UMA_HISTOGRAM_MEDIUM_TIMES("Chrome.ProcessSingleton.TimeToNotify",
                               base::TimeTicks::Now() - begin_ticks);
  } else {
    UMA_HISTOGRAM_MEDIUM_TIMES("Chrome.ProcessSingleton.TimeToFailure",
                               base::TimeTicks::Now() - begin_ticks);
  }

  if (result != PROCESS_NONE)
    return result;

  return LOCK_ERROR;
}

void ProcessSingleton::OverrideCurrentPidForTesting(base::ProcessId pid) {
  current_pid_ = pid;
}

void ProcessSingleton::OverrideKillCallbackForTesting(
    const base::Callback<void(int)>& callback) {
  kill_callback_ = callback;
}

void ProcessSingleton::DisablePromptForTesting() {
  g_disable_prompt = true;
}

bool ProcessSingleton::Create() {
  base::ThreadRestrictions::ScopedAllowIO allow_io;
  int sock;
  sockaddr_un addr;

  // The symlink lock is pointed to the hostname and process id, so other
  // processes can find it out.
  base::FilePath symlink_content(base::StringPrintf(
      "%s%c%u", net::GetHostName().c_str(), kLockDelimiter, current_pid_));

  // Create symbol link before binding the socket, to ensure only one instance
  // can have the socket open.
  if (!SymlinkPath(symlink_content, lock_path_)) {
    // TODO(jackhou): Remove this case once this code is stable on Mac.
    // http://crbug.com/367612
#if defined(OS_MACOSX)
    // On Mac, an existing non-symlink lock file means the lock could be held by
    // the old process singleton code. If we can successfully replace the lock,
    // continue as normal.
    if (base::IsLink(lock_path_) ||
        !ReplaceOldSingletonLock(symlink_content, lock_path_)) {
      return false;
    }
#else
    // If we failed to create the lock, most likely another instance won the
    // startup race.
    return false;
#endif
  }

  if (IsAppSandboxed()) {
    // For sandboxed applications, the tmp dir could be too long to fit
    // addr->sun_path, so we need to make it as short as possible.
    base::FilePath tmp_dir;
    if (!base::GetTempDir(&tmp_dir)) {
      LOG(ERROR) << "Failed to get temporary directory.";
      return false;
    }
    if (!socket_dir_.Set(tmp_dir.Append("S"))) {
      LOG(ERROR) << "Failed to set socket directory.";
      return false;
    }
  } else {
    // Create the socket file somewhere in /tmp which is usually mounted as a
    // normal filesystem. Some network filesystems (notably AFS) are screwy and
    // do not support Unix domain sockets.
    if (!socket_dir_.CreateUniqueTempDir()) {
      LOG(ERROR) << "Failed to create socket directory.";
      return false;
    }
  }

  // Check that the directory was created with the correct permissions.
  int dir_mode = 0;
  CHECK(base::GetPosixFilePermissions(socket_dir_.GetPath(), &dir_mode) &&
        dir_mode == base::FILE_PERMISSION_USER_MASK)
      << "Temp directory mode is not 700: " << std::oct << dir_mode;

  // Setup the socket symlink and the two cookies.
  base::FilePath socket_target_path =
      socket_dir_.GetPath().Append(kSingletonSocketFilename);
  base::FilePath cookie(GenerateCookie());
  base::FilePath remote_cookie_path =
      socket_dir_.GetPath().Append(kSingletonCookieFilename);
  UnlinkPath(socket_path_);
  UnlinkPath(cookie_path_);
  if (!SymlinkPath(socket_target_path, socket_path_) ||
      !SymlinkPath(cookie, cookie_path_) ||
      !SymlinkPath(cookie, remote_cookie_path)) {
    // We've already locked things, so we can't have lost the startup race,
    // but something doesn't like us.
    LOG(ERROR) << "Failed to create symlinks.";
    if (!socket_dir_.Delete())
      LOG(ERROR) << "Encountered a problem when deleting socket directory.";
    return false;
  }

  SetupSocket(socket_target_path.value(), &sock, &addr);

  if (bind(sock, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
    PLOG(ERROR) << "Failed to bind() " << socket_target_path.value();
    CloseSocket(sock);
    return false;
  }

  if (listen(sock, 5) < 0)
    NOTREACHED() << "listen failed: " << base::safe_strerror(errno);

  sock_ = sock;

  if (BrowserThread::IsThreadInitialized(BrowserThread::IO)) {
    StartListeningOnSocket();
  } else {
    listen_on_ready_ = true;
  }

  return true;
}

void ProcessSingleton::Cleanup() {
  UnlinkPath(socket_path_);
  UnlinkPath(cookie_path_);
  UnlinkPath(lock_path_);
}

bool ProcessSingleton::IsSameChromeInstance(pid_t pid) {
  pid_t cur_pid = current_pid_;
  while (pid != cur_pid) {
    pid = base::GetParentProcessId(pid);
    if (pid < 0)
      return false;
    if (!IsChromeProcess(pid))
      return false;
  }
  return true;
}

bool ProcessSingleton::KillProcessByLockPath() {
  std::string hostname;
  int pid;
  ParseLockPath(lock_path_, &hostname, &pid);

  if (!hostname.empty() && hostname != net::GetHostName()) {
    return DisplayProfileInUseError(lock_path_, hostname, pid);
  }
  UnlinkPath(lock_path_);

  if (IsSameChromeInstance(pid))
    return true;

  if (pid > 0) {
    kill_callback_.Run(pid);
    return true;
  }

  LOG(ERROR) << "Failed to extract pid from path: " << lock_path_.value();
  return true;
}

void ProcessSingleton::KillProcess(int pid) {
  // TODO(james.su@gmail.com): Is SIGKILL ok?
  int rv = kill(static_cast<base::ProcessHandle>(pid), SIGKILL);
  // ESRCH = No Such Process (can happen if the other process is already in
  // progress of shutting down and finishes before we try to kill it).
  DCHECK(rv == 0 || errno == ESRCH)
      << "Error killing process: " << base::safe_strerror(errno);
}
