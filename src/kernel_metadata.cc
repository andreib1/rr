/* -*- Mode: C++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil; -*- */

#include "kernel_metadata.h"

#include <errno.h>
#include <linux/shm.h>
#include <signal.h>
#include <syscall.h>

#include <sstream>

#include "kernel_abi.h"
#include "kernel_supplement.h"
#include "log.h"

using namespace std;

namespace rr {

#include "SyscallnameArch.generated"

string syscall_name(int syscall, SupportedArch arch) {
  RR_ARCH_FUNCTION(syscallname_arch, arch, syscall)
}

#define CASE(_id)                                                              \
  case _id:                                                                    \
    return #_id;

string arch_name(SupportedArch arch) {
  switch (arch) {
    CASE(x86_64);
    CASE(x86);
    CASE(aarch64);
  default:
    char buf[100];
    snprintf(buf, sizeof(buf), "Unknown architecture %d", arch);
    return string(buf);
  }
}

string ptrace_event_name(int event) {
  switch (event) {
    CASE(PTRACE_EVENT_FORK);
    CASE(PTRACE_EVENT_VFORK);
    CASE(PTRACE_EVENT_CLONE);
    CASE(PTRACE_EVENT_EXEC);
    CASE(PTRACE_EVENT_VFORK_DONE);
    CASE(PTRACE_EVENT_EXIT);
    /* XXX Ubuntu 12.04 defines a "PTRACE_EVENT_STOP", but that
     * has the same value as the newer EVENT_SECCOMP, so we'll
     * ignore STOP. */
    CASE(PTRACE_EVENT_SECCOMP_OBSOLETE);
    CASE(PTRACE_EVENT_SECCOMP);
    CASE(PTRACE_EVENT_STOP);
    /* Special-case this so we don't need to sprintf in this common case.
     * This case is common because we often pass ptrace_event_name(event) to
     * assertions when event is 0.
     */
    case 0:
      return "PTRACE_EVENT(0)";
    default: {
      char buf[100];
      snprintf(buf, sizeof(buf), "PTRACE_EVENT(%d)", event);
      return string(buf);
    }
  }
}

#define PTRACE_ARCH_CASE(_id)                                                  \
  case Arch::_id:                                                              \
    return #_id

template <typename Arch>
string ptrace_req_name(int request) {
  switch (request >= 0 ? request : INT32_MAX) {
    PTRACE_ARCH_CASE(PTRACE_TRACEME);
    PTRACE_ARCH_CASE(PTRACE_PEEKTEXT);
    PTRACE_ARCH_CASE(PTRACE_PEEKDATA);
    PTRACE_ARCH_CASE(PTRACE_PEEKUSR);
    PTRACE_ARCH_CASE(PTRACE_POKETEXT);
    PTRACE_ARCH_CASE(PTRACE_POKEDATA);
    PTRACE_ARCH_CASE(PTRACE_POKEUSR);
    PTRACE_ARCH_CASE(PTRACE_CONT);
    PTRACE_ARCH_CASE(PTRACE_KILL);
    PTRACE_ARCH_CASE(PTRACE_SINGLESTEP);
    PTRACE_ARCH_CASE(PTRACE_GETREGS);
    PTRACE_ARCH_CASE(PTRACE_SETREGS);
    PTRACE_ARCH_CASE(PTRACE_GETFPREGS);
    PTRACE_ARCH_CASE(PTRACE_SETFPREGS);
    PTRACE_ARCH_CASE(PTRACE_GETFPXREGS);
    PTRACE_ARCH_CASE(PTRACE_SETFPXREGS);
    PTRACE_ARCH_CASE(PTRACE_ATTACH);
    PTRACE_ARCH_CASE(PTRACE_DETACH);
    PTRACE_ARCH_CASE(PTRACE_SYSCALL);
    PTRACE_ARCH_CASE(PTRACE_SETOPTIONS);
    PTRACE_ARCH_CASE(PTRACE_GETEVENTMSG);
    PTRACE_ARCH_CASE(PTRACE_GETSIGINFO);
    PTRACE_ARCH_CASE(PTRACE_SETSIGINFO);
    PTRACE_ARCH_CASE(PTRACE_GETREGSET);
    PTRACE_ARCH_CASE(PTRACE_SETREGSET);
    PTRACE_ARCH_CASE(PTRACE_SEIZE);
    PTRACE_ARCH_CASE(PTRACE_INTERRUPT);
    PTRACE_ARCH_CASE(PTRACE_LISTEN);
    PTRACE_ARCH_CASE(PTRACE_GETSIGMASK);
    PTRACE_ARCH_CASE(PTRACE_SETSIGMASK);
    PTRACE_ARCH_CASE(PTRACE_GET_SYSCALL_INFO);
    // These aren't part of the official ptrace-request enum.
    PTRACE_ARCH_CASE(PTRACE_SYSEMU);
    PTRACE_ARCH_CASE(PTRACE_SYSEMU_SINGLESTEP);
    default: {
      char buf[100];
      snprintf(buf, sizeof(buf), "PTRACE_REQUEST(%d)", request);
      return string(buf);
    }
  }
}

template string ptrace_req_name<X86Arch>(int request);
template string ptrace_req_name<X64Arch>(int request);
template string ptrace_req_name<ARM64Arch>(int request);

string signal_name(int sig) {
  /* strsignal() would be nice to use here, but it provides TMI. */
  if (32 <= sig && sig <= 64) {
    char buf[100];
    snprintf(buf, sizeof(buf) - 1, "SIGRT%d", sig);
    return buf;
  }

  switch (sig) {
    CASE(SIGHUP);
    CASE(SIGINT);
    CASE(SIGQUIT);
    CASE(SIGILL);
    CASE(SIGTRAP);
    CASE(SIGABRT); /*CASE(SIGIOT);*/
    CASE(SIGBUS);
    CASE(SIGFPE);
    CASE(SIGKILL);
    CASE(SIGUSR1);
    CASE(SIGSEGV);
    CASE(SIGUSR2);
    CASE(SIGPIPE);
    CASE(SIGALRM);
    CASE(SIGTERM);
    CASE(SIGSTKFLT); /*CASE(SIGCLD);*/
    CASE(SIGCHLD);
    CASE(SIGCONT);
    CASE(SIGSTOP);
    CASE(SIGTSTP);
    CASE(SIGTTIN);
    CASE(SIGTTOU);
    CASE(SIGURG);
    CASE(SIGXCPU);
    CASE(SIGXFSZ);
    CASE(SIGVTALRM);
    CASE(SIGPROF);
    CASE(SIGWINCH); /*CASE(SIGPOLL);*/
    CASE(SIGIO);
    CASE(SIGPWR);
    CASE(SIGSYS);
    /* Special-case this so we don't need to sprintf in this common case.
     * This case is common because we often pass signal_name(sig) to assertions
     * when sig is 0.
     */
    case 0:
      return "signal(0)";
    default: {
      char buf[100];
      snprintf(buf, sizeof(buf), "signal(%d)", sig);
      return string(buf);
    }
  }
}

bool is_sigreturn(int syscallno, SupportedArch arch) {
  return is_sigreturn_syscall(syscallno, arch) ||
         is_rt_sigreturn_syscall(syscallno, arch);
}

const char *errno_name_cstr(int err) {
  switch (err) {
    case 0:
      return "SUCCESS";
      CASE(EPERM);
      CASE(ENOENT);
      CASE(ESRCH);
      CASE(EINTR);
      CASE(EIO);
      CASE(ENXIO);
      CASE(E2BIG);
      CASE(ENOEXEC);
      CASE(EBADF);
      CASE(ECHILD);
      CASE(EAGAIN);
      CASE(ENOMEM);
      CASE(EACCES);
      CASE(EFAULT);
      CASE(ENOTBLK);
      CASE(EBUSY);
      CASE(EEXIST);
      CASE(EXDEV);
      CASE(ENODEV);
      CASE(ENOTDIR);
      CASE(EISDIR);
      CASE(EINVAL);
      CASE(ENFILE);
      CASE(EMFILE);
      CASE(ENOTTY);
      CASE(ETXTBSY);
      CASE(EFBIG);
      CASE(ENOSPC);
      CASE(ESPIPE);
      CASE(EROFS);
      CASE(EMLINK);
      CASE(EPIPE);
      CASE(EDOM);
      CASE(ERANGE);
      CASE(EDEADLK);
      CASE(ENAMETOOLONG);
      CASE(ENOLCK);
      CASE(ENOSYS);
      CASE(ENOTEMPTY);
      CASE(ELOOP);
      CASE(ENOMSG);
      CASE(EIDRM);
      CASE(ECHRNG);
      CASE(EL2NSYNC);
      CASE(EL3HLT);
      CASE(EL3RST);
      CASE(ELNRNG);
      CASE(EUNATCH);
      CASE(ENOCSI);
      CASE(EL2HLT);
      CASE(EBADE);
      CASE(EBADR);
      CASE(EXFULL);
      CASE(ENOANO);
      CASE(EBADRQC);
      CASE(EBADSLT);
      CASE(EBFONT);
      CASE(ENOSTR);
      CASE(ENODATA);
      CASE(ETIME);
      CASE(ENOSR);
      CASE(ENONET);
      CASE(ENOPKG);
      CASE(EREMOTE);
      CASE(ENOLINK);
      CASE(EADV);
      CASE(ESRMNT);
      CASE(ECOMM);
      CASE(EPROTO);
      CASE(EMULTIHOP);
      CASE(EDOTDOT);
      CASE(EBADMSG);
      CASE(EOVERFLOW);
      CASE(ENOTUNIQ);
      CASE(EBADFD);
      CASE(EREMCHG);
      CASE(ELIBACC);
      CASE(ELIBBAD);
      CASE(ELIBSCN);
      CASE(ELIBMAX);
      CASE(ELIBEXEC);
      CASE(EILSEQ);
      CASE(ERESTART);
      CASE(ESTRPIPE);
      CASE(EUSERS);
      CASE(ENOTSOCK);
      CASE(EDESTADDRREQ);
      CASE(EMSGSIZE);
      CASE(EPROTOTYPE);
      CASE(ENOPROTOOPT);
      CASE(EPROTONOSUPPORT);
      CASE(ESOCKTNOSUPPORT);
      CASE(EOPNOTSUPP);
      CASE(EPFNOSUPPORT);
      CASE(EAFNOSUPPORT);
      CASE(EADDRINUSE);
      CASE(EADDRNOTAVAIL);
      CASE(ENETDOWN);
      CASE(ENETUNREACH);
      CASE(ENETRESET);
      CASE(ECONNABORTED);
      CASE(ECONNRESET);
      CASE(ENOBUFS);
      CASE(EISCONN);
      CASE(ENOTCONN);
      CASE(ESHUTDOWN);
      CASE(ETOOMANYREFS);
      CASE(ETIMEDOUT);
      CASE(ECONNREFUSED);
      CASE(EHOSTDOWN);
      CASE(EHOSTUNREACH);
      CASE(EALREADY);
      CASE(EINPROGRESS);
      CASE(ESTALE);
      CASE(EUCLEAN);
      CASE(ENOTNAM);
      CASE(ENAVAIL);
      CASE(EISNAM);
      CASE(EREMOTEIO);
      CASE(EDQUOT);
      CASE(ENOMEDIUM);
      CASE(EMEDIUMTYPE);
      CASE(ECANCELED);
      CASE(ENOKEY);
      CASE(EKEYEXPIRED);
      CASE(EKEYREVOKED);
      CASE(EKEYREJECTED);
      CASE(EOWNERDEAD);
      CASE(ENOTRECOVERABLE);
      CASE(ERFKILL);
      CASE(EHWPOISON);
    default: return NULL;
  }
}

string errno_name(int err) {
  const char *name = errno_name_cstr(err);
  if (name == NULL) {
    char buf[100];
    snprintf(buf, sizeof(buf), "errno(%d)", err);
    return string(buf);
  }
  return string(name);
}

string sicode_name(int code, int sig) {
  switch (code) {
    CASE(SI_USER);
    CASE(SI_KERNEL);
    CASE(SI_QUEUE);
    CASE(SI_TIMER);
    CASE(SI_MESGQ);
    CASE(SI_ASYNCIO);
    CASE(SI_SIGIO);
    CASE(SI_TKILL);
    CASE(SI_ASYNCNL);
  }

  switch (sig) {
    case SIGSEGV:
      switch (code) {
        CASE(SEGV_MAPERR);
        CASE(SEGV_ACCERR);
      }
      break;
    case SIGTRAP:
      switch (code) {
        CASE(TRAP_BRKPT);
        CASE(TRAP_TRACE);
        CASE(TRAP_HWBKPT);
      }
      break;
    case SIGILL:
      switch (code) {
        CASE(ILL_ILLOPC);
        CASE(ILL_ILLOPN);
        CASE(ILL_ILLADR);
        CASE(ILL_ILLTRP);
        CASE(ILL_PRVOPC);
        CASE(ILL_PRVREG);
        CASE(ILL_COPROC);
        CASE(ILL_BADSTK);
      }
      break;
    case SIGFPE:
      switch (code) {
        CASE(FPE_INTDIV);
        CASE(FPE_INTOVF);
        CASE(FPE_FLTDIV);
        CASE(FPE_FLTOVF);
        CASE(FPE_FLTUND);
        CASE(FPE_FLTRES);
        CASE(FPE_FLTINV);
        CASE(FPE_FLTSUB);
      }
      break;
    case SIGBUS:
      switch (code) {
        CASE(BUS_ADRALN);
        CASE(BUS_ADRERR);
        CASE(BUS_OBJERR);
        CASE(BUS_MCEERR_AR);
        CASE(BUS_MCEERR_AO);
      }
      break;
    case SIGCHLD:
      switch (code) {
        CASE(CLD_EXITED);
        CASE(CLD_KILLED);
        CASE(CLD_DUMPED);
        CASE(CLD_TRAPPED);
        CASE(CLD_STOPPED);
        CASE(CLD_CONTINUED);
      }
      break;
    case SIGPOLL:
      switch (code) {
        CASE(POLL_IN);
        CASE(POLL_OUT);
        CASE(POLL_MSG);
        CASE(POLL_ERR);
        CASE(POLL_PRI);
        CASE(POLL_HUP);
      }
      break;
  }

  char buf[100];
  snprintf(buf, sizeof(buf), "sicode(%d)", code);
  return string(buf);
}

int shm_flags_to_mmap_prot(int flags) {
  return PROT_READ | ((flags & SHM_RDONLY) ? 0 : PROT_WRITE) |
         ((flags & SHM_EXEC) ? PROT_EXEC : 0);
}

string xsave_feature_string(uint64_t xsave_features) {
  string ret;
  if (xsave_features & 0x01) {
    ret += "x87 ";
  }
  if (xsave_features & 0x02) {
    ret += "SSE ";
  }
  if (xsave_features & 0x04) {
    ret += "AVX ";
  }
  if (xsave_features & 0x08) {
    ret += "MPX-BNDREGS ";
  }
  if (xsave_features & 0x10) {
    ret += "MPX-BNDCSR ";
  }
  if (xsave_features & 0x20) {
    ret += "AVX512-opmask ";
  }
  if (xsave_features & 0x40) {
    ret += "AVX512-ZMM_Hi256 ";
  }
  if (xsave_features & 0x80) {
    ret += "AVX512-Hi16_ZMM ";
  }
  if (xsave_features & 0x100) {
    ret += "PT ";
  }
  if (xsave_features & 0x200) {
    ret += "PKRU ";
  }
  if (xsave_features & 0x2000) {
    ret += "HDC ";
  }
  if (ret.size() > 0) {
    ret = ret.substr(0, ret.size() - 1);
  }
  return ret;
}

bool is_coredumping_signal(int signo) {
  switch (signo) {
    case SIGQUIT:
    case SIGILL:
    case SIGTRAP:
    case SIGABRT:
    case SIGFPE:
    case SIGSEGV:
    case SIGBUS:
    case SIGSYS:
    case SIGXCPU:
    case SIGXFSZ:
        return true;
    default:
        return false;
  }
}

#define SI_COPY(f) result._sifields.f = si._sifields.f

template <typename Arch>
NativeArch::siginfo_t convert_to_native_siginfo_arch(const void* data,
    size_t size) {
  typename Arch::siginfo_t si;
  if (size != sizeof(si)) {
    FATAL() << "Siginfo has wrong size";
  }

  NativeArch::siginfo_t result;
  if (Arch::arch() == NativeArch::arch()) {
    // Do the simple correct thing to make sure there are no bugs in this all-important case.
    memcpy(&result, data, sizeof(result));
    return result;
  }

  // We need to translate formats :-(.
  memcpy(&si, data, sizeof(si));

  result.si_signo = si.si_signo;
  result.si_errno = si.si_errno;
  result.si_code = si.si_code;
  memset(result._sifields.padding, 0, sizeof(result._sifields.padding));
  if (result.si_code <= 0) {
    switch (result.si_code) {
      case SI_USER:
        SI_COPY(_kill.si_pid_);
        SI_COPY(_kill.si_uid_);
        break;
      case SI_QUEUE:
      case SI_MESGQ:
        SI_COPY(_rt.si_pid_);
        SI_COPY(_rt.si_uid_);
        SI_COPY(_rt.si_sigval_.sival_ptr.val);
        break;
      case SI_TIMER:
        SI_COPY(_timer.si_tid_);
        SI_COPY(_timer.si_overrun_);
        SI_COPY(_timer.si_sigval_.sival_ptr.val);
        break;
      default:
        break;
    }
  } else {
    switch (result.si_signo) {
      case SIGCHLD:
        SI_COPY(_sigchld.si_pid_);
        SI_COPY(_sigchld.si_uid_);
        SI_COPY(_sigchld.si_status_);
        SI_COPY(_sigchld.si_stime_);
        SI_COPY(_sigchld.si_utime_);
        break;
      case SIGILL:
      case SIGFPE:
      case SIGSEGV:
      case SIGBUS:
      case SIGTRAP:
        SI_COPY(_sigfault.si_addr_.val);
        SI_COPY(_sigfault.si_addr_lsb_);
        SI_COPY(_sigfault._bounds._addr_bnds._lower.val);
        SI_COPY(_sigfault._bounds._addr_bnds._upper.val);
        break;
      case SIGPOLL:
        SI_COPY(_sigpoll.si_band_);
        SI_COPY(_sigpoll.si_fd_);
        break;
      case SIGSYS:
        SI_COPY(_sigsys._call_addr.val);
        SI_COPY(_sigsys._syscall);
        SI_COPY(_sigsys._arch);
        break;
    }
  }

  return result;
}

NativeArch::siginfo_t convert_to_native_siginfo(SupportedArch arch,
    const void* data, size_t size) {
  RR_ARCH_FUNCTION(convert_to_native_siginfo_arch, arch, data, size);
}

string prot_flags_string(int prot) {
  char prot_flags[] = "rwx";
  if (!(prot & PROT_READ)) {
    prot_flags[0] = '-';
  }
  if (!(prot & PROT_WRITE)) {
    prot_flags[1] = '-';
  }
  if (!(prot & PROT_EXEC)) {
    prot_flags[2] = '-';
  }
  stringstream ret;
  ret << prot_flags;
  if (prot & ~(PROT_READ | PROT_WRITE | PROT_EXEC)) {
    ret << " (" << HEX(prot) << ")";
  }
  return ret.str();
}

} // namespace rr
