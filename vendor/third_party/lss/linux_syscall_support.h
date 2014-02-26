/* Copyright (c) 2005-2011, Google Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * ---
 * Author: Markus Gutschke
 */

/* This file includes Linux-specific support functions common to the
 * coredumper and the thread lister; primarily, this is a collection
 * of direct system calls, and a couple of symbols missing from
 * standard header files.
 * There are a few options that the including file can set to control
 * the behavior of this file:
 *
 * SYS_CPLUSPLUS:
 *   The entire header file will normally be wrapped in 'extern "C" { }",
 *   making it suitable for compilation as both C and C++ source. If you
 *   do not want to do this, you can set the SYS_CPLUSPLUS macro to inhibit
 *   the wrapping. N.B. doing so will suppress inclusion of all prerequisite
 *   system header files, too. It is the caller's responsibility to provide
 *   the necessary definitions.
 *
 * SYS_ERRNO:
 *   All system calls will update "errno" unless overriden by setting the
 *   SYS_ERRNO macro prior to including this file. SYS_ERRNO should be
 *   an l-value.
 *
 * SYS_INLINE:
 *   New symbols will be defined "static inline", unless overridden by
 *   the SYS_INLINE macro.
 *
 * SYS_LINUX_SYSCALL_SUPPORT_H
 *   This macro is used to avoid multiple inclusions of this header file.
 *   If you need to include this file more than once, make sure to
 *   unset SYS_LINUX_SYSCALL_SUPPORT_H before each inclusion.
 *
 * SYS_PREFIX:
 *   New system calls will have a prefix of "sys_" unless overridden by
 *   the SYS_PREFIX macro. Valid values for this macro are [0..9] which
 *   results in prefixes "sys[0..9]_". It is also possible to set this
 *   macro to -1, which avoids all prefixes.
 *
 * SYS_SYSCALL_ENTRYPOINT:
 *   Some applications (such as sandboxes that filter system calls), need
 *   to be able to run custom-code each time a system call is made. If this
 *   macro is defined, it expands to the name of a "common" symbol. If
 *   this symbol is assigned a non-NULL pointer value, it is used as the
 *   address of the system call entrypoint.
 *   A pointer to this symbol can be obtained by calling
 *   get_syscall_entrypoint()
 *
 * This file defines a few internal symbols that all start with "LSS_".
 * Do not access these symbols from outside this file. They are not part
 * of the supported API.
 */
#ifndef SYS_LINUX_SYSCALL_SUPPORT_H
#define SYS_LINUX_SYSCALL_SUPPORT_H

/* We currently only support x86-32, x86-64, ARM, MIPS, and PPC on Linux.
 * Porting to other related platforms should not be difficult.
 */
#if (defined(__i386__) || defined(__x86_64__) || defined(__ARM_ARCH_3__) ||   \
     defined(__mips__) || defined(__PPC__) || defined(__ARM_EABI__)) \
  && (defined(__linux) || defined(__ANDROID__))

#ifndef SYS_CPLUSPLUS
#ifdef __cplusplus
/* Some system header files in older versions of gcc neglect to properly
 * handle being included from C++. As it appears to be harmless to have
 * multiple nested 'extern "C"' blocks, just add another one here.
 */
extern "C" {
#endif

#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <sys/ptrace.h>
#include <sys/resource.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <linux/unistd.h>
#include <endian.h>

#ifdef __mips__
/* Include definitions of the ABI currently in use.                          */
#include <sgidefs.h>
#endif
#endif

/* The Android NDK's <sys/stat.h> #defines these macros as aliases
 * to their non-64 counterparts. To avoid naming conflict, remove them. */
#ifdef __ANDROID__
  /* These are restored by the corresponding #pragma pop_macro near
   * the end of this file. */
# pragma push_macro("stat64")
# pragma push_macro("fstat64")
# pragma push_macro("lstat64")
# undef stat64
# undef fstat64
# undef lstat64
#endif

/* As glibc often provides subtly incompatible data structures (and implicit
 * wrapper functions that convert them), we provide our own kernel data
 * structures for use by the system calls.
 * These structures have been developed by using Linux 2.6.23 headers for
 * reference. Note though, we do not care about exact API compatibility
 * with the kernel, and in fact the kernel often does not have a single
 * API that works across architectures. Instead, we try to mimic the glibc
 * API where reasonable, and only guarantee ABI compatibility with the
 * kernel headers.
 * Most notably, here are a few changes that were made to the structures
 * defined by kernel headers:
 *
 * - we only define structures, but not symbolic names for kernel data
 *   types. For the latter, we directly use the native C datatype
 *   (i.e. "unsigned" instead of "mode_t").
 * - in a few cases, it is possible to define identical structures for
 *   both 32bit (e.g. i386) and 64bit (e.g. x86-64) platforms by
 *   standardizing on the 64bit version of the data types. In particular,
 *   this means that we use "unsigned" where the 32bit headers say
 *   "unsigned long".
 * - overall, we try to minimize the number of cases where we need to
 *   conditionally define different structures.
 * - the "struct kernel_sigaction" class of structures have been
 *   modified to more closely mimic glibc's API by introducing an
 *   anonymous union for the function pointer.
 * - a small number of field names had to have an underscore appended to
 *   them, because glibc defines a global macro by the same name.
 */

/* include/linux/dirent.h                                                    */
struct kernel_dirent64 {
  unsigned long long d_ino;
  long long          d_off;
  unsigned short     d_reclen;
  unsigned char      d_type;
  char               d_name[256];
};

/* include/linux/dirent.h                                                    */
struct kernel_dirent {
  long               d_ino;
  long               d_off;
  unsigned short     d_reclen;
  char               d_name[256];
};

/* include/linux/uio.h                                                       */
struct kernel_iovec {
  void               *iov_base;
  unsigned long      iov_len;
};

/* include/linux/socket.h                                                    */
struct kernel_msghdr {
  void               *msg_name;
  int                msg_namelen;
  struct kernel_iovec*msg_iov;
  unsigned long      msg_iovlen;
  void               *msg_control;
  unsigned long      msg_controllen;
  unsigned           msg_flags;
};

/* include/asm-generic/poll.h                                                */
struct kernel_pollfd {
  int                fd;
  short              events;
  short              revents;
};

/* include/linux/resource.h                                                  */
struct kernel_rlimit {
  unsigned long      rlim_cur;
  unsigned long      rlim_max;
};

/* include/linux/time.h                                                      */
struct kernel_timespec {
  long               tv_sec;
  long               tv_nsec;
};

/* include/linux/time.h                                                      */
struct kernel_timeval {
  long               tv_sec;
  long               tv_usec;
};

/* include/linux/resource.h                                                  */
struct kernel_rusage {
  struct kernel_timeval ru_utime;
  struct kernel_timeval ru_stime;
  long               ru_maxrss;
  long               ru_ixrss;
  long               ru_idrss;
  long               ru_isrss;
  long               ru_minflt;
  long               ru_majflt;
  long               ru_nswap;
  long               ru_inblock;
  long               ru_oublock;
  long               ru_msgsnd;
  long               ru_msgrcv;
  long               ru_nsignals;
  long               ru_nvcsw;
  long               ru_nivcsw;
};

#if defined(__i386__) || defined(__ARM_EABI__) || defined(__ARM_ARCH_3__) \
  || defined(__PPC__)

/* include/asm-{arm,i386,mips,ppc}/signal.h                                  */
struct kernel_old_sigaction {
  union {
    void             (*sa_handler_)(int);
    void             (*sa_sigaction_)(int, siginfo_t *, void *);
  };
  unsigned long      sa_mask;
  unsigned long      sa_flags;
  void               (*sa_restorer)(void);
} __attribute__((packed,aligned(4)));
#elif (defined(__mips__) && _MIPS_SIM == _MIPS_SIM_ABI32)
  #define kernel_old_sigaction kernel_sigaction
#endif

/* Some kernel functions (e.g. sigaction() in 2.6.23) require that the
 * exactly match the size of the signal set, even though the API was
 * intended to be extensible. We define our own KERNEL_NSIG to deal with
 * this.
 * Please note that glibc provides signals [1.._NSIG-1], whereas the
 * kernel (and this header) provides the range [1..KERNEL_NSIG]. The
 * actual number of signals is obviously the same, but the constants
 * differ by one.
 */
#ifdef __mips__
#define KERNEL_NSIG 128
#else
#define KERNEL_NSIG  64
#endif

/* include/asm-{arm,i386,mips,x86_64}/signal.h                               */
struct kernel_sigset_t {
  unsigned long sig[(KERNEL_NSIG + 8*sizeof(unsigned long) - 1)/
                    (8*sizeof(unsigned long))];
};

/* include/asm-{arm,i386,mips,x86_64,ppc}/signal.h                           */
struct kernel_sigaction {
#ifdef __mips__
  unsigned long      sa_flags;
  union {
    void             (*sa_handler_)(int);
    void             (*sa_sigaction_)(int, siginfo_t *, void *);
  };
  struct kernel_sigset_t sa_mask;
#else
  union {
    void             (*sa_handler_)(int);
    void             (*sa_sigaction_)(int, siginfo_t *, void *);
  };
  unsigned long      sa_flags;
  void               (*sa_restorer)(void);
  struct kernel_sigset_t sa_mask;
#endif
};

/* include/linux/socket.h                                                    */
struct kernel_sockaddr {
  unsigned short     sa_family;
  char               sa_data[14];
};

/* include/asm-{arm,i386,mips,ppc}/stat.h                                    */
#ifdef __mips__
#if _MIPS_SIM == _MIPS_SIM_ABI64
struct kernel_stat {
#else
struct kernel_stat64 {
#endif
  unsigned           st_dev;
  unsigned           __pad0[3];
  unsigned long long st_ino;
  unsigned           st_mode;
  unsigned           st_nlink;
  unsigned           st_uid;
  unsigned           st_gid;
  unsigned           st_rdev;
  unsigned           __pad1[3];
  long long          st_size;
  unsigned           st_atime_;
  unsigned           st_atime_nsec_;
  unsigned           st_mtime_;
  unsigned           st_mtime_nsec_;
  unsigned           st_ctime_;
  unsigned           st_ctime_nsec_;
  unsigned           st_blksize;
  unsigned           __pad2;
  unsigned long long st_blocks;
};
#elif defined __PPC__
struct kernel_stat64 {
  unsigned long long st_dev;
  unsigned long long st_ino;
  unsigned           st_mode;
  unsigned           st_nlink;
  unsigned           st_uid;
  unsigned           st_gid;
  unsigned long long st_rdev;
  unsigned short int __pad2;
  long long          st_size;
  long               st_blksize;
  long long          st_blocks;
  long               st_atime_;
  unsigned long      st_atime_nsec_;
  long               st_mtime_;
  unsigned long      st_mtime_nsec_;
  long               st_ctime_;
  unsigned long      st_ctime_nsec_;
  unsigned long      __unused4;
  unsigned long      __unused5;
};
#else
struct kernel_stat64 {
  unsigned long long st_dev;
  unsigned char      __pad0[4];
  unsigned           __st_ino;
  unsigned           st_mode;
  unsigned           st_nlink;
  unsigned           st_uid;
  unsigned           st_gid;
  unsigned long long st_rdev;
  unsigned char      __pad3[4];
  long long          st_size;
  unsigned           st_blksize;
  unsigned long long st_blocks;
  unsigned           st_atime_;
  unsigned           st_atime_nsec_;
  unsigned           st_mtime_;
  unsigned           st_mtime_nsec_;
  unsigned           st_ctime_;
  unsigned           st_ctime_nsec_;
  unsigned long long st_ino;
};
#endif

/* include/asm-{arm,i386,mips,x86_64,ppc}/stat.h                             */
#if defined(__i386__) || defined(__ARM_ARCH_3__) || defined(__ARM_EABI__)
struct kernel_stat {
  /* The kernel headers suggest that st_dev and st_rdev should be 32bit
   * quantities encoding 12bit major and 20bit minor numbers in an interleaved
   * format. In reality, we do not see useful data in the top bits. So,
   * we'll leave the padding in here, until we find a better solution.
   */
  unsigned short     st_dev;
  short              pad1;
  unsigned           st_ino;
  unsigned short     st_mode;
  unsigned short     st_nlink;
  unsigned short     st_uid;
  unsigned short     st_gid;
  unsigned short     st_rdev;
  short              pad2;
  unsigned           st_size;
  unsigned           st_blksize;
  unsigned           st_blocks;
  unsigned           st_atime_;
  unsigned           st_atime_nsec_;
  unsigned           st_mtime_;
  unsigned           st_mtime_nsec_;
  unsigned           st_ctime_;
  unsigned           st_ctime_nsec_;
  unsigned           __unused4;
  unsigned           __unused5;
};
#elif defined(__x86_64__)
struct kernel_stat {
  uint64_t           st_dev;
  uint64_t           st_ino;
  uint64_t           st_nlink;
  unsigned           st_mode;
  unsigned           st_uid;
  unsigned           st_gid;
  unsigned           __pad0;
  uint64_t           st_rdev;
  int64_t            st_size;
  int64_t            st_blksize;
  int64_t            st_blocks;
  uint64_t           st_atime_;
  uint64_t           st_atime_nsec_;
  uint64_t           st_mtime_;
  uint64_t           st_mtime_nsec_;
  uint64_t           st_ctime_;
  uint64_t           st_ctime_nsec_;
  int64_t            __unused[3];
};
#elif defined(__PPC__)
struct kernel_stat {
  unsigned           st_dev;
  unsigned long      st_ino;      // ino_t
  unsigned long      st_mode;     // mode_t
  unsigned short     st_nlink;    // nlink_t
  unsigned           st_uid;      // uid_t
  unsigned           st_gid;      // gid_t
  unsigned           st_rdev;
  long               st_size;     // off_t
  unsigned long      st_blksize;
  unsigned long      st_blocks;
  unsigned long      st_atime_;
  unsigned long      st_atime_nsec_;
  unsigned long      st_mtime_;
  unsigned long      st_mtime_nsec_;
  unsigned long      st_ctime_;
  unsigned long      st_ctime_nsec_;
  unsigned long      __unused4;
  unsigned long      __unused5;
};
#elif (defined(__mips__) && _MIPS_SIM != _MIPS_SIM_ABI64)
struct kernel_stat {
  unsigned           st_dev;
  int                st_pad1[3];
  unsigned           st_ino;
  unsigned           st_mode;
  unsigned           st_nlink;
  unsigned           st_uid;
  unsigned           st_gid;
  unsigned           st_rdev;
  int                st_pad2[2];
  long               st_size;
  int                st_pad3;
  long               st_atime_;
  long               st_atime_nsec_;
  long               st_mtime_;
  long               st_mtime_nsec_;
  long               st_ctime_;
  long               st_ctime_nsec_;
  int                st_blksize;
  int                st_blocks;
  int                st_pad4[14];
};
#endif

/* include/asm-{arm,i386,mips,x86_64,ppc}/statfs.h                           */
#ifdef __mips__
#if _MIPS_SIM != _MIPS_SIM_ABI64
struct kernel_statfs64 {
  unsigned long      f_type;
  unsigned long      f_bsize;
  unsigned long      f_frsize;
  unsigned long      __pad;
  unsigned long long f_blocks;
  unsigned long long f_bfree;
  unsigned long long f_files;
  unsigned long long f_ffree;
  unsigned long long f_bavail;
  struct { int val[2]; } f_fsid;
  unsigned long      f_namelen;
  unsigned long      f_spare[6];
};
#endif
#elif !defined(__x86_64__)
struct kernel_statfs64 {
  unsigned long      f_type;
  unsigned long      f_bsize;
  unsigned long long f_blocks;
  unsigned long long f_bfree;
  unsigned long long f_bavail;
  unsigned long long f_files;
  unsigned long long f_ffree;
  struct { int val[2]; } f_fsid;
  unsigned long      f_namelen;
  unsigned long      f_frsize;
  unsigned long      f_spare[5];
};
#endif

/* include/asm-{arm,i386,mips,x86_64,ppc,generic}/statfs.h                   */
#ifdef __mips__
struct kernel_statfs {
  long               f_type;
  long               f_bsize;
  long               f_frsize;
  long               f_blocks;
  long               f_bfree;
  long               f_files;
  long               f_ffree;
  long               f_bavail;
  struct { int val[2]; } f_fsid;
  long               f_namelen;
  long               f_spare[6];
};
#elif defined(__x86_64__)
struct kernel_statfs {
  /* x86_64 actually defines all these fields as signed, whereas all other  */
  /* platforms define them as unsigned. Leaving them at unsigned should not */
  /* cause any problems. Make sure these are 64-bit even on x32.            */
  uint64_t           f_type;
  uint64_t           f_bsize;
  uint64_t           f_blocks;
  uint64_t           f_bfree;
  uint64_t           f_bavail;
  uint64_t           f_files;
  uint64_t           f_ffree;
  struct { int val[2]; } f_fsid;
  uint64_t           f_namelen;
  uint64_t           f_frsize;
  uint64_t           f_spare[5];
};
#else
struct kernel_statfs {
  unsigned long      f_type;
  unsigned long      f_bsize;
  unsigned long      f_blocks;
  unsigned long      f_bfree;
  unsigned long      f_bavail;
  unsigned long      f_files;
  unsigned long      f_ffree;
  struct { int val[2]; } f_fsid;
  unsigned long      f_namelen;
  unsigned long      f_frsize;
  unsigned long      f_spare[5];
};
#endif


/* Definitions missing from the standard header files                        */
#ifndef O_DIRECTORY
#if defined(__ARM_ARCH_3__) || defined(__ARM_EABI__)
#define O_DIRECTORY             0040000
#else
#define O_DIRECTORY             0200000
#endif
#endif
#ifndef NT_PRXFPREG
#define NT_PRXFPREG             0x46e62b7f
#endif
#ifndef PTRACE_GETFPXREGS
#define PTRACE_GETFPXREGS       ((enum __ptrace_request)18)
#endif
#ifndef PR_GET_DUMPABLE
#define PR_GET_DUMPABLE         3
#endif
#ifndef PR_SET_DUMPABLE
#define PR_SET_DUMPABLE         4
#endif
#ifndef PR_GET_SECCOMP
#define PR_GET_SECCOMP          21
#endif
#ifndef PR_SET_SECCOMP
#define PR_SET_SECCOMP          22
#endif
#ifndef AT_FDCWD
#define AT_FDCWD                (-100)
#endif
#ifndef AT_SYMLINK_NOFOLLOW
#define AT_SYMLINK_NOFOLLOW     0x100
#endif
#ifndef AT_REMOVEDIR
#define AT_REMOVEDIR            0x200
#endif
#ifndef MREMAP_FIXED
#define MREMAP_FIXED            2
#endif
#ifndef SA_RESTORER
#define SA_RESTORER             0x04000000
#endif
#ifndef CPUCLOCK_PROF
#define CPUCLOCK_PROF           0
#endif
#ifndef CPUCLOCK_VIRT
#define CPUCLOCK_VIRT           1
#endif
#ifndef CPUCLOCK_SCHED
#define CPUCLOCK_SCHED          2
#endif
#ifndef CPUCLOCK_PERTHREAD_MASK
#define CPUCLOCK_PERTHREAD_MASK 4
#endif
#ifndef MAKE_PROCESS_CPUCLOCK
#define MAKE_PROCESS_CPUCLOCK(pid, clock)                                     \
        ((~(int)(pid) << 3) | (int)(clock))
#endif
#ifndef MAKE_THREAD_CPUCLOCK
#define MAKE_THREAD_CPUCLOCK(tid, clock)                                      \
        ((~(int)(tid) << 3) | (int)((clock) | CPUCLOCK_PERTHREAD_MASK))
#endif

#ifndef FUTEX_WAIT
#define FUTEX_WAIT                0
#endif
#ifndef FUTEX_WAKE
#define FUTEX_WAKE                1
#endif
#ifndef FUTEX_FD
#define FUTEX_FD                  2
#endif
#ifndef FUTEX_REQUEUE
#define FUTEX_REQUEUE             3
#endif
#ifndef FUTEX_CMP_REQUEUE
#define FUTEX_CMP_REQUEUE         4
#endif
#ifndef FUTEX_WAKE_OP
#define FUTEX_WAKE_OP             5
#endif
#ifndef FUTEX_LOCK_PI
#define FUTEX_LOCK_PI             6
#endif
#ifndef FUTEX_UNLOCK_PI
#define FUTEX_UNLOCK_PI           7
#endif
#ifndef FUTEX_TRYLOCK_PI
#define FUTEX_TRYLOCK_PI          8
#endif
#ifndef FUTEX_PRIVATE_FLAG
#define FUTEX_PRIVATE_FLAG        128
#endif
#ifndef FUTEX_CMD_MASK
#define FUTEX_CMD_MASK            ~FUTEX_PRIVATE_FLAG
#endif
#ifndef FUTEX_WAIT_PRIVATE
#define FUTEX_WAIT_PRIVATE        (FUTEX_WAIT | FUTEX_PRIVATE_FLAG)
#endif
#ifndef FUTEX_WAKE_PRIVATE
#define FUTEX_WAKE_PRIVATE        (FUTEX_WAKE | FUTEX_PRIVATE_FLAG)
#endif
#ifndef FUTEX_REQUEUE_PRIVATE
#define FUTEX_REQUEUE_PRIVATE     (FUTEX_REQUEUE | FUTEX_PRIVATE_FLAG)
#endif
#ifndef FUTEX_CMP_REQUEUE_PRIVATE
#define FUTEX_CMP_REQUEUE_PRIVATE (FUTEX_CMP_REQUEUE | FUTEX_PRIVATE_FLAG)
#endif
#ifndef FUTEX_WAKE_OP_PRIVATE
#define FUTEX_WAKE_OP_PRIVATE     (FUTEX_WAKE_OP | FUTEX_PRIVATE_FLAG)
#endif
#ifndef FUTEX_LOCK_PI_PRIVATE
#define FUTEX_LOCK_PI_PRIVATE     (FUTEX_LOCK_PI | FUTEX_PRIVATE_FLAG)
#endif
#ifndef FUTEX_UNLOCK_PI_PRIVATE
#define FUTEX_UNLOCK_PI_PRIVATE   (FUTEX_UNLOCK_PI | FUTEX_PRIVATE_FLAG)
#endif
#ifndef FUTEX_TRYLOCK_PI_PRIVATE
#define FUTEX_TRYLOCK_PI_PRIVATE  (FUTEX_TRYLOCK_PI | FUTEX_PRIVATE_FLAG)
#endif


#if defined(__x86_64__)
#ifndef ARCH_SET_GS
#define ARCH_SET_GS             0x1001
#endif
#ifndef ARCH_GET_GS
#define ARCH_GET_GS             0x1004
#endif
#endif

#if defined(__i386__)
#ifndef __NR_quotactl
#define __NR_quotactl           131
#endif
#ifndef __NR_setresuid
#define __NR_setresuid          164
#define __NR_getresuid          165
#define __NR_setresgid          170
#define __NR_getresgid          171
#endif
#ifndef __NR_rt_sigaction
#define __NR_rt_sigreturn       173
#define __NR_rt_sigaction       174
#define __NR_rt_sigprocmask     175
#define __NR_rt_sigpending      176
#define __NR_rt_sigsuspend      179
#endif
#ifndef __NR_pread64
#define __NR_pread64            180
#endif
#ifndef __NR_pwrite64
#define __NR_pwrite64           181
#endif
#ifndef __NR_ugetrlimit
#define __NR_ugetrlimit         191
#endif
#ifndef __NR_stat64
#define __NR_stat64             195
#endif
#ifndef __NR_fstat64
#define __NR_fstat64            197
#endif
#ifndef __NR_setresuid32
#define __NR_setresuid32        208
#define __NR_getresuid32        209
#define __NR_setresgid32        210
#define __NR_getresgid32        211
#endif
#ifndef __NR_setfsuid32
#define __NR_setfsuid32         215
#define __NR_setfsgid32         216
#endif
#ifndef __NR_getdents64
#define __NR_getdents64         220
#endif
#ifndef __NR_gettid
#define __NR_gettid             224
#endif
#ifndef __NR_readahead
#define __NR_readahead          225
#endif
#ifndef __NR_setxattr
#define __NR_setxattr           226
#endif
#ifndef __NR_lsetxattr
#define __NR_lsetxattr          227
#endif
#ifndef __NR_getxattr
#define __NR_getxattr           229
#endif
#ifndef __NR_lgetxattr
#define __NR_lgetxattr          230
#endif
#ifndef __NR_listxattr
#define __NR_listxattr          232
#endif
#ifndef __NR_llistxattr
#define __NR_llistxattr         233
#endif
#ifndef __NR_tkill
#define __NR_tkill              238
#endif
#ifndef __NR_futex
#define __NR_futex              240
#endif
#ifndef __NR_sched_setaffinity
#define __NR_sched_setaffinity  241
#define __NR_sched_getaffinity  242
#endif
#ifndef __NR_set_tid_address
#define __NR_set_tid_address    258
#endif
#ifndef __NR_clock_gettime
#define __NR_clock_gettime      265
#endif
#ifndef __NR_clock_getres
#define __NR_clock_getres       266
#endif
#ifndef __NR_statfs64
#define __NR_statfs64           268
#endif
#ifndef __NR_fstatfs64
#define __NR_fstatfs64          269
#endif
#ifndef __NR_fadvise64_64
#define __NR_fadvise64_64       272
#endif
#ifndef __NR_ioprio_set
#define __NR_ioprio_set         289
#endif
#ifndef __NR_ioprio_get
#define __NR_ioprio_get         290
#endif
#ifndef __NR_openat
#define __NR_openat             295
#endif
#ifndef __NR_fstatat64
#define __NR_fstatat64          300
#endif
#ifndef __NR_unlinkat
#define __NR_unlinkat           301
#endif
#ifndef __NR_move_pages
#define __NR_move_pages         317
#endif
#ifndef __NR_getcpu
#define __NR_getcpu             318
#endif
#ifndef __NR_fallocate
#define __NR_fallocate          324
#endif
/* End of i386 definitions                                                   */
#elif defined(__ARM_ARCH_3__) || defined(__ARM_EABI__)
#ifndef __NR_setresuid
#define __NR_setresuid          (__NR_SYSCALL_BASE + 164)
#define __NR_getresuid          (__NR_SYSCALL_BASE + 165)
#define __NR_setresgid          (__NR_SYSCALL_BASE + 170)
#define __NR_getresgid          (__NR_SYSCALL_BASE + 171)
#endif
#ifndef __NR_rt_sigaction
#define __NR_rt_sigreturn       (__NR_SYSCALL_BASE + 173)
#define __NR_rt_sigaction       (__NR_SYSCALL_BASE + 174)
#define __NR_rt_sigprocmask     (__NR_SYSCALL_BASE + 175)
#define __NR_rt_sigpending      (__NR_SYSCALL_BASE + 176)
#define __NR_rt_sigsuspend      (__NR_SYSCALL_BASE + 179)
#endif
#ifndef __NR_pread64
#define __NR_pread64            (__NR_SYSCALL_BASE + 180)
#endif
#ifndef __NR_pwrite64
#define __NR_pwrite64           (__NR_SYSCALL_BASE + 181)
#endif
#ifndef __NR_ugetrlimit
#define __NR_ugetrlimit         (__NR_SYSCALL_BASE + 191)
#endif
#ifndef __NR_stat64
#define __NR_stat64             (__NR_SYSCALL_BASE + 195)
#endif
#ifndef __NR_fstat64
#define __NR_fstat64            (__NR_SYSCALL_BASE + 197)
#endif
#ifndef __NR_setresuid32
#define __NR_setresuid32        (__NR_SYSCALL_BASE + 208)
#define __NR_getresuid32        (__NR_SYSCALL_BASE + 209)
#define __NR_setresgid32        (__NR_SYSCALL_BASE + 210)
#define __NR_getresgid32        (__NR_SYSCALL_BASE + 211)
#endif
#ifndef __NR_setfsuid32
#define __NR_setfsuid32         (__NR_SYSCALL_BASE + 215)
#define __NR_setfsgid32         (__NR_SYSCALL_BASE + 216)
#endif
#ifndef __NR_getdents64
#define __NR_getdents64         (__NR_SYSCALL_BASE + 217)
#endif
#ifndef __NR_gettid
#define __NR_gettid             (__NR_SYSCALL_BASE + 224)
#endif
#ifndef __NR_readahead
#define __NR_readahead          (__NR_SYSCALL_BASE + 225)
#endif
#ifndef __NR_setxattr
#define __NR_setxattr           (__NR_SYSCALL_BASE + 226)
#endif
#ifndef __NR_lsetxattr
#define __NR_lsetxattr          (__NR_SYSCALL_BASE + 227)
#endif
#ifndef __NR_getxattr
#define __NR_getxattr           (__NR_SYSCALL_BASE + 229)
#endif
#ifndef __NR_lgetxattr
#define __NR_lgetxattr          (__NR_SYSCALL_BASE + 230)
#endif
#ifndef __NR_listxattr
#define __NR_listxattr          (__NR_SYSCALL_BASE + 232)
#endif
#ifndef __NR_llistxattr
#define __NR_llistxattr         (__NR_SYSCALL_BASE + 233)
#endif
#ifndef __NR_tkill
#define __NR_tkill              (__NR_SYSCALL_BASE + 238)
#endif
#ifndef __NR_futex
#define __NR_futex              (__NR_SYSCALL_BASE + 240)
#endif
#ifndef __NR_sched_setaffinity
#define __NR_sched_setaffinity  (__NR_SYSCALL_BASE + 241)
#define __NR_sched_getaffinity  (__NR_SYSCALL_BASE + 242)
#endif
#ifndef __NR_set_tid_address
#define __NR_set_tid_address    (__NR_SYSCALL_BASE + 256)
#endif
#ifndef __NR_clock_gettime
#define __NR_clock_gettime      (__NR_SYSCALL_BASE + 263)
#endif
#ifndef __NR_clock_getres
#define __NR_clock_getres       (__NR_SYSCALL_BASE + 264)
#endif
#ifndef __NR_statfs64
#define __NR_statfs64           (__NR_SYSCALL_BASE + 266)
#endif
#ifndef __NR_fstatfs64
#define __NR_fstatfs64          (__NR_SYSCALL_BASE + 267)
#endif
#ifndef __NR_ioprio_set
#define __NR_ioprio_set         (__NR_SYSCALL_BASE + 314)
#endif
#ifndef __NR_ioprio_get
#define __NR_ioprio_get         (__NR_SYSCALL_BASE + 315)
#endif
#ifndef __NR_move_pages
#define __NR_move_pages         (__NR_SYSCALL_BASE + 344)
#endif
#ifndef __NR_getcpu
#define __NR_getcpu             (__NR_SYSCALL_BASE + 345)
#endif
/* End of ARM 3/EABI definitions                                                */
#elif defined(__x86_64__)
#ifndef __NR_pread64
#define __NR_pread64             17
#endif
#ifndef __NR_pwrite64
#define __NR_pwrite64            18
#endif
#ifndef __NR_setresuid
#define __NR_setresuid          117
#define __NR_getresuid          118
#define __NR_setresgid          119
#define __NR_getresgid          120
#endif
#ifndef __NR_quotactl
#define __NR_quotactl           179
#endif
#ifndef __NR_gettid
#define __NR_gettid             186
#endif
#ifndef __NR_readahead
#define __NR_readahead          187
#endif
#ifndef __NR_setxattr
#define __NR_setxattr           188
#endif
#ifndef __NR_lsetxattr
#define __NR_lsetxattr          189
#endif
#ifndef __NR_getxattr
#define __NR_getxattr           191
#endif
#ifndef __NR_lgetxattr
#define __NR_lgetxattr          192
#endif
#ifndef __NR_listxattr
#define __NR_listxattr          194
#endif
#ifndef __NR_llistxattr
#define __NR_llistxattr         195
#endif
#ifndef __NR_tkill
#define __NR_tkill              200
#endif
#ifndef __NR_futex
#define __NR_futex              202
#endif
#ifndef __NR_sched_setaffinity
#define __NR_sched_setaffinity  203
#define __NR_sched_getaffinity  204
#endif
#ifndef __NR_getdents64
#define __NR_getdents64         217
#endif
#ifndef __NR_set_tid_address
#define __NR_set_tid_address    218
#endif
#ifndef __NR_fadvise64
#define __NR_fadvise64          221
#endif
#ifndef __NR_clock_gettime
#define __NR_clock_gettime      228
#endif
#ifndef __NR_clock_getres
#define __NR_clock_getres       229
#endif
#ifndef __NR_ioprio_set
#define __NR_ioprio_set         251
#endif
#ifndef __NR_ioprio_get
#define __NR_ioprio_get         252
#endif
#ifndef __NR_openat
#define __NR_openat             257
#endif
#ifndef __NR_newfstatat
#define __NR_newfstatat         262
#endif
#ifndef __NR_unlinkat
#define __NR_unlinkat           263
#endif
#ifndef __NR_move_pages
#define __NR_move_pages         279
#endif
#ifndef __NR_fallocate
#define __NR_fallocate          285
#endif
/* End of x86-64 definitions                                                 */
#elif defined(__mips__)
#if _MIPS_SIM == _MIPS_SIM_ABI32
#ifndef __NR_setresuid
#define __NR_setresuid          (__NR_Linux + 185)
#define __NR_getresuid          (__NR_Linux + 186)
#define __NR_setresgid          (__NR_Linux + 190)
#define __NR_getresgid          (__NR_Linux + 191)
#endif
#ifndef __NR_rt_sigaction
#define __NR_rt_sigreturn       (__NR_Linux + 193)
#define __NR_rt_sigaction       (__NR_Linux + 194)
#define __NR_rt_sigprocmask     (__NR_Linux + 195)
#define __NR_rt_sigpending      (__NR_Linux + 196)
#define __NR_rt_sigsuspend      (__NR_Linux + 199)
#endif
#ifndef __NR_pread64
#define __NR_pread64            (__NR_Linux + 200)
#endif
#ifndef __NR_pwrite64
#define __NR_pwrite64           (__NR_Linux + 201)
#endif
#ifndef __NR_stat64
#define __NR_stat64             (__NR_Linux + 213)
#endif
#ifndef __NR_fstat64
#define __NR_fstat64            (__NR_Linux + 215)
#endif
#ifndef __NR_getdents64
#define __NR_getdents64         (__NR_Linux + 219)
#endif
#ifndef __NR_gettid
#define __NR_gettid             (__NR_Linux + 222)
#endif
#ifndef __NR_readahead
#define __NR_readahead          (__NR_Linux + 223)
#endif
#ifndef __NR_setxattr
#define __NR_setxattr           (__NR_Linux + 224)
#endif
#ifndef __NR_lsetxattr
#define __NR_lsetxattr          (__NR_Linux + 225)
#endif
#ifndef __NR_getxattr
#define __NR_getxattr           (__NR_Linux + 227)
#endif
#ifndef __NR_lgetxattr
#define __NR_lgetxattr          (__NR_Linux + 228)
#endif
#ifndef __NR_listxattr
#define __NR_listxattr          (__NR_Linux + 230)
#endif
#ifndef __NR_llistxattr
#define __NR_llistxattr         (__NR_Linux + 231)
#endif
#ifndef __NR_tkill
#define __NR_tkill              (__NR_Linux + 236)
#endif
#ifndef __NR_futex
#define __NR_futex              (__NR_Linux + 238)
#endif
#ifndef __NR_sched_setaffinity
#define __NR_sched_setaffinity  (__NR_Linux + 239)
#define __NR_sched_getaffinity  (__NR_Linux + 240)
#endif
#ifndef __NR_set_tid_address
#define __NR_set_tid_address    (__NR_Linux + 252)
#endif
#ifndef __NR_statfs64
#define __NR_statfs64           (__NR_Linux + 255)
#endif
#ifndef __NR_fstatfs64
#define __NR_fstatfs64          (__NR_Linux + 256)
#endif
#ifndef __NR_clock_gettime
#define __NR_clock_gettime      (__NR_Linux + 263)
#endif
#ifndef __NR_clock_getres
#define __NR_clock_getres       (__NR_Linux + 264)
#endif
#ifndef __NR_openat
#define __NR_openat             (__NR_Linux + 288)
#endif
#ifndef __NR_fstatat
#define __NR_fstatat            (__NR_Linux + 293)
#endif
#ifndef __NR_unlinkat
#define __NR_unlinkat           (__NR_Linux + 294)
#endif
#ifndef __NR_move_pages
#define __NR_move_pages         (__NR_Linux + 308)
#endif
#ifndef __NR_getcpu
#define __NR_getcpu             (__NR_Linux + 312)
#endif
#ifndef __NR_ioprio_set
#define __NR_ioprio_set         (__NR_Linux + 314)
#endif
#ifndef __NR_ioprio_get
#define __NR_ioprio_get         (__NR_Linux + 315)
#endif
/* End of MIPS (old 32bit API) definitions */
#elif  _MIPS_SIM == _MIPS_SIM_ABI64
#ifndef __NR_pread64
#define __NR_pread64            (__NR_Linux +  16)
#endif
#ifndef __NR_pwrite64
#define __NR_pwrite64           (__NR_Linux +  17)
#endif
#ifndef __NR_setresuid
#define __NR_setresuid          (__NR_Linux + 115)
#define __NR_getresuid          (__NR_Linux + 116)
#define __NR_setresgid          (__NR_Linux + 117)
#define __NR_getresgid          (__NR_Linux + 118)
#endif
#ifndef __NR_gettid
#define __NR_gettid             (__NR_Linux + 178)
#endif
#ifndef __NR_readahead
#define __NR_readahead          (__NR_Linux + 179)
#endif
#ifndef __NR_setxattr
#define __NR_setxattr           (__NR_Linux + 180)
#endif
#ifndef __NR_lsetxattr
#define __NR_lsetxattr          (__NR_Linux + 181)
#endif
#ifndef __NR_getxattr
#define __NR_getxattr           (__NR_Linux + 183)
#endif
#ifndef __NR_lgetxattr
#define __NR_lgetxattr          (__NR_Linux + 184)
#endif
#ifndef __NR_listxattr
#define __NR_listxattr          (__NR_Linux + 186)
#endif
#ifndef __NR_llistxattr
#define __NR_llistxattr         (__NR_Linux + 187)
#endif
#ifndef __NR_tkill
#define __NR_tkill              (__NR_Linux + 192)
#endif
#ifndef __NR_futex
#define __NR_futex              (__NR_Linux + 194)
#endif
#ifndef __NR_sched_setaffinity
#define __NR_sched_setaffinity  (__NR_Linux + 195)
#define __NR_sched_getaffinity  (__NR_Linux + 196)
#endif
#ifndef __NR_set_tid_address
#define __NR_set_tid_address    (__NR_Linux + 212)
#endif
#ifndef __NR_clock_gettime
#define __NR_clock_gettime      (__NR_Linux + 222)
#endif
#ifndef __NR_clock_getres
#define __NR_clock_getres       (__NR_Linux + 223)
#endif
#ifndef __NR_openat
#define __NR_openat             (__NR_Linux + 247)
#endif
#ifndef __NR_fstatat
#define __NR_fstatat            (__NR_Linux + 252)
#endif
#ifndef __NR_unlinkat
#define __NR_unlinkat           (__NR_Linux + 253)
#endif
#ifndef __NR_move_pages
#define __NR_move_pages         (__NR_Linux + 267)
#endif
#ifndef __NR_getcpu
#define __NR_getcpu             (__NR_Linux + 271)
#endif
#ifndef __NR_ioprio_set
#define __NR_ioprio_set         (__NR_Linux + 273)
#endif
#ifndef __NR_ioprio_get
#define __NR_ioprio_get         (__NR_Linux + 274)
#endif
/* End of MIPS (64bit API) definitions */
#else
#ifndef __NR_setresuid
#define __NR_setresuid          (__NR_Linux + 115)
#define __NR_getresuid          (__NR_Linux + 116)
#define __NR_setresgid          (__NR_Linux + 117)
#define __NR_getresgid          (__NR_Linux + 118)
#endif
#ifndef __NR_gettid
#define __NR_gettid             (__NR_Linux + 178)
#endif
#ifndef __NR_readahead
#define __NR_readahead          (__NR_Linux + 179)
#endif
#ifndef __NR_setxattr
#define __NR_setxattr           (__NR_Linux + 180)
#endif
#ifndef __NR_lsetxattr
#define __NR_lsetxattr          (__NR_Linux + 181)
#endif
#ifndef __NR_getxattr
#define __NR_getxattr           (__NR_Linux + 183)
#endif
#ifndef __NR_lgetxattr
#define __NR_lgetxattr          (__NR_Linux + 184)
#endif
#ifndef __NR_listxattr
#define __NR_listxattr          (__NR_Linux + 186)
#endif
#ifndef __NR_llistxattr
#define __NR_llistxattr         (__NR_Linux + 187)
#endif
#ifndef __NR_tkill
#define __NR_tkill              (__NR_Linux + 192)
#endif
#ifndef __NR_futex
#define __NR_futex              (__NR_Linux + 194)
#endif
#ifndef __NR_sched_setaffinity
#define __NR_sched_setaffinity  (__NR_Linux + 195)
#define __NR_sched_getaffinity  (__NR_Linux + 196)
#endif
#ifndef __NR_set_tid_address
#define __NR_set_tid_address    (__NR_Linux + 213)
#endif
#ifndef __NR_statfs64
#define __NR_statfs64           (__NR_Linux + 217)
#endif
#ifndef __NR_fstatfs64
#define __NR_fstatfs64          (__NR_Linux + 218)
#endif
#ifndef __NR_clock_gettime
#define __NR_clock_gettime      (__NR_Linux + 226)
#endif
#ifndef __NR_clock_getres
#define __NR_clock_getres       (__NR_Linux + 227)
#endif
#ifndef __NR_openat
#define __NR_openat             (__NR_Linux + 251)
#endif
#ifndef __NR_fstatat
#define __NR_fstatat            (__NR_Linux + 256)
#endif
#ifndef __NR_unlinkat
#define __NR_unlinkat           (__NR_Linux + 257)
#endif
#ifndef __NR_move_pages
#define __NR_move_pages         (__NR_Linux + 271)
#endif
#ifndef __NR_getcpu
#define __NR_getcpu             (__NR_Linux + 275)
#endif
#ifndef __NR_ioprio_set
#define __NR_ioprio_set         (__NR_Linux + 277)
#endif
#ifndef __NR_ioprio_get
#define __NR_ioprio_get         (__NR_Linux + 278)
#endif
/* End of MIPS (new 32bit API) definitions                                   */
#endif
/* End of MIPS definitions                                                   */
#elif defined(__PPC__)
#ifndef __NR_setfsuid
#define __NR_setfsuid           138
#define __NR_setfsgid           139
#endif
#ifndef __NR_setresuid
#define __NR_setresuid          164
#define __NR_getresuid          165
#define __NR_setresgid          169
#define __NR_getresgid          170
#endif
#ifndef __NR_rt_sigaction
#define __NR_rt_sigreturn       172
#define __NR_rt_sigaction       173
#define __NR_rt_sigprocmask     174
#define __NR_rt_sigpending      175
#define __NR_rt_sigsuspend      178
#endif
#ifndef __NR_pread64
#define __NR_pread64            179
#endif
#ifndef __NR_pwrite64
#define __NR_pwrite64           180
#endif
#ifndef __NR_ugetrlimit
#define __NR_ugetrlimit         190
#endif
#ifndef __NR_readahead
#define __NR_readahead          191
#endif
#ifndef __NR_stat64
#define __NR_stat64             195
#endif
#ifndef __NR_fstat64
#define __NR_fstat64            197
#endif
#ifndef __NR_getdents64
#define __NR_getdents64         202
#endif
#ifndef __NR_gettid
#define __NR_gettid             207
#endif
#ifndef __NR_tkill
#define __NR_tkill              208
#endif
#ifndef __NR_setxattr
#define __NR_setxattr           209
#endif
#ifndef __NR_lsetxattr
#define __NR_lsetxattr          210
#endif
#ifndef __NR_getxattr
#define __NR_getxattr           212
#endif
#ifndef __NR_lgetxattr
#define __NR_lgetxattr          213
#endif
#ifndef __NR_listxattr
#define __NR_listxattr          215
#endif
#ifndef __NR_llistxattr
#define __NR_llistxattr         216
#endif
#ifndef __NR_futex
#define __NR_futex              221
#endif
#ifndef __NR_sched_setaffinity
#define __NR_sched_setaffinity  222
#define __NR_sched_getaffinity  223
#endif
#ifndef __NR_set_tid_address
#define __NR_set_tid_address    232
#endif
#ifndef __NR_clock_gettime
#define __NR_clock_gettime      246
#endif
#ifndef __NR_clock_getres
#define __NR_clock_getres       247
#endif
#ifndef __NR_statfs64
#define __NR_statfs64           252
#endif
#ifndef __NR_fstatfs64
#define __NR_fstatfs64          253
#endif
#ifndef __NR_fadvise64_64
#define __NR_fadvise64_64       254
#endif
#ifndef __NR_ioprio_set
#define __NR_ioprio_set         273
#endif
#ifndef __NR_ioprio_get
#define __NR_ioprio_get         274
#endif
#ifndef __NR_openat
#define __NR_openat             286
#endif
#ifndef __NR_fstatat64
#define __NR_fstatat64          291
#endif
#ifndef __NR_unlinkat
#define __NR_unlinkat           292
#endif
#ifndef __NR_move_pages
#define __NR_move_pages         301
#endif
#ifndef __NR_getcpu
#define __NR_getcpu             302
#endif
/* End of powerpc defininitions                                              */
#endif


/* After forking, we must make sure to only call system calls.               */
#if defined(__BOUNDED_POINTERS__)
  #error "Need to port invocations of syscalls for bounded ptrs"
#else
  /* The core dumper and the thread lister get executed after threads
   * have been suspended. As a consequence, we cannot call any functions
   * that acquire locks. Unfortunately, libc wraps most system calls
   * (e.g. in order to implement pthread_atfork, and to make calls
   * cancellable), which means we cannot call these functions. Instead,
   * we have to call syscall() directly.
   */
  #undef LSS_ERRNO
  #ifdef SYS_ERRNO
    /* Allow the including file to override the location of errno. This can
     * be useful when using clone() with the CLONE_VM option.
     */
    #define LSS_ERRNO SYS_ERRNO
  #else
    #define LSS_ERRNO errno
  #endif

  #undef LSS_INLINE
  #ifdef SYS_INLINE
    #define LSS_INLINE SYS_INLINE
  #else
    #define LSS_INLINE static inline
  #endif

  /* Allow the including file to override the prefix used for all new
   * system calls. By default, it will be set to "sys_".
   */
  #undef LSS_NAME
  #ifndef SYS_PREFIX
    #define LSS_NAME(name) sys_##name
  #elif defined(SYS_PREFIX) && SYS_PREFIX < 0
    #define LSS_NAME(name) name
  #elif defined(SYS_PREFIX) && SYS_PREFIX == 0
    #define LSS_NAME(name) sys0_##name
  #elif defined(SYS_PREFIX) && SYS_PREFIX == 1
    #define LSS_NAME(name) sys1_##name
  #elif defined(SYS_PREFIX) && SYS_PREFIX == 2
    #define LSS_NAME(name) sys2_##name
  #elif defined(SYS_PREFIX) && SYS_PREFIX == 3
    #define LSS_NAME(name) sys3_##name
  #elif defined(SYS_PREFIX) && SYS_PREFIX == 4
    #define LSS_NAME(name) sys4_##name
  #elif defined(SYS_PREFIX) && SYS_PREFIX == 5
    #define LSS_NAME(name) sys5_##name
  #elif defined(SYS_PREFIX) && SYS_PREFIX == 6
    #define LSS_NAME(name) sys6_##name
  #elif defined(SYS_PREFIX) && SYS_PREFIX == 7
    #define LSS_NAME(name) sys7_##name
  #elif defined(SYS_PREFIX) && SYS_PREFIX == 8
    #define LSS_NAME(name) sys8_##name
  #elif defined(SYS_PREFIX) && SYS_PREFIX == 9
    #define LSS_NAME(name) sys9_##name
  #endif

  #undef  LSS_RETURN
  #if (defined(__i386__) || defined(__x86_64__) || defined(__ARM_ARCH_3__) \
       || defined(__ARM_EABI__))
  /* Failing system calls return a negative result in the range of
   * -1..-4095. These are "errno" values with the sign inverted.
   */
  #define LSS_RETURN(type, res)                                               \
    do {                                                                      \
      if ((unsigned long)(res) >= (unsigned long)(-4095)) {                   \
        LSS_ERRNO = -(res);                                                   \
        res = -1;                                                             \
      }                                                                       \
      return (type) (res);                                                    \
    } while (0)
  #elif defined(__mips__)
  /* On MIPS, failing system calls return -1, and set errno in a
   * separate CPU register.
   */
  #define LSS_RETURN(type, res, err)                                          \
    do {                                                                      \
      if (err) {                                                              \
        unsigned long __errnovalue = (res);                                   \
        LSS_ERRNO = __errnovalue;                                             \
        res = -1;                                                             \
      }                                                                       \
      return (type) (res);                                                    \
    } while (0)
  #elif defined(__PPC__)
  /* On PPC, failing system calls return -1, and set errno in a
   * separate CPU register. See linux/unistd.h.
   */
  #define LSS_RETURN(type, res, err)                                          \
   do {                                                                       \
     if (err & 0x10000000 ) {                                                 \
       LSS_ERRNO = (res);                                                     \
       res = -1;                                                              \
     }                                                                        \
     return (type) (res);                                                     \
   } while (0)
  #endif
  #if defined(__i386__)
    /* In PIC mode (e.g. when building shared libraries), gcc for i386
     * reserves ebx. Unfortunately, most distribution ship with implementations
     * of _syscallX() which clobber ebx.
     * Also, most definitions of _syscallX() neglect to mark "memory" as being
     * clobbered. This causes problems with compilers, that do a better job
     * at optimizing across __asm__ calls.
     * So, we just have to redefine all of the _syscallX() macros.
     */
    #undef LSS_ENTRYPOINT
    #ifdef SYS_SYSCALL_ENTRYPOINT
    static inline void (**LSS_NAME(get_syscall_entrypoint)(void))(void) {
      void (**entrypoint)(void);
      asm volatile(".bss\n"
                   ".align 8\n"
                   ".globl "SYS_SYSCALL_ENTRYPOINT"\n"
                   ".common "SYS_SYSCALL_ENTRYPOINT",8,8\n"
                   ".previous\n"
                   /* This logically does 'lea "SYS_SYSCALL_ENTRYPOINT", %0' */
                   "call 0f\n"
                 "0:pop  %0\n"
                   "add  $_GLOBAL_OFFSET_TABLE_+[.-0b], %0\n"
                   "mov  "SYS_SYSCALL_ENTRYPOINT"@GOT(%0), %0\n"
                   : "=r"(entrypoint));
      return entrypoint;
    }

    #define LSS_ENTRYPOINT ".bss\n"                                           \
                           ".align 8\n"                                       \
                           ".globl "SYS_SYSCALL_ENTRYPOINT"\n"                \
                           ".common "SYS_SYSCALL_ENTRYPOINT",8,8\n"           \
                           ".previous\n"                                      \
                           /* Check the SYS_SYSCALL_ENTRYPOINT vector      */ \
                           "push %%eax\n"                                     \
                           "call 10000f\n"                                    \
                     "10000:pop  %%eax\n"                                     \
                           "add  $_GLOBAL_OFFSET_TABLE_+[.-10000b], %%eax\n"  \
                           "mov  "SYS_SYSCALL_ENTRYPOINT"@GOT(%%eax), %%eax\n"\
                           "mov  0(%%eax), %%eax\n"                           \
                           "test %%eax, %%eax\n"                              \
                           "jz   10002f\n"                                    \
                           "push %%eax\n"                                     \
                           "call 10001f\n"                                    \
                     "10001:pop  %%eax\n"                                     \
                           "add  $(10003f-10001b), %%eax\n"                   \
                           "xchg 4(%%esp), %%eax\n"                           \
                           "ret\n"                                            \
                     "10002:pop  %%eax\n"                                     \
                           "int $0x80\n"                                      \
                     "10003:\n"
    #else
    #define LSS_ENTRYPOINT "int $0x80\n"
    #endif
    #undef  LSS_BODY
    #define LSS_BODY(type,args...)                                            \
      long __res;                                                             \
      __asm__ __volatile__("push %%ebx\n"                                     \
                           "movl %2,%%ebx\n"                                  \
                           LSS_ENTRYPOINT                                     \
                           "pop %%ebx"                                        \
                           args                                               \
                           : "esp", "memory");                                \
      LSS_RETURN(type,__res)
    #undef  _syscall0
    #define _syscall0(type,name)                                              \
      type LSS_NAME(name)(void) {                                             \
        long __res;                                                           \
        __asm__ volatile(LSS_ENTRYPOINT                                       \
                         : "=a" (__res)                                       \
                         : "0" (__NR_##name)                                  \
                         : "esp", "memory");                                  \
        LSS_RETURN(type,__res);                                               \
      }
    #undef  _syscall1
    #define _syscall1(type,name,type1,arg1)                                   \
      type LSS_NAME(name)(type1 arg1) {                                       \
        LSS_BODY(type,                                                        \
             : "=a" (__res)                                                   \
             : "0" (__NR_##name), "ri" ((long)(arg1)));                       \
      }
    #undef  _syscall2
    #define _syscall2(type,name,type1,arg1,type2,arg2)                        \
      type LSS_NAME(name)(type1 arg1,type2 arg2) {                            \
        LSS_BODY(type,                                                        \
             : "=a" (__res)                                                   \
             : "0" (__NR_##name),"ri" ((long)(arg1)), "c" ((long)(arg2)));    \
      }
    #undef  _syscall3
    #define _syscall3(type,name,type1,arg1,type2,arg2,type3,arg3)             \
      type LSS_NAME(name)(type1 arg1,type2 arg2,type3 arg3) {                 \
        LSS_BODY(type,                                                        \
             : "=a" (__res)                                                   \
             : "0" (__NR_##name), "ri" ((long)(arg1)), "c" ((long)(arg2)),    \
               "d" ((long)(arg3)));                                           \
      }
    #undef  _syscall4
    #define _syscall4(type,name,type1,arg1,type2,arg2,type3,arg3,type4,arg4)  \
      type LSS_NAME(name)(type1 arg1, type2 arg2, type3 arg3, type4 arg4) {   \
        LSS_BODY(type,                                                        \
             : "=a" (__res)                                                   \
             : "0" (__NR_##name), "ri" ((long)(arg1)), "c" ((long)(arg2)),    \
               "d" ((long)(arg3)),"S" ((long)(arg4)));                        \
      }
    #undef  _syscall5
    #define _syscall5(type,name,type1,arg1,type2,arg2,type3,arg3,type4,arg4,  \
                      type5,arg5)                                             \
      type LSS_NAME(name)(type1 arg1, type2 arg2, type3 arg3, type4 arg4,     \
                          type5 arg5) {                                       \
        long __res;                                                           \
        __asm__ __volatile__("push %%ebx\n"                                   \
                             "movl %2,%%ebx\n"                                \
                             "movl %1,%%eax\n"                                \
                             LSS_ENTRYPOINT                                   \
                             "pop  %%ebx"                                     \
                             : "=a" (__res)                                   \
                             : "i" (__NR_##name), "ri" ((long)(arg1)),        \
                               "c" ((long)(arg2)), "d" ((long)(arg3)),        \
                               "S" ((long)(arg4)), "D" ((long)(arg5))         \
                             : "esp", "memory");                              \
        LSS_RETURN(type,__res);                                               \
      }
    #undef  _syscall6
    #define _syscall6(type,name,type1,arg1,type2,arg2,type3,arg3,type4,arg4,  \
                      type5,arg5,type6,arg6)                                  \
      type LSS_NAME(name)(type1 arg1, type2 arg2, type3 arg3, type4 arg4,     \
                          type5 arg5, type6 arg6) {                           \
        long __res;                                                           \
        struct { long __a1; long __a6; } __s = { (long)arg1, (long) arg6 };   \
        __asm__ __volatile__("push %%ebp\n"                                   \
                             "push %%ebx\n"                                   \
                             "movl 4(%2),%%ebp\n"                             \
                             "movl 0(%2), %%ebx\n"                            \
                             "movl %1,%%eax\n"                                \
                             LSS_ENTRYPOINT                                   \
                             "pop  %%ebx\n"                                   \
                             "pop  %%ebp"                                     \
                             : "=a" (__res)                                   \
                             : "i" (__NR_##name),  "0" ((long)(&__s)),        \
                               "c" ((long)(arg2)), "d" ((long)(arg3)),        \
                               "S" ((long)(arg4)), "D" ((long)(arg5))         \
                             : "esp", "memory");                              \
        LSS_RETURN(type,__res);                                               \
      }
    LSS_INLINE int LSS_NAME(clone)(int (*fn)(void *), void *child_stack,
                                   int flags, void *arg, int *parent_tidptr,
                                   void *newtls, int *child_tidptr) {
      long __res;
      __asm__ __volatile__(/* if (fn == NULL)
                            *   return -EINVAL;
                            */
                           "movl   %3,%%ecx\n"
                           "jecxz  1f\n"

                           /* if (child_stack == NULL)
                            *   return -EINVAL;
                            */
                           "movl   %4,%%ecx\n"
                           "jecxz  1f\n"

                           /* Set up alignment of the child stack:
                            * child_stack = (child_stack & ~0xF) - 20;
                            */
                           "andl   $-16,%%ecx\n"
                           "subl   $20,%%ecx\n"

                           /* Push "arg" and "fn" onto the stack that will be
                            * used by the child.
                            */
                           "movl   %6,%%eax\n"
                           "movl   %%eax,4(%%ecx)\n"
                           "movl   %3,%%eax\n"
                           "movl   %%eax,(%%ecx)\n"

                           /* %eax = syscall(%eax = __NR_clone,
                            *                %ebx = flags,
                            *                %ecx = child_stack,
                            *                %edx = parent_tidptr,
                            *                %esi = newtls,
                            *                %edi = child_tidptr)
                            * Also, make sure that %ebx gets preserved as it is
                            * used in PIC mode.
                            */
                           "movl   %8,%%esi\n"
                           "movl   %7,%%edx\n"
                           "movl   %5,%%eax\n"
                           "movl   %9,%%edi\n"
                           "pushl  %%ebx\n"
                           "movl   %%eax,%%ebx\n"
                           "movl   %2,%%eax\n"
                           LSS_ENTRYPOINT

                           /* In the parent: restore %ebx
                            * In the child:  move "fn" into %ebx
                            */
                           "popl   %%ebx\n"

                           /* if (%eax != 0)
                            *   return %eax;
                            */
                           "test   %%eax,%%eax\n"
                           "jnz    1f\n"

                           /* In the child, now. Terminate frame pointer chain.
                            */
                           "movl   $0,%%ebp\n"

                           /* Call "fn". "arg" is already on the stack.
                            */
                           "call   *%%ebx\n"

                           /* Call _exit(%ebx). Unfortunately older versions
                            * of gcc restrict the number of arguments that can
                            * be passed to asm(). So, we need to hard-code the
                            * system call number.
                            */
                           "movl   %%eax,%%ebx\n"
                           "movl   $1,%%eax\n"
                           LSS_ENTRYPOINT

                           /* Return to parent.
                            */
                         "1:\n"
                           : "=a" (__res)
                           : "0"(-EINVAL), "i"(__NR_clone),
                             "m"(fn), "m"(child_stack), "m"(flags), "m"(arg),
                             "m"(parent_tidptr), "m"(newtls), "m"(child_tidptr)
                           : "esp", "memory", "ecx", "edx", "esi", "edi");
      LSS_RETURN(int, __res);
    }

    #define __NR__fadvise64_64 __NR_fadvise64_64
    LSS_INLINE _syscall6(int, _fadvise64_64, int, fd,
                         unsigned, offset_lo, unsigned, offset_hi,
                         unsigned, len_lo, unsigned, len_hi,
                         int, advice)

    LSS_INLINE int LSS_NAME(fadvise64)(int fd, loff_t offset,
                                       loff_t len, int advice) {
      return LSS_NAME(_fadvise64_64)(fd,
                                     (unsigned)offset, (unsigned)(offset >>32),
                                     (unsigned)len, (unsigned)(len >> 32),
                                     advice);
    }

    #define __NR__fallocate __NR_fallocate
    LSS_INLINE _syscall6(int, _fallocate, int, fd,
                         int, mode,
                         unsigned, offset_lo, unsigned, offset_hi,
                         unsigned, len_lo, unsigned, len_hi)

    LSS_INLINE int LSS_NAME(fallocate)(int fd, int mode,
                                       loff_t offset, loff_t len) {
      union { loff_t off; unsigned w[2]; } o = { offset }, l = { len };
      return LSS_NAME(_fallocate)(fd, mode, o.w[0], o.w[1], l.w[0], l.w[1]);
    }

    LSS_INLINE _syscall1(int, set_thread_area, void *, u)
    LSS_INLINE _syscall1(int, get_thread_area, void *, u)

    LSS_INLINE void (*LSS_NAME(restore_rt)(void))(void) {
      /* On i386, the kernel does not know how to return from a signal
       * handler. Instead, it relies on user space to provide a
       * restorer function that calls the {rt_,}sigreturn() system call.
       * Unfortunately, we cannot just reference the glibc version of this
       * function, as glibc goes out of its way to make it inaccessible.
       */
      void (*res)(void);
      __asm__ __volatile__("call   2f\n"
                         "0:.align 16\n"
                         "1:movl   %1,%%eax\n"
                           LSS_ENTRYPOINT
                         "2:popl   %0\n"
                           "addl   $(1b-0b),%0\n"
                           : "=a" (res)
                           : "i"  (__NR_rt_sigreturn));
      return res;
    }
    LSS_INLINE void (*LSS_NAME(restore)(void))(void) {
      /* On i386, the kernel does not know how to return from a signal
       * handler. Instead, it relies on user space to provide a
       * restorer function that calls the {rt_,}sigreturn() system call.
       * Unfortunately, we cannot just reference the glibc version of this
       * function, as glibc goes out of its way to make it inaccessible.
       */
      void (*res)(void);
      __asm__ __volatile__("call   2f\n"
                         "0:.align 16\n"
                         "1:pop    %%eax\n"
                           "movl   %1,%%eax\n"
                           LSS_ENTRYPOINT
                         "2:popl   %0\n"
                           "addl   $(1b-0b),%0\n"
                           : "=a" (res)
                           : "i"  (__NR_sigreturn));
      return res;
    }
  #elif defined(__x86_64__)
    /* There are no known problems with any of the _syscallX() macros
     * currently shipping for x86_64, but we still need to be able to define
     * our own version so that we can override the location of the errno
     * location (e.g. when using the clone() system call with the CLONE_VM
     * option).
     */
    #undef LSS_ENTRYPOINT
    #ifdef SYS_SYSCALL_ENTRYPOINT
    static inline void (**LSS_NAME(get_syscall_entrypoint)(void))(void) {
      void (**entrypoint)(void);
      asm volatile(".bss\n"
                   ".align 8\n"
                   ".globl "SYS_SYSCALL_ENTRYPOINT"\n"
                   ".common "SYS_SYSCALL_ENTRYPOINT",8,8\n"
                   ".previous\n"
                   "mov "SYS_SYSCALL_ENTRYPOINT"@GOTPCREL(%%rip), %0\n"
                   : "=r"(entrypoint));
      return entrypoint;
    }

    #define LSS_ENTRYPOINT                                                    \
              ".bss\n"                                                        \
              ".align 8\n"                                                    \
              ".globl "SYS_SYSCALL_ENTRYPOINT"\n"                             \
              ".common "SYS_SYSCALL_ENTRYPOINT",8,8\n"                        \
              ".previous\n"                                                   \
              "mov "SYS_SYSCALL_ENTRYPOINT"@GOTPCREL(%%rip), %%rcx\n"         \
              "mov  0(%%rcx), %%rcx\n"                                        \
              "test %%rcx, %%rcx\n"                                           \
              "jz   10001f\n"                                                 \
              "call *%%rcx\n"                                                 \
              "jmp  10002f\n"                                                 \
        "10001:syscall\n"                                                     \
        "10002:\n"

    #else
    #define LSS_ENTRYPOINT "syscall\n"
    #endif

    /* The x32 ABI has 32 bit longs, but the syscall interface is 64 bit.
     * We need to explicitly cast to an unsigned 64 bit type to avoid implicit
     * sign extension.  We can't cast pointers directly because those are
     * 32 bits, and gcc will dump ugly warnings about casting from a pointer
     * to an integer of a different size.
     */
    #undef  LSS_SYSCALL_ARG
    #define LSS_SYSCALL_ARG(a) ((uint64_t)(uintptr_t)(a))
    #undef  _LSS_RETURN
    #define _LSS_RETURN(type, res, cast)                                      \
      do {                                                                    \
        if ((uint64_t)(res) >= (uint64_t)(-4095)) {                           \
          LSS_ERRNO = -(res);                                                 \
          res = -1;                                                           \
        }                                                                     \
        return (type)(cast)(res);                                             \
      } while (0)
    #undef  LSS_RETURN
    #define LSS_RETURN(type, res) _LSS_RETURN(type, res, uintptr_t)

    #undef  _LSS_BODY
    #define _LSS_BODY(nr, type, name, cast, ...)                              \
          long long __res;                                                    \
          __asm__ __volatile__(LSS_BODY_ASM##nr LSS_ENTRYPOINT                \
            : "=a" (__res)                                                    \
            : "0" (__NR_##name) LSS_BODY_ARG##nr(__VA_ARGS__)                 \
            : LSS_BODY_CLOBBER##nr "r11", "rcx", "memory");                   \
          _LSS_RETURN(type, __res, cast)
    #undef  LSS_BODY
    #define LSS_BODY(nr, type, name, args...) \
      _LSS_BODY(nr, type, name, uintptr_t, ## args)

    #undef  LSS_BODY_ASM0
    #undef  LSS_BODY_ASM1
    #undef  LSS_BODY_ASM2
    #undef  LSS_BODY_ASM3
    #undef  LSS_BODY_ASM4
    #undef  LSS_BODY_ASM5
    #undef  LSS_BODY_ASM6
    #define LSS_BODY_ASM0
    #define LSS_BODY_ASM1 LSS_BODY_ASM0
    #define LSS_BODY_ASM2 LSS_BODY_ASM1
    #define LSS_BODY_ASM3 LSS_BODY_ASM2
    #define LSS_BODY_ASM4 LSS_BODY_ASM3 "movq %5,%%r10;"
    #define LSS_BODY_ASM5 LSS_BODY_ASM4 "movq %6,%%r8;"
    #define LSS_BODY_ASM6 LSS_BODY_ASM5 "movq %7,%%r9;"

    #undef  LSS_BODY_CLOBBER0
    #undef  LSS_BODY_CLOBBER1
    #undef  LSS_BODY_CLOBBER2
    #undef  LSS_BODY_CLOBBER3
    #undef  LSS_BODY_CLOBBER4
    #undef  LSS_BODY_CLOBBER5
    #undef  LSS_BODY_CLOBBER6
    #define LSS_BODY_CLOBBER0
    #define LSS_BODY_CLOBBER1 LSS_BODY_CLOBBER0
    #define LSS_BODY_CLOBBER2 LSS_BODY_CLOBBER1
    #define LSS_BODY_CLOBBER3 LSS_BODY_CLOBBER2
    #define LSS_BODY_CLOBBER4 LSS_BODY_CLOBBER3 "r10",
    #define LSS_BODY_CLOBBER5 LSS_BODY_CLOBBER4 "r8",
    #define LSS_BODY_CLOBBER6 LSS_BODY_CLOBBER5 "r9",

    #undef  LSS_BODY_ARG0
    #undef  LSS_BODY_ARG1
    #undef  LSS_BODY_ARG2
    #undef  LSS_BODY_ARG3
    #undef  LSS_BODY_ARG4
    #undef  LSS_BODY_ARG5
    #undef  LSS_BODY_ARG6
    #define LSS_BODY_ARG0()
    #define LSS_BODY_ARG1(arg1) \
      LSS_BODY_ARG0(), "D" (arg1)
    #define LSS_BODY_ARG2(arg1, arg2) \
      LSS_BODY_ARG1(arg1), "S" (arg2)
    #define LSS_BODY_ARG3(arg1, arg2, arg3) \
      LSS_BODY_ARG2(arg1, arg2), "d" (arg3)
    #define LSS_BODY_ARG4(arg1, arg2, arg3, arg4) \
      LSS_BODY_ARG3(arg1, arg2, arg3), "r" (arg4)
    #define LSS_BODY_ARG5(arg1, arg2, arg3, arg4, arg5) \
      LSS_BODY_ARG4(arg1, arg2, arg3, arg4), "r" (arg5)
    #define LSS_BODY_ARG6(arg1, arg2, arg3, arg4, arg5, arg6) \
      LSS_BODY_ARG5(arg1, arg2, arg3, arg4, arg5), "r" (arg6)

    #undef _syscall0
    #define _syscall0(type,name)                                              \
      type LSS_NAME(name)(void) {                                             \
        LSS_BODY(0, type, name);                                              \
      }
    #undef _syscall1
    #define _syscall1(type,name,type1,arg1)                                   \
      type LSS_NAME(name)(type1 arg1) {                                       \
        LSS_BODY(1, type, name, LSS_SYSCALL_ARG(arg1));                       \
      }
    #undef _syscall2
    #define _syscall2(type,name,type1,arg1,type2,arg2)                        \
      type LSS_NAME(name)(type1 arg1, type2 arg2) {                           \
        LSS_BODY(2, type, name, LSS_SYSCALL_ARG(arg1), LSS_SYSCALL_ARG(arg2));\
      }
    #undef _syscall3
    #define _syscall3(type,name,type1,arg1,type2,arg2,type3,arg3)             \
      type LSS_NAME(name)(type1 arg1, type2 arg2, type3 arg3) {               \
        LSS_BODY(3, type, name, LSS_SYSCALL_ARG(arg1), LSS_SYSCALL_ARG(arg2), \
                                LSS_SYSCALL_ARG(arg3));                       \
      }
    #undef _syscall4
    #define _syscall4(type,name,type1,arg1,type2,arg2,type3,arg3,type4,arg4)  \
      type LSS_NAME(name)(type1 arg1, type2 arg2, type3 arg3, type4 arg4) {   \
        LSS_BODY(4, type, name, LSS_SYSCALL_ARG(arg1), LSS_SYSCALL_ARG(arg2), \
                                LSS_SYSCALL_ARG(arg3), LSS_SYSCALL_ARG(arg4));\
      }
    #undef _syscall5
    #define _syscall5(type,name,type1,arg1,type2,arg2,type3,arg3,type4,arg4,  \
                      type5,arg5)                                             \
      type LSS_NAME(name)(type1 arg1, type2 arg2, type3 arg3, type4 arg4,     \
                          type5 arg5) {                                       \
        LSS_BODY(5, type, name, LSS_SYSCALL_ARG(arg1), LSS_SYSCALL_ARG(arg2), \
                                LSS_SYSCALL_ARG(arg3), LSS_SYSCALL_ARG(arg4), \
                                LSS_SYSCALL_ARG(arg5));                       \
      }
    #undef _syscall6
    #define _syscall6(type,name,type1,arg1,type2,arg2,type3,arg3,type4,arg4,  \
                      type5,arg5,type6,arg6)                                  \
      type LSS_NAME(name)(type1 arg1, type2 arg2, type3 arg3, type4 arg4,     \
                          type5 arg5, type6 arg6) {                           \
        LSS_BODY(6, type, name, LSS_SYSCALL_ARG(arg1), LSS_SYSCALL_ARG(arg2), \
                                LSS_SYSCALL_ARG(arg3), LSS_SYSCALL_ARG(arg4), \
                                LSS_SYSCALL_ARG(arg5), LSS_SYSCALL_ARG(arg6));\
      }
    LSS_INLINE int LSS_NAME(clone)(int (*fn)(void *), void *child_stack,
                                   int flags, void *arg, int *parent_tidptr,
                                   void *newtls, int *child_tidptr) {
      long long __res;
      {
        __asm__ __volatile__(/* if (fn == NULL)
                              *   return -EINVAL;
                              */
                             "testq  %4,%4\n"
                             "jz     1f\n"

                             /* if (child_stack == NULL)
                              *   return -EINVAL;
                              */
                             "testq  %5,%5\n"
                             "jz     1f\n"

                             /* childstack -= 2*sizeof(void *);
                              */
                             "subq   $16,%5\n"

                             /* Push "arg" and "fn" onto the stack that will be
                              * used by the child.
                              */
                             "movq   %7,8(%5)\n"
                             "movq   %4,0(%5)\n"

                             /* %rax = syscall(%rax = __NR_clone,
                              *                %rdi = flags,
                              *                %rsi = child_stack,
                              *                %rdx = parent_tidptr,
                              *                %r8  = new_tls,
                              *                %r10 = child_tidptr)
                              */
                             "movq   %2,%%rax\n"
                             "movq   %9,%%r8\n"
                             "movq   %10,%%r10\n"
                             LSS_ENTRYPOINT

                             /* if (%rax != 0)
                              *   return;
                              */
                             "testq  %%rax,%%rax\n"
                             "jnz    1f\n"

                             /* In the child. Terminate frame pointer chain.
                              */
                             "xorq   %%rbp,%%rbp\n"

                             /* Call "fn(arg)".
                              */
                             "popq   %%rax\n"
                             "popq   %%rdi\n"
                             "call   *%%rax\n"

                             /* Call _exit(%ebx).
                              */
                             "movq   %%rax,%%rdi\n"
                             "movq   %3,%%rax\n"
                             LSS_ENTRYPOINT

                             /* Return to parent.
                              */
                           "1:\n"
                             : "=a" (__res)
                             : "0"(-EINVAL), "i"(__NR_clone), "i"(__NR_exit),
                               "r"(LSS_SYSCALL_ARG(fn)),
                               "S"(LSS_SYSCALL_ARG(child_stack)),
                               "D"(LSS_SYSCALL_ARG(flags)),
                               "r"(LSS_SYSCALL_ARG(arg)),
                               "d"(LSS_SYSCALL_ARG(parent_tidptr)),
                               "r"(LSS_SYSCALL_ARG(newtls)),
                               "r"(LSS_SYSCALL_ARG(child_tidptr))
                             : "rsp", "memory", "r8", "r10", "r11", "rcx");
      }
      LSS_RETURN(int, __res);
    }
    LSS_INLINE _syscall2(int, arch_prctl, int, c, void *, a)

    /* Need to make sure loff_t isn't truncated to 32-bits under x32.  */
    LSS_INLINE int LSS_NAME(fadvise64)(int fd, loff_t offset, loff_t len,
                                       int advice) {
      LSS_BODY(4, int, fadvise64, LSS_SYSCALL_ARG(fd), (uint64_t)(offset),
                                  (uint64_t)(len), LSS_SYSCALL_ARG(advice));
    }

    LSS_INLINE void (*LSS_NAME(restore_rt)(void))(void) {
      /* On x86-64, the kernel does not know how to return from
       * a signal handler. Instead, it relies on user space to provide a
       * restorer function that calls the rt_sigreturn() system call.
       * Unfortunately, we cannot just reference the glibc version of this
       * function, as glibc goes out of its way to make it inaccessible.
       */
      long long res;
      __asm__ __volatile__("jmp    2f\n"
                           ".align 16\n"
                         "1:movq   %1,%%rax\n"
                           LSS_ENTRYPOINT
                         "2:leaq   1b(%%rip),%0\n"
                           : "=r" (res)
                           : "i"  (__NR_rt_sigreturn));
      return (void (*)(void))(uintptr_t)res;
    }
  #elif defined(__ARM_ARCH_3__)
    /* Most definitions of _syscallX() neglect to mark "memory" as being
     * clobbered. This causes problems with compilers, that do a better job
     * at optimizing across __asm__ calls.
     * So, we just have to redefine all of the _syscallX() macros.
     */
    #undef LSS_REG
    #define LSS_REG(r,a) register long __r##r __asm__("r"#r) = (long)a
    #undef  LSS_BODY
    #define LSS_BODY(type,name,args...)                                       \
          register long __res_r0 __asm__("r0");                               \
          long __res;                                                         \
          __asm__ __volatile__ (__syscall(name)                               \
                                : "=r"(__res_r0) : args : "lr", "memory");    \
          __res = __res_r0;                                                   \
          LSS_RETURN(type, __res)
    #undef _syscall0
    #define _syscall0(type, name)                                             \
      type LSS_NAME(name)(void) {                                             \
        LSS_BODY(type, name);                                                 \
      }
    #undef _syscall1
    #define _syscall1(type, name, type1, arg1)                                \
      type LSS_NAME(name)(type1 arg1) {                                       \
        LSS_REG(0, arg1); LSS_BODY(type, name, "r"(__r0));                    \
      }
    #undef _syscall2
    #define _syscall2(type, name, type1, arg1, type2, arg2)                   \
      type LSS_NAME(name)(type1 arg1, type2 arg2) {                           \
        LSS_REG(0, arg1); LSS_REG(1, arg2);                                   \
        LSS_BODY(type, name, "r"(__r0), "r"(__r1));                           \
      }
    #undef _syscall3
    #define _syscall3(type, name, type1, arg1, type2, arg2, type3, arg3)      \
      type LSS_NAME(name)(type1 arg1, type2 arg2, type3 arg3) {               \
        LSS_REG(0, arg1); LSS_REG(1, arg2); LSS_REG(2, arg3);                 \
        LSS_BODY(type, name, "r"(__r0), "r"(__r1), "r"(__r2));                \
      }
    #undef _syscall4
    #define _syscall4(type,name,type1,arg1,type2,arg2,type3,arg3,type4,arg4)  \
      type LSS_NAME(name)(type1 arg1, type2 arg2, type3 arg3, type4 arg4) {   \
        LSS_REG(0, arg1); LSS_REG(1, arg2); LSS_REG(2, arg3);                 \
        LSS_REG(3, arg4);                                                     \
        LSS_BODY(type, name, "r"(__r0), "r"(__r1), "r"(__r2), "r"(__r3));     \
      }
    #undef _syscall5
    #define _syscall5(type,name,type1,arg1,type2,arg2,type3,arg3,type4,arg4,  \
                      type5,arg5)                                             \
      type LSS_NAME(name)(type1 arg1, type2 arg2, type3 arg3, type4 arg4,     \
                          type5 arg5) {                                       \
        LSS_REG(0, arg1); LSS_REG(1, arg2); LSS_REG(2, arg3);                 \
        LSS_REG(3, arg4); LSS_REG(4, arg5);                                   \
        LSS_BODY(type, name, "r"(__r0), "r"(__r1), "r"(__r2), "r"(__r3),      \
                             "r"(__r4));                                      \
      }
    #undef _syscall6
    #define _syscall6(type,name,type1,arg1,type2,arg2,type3,arg3,type4,arg4,  \
                      type5,arg5,type6,arg6)                                  \
      type LSS_NAME(name)(type1 arg1, type2 arg2, type3 arg3, type4 arg4,     \
                          type5 arg5, type6 arg6) {                           \
        LSS_REG(0, arg1); LSS_REG(1, arg2); LSS_REG(2, arg3);                 \
        LSS_REG(3, arg4); LSS_REG(4, arg5); LSS_REG(5, arg6);                 \
        LSS_BODY(type, name, "r"(__r0), "r"(__r1), "r"(__r2), "r"(__r3),      \
                             "r"(__r4), "r"(__r5));                           \
      }
    LSS_INLINE int LSS_NAME(clone)(int (*fn)(void *), void *child_stack,
                                   int flags, void *arg, int *parent_tidptr,
                                   void *newtls, int *child_tidptr) {
      long __res;
      {
        register int   __flags __asm__("r0") = flags;
        register void *__stack __asm__("r1") = child_stack;
        register void *__ptid  __asm__("r2") = parent_tidptr;
        register void *__tls   __asm__("r3") = newtls;
        register int  *__ctid  __asm__("r4") = child_tidptr;
        __asm__ __volatile__(/* if (fn == NULL || child_stack == NULL)
                              *   return -EINVAL;
                              */
                             "cmp   %2,#0\n"
                             "cmpne %3,#0\n"
                             "moveq %0,%1\n"
                             "beq   1f\n"

                             /* Push "arg" and "fn" onto the stack that will be
                              * used by the child.
                              */
                             "str   %5,[%3,#-4]!\n"
                             "str   %2,[%3,#-4]!\n"

                             /* %r0 = syscall(%r0 = flags,
                              *               %r1 = child_stack,
                              *               %r2 = parent_tidptr,
                              *               %r3 = newtls,
                              *               %r4 = child_tidptr)
                              */
                             __syscall(clone)"\n"

                             /* if (%r0 != 0)
                              *   return %r0;
                              */
                             "movs  %0,r0\n"
                             "bne   1f\n"

                             /* In the child, now. Call "fn(arg)".
                              */
                             "ldr   r0,[sp, #4]\n"
                             "mov   lr,pc\n"
                             "ldr   pc,[sp]\n"

                             /* Call _exit(%r0).
                              */
                             __syscall(exit)"\n"
                           "1:\n"
                             : "=r" (__res)
                             : "i"(-EINVAL),
                               "r"(fn), "r"(__stack), "r"(__flags), "r"(arg),
                               "r"(__ptid), "r"(__tls), "r"(__ctid)
                             : "cc", "lr", "memory");
      }
      LSS_RETURN(int, __res);
    }
  #elif defined(__ARM_EABI__)
    /* Most definitions of _syscallX() neglect to mark "memory" as being
     * clobbered. This causes problems with compilers, that do a better job
     * at optimizing across __asm__ calls.
     * So, we just have to redefine all fo the _syscallX() macros.
     */
    #undef LSS_REG
    #define LSS_REG(r,a) register long __r##r __asm__("r"#r) = (long)a
    #undef  LSS_BODY
    #define LSS_BODY(type,name,args...)                                       \
          register long __res_r0 __asm__("r0");                               \
          long __res;                                                         \
          __asm__ __volatile__ ("push {r7}\n"                                 \
                                "mov r7, %1\n"                                \
                                "swi 0x0\n"                                   \
                                "pop {r7}\n"                                  \
                                : "=r"(__res_r0)                              \
                                : "i"(__NR_##name) , ## args                  \
                                : "lr", "memory");                            \
          __res = __res_r0;                                                   \
          LSS_RETURN(type, __res)
    #undef _syscall0
    #define _syscall0(type, name)                                             \
      type LSS_NAME(name)(void) {                                             \
        LSS_BODY(type, name);                                                 \
      }
    #undef _syscall1
    #define _syscall1(type, name, type1, arg1)                                \
      type LSS_NAME(name)(type1 arg1) {                                       \
        LSS_REG(0, arg1); LSS_BODY(type, name, "r"(__r0));                    \
      }
    #undef _syscall2
    #define _syscall2(type, name, type1, arg1, type2, arg2)                   \
      type LSS_NAME(name)(type1 arg1, type2 arg2) {                           \
        LSS_REG(0, arg1); LSS_REG(1, arg2);                                   \
        LSS_BODY(type, name, "r"(__r0), "r"(__r1));                           \
      }
    #undef _syscall3
    #define _syscall3(type, name, type1, arg1, type2, arg2, type3, arg3)      \
      type LSS_NAME(name)(type1 arg1, type2 arg2, type3 arg3) {               \
        LSS_REG(0, arg1); LSS_REG(1, arg2); LSS_REG(2, arg3);                 \
        LSS_BODY(type, name, "r"(__r0), "r"(__r1), "r"(__r2));                \
      }
    #undef _syscall4
    #define _syscall4(type,name,type1,arg1,type2,arg2,type3,arg3,type4,arg4)  \
      type LSS_NAME(name)(type1 arg1, type2 arg2, type3 arg3, type4 arg4) {   \
        LSS_REG(0, arg1); LSS_REG(1, arg2); LSS_REG(2, arg3);                 \
        LSS_REG(3, arg4);                                                     \
        LSS_BODY(type, name, "r"(__r0), "r"(__r1), "r"(__r2), "r"(__r3));     \
      }
    #undef _syscall5
    #define _syscall5(type,name,type1,arg1,type2,arg2,type3,arg3,type4,arg4,  \
                      type5,arg5)                                             \
      type LSS_NAME(name)(type1 arg1, type2 arg2, type3 arg3, type4 arg4,     \
                          type5 arg5) {                                       \
        LSS_REG(0, arg1); LSS_REG(1, arg2); LSS_REG(2, arg3);                 \
        LSS_REG(3, arg4); LSS_REG(4, arg5);                                   \
        LSS_BODY(type, name, "r"(__r0), "r"(__r1), "r"(__r2), "r"(__r3),      \
                             "r"(__r4));                                      \
      }
    #undef _syscall6
    #define _syscall6(type,name,type1,arg1,type2,arg2,type3,arg3,type4,arg4,  \
                      type5,arg5,type6,arg6)                                  \
      type LSS_NAME(name)(type1 arg1, type2 arg2, type3 arg3, type4 arg4,     \
                          type5 arg5, type6 arg6) {                           \
        LSS_REG(0, arg1); LSS_REG(1, arg2); LSS_REG(2, arg3);                 \
        LSS_REG(3, arg4); LSS_REG(4, arg5); LSS_REG(5, arg6);                 \
        LSS_BODY(type, name, "r"(__r0), "r"(__r1), "r"(__r2), "r"(__r3),      \
                             "r"(__r4), "r"(__r5));                           \
      }
    LSS_INLINE int LSS_NAME(clone)(int (*fn)(void *), void *child_stack,
                                   int flags, void *arg, int *parent_tidptr,
                                   void *newtls, int *child_tidptr) {
      long __res;
      {
        register int   __flags __asm__("r0") = flags;
        register void *__stack __asm__("r1") = child_stack;
        register void *__ptid  __asm__("r2") = parent_tidptr;
        register void *__tls   __asm__("r3") = newtls;
        register int  *__ctid  __asm__("r4") = child_tidptr;
        __asm__ __volatile__(/* if (fn == NULL || child_stack == NULL)
                              *   return -EINVAL;
                              */
#ifdef __thumb2__			     
			     "push  {r7}\n"
#endif			     
                             "cmp   %2,#0\n"
                             "it    ne\n"
                             "cmpne %3,#0\n"
                             "it    eq\n"
                             "moveq %0,%1\n"
                             "beq   1f\n"

                             /* Push "arg" and "fn" onto the stack that will be
                              * used by the child.
                              */
                             "str   %5,[%3,#-4]!\n"
                             "str   %2,[%3,#-4]!\n"

                             /* %r0 = syscall(%r0 = flags,
                              *               %r1 = child_stack,
                              *               %r2 = parent_tidptr,
                              *               %r3 = newtls,
                              *               %r4 = child_tidptr)
                              */
                             "mov r7, %9\n"
                             "swi 0x0\n"

                             /* if (%r0 != 0)
                              *   return %r0;
                              */
                             "movs  %0,r0\n"
                             "bne   1f\n"

                             /* In the child, now. Call "fn(arg)".
                              */
                             "ldr   r0,[sp, #4]\n"

                             /* When compiling for Thumb-2 the "MOV LR,PC" here
                              * won't work because it loads PC+4 into LR,
                              * whereas the LDR is a 4-byte instruction.
                              * This results in the child thread always
                              * crashing with an "Illegal Instruction" when it
                              * returned into the middle of the LDR instruction
                              * The instruction sequence used instead was
                              * recommended by
                              * "https://wiki.edubuntu.org/ARM/Thumb2PortingHowto#Quick_Reference".
                              */
                           #ifdef __thumb2__
                             "ldr   r7,[sp]\n"
                             "blx   r7\n"
                           #else
                             "mov   lr,pc\n"
                             "ldr   pc,[sp]\n"
                           #endif

                             /* Call _exit(%r0).
                              */
                             "mov r7, %10\n"
                             "swi 0x0\n"
                           "1:\n"
#ifdef __thumb2__
			     "pop {r7}"
#endif			     
                             : "=r" (__res)
                             : "i"(-EINVAL),
                               "r"(fn), "r"(__stack), "r"(__flags), "r"(arg),
                               "r"(__ptid), "r"(__tls), "r"(__ctid),
                               "i"(__NR_clone), "i"(__NR_exit)
#ifdef __thumb2__
			     : "cc", "lr", "memory");
#else
                             : "cc", "r7", "lr", "memory");
#endif
      }
      LSS_RETURN(int, __res);
    }
  #elif defined(__mips__)
    #undef LSS_REG
    #define LSS_REG(r,a) register unsigned long __r##r __asm__("$"#r) =       \
                                 (unsigned long)(a)
    #undef  LSS_BODY
    #define LSS_BODY(type,name,r7,...)                                        \
          register unsigned long __v0 __asm__("$2") = __NR_##name;            \
          __asm__ __volatile__ ("syscall\n"                                   \
                                : "+r"(__v0), r7 (__r7)                       \
                                : "0"(__v0), ##__VA_ARGS__                    \
                                : "$8", "$9", "$10", "$11", "$12",            \
                                  "$13", "$14", "$15", "$24", "$25",          \
                                  "memory");                                  \
          LSS_RETURN(type, __v0, __r7)
    #undef _syscall0
    #define _syscall0(type, name)                                             \
      type LSS_NAME(name)(void) {                                             \
        register unsigned long __r7 __asm__("$7");                            \
        LSS_BODY(type, name, "=r");                                           \
      }
    #undef _syscall1
    #define _syscall1(type, name, type1, arg1)                                \
      type LSS_NAME(name)(type1 arg1) {                                       \
        register unsigned long __r7 __asm__("$7");                            \
        LSS_REG(4, arg1); LSS_BODY(type, name, "=r", "r"(__r4));              \
      }
    #undef _syscall2
    #define _syscall2(type, name, type1, arg1, type2, arg2)                   \
      type LSS_NAME(name)(type1 arg1, type2 arg2) {                           \
        register unsigned long __r7 __asm__("$7");                            \
        LSS_REG(4, arg1); LSS_REG(5, arg2);                                   \
        LSS_BODY(type, name, "=r", "r"(__r4), "r"(__r5));                     \
      }
    #undef _syscall3
    #define _syscall3(type, name, type1, arg1, type2, arg2, type3, arg3)      \
      type LSS_NAME(name)(type1 arg1, type2 arg2, type3 arg3) {               \
        register unsigned long __r7 __asm__("$7");                            \
        LSS_REG(4, arg1); LSS_REG(5, arg2); LSS_REG(6, arg3);                 \
        LSS_BODY(type, name, "=r", "r"(__r4), "r"(__r5), "r"(__r6));          \
      }
    #undef _syscall4
    #define _syscall4(type,name,type1,arg1,type2,arg2,type3,arg3,type4,arg4)  \
      type LSS_NAME(name)(type1 arg1, type2 arg2, type3 arg3, type4 arg4) {   \
        LSS_REG(4, arg1); LSS_REG(5, arg2); LSS_REG(6, arg3);                 \
        LSS_REG(7, arg4);                                                     \
        LSS_BODY(type, name, "+r", "r"(__r4), "r"(__r5), "r"(__r6));          \
      }
    #undef _syscall5
    #if _MIPS_SIM == _MIPS_SIM_ABI32
    /* The old 32bit MIPS system call API passes the fifth and sixth argument
     * on the stack, whereas the new APIs use registers "r8" and "r9".
     */
    #define _syscall5(type,name,type1,arg1,type2,arg2,type3,arg3,type4,arg4,  \
                      type5,arg5)                                             \
      type LSS_NAME(name)(type1 arg1, type2 arg2, type3 arg3, type4 arg4,     \
                          type5 arg5) {                                       \
        LSS_REG(4, arg1); LSS_REG(5, arg2); LSS_REG(6, arg3);                 \
        LSS_REG(7, arg4);                                                     \
        register unsigned long __v0 __asm__("$2") = __NR_##name;              \
        __asm__ __volatile__ (".set noreorder\n"                              \
                              "subu  $29, 32\n"                               \
                              "sw    %5, 16($29)\n"                           \
                              "syscall\n"                                     \
                              "addiu $29, 32\n"                               \
                              ".set reorder\n"                                \
                              : "+r"(__v0), "+r" (__r7)                       \
                              : "r"(__r4), "r"(__r5),                         \
                                "r"(__r6), "r" ((unsigned long)arg5)          \
                              : "$8", "$9", "$10", "$11", "$12",              \
                                "$13", "$14", "$15", "$24", "$25",            \
                                "memory");                                    \
        LSS_RETURN(type, __v0, __r7);                                         \
      }
    #else
    #define _syscall5(type,name,type1,arg1,type2,arg2,type3,arg3,type4,arg4,  \
                      type5,arg5)                                             \
      type LSS_NAME(name)(type1 arg1, type2 arg2, type3 arg3, type4 arg4,     \
                          type5 arg5) {                                       \
        LSS_REG(4, arg1); LSS_REG(5, arg2); LSS_REG(6, arg3);                 \
        LSS_REG(7, arg4); LSS_REG(8, arg5);                                   \
        LSS_BODY(type, name, "+r", "r"(__r4), "r"(__r5), "r"(__r6),           \
                 "r"(__r8));                                                  \
      }
    #endif
    #undef _syscall6
    #if _MIPS_SIM == _MIPS_SIM_ABI32
    /* The old 32bit MIPS system call API passes the fifth and sixth argument
     * on the stack, whereas the new APIs use registers "r8" and "r9".
     */
    #define _syscall6(type,name,type1,arg1,type2,arg2,type3,arg3,type4,arg4,  \
                      type5,arg5,type6,arg6)                                  \
      type LSS_NAME(name)(type1 arg1, type2 arg2, type3 arg3, type4 arg4,     \
                          type5 arg5, type6 arg6) {                           \
        LSS_REG(4, arg1); LSS_REG(5, arg2); LSS_REG(6, arg3);                 \
        LSS_REG(7, arg4);                                                     \
        register unsigned long __v0 __asm__("$2") = __NR_##name;              \
        __asm__ __volatile__ (".set noreorder\n"                              \
                              "subu  $29, 32\n"                               \
                              "sw    %5, 16($29)\n"                           \
                              "sw    %6, 20($29)\n"                           \
                              "syscall\n"                                     \
                              "addiu $29, 32\n"                               \
                              ".set reorder\n"                                \
                              : "+r"(__v0), "+r" (__r7)                       \
                              : "r"(__r4), "r"(__r5),                         \
                                "r"(__r6), "r" ((unsigned long)arg5),         \
                                "r" ((unsigned long)arg6)                     \
                              : "$8", "$9", "$10", "$11", "$12",              \
                                "$13", "$14", "$15", "$24", "$25",            \
                                "memory");                                    \
        LSS_RETURN(type, __v0, __r7);                                         \
      }
    #else
    #define _syscall6(type,name,type1,arg1,type2,arg2,type3,arg3,type4,arg4,  \
                      type5,arg5,type6,arg6)                                  \
      type LSS_NAME(name)(type1 arg1, type2 arg2, type3 arg3, type4 arg4,     \
                          type5 arg5,type6 arg6) {                            \
        LSS_REG(4, arg1); LSS_REG(5, arg2); LSS_REG(6, arg3);                 \
        LSS_REG(7, arg4); LSS_REG(8, arg5); LSS_REG(9, arg6);                 \
        LSS_BODY(type, name, "+r", "r"(__r4), "r"(__r5), "r"(__r6),           \
                 "r"(__r8), "r"(__r9));                                       \
      }
    #endif
    LSS_INLINE int LSS_NAME(clone)(int (*fn)(void *), void *child_stack,
                                   int flags, void *arg, int *parent_tidptr,
                                   void *newtls, int *child_tidptr) {
      register unsigned long __v0 __asm__("$2");
      register unsigned long __r7 __asm__("$7") = (unsigned long)newtls;
      {
        register int   __flags __asm__("$4") = flags;
        register void *__stack __asm__("$5") = child_stack;
        register void *__ptid  __asm__("$6") = parent_tidptr;
        register int  *__ctid  __asm__("$8") = child_tidptr;
        __asm__ __volatile__(
          #if _MIPS_SIM == _MIPS_SIM_ABI32 && _MIPS_SZPTR == 32
                             "subu  $29,24\n"
          #elif _MIPS_SIM == _MIPS_SIM_NABI32
                             "sub   $29,16\n"
          #else
                             "dsubu $29,16\n"
          #endif

                             /* if (fn == NULL || child_stack == NULL)
                              *   return -EINVAL;
                              */
                             "li    %0,%2\n"
                             "beqz  %5,1f\n"
                             "beqz  %6,1f\n"

                             /* Push "arg" and "fn" onto the stack that will be
                              * used by the child.
                              */
          #if _MIPS_SIM == _MIPS_SIM_ABI32 && _MIPS_SZPTR == 32
                             "subu  %6,32\n"
                             "sw    %5,0(%6)\n"
                             "sw    %8,4(%6)\n"
          #elif _MIPS_SIM == _MIPS_SIM_NABI32
                             "sub   %6,32\n"
                             "sw    %5,0(%6)\n"
                             "sw    %8,8(%6)\n"
          #else
                             "dsubu %6,32\n"
                             "sd    %5,0(%6)\n"
                             "sd    %8,8(%6)\n"
          #endif

                             /* $7 = syscall($4 = flags,
                              *              $5 = child_stack,
                              *              $6 = parent_tidptr,
                              *              $7 = newtls,
                              *              $8 = child_tidptr)
                              */
                             "li    $2,%3\n"
                             "syscall\n"

                             /* if ($7 != 0)
                              *   return $2;
                              */
                             "bnez  $7,1f\n"
                             "bnez  $2,1f\n"

                             /* In the child, now. Call "fn(arg)".
                              */
          #if _MIPS_SIM == _MIPS_SIM_ABI32 && _MIPS_SZPTR == 32
                            "lw    $25,0($29)\n"
                            "lw    $4,4($29)\n"
          #elif _MIPS_SIM == _MIPS_SIM_NABI32
                            "lw    $25,0($29)\n"
                            "lw    $4,8($29)\n"
          #else
                            "ld    $25,0($29)\n"
                            "ld    $4,8($29)\n"
          #endif
                            "jalr  $25\n"

                             /* Call _exit($2)
                              */
                            "move  $4,$2\n"
                            "li    $2,%4\n"
                            "syscall\n"

                           "1:\n"
          #if _MIPS_SIM == _MIPS_SIM_ABI32 && _MIPS_SZPTR == 32
                             "addu  $29, 24\n"
          #elif _MIPS_SIM == _MIPS_SIM_NABI32
                             "add   $29, 16\n"
          #else
                             "daddu $29,16\n"
          #endif
                             : "+r" (__v0), "+r" (__r7)
                             : "i"(-EINVAL), "i"(__NR_clone), "i"(__NR_exit),
                               "r"(fn), "r"(__stack), "r"(__flags), "r"(arg),
                               "r"(__ptid), "r"(__r7), "r"(__ctid)
                             : "$9", "$10", "$11", "$12", "$13", "$14", "$15",
                               "$24", "$25", "memory");
      }
      LSS_RETURN(int, __v0, __r7);
    }
  #elif defined (__PPC__)
    #undef  LSS_LOADARGS_0
    #define LSS_LOADARGS_0(name, dummy...)                                    \
        __sc_0 = __NR_##name
    #undef  LSS_LOADARGS_1
    #define LSS_LOADARGS_1(name, arg1)                                        \
            LSS_LOADARGS_0(name);                                             \
            __sc_3 = (unsigned long) (arg1)
    #undef  LSS_LOADARGS_2
    #define LSS_LOADARGS_2(name, arg1, arg2)                                  \
            LSS_LOADARGS_1(name, arg1);                                       \
            __sc_4 = (unsigned long) (arg2)
    #undef  LSS_LOADARGS_3
    #define LSS_LOADARGS_3(name, arg1, arg2, arg3)                            \
            LSS_LOADARGS_2(name, arg1, arg2);                                 \
            __sc_5 = (unsigned long) (arg3)
    #undef  LSS_LOADARGS_4
    #define LSS_LOADARGS_4(name, arg1, arg2, arg3, arg4)                      \
            LSS_LOADARGS_3(name, arg1, arg2, arg3);                           \
            __sc_6 = (unsigned long) (arg4)
    #undef  LSS_LOADARGS_5
    #define LSS_LOADARGS_5(name, arg1, arg2, arg3, arg4, arg5)                \
            LSS_LOADARGS_4(name, arg1, arg2, arg3, arg4);                     \
            __sc_7 = (unsigned long) (arg5)
    #undef  LSS_LOADARGS_6
    #define LSS_LOADARGS_6(name, arg1, arg2, arg3, arg4, arg5, arg6)          \
            LSS_LOADARGS_5(name, arg1, arg2, arg3, arg4, arg5);               \
            __sc_8 = (unsigned long) (arg6)
    #undef  LSS_ASMINPUT_0
    #define LSS_ASMINPUT_0 "0" (__sc_0)
    #undef  LSS_ASMINPUT_1
    #define LSS_ASMINPUT_1 LSS_ASMINPUT_0, "1" (__sc_3)
    #undef  LSS_ASMINPUT_2
    #define LSS_ASMINPUT_2 LSS_ASMINPUT_1, "2" (__sc_4)
    #undef  LSS_ASMINPUT_3
    #define LSS_ASMINPUT_3 LSS_ASMINPUT_2, "3" (__sc_5)
    #undef  LSS_ASMINPUT_4
    #define LSS_ASMINPUT_4 LSS_ASMINPUT_3, "4" (__sc_6)
    #undef  LSS_ASMINPUT_5
    #define LSS_ASMINPUT_5 LSS_ASMINPUT_4, "5" (__sc_7)
    #undef  LSS_ASMINPUT_6
    #define LSS_ASMINPUT_6 LSS_ASMINPUT_5, "6" (__sc_8)
    #undef  LSS_BODY
    #define LSS_BODY(nr, type, name, args...)                                 \
        long __sc_ret, __sc_err;                                              \
        {                                                                     \
                        register unsigned long __sc_0 __asm__ ("r0");         \
                        register unsigned long __sc_3 __asm__ ("r3");         \
                        register unsigned long __sc_4 __asm__ ("r4");         \
                        register unsigned long __sc_5 __asm__ ("r5");         \
                        register unsigned long __sc_6 __asm__ ("r6");         \
                        register unsigned long __sc_7 __asm__ ("r7");         \
                        register unsigned long __sc_8 __asm__ ("r8");         \
                                                                              \
            LSS_LOADARGS_##nr(name, args);                                    \
            __asm__ __volatile__                                              \
                ("sc\n\t"                                                     \
                 "mfcr %0"                                                    \
                 : "=&r" (__sc_0),                                            \
                   "=&r" (__sc_3), "=&r" (__sc_4),                            \
                   "=&r" (__sc_5), "=&r" (__sc_6),                            \
                   "=&r" (__sc_7), "=&r" (__sc_8)                             \
                 : LSS_ASMINPUT_##nr                                          \
                 : "cr0", "ctr", "memory",                                    \
                   "r9", "r10", "r11", "r12");                                \
            __sc_ret = __sc_3;                                                \
            __sc_err = __sc_0;                                                \
        }                                                                     \
        LSS_RETURN(type, __sc_ret, __sc_err)
    #undef _syscall0
    #define _syscall0(type, name)                                             \
       type LSS_NAME(name)(void) {                                            \
          LSS_BODY(0, type, name);                                            \
       }
    #undef _syscall1
    #define _syscall1(type, name, type1, arg1)                                \
       type LSS_NAME(name)(type1 arg1) {                                      \
          LSS_BODY(1, type, name, arg1);                                      \
       }
    #undef _syscall2
    #define _syscall2(type, name, type1, arg1, type2, arg2)                   \
       type LSS_NAME(name)(type1 arg1, type2 arg2) {                          \
          LSS_BODY(2, type, name, arg1, arg2);                                \
       }
    #undef _syscall3
    #define _syscall3(type, name, type1, arg1, type2, arg2, type3, arg3)      \
       type LSS_NAME(name)(type1 arg1, type2 arg2, type3 arg3) {              \
          LSS_BODY(3, type, name, arg1, arg2, arg3);                          \
       }
    #undef _syscall4
    #define _syscall4(type, name, type1, arg1, type2, arg2, type3, arg3,      \
                                  type4, arg4)                                \
       type LSS_NAME(name)(type1 arg1, type2 arg2, type3 arg3, type4 arg4) {  \
          LSS_BODY(4, type, name, arg1, arg2, arg3, arg4);                    \
       }
    #undef _syscall5
    #define _syscall5(type, name, type1, arg1, type2, arg2, type3, arg3,      \
                                  type4, arg4, type5, arg5)                   \
       type LSS_NAME(name)(type1 arg1, type2 arg2, type3 arg3, type4 arg4,    \
                                               type5 arg5) {                  \
          LSS_BODY(5, type, name, arg1, arg2, arg3, arg4, arg5);              \
       }
    #undef _syscall6
    #define _syscall6(type, name, type1, arg1, type2, arg2, type3, arg3,      \
                                  type4, arg4, type5, arg5, type6, arg6)      \
       type LSS_NAME(name)(type1 arg1, type2 arg2, type3 arg3, type4 arg4,    \
                                               type5 arg5, type6 arg6) {      \
          LSS_BODY(6, type, name, arg1, arg2, arg3, arg4, arg5, arg6);        \
       }
    /* clone function adapted from glibc 2.3.6 clone.S                       */
    /* TODO(csilvers): consider wrapping some args up in a struct, like we
     * do for i386's _syscall6, so we can compile successfully on gcc 2.95
     */
    LSS_INLINE int LSS_NAME(clone)(int (*fn)(void *), void *child_stack,
                                   int flags, void *arg, int *parent_tidptr,
                                   void *newtls, int *child_tidptr) {
      long __ret, __err;
      {
        register int (*__fn)(void *)    __asm__ ("r8")  = fn;
        register void *__cstack                 __asm__ ("r4")  = child_stack;
        register int __flags                    __asm__ ("r3")  = flags;
        register void * __arg                   __asm__ ("r9")  = arg;
        register int * __ptidptr                __asm__ ("r5")  = parent_tidptr;
        register void * __newtls                __asm__ ("r6")  = newtls;
        register int * __ctidptr                __asm__ ("r7")  = child_tidptr;
        __asm__ __volatile__(
            /* check for fn == NULL
             * and child_stack == NULL
             */
            "cmpwi cr0, %6, 0\n\t"
            "cmpwi cr1, %7, 0\n\t"
            "cror cr0*4+eq, cr1*4+eq, cr0*4+eq\n\t"
            "beq- cr0, 1f\n\t"

            /* set up stack frame for child                                  */
            "clrrwi %7, %7, 4\n\t"
            "li 0, 0\n\t"
            "stwu 0, -16(%7)\n\t"

            /* fn, arg, child_stack are saved across the syscall: r28-30     */
            "mr 28, %6\n\t"
            "mr 29, %7\n\t"
            "mr 27, %9\n\t"

            /* syscall                                                       */
            "li 0, %4\n\t"
            /* flags already in r3
             * child_stack already in r4
             * ptidptr already in r5
             * newtls already in r6
             * ctidptr already in r7
             */
            "sc\n\t"

            /* Test if syscall was successful                                */
            "cmpwi cr1, 3, 0\n\t"
            "crandc cr1*4+eq, cr1*4+eq, cr0*4+so\n\t"
            "bne- cr1, 1f\n\t"

            /* Do the function call                                          */
            "mtctr 28\n\t"
            "mr 3, 27\n\t"
            "bctrl\n\t"

            /* Call _exit(r3)                                                */
            "li 0, %5\n\t"
            "sc\n\t"

            /* Return to parent                                              */
            "1:\n"
            "mfcr %1\n\t"
            "mr %0, 3\n\t"
              : "=r" (__ret), "=r" (__err)
              : "0" (-1), "1" (EINVAL),
                "i" (__NR_clone), "i" (__NR_exit),
                "r" (__fn), "r" (__cstack), "r" (__flags),
                "r" (__arg), "r" (__ptidptr), "r" (__newtls),
                "r" (__ctidptr)
              : "cr0", "cr1", "memory", "ctr",
                "r0", "r29", "r27", "r28");
      }
      LSS_RETURN(int, __ret, __err);
    }
  #endif
  #define __NR__exit   __NR_exit
  #define __NR__gettid __NR_gettid
  #define __NR__mremap __NR_mremap
  LSS_INLINE _syscall1(void *,  brk,             void *,      e)
  LSS_INLINE _syscall1(int,     chdir,           const char *,p)
  LSS_INLINE _syscall1(int,     close,           int,         f)
  LSS_INLINE _syscall2(int,     clock_getres,    int,         c,
                       struct kernel_timespec*, t)
  LSS_INLINE _syscall2(int,     clock_gettime,   int,         c,
                       struct kernel_timespec*, t)
  LSS_INLINE _syscall1(int,     dup,             int,         f)
  LSS_INLINE _syscall2(int,     dup2,            int,         s,
                       int,            d)
  LSS_INLINE _syscall3(int,     execve,          const char*, f,
                       const char*const*,a,const char*const*, e)
  LSS_INLINE _syscall1(int,     _exit,           int,         e)
  LSS_INLINE _syscall1(int,     exit_group,      int,         e)
  LSS_INLINE _syscall3(int,     fcntl,           int,         f,
                       int,            c, long,   a)
  LSS_INLINE _syscall0(pid_t,   fork)
  LSS_INLINE _syscall2(int,     fstat,           int,         f,
                      struct kernel_stat*,   b)
  LSS_INLINE _syscall2(int,     fstatfs,         int,         f,
                      struct kernel_statfs*, b)
  #if defined(__x86_64__)
    /* Need to make sure off_t isn't truncated to 32-bits under x32.  */
    LSS_INLINE int LSS_NAME(ftruncate)(int f, off_t l) {
      LSS_BODY(2, int, ftruncate, LSS_SYSCALL_ARG(f), (uint64_t)(l));
    }
  #else
    LSS_INLINE _syscall2(int, ftruncate,           int,         f,
                         off_t,          l)
  #endif
  LSS_INLINE _syscall4(int,     futex,           int*,        a,
                       int,            o, int,    v,
                      struct kernel_timespec*, t)
  LSS_INLINE _syscall3(int,     getdents,        int,         f,
                      struct kernel_dirent*, d, int,    c)
  LSS_INLINE _syscall3(int,     getdents64,      int,         f,
                      struct kernel_dirent64*, d, int,    c)
  LSS_INLINE _syscall0(gid_t,   getegid)
  LSS_INLINE _syscall0(uid_t,   geteuid)
  LSS_INLINE _syscall0(pid_t,   getpgrp)
  LSS_INLINE _syscall0(pid_t,   getpid)
  LSS_INLINE _syscall0(pid_t,   getppid)
  LSS_INLINE _syscall2(int,     getpriority,     int,         a,
                       int,            b)
  LSS_INLINE _syscall3(int,     getresgid,       gid_t *,     r,
                       gid_t *,         e,       gid_t *,     s)
  LSS_INLINE _syscall3(int,     getresuid,       uid_t *,     r,
                       uid_t *,         e,       uid_t *,     s)
#if !defined(__ARM_EABI__)
  LSS_INLINE _syscall2(int,     getrlimit,       int,         r,
                      struct kernel_rlimit*, l)
#endif
  LSS_INLINE _syscall1(pid_t,   getsid,          pid_t,       p)
  LSS_INLINE _syscall0(pid_t,   _gettid)
  LSS_INLINE _syscall2(pid_t,   gettimeofday,    struct kernel_timeval*, t,
                       void*, tz)
  LSS_INLINE _syscall5(int,     setxattr,        const char *,p,
                       const char *,   n,        const void *,v,
                       size_t,         s,        int,         f)
  LSS_INLINE _syscall5(int,     lsetxattr,       const char *,p,
                       const char *,   n,        const void *,v,
                       size_t,         s,        int,         f)
  LSS_INLINE _syscall4(ssize_t, getxattr,        const char *,p,
                       const char *,   n,        void *,      v, size_t, s)
  LSS_INLINE _syscall4(ssize_t, lgetxattr,       const char *,p,
                       const char *,   n,        void *,      v, size_t, s)
  LSS_INLINE _syscall3(ssize_t, listxattr,       const char *,p,
                       char *,   l,              size_t,      s)
  LSS_INLINE _syscall3(ssize_t, llistxattr,      const char *,p,
                       char *,   l,              size_t,      s)
  LSS_INLINE _syscall3(int,     ioctl,           int,         d,
                       int,     r,               void *,      a)
  LSS_INLINE _syscall2(int,     ioprio_get,      int,         which,
                       int,     who)
  LSS_INLINE _syscall3(int,     ioprio_set,      int,         which,
                       int,     who,             int,         ioprio)
  LSS_INLINE _syscall2(int,     kill,            pid_t,       p,
                       int,            s)
  #if defined(__x86_64__)
    /* Need to make sure off_t isn't truncated to 32-bits under x32.  */
    LSS_INLINE off_t LSS_NAME(lseek)(int f, off_t o, int w) {
      _LSS_BODY(3, off_t, lseek, off_t, LSS_SYSCALL_ARG(f), (uint64_t)(o),
                                        LSS_SYSCALL_ARG(w));
    }
  #else
    LSS_INLINE _syscall3(off_t,   lseek,           int,         f,
                         off_t,          o, int,    w)
  #endif
  LSS_INLINE _syscall2(int,     munmap,          void*,       s,
                       size_t,         l)
  LSS_INLINE _syscall6(long,    move_pages,      pid_t,       p,
                       unsigned long,  n, void **,g, int *,   d,
                       int *,          s, int,    f)
  LSS_INLINE _syscall3(int,     mprotect,        const void *,a,
                       size_t,         l,        int,         p)
  LSS_INLINE _syscall5(void*,   _mremap,         void*,       o,
                       size_t,         os,       size_t,      ns,
                       unsigned long,  f, void *, a)
  LSS_INLINE _syscall3(int,     open,            const char*, p,
                       int,            f, int,    m)
  LSS_INLINE _syscall3(int,     poll,           struct kernel_pollfd*, u,
                       unsigned int,   n, int,    t)
  LSS_INLINE _syscall5(int,     prctl,           int,         option,
                       unsigned long,  arg2,
                       unsigned long,  arg3,
                       unsigned long,  arg4,
                       unsigned long,  arg5)
  LSS_INLINE _syscall4(long,    ptrace,          int,         r,
                       pid_t,          p, void *, a, void *, d)
  #if defined(__NR_quotactl)
    // Defined on x86_64 / i386 only
    LSS_INLINE _syscall4(int,  quotactl,  int,  cmd,  const char *, special,
                         int, id, caddr_t, addr)
  #endif
  LSS_INLINE _syscall3(ssize_t, read,            int,         f,
                       void *,         b, size_t, c)
  LSS_INLINE _syscall3(int,     readlink,        const char*, p,
                       char*,          b, size_t, s)
  LSS_INLINE _syscall4(int,     rt_sigaction,    int,         s,
                       const struct kernel_sigaction*, a,
                       struct kernel_sigaction*, o, size_t,   c)
  LSS_INLINE _syscall2(int, rt_sigpending, struct kernel_sigset_t *, s,
                       size_t,         c)
  LSS_INLINE _syscall4(int, rt_sigprocmask,      int,         h,
                       const struct kernel_sigset_t*,  s,
                       struct kernel_sigset_t*,        o, size_t, c)
  LSS_INLINE _syscall2(int, rt_sigsuspend,
                       const struct kernel_sigset_t*, s,  size_t, c)
  LSS_INLINE _syscall3(int,     sched_getaffinity,pid_t,      p,
                       unsigned int,   l, unsigned long *, m)
  LSS_INLINE _syscall3(int,     sched_setaffinity,pid_t,      p,
                       unsigned int,   l, unsigned long *, m)
  LSS_INLINE _syscall0(int,     sched_yield)
  LSS_INLINE _syscall1(long,    set_tid_address, int *,       t)
  LSS_INLINE _syscall1(int,     setfsgid,        gid_t,       g)
  LSS_INLINE _syscall1(int,     setfsuid,        uid_t,       u)
  LSS_INLINE _syscall1(int,     setuid,          uid_t,       u)
  LSS_INLINE _syscall1(int,     setgid,          gid_t,       g)
  LSS_INLINE _syscall2(int,     setpgid,         pid_t,       p,
                       pid_t,          g)
  LSS_INLINE _syscall3(int,     setpriority,     int,         a,
                       int,            b, int,    p)
  LSS_INLINE _syscall3(int,     setresgid,       gid_t,       r,
                       gid_t,          e, gid_t,  s)
  LSS_INLINE _syscall3(int,     setresuid,       uid_t,       r,
                       uid_t,          e, uid_t,  s)
  LSS_INLINE _syscall2(int,     setrlimit,       int,         r,
                       const struct kernel_rlimit*, l)
  LSS_INLINE _syscall0(pid_t,    setsid)
  LSS_INLINE _syscall2(int,     sigaltstack,     const stack_t*, s,
                       const stack_t*, o)
  #if defined(__NR_sigreturn)
  LSS_INLINE _syscall1(int,     sigreturn,       unsigned long, u)
  #endif
  LSS_INLINE _syscall2(int,     stat,            const char*, f,
                      struct kernel_stat*,   b)
  LSS_INLINE _syscall2(int,     statfs,          const char*, f,
                      struct kernel_statfs*, b)
  LSS_INLINE _syscall3(int,     tgkill,          pid_t,       p,
                       pid_t,          t, int,            s)
  LSS_INLINE _syscall2(int,     tkill,           pid_t,       p,
                       int,            s)
  LSS_INLINE _syscall1(int,     unlink,           const char*, f)
  LSS_INLINE _syscall3(ssize_t, write,            int,        f,
                       const void *,   b, size_t, c)
  LSS_INLINE _syscall3(ssize_t, writev,           int,        f,
                       const struct kernel_iovec*, v, size_t, c)
  #if defined(__NR_getcpu)
    LSS_INLINE _syscall3(long, getcpu, unsigned *, cpu,
                         unsigned *, node, void *, unused)
  #endif
  #if defined(__x86_64__) ||                                                  \
     (defined(__mips__) && _MIPS_SIM != _MIPS_SIM_ABI32)
    LSS_INLINE _syscall3(int, recvmsg,            int,   s,
                        struct kernel_msghdr*,     m, int, f)
    LSS_INLINE _syscall3(int, sendmsg,            int,   s,
                         const struct kernel_msghdr*, m, int, f)
    LSS_INLINE _syscall6(int, sendto,             int,   s,
                         const void*,             m, size_t, l,
                         int,                     f,
                         const struct kernel_sockaddr*, a, int, t)
    LSS_INLINE _syscall2(int, shutdown,           int,   s,
                         int,                     h)
    LSS_INLINE _syscall3(int, socket,             int,   d,
                         int,                     t, int,       p)
    LSS_INLINE _syscall4(int, socketpair,         int,   d,
                         int,                     t, int,       p, int*, s)
  #endif
  #if defined(__x86_64__)
    /* Need to make sure loff_t isn't truncated to 32-bits under x32.  */
    LSS_INLINE int LSS_NAME(fallocate)(int f, int mode, loff_t offset,
                                       loff_t len) {
      LSS_BODY(4, int, fallocate, LSS_SYSCALL_ARG(f), LSS_SYSCALL_ARG(mode),
                                  (uint64_t)(offset), (uint64_t)(len));
    }

    LSS_INLINE int LSS_NAME(getresgid32)(gid_t *rgid,
                                         gid_t *egid,
                                         gid_t *sgid) {
      return LSS_NAME(getresgid)(rgid, egid, sgid);
    }

    LSS_INLINE int LSS_NAME(getresuid32)(uid_t *ruid,
                                         uid_t *euid,
                                         uid_t *suid) {
      return LSS_NAME(getresuid)(ruid, euid, suid);
    }

    /* Need to make sure __off64_t isn't truncated to 32-bits under x32.  */
    LSS_INLINE void* LSS_NAME(mmap)(void *s, size_t l, int p, int f, int d,
                                    __off64_t o) {
      LSS_BODY(6, void*, mmap, LSS_SYSCALL_ARG(s), LSS_SYSCALL_ARG(l),
                               LSS_SYSCALL_ARG(p), LSS_SYSCALL_ARG(f),
                               LSS_SYSCALL_ARG(d), (uint64_t)(o));
    }

    LSS_INLINE _syscall4(int, newfstatat,         int,   d,
                         const char *,            p,
                        struct kernel_stat*,       b, int, f)

    LSS_INLINE int LSS_NAME(setfsgid32)(gid_t gid) {
      return LSS_NAME(setfsgid)(gid);
    }

    LSS_INLINE int LSS_NAME(setfsuid32)(uid_t uid) {
      return LSS_NAME(setfsuid)(uid);
    }

    LSS_INLINE int LSS_NAME(setresgid32)(gid_t rgid, gid_t egid, gid_t sgid) {
      return LSS_NAME(setresgid)(rgid, egid, sgid);
    }

    LSS_INLINE int LSS_NAME(setresuid32)(uid_t ruid, uid_t euid, uid_t suid) {
      return LSS_NAME(setresuid)(ruid, euid, suid);
    }

    LSS_INLINE int LSS_NAME(sigaction)(int signum,
                                       const struct kernel_sigaction *act,
                                       struct kernel_sigaction *oldact) {
      /* On x86_64, the kernel requires us to always set our own
       * SA_RESTORER in order to be able to return from a signal handler.
       * This function must have a "magic" signature that the "gdb"
       * (and maybe the kernel?) can recognize.
       */
      if (act != NULL && !(act->sa_flags & SA_RESTORER)) {
        struct kernel_sigaction a = *act;
        a.sa_flags   |= SA_RESTORER;
        a.sa_restorer = LSS_NAME(restore_rt)();
        return LSS_NAME(rt_sigaction)(signum, &a, oldact,
                                      (KERNEL_NSIG+7)/8);
      } else {
        return LSS_NAME(rt_sigaction)(signum, act, oldact,
                                      (KERNEL_NSIG+7)/8);
      }
    }

    LSS_INLINE int LSS_NAME(sigpending)(struct kernel_sigset_t *set) {
      return LSS_NAME(rt_sigpending)(set, (KERNEL_NSIG+7)/8);
    }

    LSS_INLINE int LSS_NAME(sigprocmask)(int how,
                                         const struct kernel_sigset_t *set,
                                         struct kernel_sigset_t *oldset) {
      return LSS_NAME(rt_sigprocmask)(how, set, oldset, (KERNEL_NSIG+7)/8);
    }

    LSS_INLINE int LSS_NAME(sigsuspend)(const struct kernel_sigset_t *set) {
      return LSS_NAME(rt_sigsuspend)(set, (KERNEL_NSIG+7)/8);
    }
  #endif
  #if defined(__x86_64__) || defined(__ARM_ARCH_3__) ||                       \
      defined(__ARM_EABI__) ||                                             \
     (defined(__mips__) && _MIPS_SIM != _MIPS_SIM_ABI32)
    LSS_INLINE _syscall4(pid_t, wait4,            pid_t, p,
                         int*,                    s, int,       o,
                        struct kernel_rusage*,     r)

    LSS_INLINE pid_t LSS_NAME(waitpid)(pid_t pid, int *status, int options){
      return LSS_NAME(wait4)(pid, status, options, 0);
    }
  #endif
  #if defined(__i386__) || defined(__x86_64__)
    LSS_INLINE _syscall4(int, openat, int, d, const char *, p, int, f, int, m)
    LSS_INLINE _syscall3(int, unlinkat, int, d, const char *, p, int, f)
  #endif
  #if defined(__i386__) || defined(__ARM_ARCH_3__) || defined(__ARM_EABI__)
    #define __NR__getresgid32 __NR_getresgid32
    #define __NR__getresuid32 __NR_getresuid32
    #define __NR__setfsgid32  __NR_setfsgid32
    #define __NR__setfsuid32  __NR_setfsuid32
    #define __NR__setresgid32 __NR_setresgid32
    #define __NR__setresuid32 __NR_setresuid32
#if defined(__ARM_EABI__)
    LSS_INLINE _syscall2(int,   ugetrlimit,        int,          r,
                        struct kernel_rlimit*, l)
#endif
    LSS_INLINE _syscall3(int,     _getresgid32,    gid_t *,      r,
                         gid_t *,            e,    gid_t *,      s)
    LSS_INLINE _syscall3(int,     _getresuid32,    uid_t *,      r,
                         uid_t *,            e,    uid_t *,      s)
    LSS_INLINE _syscall1(int,     _setfsgid32,     gid_t,        f)
    LSS_INLINE _syscall1(int,     _setfsuid32,     uid_t,        f)
    LSS_INLINE _syscall3(int,     _setresgid32,    gid_t,        r,
                         gid_t,              e,    gid_t,        s)
    LSS_INLINE _syscall3(int,     _setresuid32,    uid_t,        r,
                         uid_t,              e,    uid_t,        s)

    LSS_INLINE int LSS_NAME(getresgid32)(gid_t *rgid,
                                         gid_t *egid,
                                         gid_t *sgid) {
      int rc;
      if ((rc = LSS_NAME(_getresgid32)(rgid, egid, sgid)) < 0 &&
          LSS_ERRNO == ENOSYS) {
        if ((rgid == NULL) || (egid == NULL) || (sgid == NULL)) {
          return EFAULT;
        }
        // Clear the high bits first, since getresgid only sets 16 bits
        *rgid = *egid = *sgid = 0;
        rc = LSS_NAME(getresgid)(rgid, egid, sgid);
      }
      return rc;
    }

    LSS_INLINE int LSS_NAME(getresuid32)(uid_t *ruid,
                                         uid_t *euid,
                                         uid_t *suid) {
      int rc;
      if ((rc = LSS_NAME(_getresuid32)(ruid, euid, suid)) < 0 &&
          LSS_ERRNO == ENOSYS) {
        if ((ruid == NULL) || (euid == NULL) || (suid == NULL)) {
          return EFAULT;
        }
        // Clear the high bits first, since getresuid only sets 16 bits
        *ruid = *euid = *suid = 0;
        rc = LSS_NAME(getresuid)(ruid, euid, suid);
      }
      return rc;
    }

    LSS_INLINE int LSS_NAME(setfsgid32)(gid_t gid) {
      int rc;
      if ((rc = LSS_NAME(_setfsgid32)(gid)) < 0 &&
          LSS_ERRNO == ENOSYS) {
        if ((unsigned int)gid & ~0xFFFFu) {
          rc = EINVAL;
        } else {
          rc = LSS_NAME(setfsgid)(gid);
        }
      }
      return rc;
    }

    LSS_INLINE int LSS_NAME(setfsuid32)(uid_t uid) {
      int rc;
      if ((rc = LSS_NAME(_setfsuid32)(uid)) < 0 &&
          LSS_ERRNO == ENOSYS) {
        if ((unsigned int)uid & ~0xFFFFu) {
          rc = EINVAL;
        } else {
          rc = LSS_NAME(setfsuid)(uid);
        }
      }
      return rc;
    }

    LSS_INLINE int LSS_NAME(setresgid32)(gid_t rgid, gid_t egid, gid_t sgid) {
      int rc;
      if ((rc = LSS_NAME(_setresgid32)(rgid, egid, sgid)) < 0 &&
          LSS_ERRNO == ENOSYS) {
        if ((unsigned int)rgid & ~0xFFFFu ||
            (unsigned int)egid & ~0xFFFFu ||
            (unsigned int)sgid & ~0xFFFFu) {
          rc = EINVAL;
        } else {
          rc = LSS_NAME(setresgid)(rgid, egid, sgid);
        }
      }
      return rc;
    }

    LSS_INLINE int LSS_NAME(setresuid32)(uid_t ruid, uid_t euid, uid_t suid) {
      int rc;
      if ((rc = LSS_NAME(_setresuid32)(ruid, euid, suid)) < 0 &&
          LSS_ERRNO == ENOSYS) {
        if ((unsigned int)ruid & ~0xFFFFu ||
            (unsigned int)euid & ~0xFFFFu ||
            (unsigned int)suid & ~0xFFFFu) {
          rc = EINVAL;
        } else {
          rc = LSS_NAME(setresuid)(ruid, euid, suid);
        }
      }
      return rc;
    }
  #endif
  LSS_INLINE int LSS_NAME(sigemptyset)(struct kernel_sigset_t *set) {
    memset(&set->sig, 0, sizeof(set->sig));
    return 0;
  }

  LSS_INLINE int LSS_NAME(sigfillset)(struct kernel_sigset_t *set) {
    memset(&set->sig, -1, sizeof(set->sig));
    return 0;
  }

  LSS_INLINE int LSS_NAME(sigaddset)(struct kernel_sigset_t *set,
                                     int signum) {
    if (signum < 1 || signum > (int)(8*sizeof(set->sig))) {
      LSS_ERRNO = EINVAL;
      return -1;
    } else {
      set->sig[(signum - 1)/(8*sizeof(set->sig[0]))]
          |= 1UL << ((signum - 1) % (8*sizeof(set->sig[0])));
      return 0;
    }
  }

  LSS_INLINE int LSS_NAME(sigdelset)(struct kernel_sigset_t *set,
                                        int signum) {
    if (signum < 1 || signum > (int)(8*sizeof(set->sig))) {
      LSS_ERRNO = EINVAL;
      return -1;
    } else {
      set->sig[(signum - 1)/(8*sizeof(set->sig[0]))]
          &= ~(1UL << ((signum - 1) % (8*sizeof(set->sig[0]))));
      return 0;
    }
  }

  LSS_INLINE int LSS_NAME(sigismember)(struct kernel_sigset_t *set,
                                          int signum) {
    if (signum < 1 || signum > (int)(8*sizeof(set->sig))) {
      LSS_ERRNO = EINVAL;
      return -1;
    } else {
      return !!(set->sig[(signum - 1)/(8*sizeof(set->sig[0]))] &
                (1UL << ((signum - 1) % (8*sizeof(set->sig[0])))));
    }
  }
  #if defined(__i386__) || defined(__ARM_ARCH_3__) ||                         \
      defined(__ARM_EABI__) ||                                             \
     (defined(__mips__) && _MIPS_SIM == _MIPS_SIM_ABI32) || defined(__PPC__)
    #define __NR__sigaction   __NR_sigaction
    #define __NR__sigpending  __NR_sigpending
    #define __NR__sigprocmask __NR_sigprocmask
    #define __NR__sigsuspend  __NR_sigsuspend
    #define __NR__socketcall  __NR_socketcall
    LSS_INLINE _syscall2(int, fstat64,             int, f,
                         struct kernel_stat64 *, b)
    LSS_INLINE _syscall5(int, _llseek,     uint, fd,
                         unsigned long, hi, unsigned long, lo,
                         loff_t *, res, uint, wh)
#if !defined(__ARM_EABI__)
    LSS_INLINE _syscall1(void*, mmap,              void*, a)
#endif
    LSS_INLINE _syscall6(void*, mmap2,             void*, s,
                         size_t,                   l, int,               p,
                         int,                      f, int,               d,
                         off_t,                o)
    LSS_INLINE _syscall3(int,   _sigaction,        int,   s,
                         const struct kernel_old_sigaction*,  a,
                         struct kernel_old_sigaction*,        o)
    LSS_INLINE _syscall1(int,   _sigpending, unsigned long*, s)
    LSS_INLINE _syscall3(int,   _sigprocmask,      int,   h,
                         const unsigned long*,     s,
                         unsigned long*,           o)
    #ifdef __PPC__
    LSS_INLINE _syscall1(int, _sigsuspend,         unsigned long, s)
    #else
    LSS_INLINE _syscall3(int, _sigsuspend,         const void*, a,
                         int,                      b,
                         unsigned long,            s)
    #endif
    LSS_INLINE _syscall2(int, stat64,              const char *, p,
                         struct kernel_stat64 *, b)

    LSS_INLINE int LSS_NAME(sigaction)(int signum,
                                       const struct kernel_sigaction *act,
                                       struct kernel_sigaction *oldact) {
      int old_errno = LSS_ERRNO;
      int rc;
      struct kernel_sigaction a;
      if (act != NULL) {
        a             = *act;
        #ifdef __i386__
        /* On i386, the kernel requires us to always set our own
         * SA_RESTORER when using realtime signals. Otherwise, it does not
         * know how to return from a signal handler. This function must have
         * a "magic" signature that the "gdb" (and maybe the kernel?) can
         * recognize.
         * Apparently, a SA_RESTORER is implicitly set by the kernel, when
         * using non-realtime signals.
         *
         * TODO: Test whether ARM needs a restorer
         */
        if (!(a.sa_flags & SA_RESTORER)) {
          a.sa_flags   |= SA_RESTORER;
          a.sa_restorer = (a.sa_flags & SA_SIGINFO)
                          ? LSS_NAME(restore_rt)() : LSS_NAME(restore)();
        }
        #endif
      }
      rc = LSS_NAME(rt_sigaction)(signum, act ? &a : act, oldact,
                                  (KERNEL_NSIG+7)/8);
      if (rc < 0 && LSS_ERRNO == ENOSYS) {
        struct kernel_old_sigaction oa, ooa, *ptr_a = &oa, *ptr_oa = &ooa;
        if (!act) {
          ptr_a            = NULL;
        } else {
          oa.sa_handler_   = act->sa_handler_;
          memcpy(&oa.sa_mask, &act->sa_mask, sizeof(oa.sa_mask));
          #ifndef __mips__
          oa.sa_restorer   = act->sa_restorer;
          #endif
          oa.sa_flags      = act->sa_flags;
        }
        if (!oldact) {
          ptr_oa           = NULL;
        }
        LSS_ERRNO = old_errno;
        rc = LSS_NAME(_sigaction)(signum, ptr_a, ptr_oa);
        if (rc == 0 && oldact) {
          if (act) {
            memcpy(oldact, act, sizeof(*act));
          } else {
            memset(oldact, 0, sizeof(*oldact));
          }
          oldact->sa_handler_    = ptr_oa->sa_handler_;
          oldact->sa_flags       = ptr_oa->sa_flags;
          memcpy(&oldact->sa_mask, &ptr_oa->sa_mask, sizeof(ptr_oa->sa_mask));
          #ifndef __mips__
          oldact->sa_restorer    = ptr_oa->sa_restorer;
          #endif
        }
      }
      return rc;
    }

    LSS_INLINE int LSS_NAME(sigpending)(struct kernel_sigset_t *set) {
      int old_errno = LSS_ERRNO;
      int rc = LSS_NAME(rt_sigpending)(set, (KERNEL_NSIG+7)/8);
      if (rc < 0 && LSS_ERRNO == ENOSYS) {
        LSS_ERRNO = old_errno;
        LSS_NAME(sigemptyset)(set);
        rc = LSS_NAME(_sigpending)(&set->sig[0]);
      }
      return rc;
    }

    LSS_INLINE int LSS_NAME(sigprocmask)(int how,
                                         const struct kernel_sigset_t *set,
                                         struct kernel_sigset_t *oldset) {
      int olderrno = LSS_ERRNO;
      int rc = LSS_NAME(rt_sigprocmask)(how, set, oldset, (KERNEL_NSIG+7)/8);
      if (rc < 0 && LSS_ERRNO == ENOSYS) {
        LSS_ERRNO = olderrno;
        if (oldset) {
          LSS_NAME(sigemptyset)(oldset);
        }
        rc = LSS_NAME(_sigprocmask)(how,
                                    set ? &set->sig[0] : NULL,
                                    oldset ? &oldset->sig[0] : NULL);
      }
      return rc;
    }

    LSS_INLINE int LSS_NAME(sigsuspend)(const struct kernel_sigset_t *set) {
      int olderrno = LSS_ERRNO;
      int rc = LSS_NAME(rt_sigsuspend)(set, (KERNEL_NSIG+7)/8);
      if (rc < 0 && LSS_ERRNO == ENOSYS) {
        LSS_ERRNO = olderrno;
        rc = LSS_NAME(_sigsuspend)(
        #ifndef __PPC__
                                   set, 0,
        #endif
                                   set->sig[0]);
      }
      return rc;
    }
  #endif
  #if defined(__PPC__)
    #undef LSS_SC_LOADARGS_0
    #define LSS_SC_LOADARGS_0(dummy...)
    #undef LSS_SC_LOADARGS_1
    #define LSS_SC_LOADARGS_1(arg1)                                           \
        __sc_4  = (unsigned long) (arg1)
    #undef LSS_SC_LOADARGS_2
    #define LSS_SC_LOADARGS_2(arg1, arg2)                                     \
        LSS_SC_LOADARGS_1(arg1);                                              \
        __sc_5  = (unsigned long) (arg2)
    #undef LSS_SC_LOADARGS_3
    #define LSS_SC_LOADARGS_3(arg1, arg2, arg3)                               \
        LSS_SC_LOADARGS_2(arg1, arg2);                                        \
        __sc_6  = (unsigned long) (arg3)
    #undef LSS_SC_LOADARGS_4
    #define LSS_SC_LOADARGS_4(arg1, arg2, arg3, arg4)                         \
        LSS_SC_LOADARGS_3(arg1, arg2, arg3);                                  \
        __sc_7  = (unsigned long) (arg4)
    #undef LSS_SC_LOADARGS_5
    #define LSS_SC_LOADARGS_5(arg1, arg2, arg3, arg4, arg5)                   \
        LSS_SC_LOADARGS_4(arg1, arg2, arg3, arg4);                            \
        __sc_8  = (unsigned long) (arg5)
    #undef LSS_SC_BODY
    #define LSS_SC_BODY(nr, type, opt, args...)                               \
        long __sc_ret, __sc_err;                                              \
        {                                                                     \
          register unsigned long __sc_0 __asm__ ("r0") = __NR_socketcall;     \
          register unsigned long __sc_3 __asm__ ("r3") = opt;                 \
          register unsigned long __sc_4 __asm__ ("r4");                       \
          register unsigned long __sc_5 __asm__ ("r5");                       \
          register unsigned long __sc_6 __asm__ ("r6");                       \
          register unsigned long __sc_7 __asm__ ("r7");                       \
          register unsigned long __sc_8 __asm__ ("r8");                       \
          LSS_SC_LOADARGS_##nr(args);                                         \
          __asm__ __volatile__                                                \
              ("stwu 1, -48(1)\n\t"                                           \
               "stw 4, 20(1)\n\t"                                             \
               "stw 5, 24(1)\n\t"                                             \
               "stw 6, 28(1)\n\t"                                             \
               "stw 7, 32(1)\n\t"                                             \
               "stw 8, 36(1)\n\t"                                             \
               "addi 4, 1, 20\n\t"                                            \
               "sc\n\t"                                                       \
               "mfcr %0"                                                      \
                 : "=&r" (__sc_0),                                            \
                   "=&r" (__sc_3), "=&r" (__sc_4),                            \
                   "=&r" (__sc_5), "=&r" (__sc_6),                            \
                   "=&r" (__sc_7), "=&r" (__sc_8)                             \
                 : LSS_ASMINPUT_##nr                                          \
                 : "cr0", "ctr", "memory");                                   \
          __sc_ret = __sc_3;                                                  \
          __sc_err = __sc_0;                                                  \
        }                                                                     \
        LSS_RETURN(type, __sc_ret, __sc_err)

    LSS_INLINE ssize_t LSS_NAME(recvmsg)(int s,struct kernel_msghdr *msg,
                                         int flags){
      LSS_SC_BODY(3, ssize_t, 17, s, msg, flags);
    }

    LSS_INLINE ssize_t LSS_NAME(sendmsg)(int s,
                                         const struct kernel_msghdr *msg,
                                         int flags) {
      LSS_SC_BODY(3, ssize_t, 16, s, msg, flags);
    }

    // TODO(csilvers): why is this ifdef'ed out?
#if 0
    LSS_INLINE ssize_t LSS_NAME(sendto)(int s, const void *buf, size_t len,
                                        int flags,
                                        const struct kernel_sockaddr *to,
                                        unsigned int tolen) {
      LSS_BODY(6, ssize_t, 11, s, buf, len, flags, to, tolen);
    }
#endif

    LSS_INLINE int LSS_NAME(shutdown)(int s, int how) {
      LSS_SC_BODY(2, int, 13, s, how);
    }

    LSS_INLINE int LSS_NAME(socket)(int domain, int type, int protocol) {
      LSS_SC_BODY(3, int, 1, domain, type, protocol);
    }

    LSS_INLINE int LSS_NAME(socketpair)(int d, int type, int protocol,
                                        int sv[2]) {
      LSS_SC_BODY(4, int, 8, d, type, protocol, sv);
    }
  #endif
  #if defined(__ARM_EABI__)
    LSS_INLINE _syscall3(ssize_t, recvmsg, int, s, struct kernel_msghdr*, msg,
                         int, flags)
    LSS_INLINE _syscall3(ssize_t, sendmsg, int, s, const struct kernel_msghdr*,
                         msg, int, flags)
    LSS_INLINE _syscall6(ssize_t, sendto, int, s, const void*, buf, size_t,len,
                         int, flags, const struct kernel_sockaddr*, to,
                         unsigned int, tolen)
    LSS_INLINE _syscall2(int, shutdown, int, s, int, how)
    LSS_INLINE _syscall3(int, socket, int, domain, int, type, int, protocol)
    LSS_INLINE _syscall4(int, socketpair, int, d, int, type, int, protocol,
                         int*, sv)
  #endif
  #if defined(__i386__) || defined(__ARM_ARCH_3__) ||                         \
      (defined(__mips__) && _MIPS_SIM == _MIPS_SIM_ABI32)
    #define __NR__socketcall  __NR_socketcall
    LSS_INLINE _syscall2(int,      _socketcall,    int,   c,
                         va_list,                  a)
    LSS_INLINE int LSS_NAME(socketcall)(int op, ...) {
      int rc;
      va_list ap;
      va_start(ap, op);
      rc = LSS_NAME(_socketcall)(op, ap);
      va_end(ap);
      return rc;
    }

    LSS_INLINE ssize_t LSS_NAME(recvmsg)(int s,struct kernel_msghdr *msg,
                                         int flags){
      return (ssize_t)LSS_NAME(socketcall)(17, s, msg, flags);
    }

    LSS_INLINE ssize_t LSS_NAME(sendmsg)(int s,
                                         const struct kernel_msghdr *msg,
                                         int flags) {
      return (ssize_t)LSS_NAME(socketcall)(16, s, msg, flags);
    }

    LSS_INLINE ssize_t LSS_NAME(sendto)(int s, const void *buf, size_t len,
                                        int flags,
                                        const struct kernel_sockaddr *to,
                                        unsigned int tolen) {
      return (ssize_t)LSS_NAME(socketcall)(11, s, buf, len, flags, to, tolen);
    }

    LSS_INLINE int LSS_NAME(shutdown)(int s, int how) {
      return LSS_NAME(socketcall)(13, s, how);
    }

    LSS_INLINE int LSS_NAME(socket)(int domain, int type, int protocol) {
      return LSS_NAME(socketcall)(1, domain, type, protocol);
    }

    LSS_INLINE int LSS_NAME(socketpair)(int d, int type, int protocol,
                                        int sv[2]) {
      return LSS_NAME(socketcall)(8, d, type, protocol, sv);
    }
  #endif
  #if defined(__i386__) || defined(__PPC__)
    LSS_INLINE _syscall4(int,   fstatat64,        int,   d,
                         const char *,      p,
                         struct kernel_stat64 *,   b,    int,   f)
  #endif
  #if defined(__i386__) || defined(__PPC__) ||                                \
     (defined(__mips__) && _MIPS_SIM == _MIPS_SIM_ABI32)
    LSS_INLINE _syscall3(pid_t, waitpid,          pid_t, p,
                         int*,              s,    int,   o)
  #endif
  #if defined(__mips__)
    /* sys_pipe() on MIPS has non-standard calling conventions, as it returns
     * both file handles through CPU registers.
     */
    LSS_INLINE int LSS_NAME(pipe)(int *p) {
      register unsigned long __v0 __asm__("$2") = __NR_pipe;
      register unsigned long __v1 __asm__("$3");
      register unsigned long __r7 __asm__("$7");
      __asm__ __volatile__ ("syscall\n"
                            : "+r"(__v0), "=r"(__v1), "=r" (__r7)
                            : "0"(__v0)
                            : "$8", "$9", "$10", "$11", "$12",
                              "$13", "$14", "$15", "$24", "$25", "memory");
      if (__r7) {
        unsigned long __errnovalue = __v0;
        LSS_ERRNO = __errnovalue;
        return -1;
      } else {
        p[0] = __v0;
        p[1] = __v1;
        return 0;
      }
    }
  #else
    LSS_INLINE _syscall1(int,     pipe,           int *, p)
  #endif
  /* TODO(csilvers): see if ppc can/should support this as well              */
  #if defined(__i386__) || defined(__ARM_ARCH_3__) ||                         \
      defined(__ARM_EABI__) ||                                             \
     (defined(__mips__) && _MIPS_SIM != _MIPS_SIM_ABI64)
    #define __NR__statfs64  __NR_statfs64
    #define __NR__fstatfs64 __NR_fstatfs64
    LSS_INLINE _syscall3(int, _statfs64,     const char*, p,
                         size_t, s,struct kernel_statfs64*, b)
    LSS_INLINE _syscall3(int, _fstatfs64,          int,   f,
                         size_t, s,struct kernel_statfs64*, b)
    LSS_INLINE int LSS_NAME(statfs64)(const char *p,
                                     struct kernel_statfs64 *b) {
      return LSS_NAME(_statfs64)(p, sizeof(*b), b);
    }
    LSS_INLINE int LSS_NAME(fstatfs64)(int f,struct kernel_statfs64 *b) {
      return LSS_NAME(_fstatfs64)(f, sizeof(*b), b);
    }
  #endif

  LSS_INLINE int LSS_NAME(execv)(const char *path, const char *const argv[]) {
    extern char **environ;
    return LSS_NAME(execve)(path, argv, (const char *const *)environ);
  }

  LSS_INLINE pid_t LSS_NAME(gettid)(void) {
    pid_t tid = LSS_NAME(_gettid)();
    if (tid != -1) {
      return tid;
    }
    return LSS_NAME(getpid)();
  }

  LSS_INLINE void *LSS_NAME(mremap)(void *old_address, size_t old_size,
                                    size_t new_size, int flags, ...) {
    va_list ap;
    void *new_address, *rc;
    va_start(ap, flags);
    new_address = va_arg(ap, void *);
    rc = LSS_NAME(_mremap)(old_address, old_size, new_size,
                           flags, new_address);
    va_end(ap);
    return rc;
  }

  LSS_INLINE int LSS_NAME(ptrace_detach)(pid_t pid) {
    /* PTRACE_DETACH can sometimes forget to wake up the tracee and it
     * then sends job control signals to the real parent, rather than to
     * the tracer. We reduce the risk of this happening by starting a
     * whole new time slice, and then quickly sending a SIGCONT signal
     * right after detaching from the tracee.
     *
     * We use tkill to ensure that we only issue a wakeup for the thread being
     * detached.  Large multi threaded apps can take a long time in the kernel
     * processing SIGCONT.
     */
    int rc, err;
    LSS_NAME(sched_yield)();
    rc = LSS_NAME(ptrace)(PTRACE_DETACH, pid, (void *)0, (void *)0);
    err = LSS_ERRNO;
    LSS_NAME(tkill)(pid, SIGCONT);
    /* Old systems don't have tkill */
    if (LSS_ERRNO == ENOSYS)
      LSS_NAME(kill)(pid, SIGCONT);
    LSS_ERRNO = err;
    return rc;
  }

  LSS_INLINE int LSS_NAME(raise)(int sig) {
    return LSS_NAME(kill)(LSS_NAME(getpid)(), sig);
  }

  LSS_INLINE int LSS_NAME(setpgrp)(void) {
    return LSS_NAME(setpgid)(0, 0);
  }

  LSS_INLINE int LSS_NAME(sysconf)(int name) {
    extern int __getpagesize(void);
    switch (name) {
      case _SC_OPEN_MAX: {
        struct kernel_rlimit limit;
#if defined(__ARM_EABI__)
        return LSS_NAME(ugetrlimit)(RLIMIT_NOFILE, &limit) < 0
            ? 8192 : limit.rlim_cur;
#else
        return LSS_NAME(getrlimit)(RLIMIT_NOFILE, &limit) < 0
            ? 8192 : limit.rlim_cur;
#endif
      }
      case _SC_PAGESIZE:
        return __getpagesize();
      default:
        LSS_ERRNO = ENOSYS;
        return -1;
    }
  }
  #if defined(__x86_64__)
    /* Need to make sure loff_t isn't truncated to 32-bits under x32.  */
    LSS_INLINE ssize_t LSS_NAME(pread64)(int f, void *b, size_t c, loff_t o) {
      LSS_BODY(4, ssize_t, pread64, LSS_SYSCALL_ARG(f), LSS_SYSCALL_ARG(b),
                                    LSS_SYSCALL_ARG(c), (uint64_t)(o));
    }

    LSS_INLINE ssize_t LSS_NAME(pwrite64)(int f, const void *b, size_t c,
                                          loff_t o) {
      LSS_BODY(4, ssize_t, pwrite64, LSS_SYSCALL_ARG(f), LSS_SYSCALL_ARG(b),
                                     LSS_SYSCALL_ARG(c), (uint64_t)(o));
    }

    LSS_INLINE int LSS_NAME(readahead)(int f, loff_t o, unsigned c) {
      LSS_BODY(3, int, readahead, LSS_SYSCALL_ARG(f), (uint64_t)(o),
                                  LSS_SYSCALL_ARG(c));
    }
  #elif defined(__mips__) && _MIPS_SIM == _MIPS_SIM_ABI64
    LSS_INLINE _syscall4(ssize_t, pread64,        int,         f,
                         void *,         b, size_t,   c,
                         loff_t,         o)
    LSS_INLINE _syscall4(ssize_t, pwrite64,       int,         f,
                         const void *,   b, size_t,   c,
                         loff_t,         o)
    LSS_INLINE _syscall3(int,     readahead,      int,         f,
                         loff_t,         o, unsigned, c)
  #else
    #define __NR__pread64   __NR_pread64
    #define __NR__pwrite64  __NR_pwrite64
    #define __NR__readahead __NR_readahead
    #if defined(__ARM_EABI__) || defined(__mips__)
      /* On ARM and MIPS, a 64-bit parameter has to be in an even-odd register
       * pair. Hence these calls ignore their fourth argument (r3) so that their
       * fifth and sixth make such a pair (r4,r5).
       */
      #define LSS_LLARG_PAD 0,
      LSS_INLINE _syscall6(ssize_t, _pread64,        int,         f,
                           void *,         b, size_t, c,
                           unsigned, skip, unsigned, o1, unsigned, o2)
      LSS_INLINE _syscall6(ssize_t, _pwrite64,       int,         f,
                           const void *,   b, size_t, c,
                           unsigned, skip, unsigned, o1, unsigned, o2)
      LSS_INLINE _syscall5(int, _readahead,          int,         f,
                           unsigned,     skip,
                           unsigned,       o1, unsigned, o2, size_t, c)
    #else
      #define LSS_LLARG_PAD
      LSS_INLINE _syscall5(ssize_t, _pread64,        int,         f,
                           void *,         b, size_t, c, unsigned, o1,
                           unsigned, o2)
      LSS_INLINE _syscall5(ssize_t, _pwrite64,       int,         f,
                           const void *,   b, size_t, c, unsigned, o1,
                           long, o2)
      LSS_INLINE _syscall4(int, _readahead,          int,         f,
                           unsigned,       o1, unsigned, o2, size_t, c)
    #endif
    /* We force 64bit-wide parameters onto the stack, then access each
     * 32-bit component individually. This guarantees that we build the
     * correct parameters independent of the native byte-order of the
     * underlying architecture.
     */
    LSS_INLINE ssize_t LSS_NAME(pread64)(int fd, void *buf, size_t count,
                                         loff_t off) {
      union { loff_t off; unsigned arg[2]; } o = { off };
      return LSS_NAME(_pread64)(fd, buf, count,
                                LSS_LLARG_PAD o.arg[0], o.arg[1]);
    }
    LSS_INLINE ssize_t LSS_NAME(pwrite64)(int fd, const void *buf,
                                          size_t count, loff_t off) {
      union { loff_t off; unsigned arg[2]; } o = { off };
      return LSS_NAME(_pwrite64)(fd, buf, count,
                                 LSS_LLARG_PAD o.arg[0], o.arg[1]);
    }
    LSS_INLINE int LSS_NAME(readahead)(int fd, loff_t off, int len) {
      union { loff_t off; unsigned arg[2]; } o = { off };
      return LSS_NAME(_readahead)(fd, LSS_LLARG_PAD o.arg[0], o.arg[1], len);
    }
  #endif
#endif

#ifdef __ANDROID__
  /* These restore the original values of these macros saved by the
   * corresponding #pragma push_macro near the top of this file. */
# pragma pop_macro("stat64")
# pragma pop_macro("fstat64")
# pragma pop_macro("lstat64")
#endif

#if defined(__cplusplus) && !defined(SYS_CPLUSPLUS)
}
#endif

#endif
#endif
