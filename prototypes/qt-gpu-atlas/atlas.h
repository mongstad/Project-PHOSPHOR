#pragma once
#include "renderer.h"
#include <QRect>
struct GlyphAtlas {
  QImage image;
  QMap<ushort, QRect> entries;
  qreal dpr;
  explicit GlyphAtlas(GlyphMaterial &material, qreal devicePixelRatio);
};
