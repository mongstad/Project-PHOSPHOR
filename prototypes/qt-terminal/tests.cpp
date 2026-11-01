#include "pty.h"
#include "terminal.h"
#include "view.h"
#include <QDebug>
#include <QElapsedTimer>
#include <QFile>
#include <QGuiApplication>
#include <QThread>
#include <cerrno>
#include <csignal>
#include <unistd.h>
static void check(bool condition, const char *message) {
  if (!condition)
    qFatal("FAIL: %s", message);
}
static bool until(const std::function<bool()> &pred, int timeout = 4000) {
  QElapsedTimer t;
  t.start();
  while (!pred() && t.elapsed() < timeout) {
    QCoreApplication::processEvents();
    QThread::msleep(2);
  }
  return pred();
}
int main(int argc, char **argv) {
  QGuiApplication app(argc, argv);
  {
    Terminal whole(6, 24), split(6, 24);
    const QByteArray bytes =
        QByteArray("ab\bC\tD\rX\nY\033[2;3H") +
        QString::fromUtf8("é α ┌").toUtf8() +
        "\033[31mR\033[0m\033[38;2;12;34;56mT\033[0m\033[3;1Herase\033[2K\033["
        "4;1HZ\033]52;c;U0VDUkVU\a\033Pignored\033\\\033[999z";
    whole.feed(bytes);
    for (char b : bytes)
      split.feed(QByteArray(1, b));
    check(whole.textDump() == split.textDump(),
          "fragmented UTF-8 / CSI / OSC / DCS state");
    check(!split.textDump().contains("SECRET") &&
              !split.textDump().contains("ignored"),
          "escape payload leaked");
    check(split.cells()[0].glyph == 'X', "CR/BS cursor editing");
    check(split.cells()[24 + 2].glyph == QChar(0xe9), "UTF-8 codepoint");
    check(split.cells()[24 + 8].foreground == QColor(12, 34, 56), "truecolor");
    check(split.cells()[24 + 7].amber == false, "SGR color classification");
    check(split.cells()[2 * 24].glyph == ' ', "erase line");
    Terminal screen(3, 12);
    screen.feed("\033[2J\033[Habc\033[2;2HX\033[1AY\033[1DZ");
    check(screen.cells()[2].glyph == 'Z', "cursor movement");
    screen.feed("\033[H\033[7mR\033[0m\033[8mhidden\033[0m");
    check(screen.cells()[0].background == QColor(255, 179, 66),
          "reverse color");
    check(screen.cells()[1].glyph == ' ', "conceal");
    screen.feed("\033[2J\033[H");
    for (int n = 0; n < 1100; ++n)
      screen.feed("line\r\n");
    check(screen.historySize() == 1000, "bounded history");
    screen.scroll(900);
    check(screen.offset() == 900, "scroll navigation");
    screen.feed("another\r\n");
    check(screen.offset() == 901, "history anchoring");
    screen.resize(5, 20);
    check(screen.rows() == 5 && screen.columns() == 20 && screen.offset() == 0,
          "resize state");
    screen.feed("\033[?1049hALT\033[?1049l");
    check(!screen.textDump().contains("ALT"), "alternate screen restoration");
    Terminal unicode(2, 20);
    unicode.feed(QString::fromUtf8("é 中 😀").toUtf8());
    check(unicode.cells()[0].glyph == QChar(0xe9), "NFC composition");
    check(unicode.cells()[2].glyph == QChar(0xfffd) &&
              unicode.cells()[3].width == 0,
          "wide replacement maintains cells");
    unicode.feed("\033[H\033[44m");
    unicode.feed(QString::fromUtf8("中").toUtf8());
    check(unicode.cells()[0].background == unicode.cells()[1].background,
          "wide continuation background");
    qInfo()
        << "PASS parser fragmentation, erase/movement, color, ignored strings, "
           "Unicode boundary, alternate screen, bounded scrollback";
  }
  {
    Terminal t(4, 20);
    RenderMetrics m;
    TerminalView view(nullptr, t, m);
    QByteArray keys;
    t.output = [&](const QByteArray &b) { keys += b; };
    QKeyEvent c(QEvent::KeyPress, Qt::Key_C, Qt::ControlModifier);
    view.handleKey(&c);
    QKeyEvent d(QEvent::KeyPress, Qt::Key_D, Qt::ControlModifier);
    view.handleKey(&d);
    QKeyEvent l(QEvent::KeyPress, Qt::Key_L, Qt::ControlModifier);
    view.handleKey(&l);
    QKeyEvent enter(QEvent::KeyPress, Qt::Key_Return, Qt::NoModifier);
    view.handleKey(&enter);
    QKeyEvent back(QEvent::KeyPress, Qt::Key_Backspace, Qt::NoModifier);
    view.handleKey(&back);
    QKeyEvent tab(QEvent::KeyPress, Qt::Key_Tab, Qt::NoModifier);
    view.handleKey(&tab);
    QKeyEvent left(QEvent::KeyPress, Qt::Key_Left, Qt::NoModifier);
    view.handleKey(&left);
    check(keys == QByteArray("\x03\x04\x0c\r\x7f\t\033[D"),
          "Qt key to terminal encoding");
    keys.clear();
    t.feed("\033[?1h");
    view.handleKey(&left);
    check(keys == "\033OD", "application cursor mode");
    qInfo() << "PASS keyboard Qt events and application cursor mode";
  }
  {
    Pty p;
    check(!p.start("/nonexistent/phosphor-shell", {}, 24, 80),
          "exec failure reported");
    check(!p.running(), "failed child reaped");
  }
  {
    Pty p;
    Terminal terminal(24, 80);
    QByteArray raw;
    int exit = -1;
    p.received = [&](const QByteArray &b) {
      raw += b;
      terminal.feed(b);
    };
    terminal.output = [&](const QByteArray &b) {
      check(p.send(b), "PTY send");
    };
    p.exited = [&](int n) { exit = n; };
    check(p.start("/bin/bash", {"--noprofile", "--norc", "-i"}, 24, 80),
          "PTY shell start");
    const auto pid = p.pid();
    check(until([&] { return raw.contains("bash-"); }), "interactive prompt");
    p.send("printf '__READY__\\n'; stty size\r");
    check(until([&] { return raw.contains("24 80"); }),
          "PTY initial geometry and command");
    p.resize(31, 99);
    p.send("stty size\r");
    check(until([&] { return raw.contains("31 99"); }), "PTY resize ioctl");
    p.send("sleep 30\r");
    QThread::msleep(80);
    raw.clear();
    p.send(QByteArray(1, '\x03'));
    check(until([&] { return raw.contains("bash-"); }),
          "prompt after interrupt");
    p.send("printf '__INTERRUPTED__\\n'\r");
    if (!until([&] { return raw.contains("\r__INTERRUPTED__\r\n"); })) {
      qInfo() << "INTERRUPT_RAW" << raw;
      qFatal("foreground Ctrl+C");
    }
    raw.clear();
    p.send("echo helo");
    p.send("\033[D");
    p.send("l\r");
    check(until([&] { return raw.contains("\rhello\r\n"); }),
          "readline left-arrow insertion");
    p.send("exit 7\r");
    check(until([&] { return exit >= 0; }), "EOF/process exit");
    check(exit == 7 && !p.running(), "exit status");
    check(kill(pid, 0) < 0 && errno == ESRCH, "shell no orphan");
    qInfo() << "PASS real interactive Bash, nonblocking output, resize, "
               "Ctrl+C, exit status/reaping";
  }
  {
    Pty p;
    int exit = -1;
    p.exited = [&](int n) { exit = n; };
    check(p.start(
              "/bin/bash",
              {"--noprofile", "--norc", "-c", "exec 0<&- 1>&- 2>&-; sleep 30"},
              24, 80),
          "EOF-only process start");
    check(until([&] { return exit >= 0; }),
          "EOF without shell exit triggers cleanup");
    check(exit == 129 && !p.running(), "EOF cleanup result");
    qInfo() << "PASS EOF with live subprocess cleanup";
  }
  {
    Pty p;
    QByteArray raw;
    p.received = [&](const QByteArray &b) { raw += b; };
    check(p.start("/bin/bash",
                  {"--noprofile", "--norc", "-c",
                   "trap '' HUP TERM; sleep 60 & echo CHILD:$!; wait"},
                  24, 80),
          "cleanup start");
    check(until([&] { return raw.contains("CHILD:") && raw.contains('\n'); }),
          "background child output");
    int child = raw.mid(raw.indexOf("CHILD:") + 6).trimmed().toInt();
    check(child > 1, "child pid");
    auto shell = p.pid();
    p.stop();
    check(kill(shell, 0) < 0 && errno == ESRCH, "closed shell reaped");
    // A child may briefly remain a zombie until the system reaper runs, but
    // must not remain an executing process. /proc state distinguishes that
    // case.
    check(until([&] {
            QFile f(QString("/proc/%1/stat").arg(child));
            if (!f.open(QIODevice::ReadOnly))
              return true;
            auto s = f.readAll();
            return s.mid(s.lastIndexOf(')') + 2, 1) == "Z";
          }),
          "background job terminated on close");
    qInfo() << "PASS close kills HUP-resistant shell and background job";
  }
  qInfo() << "ALL TESTS PASSED";
}
