#include "pty.h"
#include "view.h"
#include <QCommandLineParser>
#include <QDebug>
#include <QElapsedTimer>
#include <QFile>
#include <QGuiApplication>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QQuickWindow>
#include <QScreen>
#include <memory>
class Window : public QQuickWindow {
public:
  std::function<void()> resized;
  bool event(QEvent *e) override {
    bool r = QQuickWindow::event(e);
    if (e->type() == QEvent::DevicePixelRatioChange && resized)
      resized();
    return r;
  }
};
int main(int argc, char **argv) {
  QGuiApplication app(argc, argv);
  app.setApplicationName("phosphor-terminal");
  app.setDesktopFileName("org.phosphor.Terminal004");
  QCommandLineParser cli;
  cli.addHelpOption();
  cli.setApplicationDescription(
      "Bounded PHOSPHOR terminal milestone; fixed refined 16 pt");
  cli.addOptions(
      {{"shell", "Absolute shell executable (default: passwd shell)", "path",
        configuredShell()},
       {"clean-bash",
        "Use /bin/bash --noprofile --norc -i for reproducible tests"},
       {"width", "Client width", "pixels", "1280"},
       {"height", "Client height", "pixels", "900"},
       {"screen", "Existing output", "name", "DP-1"},
       {"fullscreen", "Fullscreen on selected output"},
       {"seconds", "Timed test exit (0 disables)", "n", "0"},
       {"metrics", "Test metrics JSON on exit", "file"},
       {"test-script",
        "Local JSON key/resize/capture test actions, scheduled after first "
        "frame",
        "file"}});
  cli.process(app);
  auto integer = [&](const char *key, int min, int max) {
    bool ok;
    int n = cli.value(key).toInt(&ok);
    if (!ok || n < min || n > max)
      qFatal("Invalid %s", key);
    return n;
  };
  const int width = integer("width", 320, 4096),
            height = integer("height", 240, 2160),
            seconds = integer("seconds", 0, 3600);
  QJsonArray actions;
  if (cli.isSet("test-script")) {
    QFile f(cli.value("test-script"));
    if (!f.open(QIODevice::ReadOnly))
      return 2;
    QJsonParseError error;
    auto d = QJsonDocument::fromJson(f.readAll(), &error);
    if (error.error != QJsonParseError::NoError || !d.isArray())
      return 2;
    actions = d.array();
  }
  Terminal terminal(24, 80);
  RenderMetrics metrics;
  Pty pty;
  Window window;
  window.setTitle("PHOSPHOR | Molten Amber | 16 pt");
  window.setColor(QColor(7, 5, 3));
  window.setMinimumSize({320, 240});
  bool found = false;
  for (auto *s : app.screens())
    if (s->name() == cli.value("screen")) {
      window.setScreen(s);
      window.setPosition(s->geometry().topLeft());
      found = true;
    }
  if (!found) {
    qCritical() << "Screen not available";
    return 2;
  }
  TerminalView view(window.contentItem(), terminal, metrics);
  QElapsedTimer elapsed;
  elapsed.start();
  qint64 readBytes = 0, parseNs = 0, firstFrame = -1;
  QTimer repaint;
  repaint.setSingleShot(true);
  repaint.setInterval(16);
  QObject::connect(&repaint, &QTimer::timeout, &app, [&] { view.update(); });
  terminal.output = [&](const QByteArray &b) {
    if (!pty.send(b))
      qWarning() << "PTY write:" << pty.error;
  };
  pty.received = [&](const QByteArray &b) {
    readBytes += b.size();
    QElapsedTimer t;
    t.start();
    terminal.feed(b);
    parseNs += t.nsecsElapsed();
    if (!repaint.isActive())
      repaint.start();
  };
  pty.exited = [&](int status) {
    qInfo() << "SHELL_EXIT" << status;
    app.exit(status);
  };
  window.resized = [&] {
    view.setSize(window.size());
    int rows =
            std::clamp(int((window.height() - 40) / view.lineHeight()), 1, 256),
        cols =
            std::clamp(int((window.width() - 48) / view.cellWidth()), 1, 512);
    terminal.resize(rows, cols);
    pty.resize(rows, cols);
    view.update();
    qInfo() << "GEOMETRY" << window.size() << "GRID" << cols << rows << "DPR"
            << window.devicePixelRatio();
  };
  QObject::connect(&window, &QWindow::widthChanged, &app, window.resized);
  QObject::connect(&window, &QWindow::heightChanged, &app, window.resized);
  QObject::connect(
      &window, &QQuickWindow::sceneGraphInitialized, &window,
      [&] {
        auto api = window.rendererInterface()->graphicsApi();
        qInfo() << "BACKEND" << api;
        if (!QSGRendererInterface::isApiRhiBased(api))
          qFatal("GPU RHI backend required");
      },
      Qt::DirectConnection);
  window.resize(width, height);
  window.resized();
  QString shell = cli.isSet("clean-bash") ? "/bin/bash" : cli.value("shell");
  QStringList args = cli.isSet("clean-bash")
                         ? QStringList{"--noprofile", "--norc", "-i"}
                         : QStringList{"-i"};
  if (!shell.startsWith('/') ||
      !pty.start(shell, args, terminal.rows(), terminal.columns())) {
    qCritical() << "SHELL_START_FAILED" << shell << pty.error;
    return 2;
  }
  qInfo() << "SHELL_PID" << pty.pid() << "SHELL" << shell << "ARGS" << args;
  auto snapshot = [&] {
    return QJsonObject{{"elapsed_ms", elapsed.elapsed()},
                       {"first_frame_ms", firstFrame},
                       {"read_bytes", readBytes},
                       {"parse_ms", parseNs / 1e6},
                       {"render_calls", metrics.frames.load()},
                       {"render_work_ms", metrics.workNs.load() / 1e6},
                       {"texture_bytes", metrics.textureBytes.load()},
                       {"cached_glyphs", metrics.glyphs.load()},
                       {"fallback_cells", metrics.fallbackCells.load()},
                       {"rows", terminal.rows()},
                       {"columns", terminal.columns()},
                       {"history_lines", terminal.historySize()},
                       {"scroll_offset", terminal.offset()},
                       {"width", window.width()},
                       {"height", window.height()},
                       {"dpr", window.devicePixelRatio()}};
  };
  auto save = [&](const QString &path, const QByteArray &bytes) {
    QFile f(path);
    if (!f.open(QIODevice::WriteOnly) || f.write(bytes) != bytes.size()) {
      qCritical() << "Evidence write failed" << path;
      app.exit(2);
    }
  };
  QObject::connect(&window, &QQuickWindow::frameSwapped, &app, [&] {
    if (firstFrame >= 0)
      return;
    firstFrame = elapsed.elapsed();
    qInfo() << "FIRST_FRAME_MS" << firstFrame;
    for (const auto &value : actions) {
      auto a = value.toObject();
      int delay = a.value("ms").toInt(-1);
      if (delay < 0 || delay > 3600000) {
        app.exit(2);
        return;
      }
      QTimer::singleShot(delay, Qt::PreciseTimer, &app, [&, a] {
        if (a.contains("text")) {
          QKeyEvent e(QEvent::KeyPress, 0, Qt::NoModifier,
                      a["text"].toString());
          QCoreApplication::sendEvent(&window, &e);
        }
        if (a.contains("key")) {
          const QMap<QString, int> keys = {
              {"Return", Qt::Key_Return},    {"Backspace", Qt::Key_Backspace},
              {"Tab", Qt::Key_Tab},          {"Left", Qt::Key_Left},
              {"Right", Qt::Key_Right},      {"Up", Qt::Key_Up},
              {"Down", Qt::Key_Down},        {"PageUp", Qt::Key_PageUp},
              {"PageDown", Qt::Key_PageDown}};
          QString name = a["key"].toString();
          int key = keys.value(
              name, name.size() == 1 ? name[0].toUpper().unicode() : 0);
          if (!key) {
            app.exit(2);
            return;
          }
          Qt::KeyboardModifiers mod = Qt::NoModifier;
          if (a["ctrl"].toBool())
            mod |= Qt::ControlModifier;
          if (a["shift"].toBool())
            mod |= Qt::ShiftModifier;
          QKeyEvent e(QEvent::KeyPress, key, mod);
          QCoreApplication::sendEvent(&window, &e);
        }
        if (a.contains("mouse")) {
          const QString kind = a["mouse"].toString();
          const auto type = kind == "press" ? QEvent::MouseButtonPress :
                            kind == "release" ? QEvent::MouseButtonRelease : QEvent::MouseMove;
          const QPointF point = a["thumb"].toBool() ? view.scrollThumb().center()
              : QPointF(a["x"].toDouble(), a["y"].toDouble());
          QMouseEvent e(type, point, window.mapToGlobal(point.toPoint()),
                        type == QEvent::MouseMove ? Qt::NoButton : Qt::LeftButton,
                        type == QEvent::MouseButtonRelease || kind == "hover"
                            ? Qt::NoButton : Qt::LeftButton,
                        Qt::NoModifier);
          e.setTimestamp(elapsed.elapsed());
          QCoreApplication::sendEvent(&window, &e);
        }
        if (a.contains("wheel")) {
          const QPointF point(100, 100);
          QWheelEvent e(point, window.mapToGlobal(point.toPoint()), {},
                        QPoint(0, a["wheel"].toInt()), Qt::NoButton,
                        Qt::NoModifier, Qt::NoScrollPhase, false);
          e.setTimestamp(elapsed.elapsed());
          QCoreApplication::sendEvent(&window, &e);
        }
        if (a.contains("width"))
          window.resize(a["width"].toInt(), a["height"].toInt());
        if (a.contains("capture")) {
          auto im = window.grabWindow();
          if (im.isNull() || !im.save(a["capture"].toString()))
            app.exit(2);
          qInfo() << "CAPTURE" << im.size();
        }
        if (a.contains("dump"))
          save(a["dump"].toString(), terminal.textDump().toUtf8());
        if (a.contains("metrics"))
          save(a["metrics"].toString(), QJsonDocument(snapshot()).toJson());
        if (a["close"].toBool())
          window.close();
      });
    }
  });
  if (cli.isSet("fullscreen"))
    window.showFullScreen();
  else
    window.show();
  view.forceActiveFocus();
  if (seconds)
    QTimer::singleShot(seconds * 1000, &app, &QCoreApplication::quit);
  int result = app.exec();
  repaint.stop();
  pty.stop();
  window.resized = {};
  qInfo().noquote() << "SUMMARY"
                    << QJsonDocument(snapshot()).toJson(QJsonDocument::Compact);
  if (cli.isSet("metrics"))
    save(cli.value("metrics"), QJsonDocument(snapshot()).toJson());
  return result;
}
