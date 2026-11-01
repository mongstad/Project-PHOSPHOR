#pragma once
#include <QObject>
#include <QSocketNotifier>
#include <QStringList>
#include <QTimer>
#include <functional>
#include <memory>
#include <sys/types.h>
class Pty : public QObject {
public:
  explicit Pty(QObject *parent = nullptr);
  ~Pty();
  bool start(const QString &shell, const QStringList &args, int rows, int cols);
  bool send(const QByteArray &bytes);
  void resize(int rows, int cols);
  void stop();
  pid_t pid() const { return child_; }
  bool running() const { return child_ > 0; }
  QString error;
  std::function<void(const QByteArray &)> received;
  std::function<void(int)> exited;

private:
  int fd_ = -1;
  pid_t child_ = -1, session_ = -1;
  QByteArray pending_;
  std::unique_ptr<QSocketNotifier> reader_, writer_;
  QTimer poll_;
  bool eof_ = false;
  void readReady();
  void writeReady();
  void reap();
  void signalSession(int signal);
};
QString configuredShell();
