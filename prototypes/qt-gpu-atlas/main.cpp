#include "view.h"
#include <QCommandLineParser>
#include <QDebug>
#include <QElapsedTimer>
#include <QFile>
#include <QGuiApplication>
#include <QJsonDocument>
#include <QJsonObject>
#include <QQuickWindow>
#include <QSGRendererInterface>
#include <QScreen>
#include <QTimer>
#include <functional>

class Window : public QQuickWindow {
public:
  std::function<void()> resizeContent;
  bool event(QEvent *e) override {
    const bool handled = QQuickWindow::event(e);
    if (e->type() == QEvent::DevicePixelRatioChange && resizeContent)
      resizeContent();
    return handled;
  }
};
int main(int argc, char **argv) {
  QGuiApplication app(argc, argv);
  app.setApplicationName("phosphor-003");
  app.setDesktopFileName("org.phosphor.Spike003");
  QCommandLineParser cli;
  cli.addHelpOption();
  cli.setApplicationDescription(
      "Fixed refined 16 pt glyph-atlas feasibility sample; no terminal engine");
  cli.addOptions(
      {{"path", "Renderer: cpu or gpu", "path", "cpu"},
       {"effects-off", "Identical fixed font with constant glyph color"},
       {"width", "Logical client width", "pixels", "1280"},
       {"height", "Logical client height", "pixels", "900"},
       {"screen", "Existing screen name", "name", "DP-1"},
       {"fullscreen", "Use selected physical screen fullscreen"},
       {"fixed-size", "Lock client size for matched benchmarks"},
       {"seconds", "Exit after this many seconds (0 stays open)", "seconds",
        "0"},
       {"update-after-ms",
        "Start 16 ms synthetic updates at this elapsed time (0 disabled)", "ms",
        "0"},
       {"capture", "Unedited client PNG at seven seconds", "path"},
       {"metrics", "Write summary JSON on exit", "path"},
       {"resize-test", "Resize to 800x600 then back to initial dimensions"}});
  cli.process(app);
  auto value = [&](const char *name, int lo, int hi) {
    bool ok = false;
    int n = cli.value(name).toInt(&ok);
    if (!ok || n < lo || n > hi)
      qFatal("Invalid %s", name);
    return n;
  };
  const auto path = cli.value("path");
  if (path != "cpu" && path != "gpu") {
    qCritical() << "--path must be cpu or gpu";
    return 2;
  }
  const int width = value("width", 320, 4096),
            height = value("height", 240, 2160);
  const int seconds = value("seconds", 0, 3600),
            updateAfter = value("update-after-ms", 0, 3600000);
  const bool gpu = path == "gpu", effects = !cli.isSet("effects-off");
  QElapsedTimer elapsed;
  elapsed.start();
  Metrics metrics;
  Window window;
  window.setColor(QColor(7, 5, 3));
  window.setTitle(QString("PHOSPHOR-003 | %1 | %2 | 16 pt")
                      .arg(path, effects ? "REFINED AMBER" : "EFFECTS OFF"));
  bool found = false;
  for (auto *screen : app.screens())
    if (screen->name() == cli.value("screen")) {
      window.setScreen(screen);
      window.setPosition(screen->geometry().topLeft());
      found = true;
    }
  if (!found) {
    qCritical() << "Screen unavailable";
    return 2;
  }
  if (cli.isSet("fixed-size")) {
    if (cli.isSet("resize-test")) {
      qCritical() << "fixed-size and resize-test are incompatible";
      return 2;
    }
    window.setMinimumSize(QSize(width, height));
    window.setMaximumSize(QSize(width, height));
  }
  window.resize(width, height);
  std::unique_ptr<CpuView> cpu;
  std::unique_ptr<GpuView> atlas;
  if (gpu)
    atlas = std::make_unique<GpuView>(window.contentItem(), metrics, effects);
  else
    cpu = std::make_unique<CpuView>(window.contentItem(), metrics, effects);
  QQuickItem *item = gpu ? static_cast<QQuickItem *>(atlas.get()) : cpu.get();
  window.resizeContent = [&] {
    qInfo() << "GEOMETRY" << elapsed.elapsed() << window.size() << "DPR"
            << window.devicePixelRatio();
    item->setSize(window.size());
    if (cpu)
      cpu->setTextureSize(window.size() * window.devicePixelRatio());
    item->update();
  };
  QObject::connect(&window, &QWindow::widthChanged, &app, window.resizeContent);
  QObject::connect(&window, &QWindow::heightChanged, &app,
                   window.resizeContent);
  QObject::connect(&window, &QWindow::screenChanged, &app,
                   window.resizeContent);
  QObject::connect(
      &window, &QQuickWindow::sceneGraphInitialized, &window,
      [&] {
        const auto api = window.rendererInterface()->graphicsApi();
        qInfo() << "BACKEND" << api;
        if (gpu && !QSGRendererInterface::isApiRhiBased(api))
          qFatal("Atlas requires default RHI scene graph backend");
      },
      Qt::DirectConnection);
  qint64 frames = 0, firstFrameMs = -1, updates = 0;
  QObject::connect(&window, &QQuickWindow::frameSwapped, &app, [&] {
    ++frames;
    if (firstFrameMs < 0) {
      firstFrameMs = elapsed.elapsed();
      qInfo() << "FIRST_FRAME_MS" << firstFrameMs;
    }
  });
  QTimer timer;
  timer.setTimerType(Qt::PreciseTimer);
  QObject::connect(&timer, &QTimer::timeout, &app, [&] {
    ++updates;
    if (cpu)
      ++cpu->revision;
    else
      ++atlas->revision;
    item->update();
  });
  auto snapshot = [&] {
    return QJsonObject{{"path", path},
                       {"effects", effects},
                       {"elapsed_ms", elapsed.elapsed()},
                       {"first_frame_ms", firstFrameMs},
                       {"cache_builds", metrics.builds.load()},
                       {"cache_wall_ms", metrics.cacheNs.load() / 1e6},
                       {"work_wall_ms", metrics.workNs.load() / 1e6},
                       {"work_calls", metrics.calls.load()},
                       {"requested_texture_bytes", metrics.textureBytes.load()},
                       {"geometry_bytes", metrics.geometryBytes.load()},
                       {"frame_swapped", frames},
                       {"synthetic_updates", updates},
                       {"dpr", window.devicePixelRatio()},
                       {"width", window.width()},
                       {"height", window.height()},
                       {"screen", window.screen()->name()}};
  };
  if (updateAfter)
    QTimer::singleShot(
        updateAfter, &app, [&] {
          qInfo().noquote()
              << "WARM_IDLE"
              << QJsonDocument(snapshot()).toJson(QJsonDocument::Compact);
          timer.start(16);
        });
  QTimer::singleShot(7000, &app, [&] {
    qInfo() << "PRESENTATION" << window.screen()->name() << window.size()
            << "DPR" << window.devicePixelRatio() << "logicalDPI"
            << window.screen()->logicalDotsPerInch();
    if (cli.isSet("capture")) {
      const auto image = window.grabWindow();
      if (image.isNull() || !image.save(cli.value("capture"))) {
        qCritical() << "Capture failed";
        app.exit(2);
      } else
        qInfo() << "CAPTURE" << image.size();
    }
  });
  window.resizeContent();
  if (cli.isSet("fullscreen"))
    window.showFullScreen();
  else
    window.show();
  if (cli.isSet("resize-test")) {
    QTimer::singleShot(2000, &app, [&] { window.resize(800, 600); });
    QTimer::singleShot(4000, &app, [&] { window.resize(width, height); });
  }
  if (seconds)
    QTimer::singleShot(seconds * 1000, &app, &QCoreApplication::quit);
  const int result = app.exec();
  timer.stop();
  const auto summary = snapshot();
  qInfo().noquote() << "SUMMARY"
                    << QJsonDocument(summary).toJson(QJsonDocument::Compact);
  if (cli.isSet("metrics")) {
    QFile f(cli.value("metrics"));
    if (!f.open(QIODevice::WriteOnly) ||
        f.write(QJsonDocument(summary).toJson()) < 0)
      return 2;
  }
  window.resizeContent = {};
  return result;
}
