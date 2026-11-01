#include "pty.h"
#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QFileInfo>
#include <QProcessEnvironment>
#include <cerrno>
#include <csignal>
#include <cstring>
#include <fcntl.h>
#include <pwd.h>
#include <sys/ioctl.h>
#include <sys/syscall.h>
#include <sys/wait.h>
#include <termios.h>
#include <unistd.h>
#include <vector>
QString configuredShell() {
  const passwd *pw = getpwuid(getuid());
  QString candidate = pw ? QString::fromLocal8Bit(pw->pw_shell) : QString();
  if (candidate.startsWith('/') && QFileInfo(candidate).isExecutable() &&
      QFileInfo(candidate).isFile())
    return candidate;
  return "/bin/bash";
}
Pty::Pty(QObject *p) : QObject(p) {
  poll_.setInterval(25);
  connect(&poll_, &QTimer::timeout, this, [this] { reap(); });
}
Pty::~Pty() { stop(); }
bool Pty::start(const QString &shell, const QStringList &args, int rows,
                int cols) {
  if (running()) {
    error = "PTY already running";
    return false;
  }
  fd_ = posix_openpt(O_RDWR | O_NOCTTY | O_CLOEXEC);
  if (fd_ < 0) {
    error = QString::fromLocal8Bit(strerror(errno));
    return false;
  }
  int slave = -1, pipefd[2] = {-1, -1};
  auto fail = [&] {
    error = QString::fromLocal8Bit(strerror(errno));
    if (slave >= 0)
      close(slave);
    if (pipefd[0] >= 0)
      close(pipefd[0]);
    if (pipefd[1] >= 0)
      close(pipefd[1]);
    close(fd_);
    fd_ = -1;
    return false;
  };
  char name[256];
  if (grantpt(fd_) < 0 || unlockpt(fd_) < 0 ||
      ptsname_r(fd_, name, sizeof(name)) != 0)
    return fail();
  slave = open(name, O_RDWR | O_NOCTTY | O_CLOEXEC);
  if (slave < 0 || pipe2(pipefd, O_CLOEXEC) < 0)
    return fail();
  winsize size{};
  size.ws_row = rows;
  size.ws_col = cols;
  if (ioctl(slave, TIOCSWINSZ, &size) < 0)
    return fail();
  std::vector<QByteArray> av{QFile::encodeName(shell)};
  for (const auto &a : args)
    av.push_back(a.toLocal8Bit());
  auto environment = QProcessEnvironment::systemEnvironment();
  environment.insert("TERM", "xterm-256color");
  environment.insert("COLORTERM", "truecolor");
  environment.remove("COLUMNS");
  environment.remove("LINES");
  std::vector<QByteArray> ev;
  for (const auto &s : environment.toStringList())
    ev.push_back(s.toLocal8Bit());
  std::vector<char *> argv, envp;
  for (auto &a : av)
    argv.push_back(a.data());
  argv.push_back(nullptr);
  for (auto &e : ev)
    envp.push_back(e.data());
  envp.push_back(nullptr);
  child_ = fork();
  if (child_ < 0)
    return fail();
  if (child_ == 0) {
    close(pipefd[0]);
    auto die = [&] {
      int e = errno;
      (void)!write(pipefd[1], &e, sizeof(e));
      _exit(127);
    };
    if (setsid() < 0 || ioctl(slave, TIOCSCTTY, 0) < 0)
      die();
    for (int n = 0; n < 3; ++n)
      if (dup2(slave, n) < 0)
        die();
    for (int s : {SIGINT, SIGQUIT, SIGHUP, SIGTERM, SIGPIPE, SIGCHLD, SIGTSTP,
                  SIGTTIN, SIGTTOU})
      signal(s, SIG_DFL);
    sigset_t empty;
    sigemptyset(&empty);
    sigprocmask(SIG_SETMASK, &empty, nullptr);
    // Keep only the exec-error pipe until successful exec; close every
    // inherited descriptor including Qt/display/socket descriptors (Linux-only
    // milestone).
    const int errorfd = pipefd[1];
    if (errorfd > 3)
      syscall(SYS_close_range, 3, unsigned(errorfd - 1), 0);
    syscall(SYS_close_range, unsigned(errorfd + 1), ~0U, 0);
    execve(argv[0], argv.data(), envp.data());
    die();
  }
  session_ = child_;
  close(slave);
  close(pipefd[1]);
  int childError = 0;
  ssize_t n;
  do {
    n = read(pipefd[0], &childError, sizeof(childError));
  } while (n < 0 && errno == EINTR);
  close(pipefd[0]);
  if (n > 0) {
    error = QString::fromLocal8Bit(strerror(childError));
    stop();
    return false;
  }
  if (fcntl(fd_, F_SETFL, fcntl(fd_, F_GETFL) | O_NONBLOCK) < 0) {
    error = "Cannot set nonblocking PTY";
    stop();
    return false;
  }
  reader_ = std::make_unique<QSocketNotifier>(fd_, QSocketNotifier::Read, this);
  writer_ =
      std::make_unique<QSocketNotifier>(fd_, QSocketNotifier::Write, this);
  writer_->setEnabled(false);
  connect(reader_.get(), &QSocketNotifier::activated, this,
          [this] { readReady(); });
  connect(writer_.get(), &QSocketNotifier::activated, this,
          [this] { writeReady(); });
  eof_ = false;
  poll_.start();
  return true;
}
void Pty::readReady() {
  QByteArray data;
  char buffer[8192];
  for (int total = 0; total < 65536;) {
    ssize_t n = read(fd_, buffer, sizeof(buffer));
    if (n > 0) {
      data.append(buffer, n);
      total += n;
      continue;
    }
    if (n < 0 && errno == EINTR)
      continue;
    if (n < 0 && (errno == EAGAIN || errno == EWOULDBLOCK))
      break;
    if (n < 0 && errno != EIO)
      error = QString::fromLocal8Bit(strerror(errno));
    eof_ = true;
    reader_->setEnabled(false);
    // A process may close all slave descriptors without exiting. Do not leave
    // an unusable window and lingering session indefinitely after EOF.
    QTimer::singleShot(250, this, [this] {
      if (eof_ && running()) {
        error = "PTY closed before shell exited";
        stop();
        if (exited)
          exited(129);
      }
    });
    break;
  }
  if (!data.isEmpty() && received)
    received(data);
}
bool Pty::send(const QByteArray &b) {
  if (fd_ < 0 || eof_)
    return false;
  if (pending_.size() + b.size() > 1024 * 1024) {
    error = "PTY input queue limit exceeded";
    return false;
  }
  pending_ += b;
  writeReady();
  return true;
}
void Pty::writeReady() {
  while (!pending_.isEmpty()) {
    ssize_t n = write(fd_, pending_.constData(), size_t(pending_.size()));
    if (n > 0) {
      pending_.remove(0, n);
      continue;
    }
    if (n < 0 && errno == EINTR)
      continue;
    if (n < 0 && (errno == EAGAIN || errno == EWOULDBLOCK))
      break;
    error = QString::fromLocal8Bit(strerror(errno));
    pending_.clear();
    break;
  }
  if (writer_)
    writer_->setEnabled(!pending_.isEmpty());
}
void Pty::resize(int rows, int cols) {
  if (fd_ < 0)
    return;
  winsize s{};
  s.ws_row = rows;
  s.ws_col = cols;
  if (ioctl(fd_, TIOCSWINSZ, &s) < 0)
    error = QString::fromLocal8Bit(strerror(errno));
  // The kernel sends SIGWINCH to the PTY foreground process group.
}
void Pty::signalSession(int sig) {
  if (session_ <= 0)
    return;
  // Include job-control background groups in our own PTY session, not merely
  // the shell's process group. Explicitly detached sessions are not owned.
  for (const auto &entry :
       QDir("/proc").entryList(QDir::Dirs | QDir::NoDotAndDotDot)) {
    bool ok = false;
    int p = entry.toInt(&ok);
    if (!ok || p <= 1)
      continue;
    if (getsid(p) == session_)
      kill(p, sig);
  }
}
void Pty::reap() {
  if (child_ <= 0)
    return;
  int status = 0;
  pid_t p = waitpid(child_, &status, WNOHANG);
  if (p == 0)
    return;
  if (p < 0 && errno == EINTR)
    return;
  if (reader_ && !eof_)
    readReady();
  signalSession(SIGHUP);
  signalSession(SIGKILL);
  child_ = -1;
  session_ = -1;
  poll_.stop();
  reader_.reset();
  writer_.reset();
  if (fd_ >= 0)
    close(fd_);
  fd_ = -1;
  pending_.clear();
  if (exited)
    exited(p > 0 && WIFEXITED(status)
               ? WEXITSTATUS(status)
               : 128 + (p > 0 && WIFSIGNALED(status) ? WTERMSIG(status) : 0));
}
void Pty::stop() {
  poll_.stop();
  reader_.reset();
  writer_.reset();
  signalSession(SIGHUP);
  if (fd_ >= 0)
    close(fd_);
  fd_ = -1;
  if (child_ > 0) {
    QElapsedTimer t;
    t.start();
    int status;
    while (t.elapsed() < 150 && waitpid(child_, &status, WNOHANG) == 0)
      usleep(5000);
    signalSession(SIGKILL);
    while (waitpid(child_, &status, 0) < 0 && errno == EINTR) {
    }
  }
  child_ = session_ = -1;
  pending_.clear();
}
