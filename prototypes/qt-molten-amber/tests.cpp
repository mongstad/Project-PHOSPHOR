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
  // Bounded refinement experiment: fixed 16 pt, same coverage and halo.
  GlyphMaterial original16(16, 1, true, 1, 1);
  GlyphMaterial refined16(16, 1, true, 1, 1, true);
  GlyphMaterial mask16(16, 1, false, 0, 0);
  require(original16.cell() == refined16.cell() &&
              original16.line() == refined16.line(),
          "refinement changes geometry");
  int materialChanges = 0, exteriorPixels = 0;
  for (QChar c : QString::fromUtf8("PHOSphor16.,:!|█┼ø")) {
    const auto before = original16.glyph(c), after = refined16.glyph(c),
               mask = mask16.glyph(c);
    require(before.size() == after.size(), "refinement changes sprite size");
    for (int y = 0; y < before.height(); ++y)
      for (int x = 0; x < before.width(); ++x) {
        require(qAlpha(before.pixel(x, y)) == qAlpha(after.pixel(x, y)),
                "refinement changes coverage or glow alpha");
        // Quantized zero-alpha pixels directly at a stroke can still contain
        // subpixel material after filtering. Compare halo away from that
        // fringe.
        bool exterior = true;
        for (int dy = -1; dy <= 1; ++dy)
          for (int dx = -1; dx <= 1; ++dx)
            if (x + dx >= 0 && y + dy >= 0 && x + dx < mask.width() &&
                y + dy < mask.height() &&
                qAlpha(mask.pixel(x + dx, y + dy)) != 0)
              exterior = false;
        if (exterior) {
          require(before.pixel(x, y) == after.pixel(x, y),
                  "refinement changes exterior glow");
          ++exteriorPixels;
        } else if (before.pixel(x, y) != after.pixel(x, y))
          ++materialChanges;
      }
  }
  require(materialChanges > 0 && exteriorPixels > 0,
          "refinement comparison did not exercise material and exterior");
  qInfo() << "PASS 16 pt refinement: identical geometry, coverage and exterior "
             "glow; changed glyph interiors";
}
