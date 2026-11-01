#include "atlas.h"
#include "fixture.h"
GlyphAtlas::GlyphAtlas(GlyphMaterial &material, qreal devicePixelRatio)
    : dpr(devicePixelRatio) {
  const QString chars = repertoire();
  const QSize glyphSize = material.glyph('M').size();
  constexpr int columns = 16, guard = 1;
  const int slotWidth = glyphSize.width() + 2 * guard,
            slotHeight = glyphSize.height() + 2 * guard;
  image = QImage(columns * slotWidth,
                 ((chars.size() + columns - 1) / columns) * slotHeight,
                 QImage::Format_ARGB32_Premultiplied);
  image.fill(Qt::transparent);
  // Copy physical premultiplied texels exactly; don't let QPainter rescale DPR.
  for (int i = 0; i < chars.size(); ++i) {
    const auto &glyph = material.glyph(chars[i]);
    const QRect rect((i % columns) * slotWidth + guard,
                     (i / columns) * slotHeight + guard, glyph.width(),
                     glyph.height());
    entries.insert(chars[i].unicode(), rect);
    for (int y = 0; y < glyph.height(); ++y)
      memcpy(image.scanLine(rect.y() + y) + rect.x() * 4,
             glyph.constScanLine(y), glyph.width() * 4);
  }
}
