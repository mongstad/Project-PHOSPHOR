#include "renderer.h"
#include <QDebug>
#include <QFontMetricsF>
#include <QGuiApplication>
#include <QSet>
#include <cmath>
int main(int argc, char **argv) {
  QGuiApplication app(argc, argv);
  auto require = [](bool yes, const char *message) {
    if (!yes)
      qFatal("%s", message);
  };
  for (int pt : {12, 14, 16})
    for (double dpr : {1., 1.5, 2.}) {
      GlyphMaterial base(pt, dpr, false, 0, 0), metal(pt, dpr, true, 1, 0),
          glow(pt, dpr, true, 1, 1);
      require(base.cell() == metal.cell() && base.line() == metal.line(),
              "geometry differs");
      for (QChar c : QString::fromUtf8("Mgi.,:!0█┼")) {
        const auto a = base.glyph(c), b = metal.glyph(c), g = glow.glyph(c);
        require(a.size() == b.size() && a.size() == g.size(),
                "sprite bounds differ");
        require(qAlpha(g.pixel(0, 0)) == 0, "glow reaches sprite corner");
        int changed = 0, halo = 0, opaque = 0;
        for (int y = 0; y < a.height(); ++y)
          for (int x = 0; x < a.width(); ++x) {
            require(qAlpha(a.pixel(x, y)) == qAlpha(b.pixel(x, y)),
                    "material changes coverage without glow");
            if (a.pixel(x, y) != b.pixel(x, y))
              ++changed;
            if (qAlpha(g.pixel(x, y)) > qAlpha(b.pixel(x, y)))
              ++halo;
            opaque += qAlpha(a.pixel(x, y));
          }
        require(opaque > 0 && changed > 0 && halo > 0,
                "missing glyph, material or independent glow");
      }
      QSet<QRgb> interiorColors;
      const auto block = metal.glyph(QChar(0x2588));
      GlyphMaterial noEdge(pt, dpr, true, 0, 0);
      const auto unlitEdge = noEdge.glyph(QChar(0x2588));
      int edgeChanges = 0;
      for (int y = 0; y < block.height(); ++y)
        for (int x = 0; x < block.width(); ++x) {
          if (qAlpha(block.pixel(x, y)) == 255)
            interiorColors.insert(block.pixel(x, y));
          if (block.pixel(x, y) != unlitEdge.pixel(x, y))
            ++edgeChanges;
        }
      require(interiorColors.size() > 3,
              "metal interior is only a flat palette");
      require(edgeChanges > 0, "edge intensity has no independent effect");
      auto space = glow.glyph(' ');
      for (int y = 0; y < space.height(); ++y)
        for (int x = 0; x < space.width(); ++x)
          require(space.pixel(x, y) == 0, "space glows");
      QFontMetricsF fm(base.font());
      require(std::abs(fm.horizontalAdvance('i') - base.cell()) < .01,
              "font is not monospaced");
      qInfo() << "PASS" << pt << "pt DPR" << dpr;
    }
}
