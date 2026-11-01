#include "renderer.h"
#include <QCommandLineParser>
#include <QDebug>
#include <QElapsedTimer>
#include <QGuiApplication>
#include <QQuickPaintedItem>
#include <QQuickWindow>
#include <QScreen>
#include <QTimer>
#include <cmath>
#include <memory>

class NativeWindow : public QQuickWindow {
public:
  bool event(QEvent *event) override {
    const bool result = QQuickWindow::event(event);
    if (event->type() == QEvent::DevicePixelRatioChange) {
      for (auto *item : contentItem()->childItems())
        if (auto *painted = dynamic_cast<QQuickPaintedItem *>(item)) {
          painted->setTextureSize(size() * devicePixelRatio());
          painted->update();
        }
    }
    return result;
  }
};

class Sample : public QQuickPaintedItem {
public:
  int points = 14, revision = 0;
  bool effects = true, refine16 = false;
  double edge = 1, glow = 1;
  std::unique_ptr<GlyphMaterial> material;
  qreal lastDpr = 0;
  qint64 paints = 0, paintNs = 0;
  explicit Sample(QQuickItem *parent) : QQuickPaintedItem(parent) {
    setOpaquePainting(true);
  }
  void paint(QPainter *p) override {
    QElapsedTimer timer;
    timer.start();
    const qreal dpr = window()->devicePixelRatio();
    if (!material || lastDpr != dpr) {
      material = std::make_unique<GlyphMaterial>(points, dpr, effects, edge,
                                                 glow, refine16);
      lastDpr = dpr;
      qInfo() << "glyph cache rebuild; DPR" << dpr << "font" << material->font()
              << "cell" << material->cell() << "line" << material->line();
    }
    p->fillRect(boundingRect(), QColor(7, 5, 3));
    const QStringList sample = {
        "PHOSPHOR  /  MOLTEN AMBER",
        "",
        "astra@phosphor:~$ ./visual-sample --no-shell",
        "/home/astra/research/PHOSPHOR-002/render.cpp",
        "",
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ  0123456789",
        "abcdefghijklmnopqrstuvwxyz  0O 1Il | ! : ; . ,",
        "!@#$%^&*()_+-=[]{};:'\"<>,.?/\\|~`",
        QString::fromUtf8("┌──────────────────────────────┬───────────────┐"),
        QString::fromUtf8("│ GLYPH MATERIAL               │ STATIC DEMO   │"),
        QString::fromUtf8("├──────────────────────────────┼───────────────┤"),
        QString::fromUtf8("│ Mask → bevel → light → bloom │ 12 / 14 / 16  │"),
        QString::fromUtf8("└──────────────────────────────┴───────────────┘"),
        QString::fromUtf8("Unicode: α β γ Δ Ω λ π ± × ÷ ≤ ≥ ← ↑ → ↓ é ø å"),
        QString::fromUtf8("Solid glyphs: █ ▓ ▒ ░ ■ ◆   Thin glyphs: .,:;!|i"),
        "",
        "No command execution. Synthetic text updates only.",
        ""};
    const int cols = std::max(1, int((width() - 48) / material->cell()));
    const int rows = std::max(0, int((height() - 40) / material->line()));
    for (int row = 0; row < rows; ++row) {
      QString line;
      if (row < sample.size())
        line = sample[row];
      else {
        const int n = row + revision;
        line = QString("[%1] material.sample  x=0.125 y=-3.750  "
                       "/src/glyph_%2.cpp  {a+=1;}  OK ")
                   .arg(n, 6, 10, QChar('0'))
                   .arg(n % 97, 2, 10, QChar('0'));
        while (line.size() < cols)
          line += line;
      }
      material->text(*p, QPointF(24, 20 + row * material->line()),
                     line.left(cols));
    }
    ++paints;
    paintNs += timer.nsecsElapsed();
  }
};
int main(int argc, char **argv) {
  QGuiApplication app(argc, argv);
  for (auto *screen : app.screens())
    qInfo() << "AVAILABLE SCREEN" << screen->name() << screen->geometry()
            << screen->devicePixelRatio();
  QGuiApplication::setApplicationName("phosphor-002");
  QGuiApplication::setDesktopFileName("org.phosphor.Spike002");
  QCommandLineParser cli;
  cli.setApplicationDescription(
      "Isolated glyph-material sample; no shell or terminal engine");
  cli.addHelpOption();
  cli.addOptions(
      {{"baseline", "Disable all material effects"},
       {"refine-16", "Opt-in 16 pt material refinement (requires --points 16)"},
       {"points", "Font size: 12..16 pt", "points", "14"},
       {"edge", "Edge intensity 0..2", "edge", "1"},
       {"glow", "Glow intensity 0..2", "glow", "1"},
       {"width", "Client width", "width", "1280"},
       {"height", "Client height", "height", "900"},
       {"seconds", "Quit after seconds (0 stays open)", "seconds", "0"},
       {"update-ms", "Synthetic bulk update interval (0 idle)", "ms", "0"},
       {"capture", "Save unedited client framebuffer PNG after warmup", "path"},
       {"screen", "Screen name", "name"},
       {"fullscreen", "Fullscreen on selected screen"},
       {"resize-test", "Exercise two native resizes before capture"}});
  cli.process(app);
  auto integer = [&](const char *name, int lo, int hi) {
    bool ok;
    int n = cli.value(name).toInt(&ok);
    if (!ok || n < lo || n > hi)
      qFatal("Invalid %s", name);
    return n;
  };
  auto intensity = [&](const char *name) {
    bool ok;
    double n = cli.value(name).toDouble(&ok);
    if (!ok || !std::isfinite(n) || n < 0 || n > 2)
      qFatal("Invalid %s", name);
    return n;
  };
  NativeWindow window;
  window.setColor(QColor(7, 5, 3));
  window.setTitle(QString("PHOSPHOR-002 — %1 — %2 pt — visual only")
                      .arg(cli.isSet("baseline") ? "BASELINE" : "MOLTEN AMBER",
                           cli.value("points")));
  window.resize(integer("width", 320, 4096), integer("height", 240, 2160));
  if (cli.isSet("screen")) {
    bool found = false;
    for (auto *s : app.screens())
      if (s->name() == cli.value("screen")) {
        window.setScreen(s);
        window.setPosition(s->geometry().topLeft());
        found = true;
      }
    if (!found)
      qFatal("Requested screen not available");
  }
  Sample sample(window.contentItem());
  sample.points = integer("points", 12, 16);
  sample.refine16 = cli.isSet("refine-16");
  if (sample.refine16 && (sample.points != 16 || cli.isSet("baseline"))) {
    qCritical() << "--refine-16 requires --points 16 and material enabled";
    return 2;
  }
  qInfo() << "MATERIAL VARIANT"
          << (sample.refine16 ? "refined-16" : "original");
  sample.effects = !cli.isSet("baseline");
  sample.edge = intensity("edge");
  sample.glow = intensity("glow");
  auto resize = [&] {
    sample.setSize(window.size());
    sample.setTextureSize(window.size() * window.devicePixelRatio());
    sample.update();
  };
  QObject::connect(&window, &QWindow::widthChanged, &app, resize);
  QObject::connect(&window, &QWindow::heightChanged, &app, resize);
  QObject::connect(&window, &QWindow::screenChanged, &app, [&] {
    sample.material.reset();
    resize();
  });
  resize();
  QTimer updates;
  QObject::connect(&updates, &QTimer::timeout, &app, [&] {
    ++sample.revision;
    sample.update();
  });
  int interval = integer("update-ms", 0, 60000);
  if (interval)
    updates.start(interval);
  qint64 frames = 0;
  QObject::connect(&window, &QQuickWindow::frameSwapped, &app,
                   [&] { ++frames; });
  if (cli.isSet("fullscreen"))
    window.showFullScreen();
  else
    window.show();
  if (cli.isSet("resize-test")) {
    QTimer::singleShot(2000, &app, [&] { window.resize(800, 600); });
    QTimer::singleShot(4000, &app, [&] { window.resize(1280, 900); });
  }
  QTimer::singleShot(7000, &app, [&] {
    qInfo() << "PRESENTATION" << window.screen()->name() << window.size()
            << "DPR" << window.devicePixelRatio() << "screen"
            << window.screen()->geometry() << "logical DPI"
            << window.screen()->logicalDotsPerInch();
    if (cli.isSet("capture")) {
      auto image = window.grabWindow();
      if (image.isNull() || !image.save(cli.value("capture"))) {
        qCritical() << "Capture failed";
        app.exit(2);
      } else
        qInfo() << "CAPTURE" << image.size();
    }
  });
  int seconds = integer("seconds", 0, 3600);
  if (seconds)
    QTimer::singleShot(seconds * 1000, &app, &QCoreApplication::quit);
  int result = app.exec();
  qInfo() << "SUMMARY paints" << sample.paints << "paint elapsed ms"
          << sample.paintNs / 1e6 << "frameSwapped" << frames
          << "synthetic updates" << sample.revision;
  return result;
}
