#include "renderer.h"
#include <QFontMetricsF>
#include <algorithm>
#include <cmath>
#include <vector>

GlyphMaterial::GlyphMaterial(int points, qreal dpr, bool effects, double edge,
                             double glow, bool refine16)
    : font_("Noto Sans Mono", points), dpr_(dpr), effects_(effects),
      refine16_(refine16 && points == 16), edge_(edge), glow_(glow) {
  font_.setWeight(QFont::DemiBold);
  font_.setStyleHint(QFont::Monospace);
  font_.setHintingPreference(QFont::PreferNoHinting);
  const QFontMetricsF fm(font_);
  cell_ = fm.horizontalAdvance('M');
  line_ = std::ceil(fm.height() + 4);
  ascent_ = fm.ascent();
}

const QImage &GlyphMaterial::glyph(QChar ch) {
  if (cache_.contains(ch.unicode()))
    return cache_[ch.unicode()];
  // Fixed cell positions in both modes; 3x raster samples per physical pixel.
  constexpr int ss = 3;
  const qreal scale = dpr_ * ss;
  const int w = std::ceil((cell_ + 2 * padding_) * dpr_);
  const int h = std::ceil((line_ + 2 * padding_) * dpr_);
  QImage mask(w * ss, h * ss, QImage::Format_ARGB32_Premultiplied);
  mask.fill(Qt::transparent);
  {
    QPainter p(&mask);
    p.scale(scale, scale);
    p.setFont(font_);
    p.setPen(Qt::white);
    p.setRenderHint(QPainter::TextAntialiasing);
    p.drawText(QPointF(padding_, padding_ + ascent_), QString(ch));
  }
  const int mw = mask.width(), mh = mask.height();
  std::vector<float> distance(mw * mh);
  const int radius = std::ceil(3 * scale);
  auto inside = [&](int x, int y) {
    return x >= 0 && y >= 0 && x < mw && y < mh &&
           qAlpha(mask.pixel(x, y)) >= 128;
  };
  // Truncated signed Euclidean distance in logical pixels, computed only once
  // per character/DPR/material. No distance is inferred from composited colors.
  if (effects_)
    for (int y = 0; y < mh; ++y)
      for (int x = 0; x < mw; ++x) {
        bool in = inside(x, y);
        int best = radius * radius;
        for (int dy = -radius; dy <= radius; ++dy)
          for (int dx = -radius; dx <= radius; ++dx) {
            const int d = dx * dx + dy * dy;
            if (d < best && inside(x + dx, y + dy) != in)
              best = d;
          }
        distance[y * mw + x] =
            (in ? 1.f : -1.f) * (std::sqrt(float(best)) - .5f) / scale;
      }
  auto dist = [&](int x, int y) {
    return distance[std::clamp(y, 0, mh - 1) * mw + std::clamp(x, 0, mw - 1)];
  };
  QImage material(mask.size(), QImage::Format_ARGB32_Premultiplied);
  material.fill(Qt::transparent);
  const auto square = [](double value) { return value * value; };
  for (int y = 0; y < mh; ++y)
    for (int x = 0; x < mw; ++x) {
      const double coverage = qAlpha(mask.pixel(x, y)) / 255.;
      double r = 255, g = 179, b = 66, a = coverage;
      if (effects_) {
        const double d = dist(x, y);
        const double nx = -(dist(x + 1, y) - dist(x - 1, y)) * scale * .5;
        const double ny = -(dist(x, y + 1) - dist(x, y - 1)) * scale * .5;
        const double bevel = std::exp(-std::max(0., d) / .65);
        const double light =
            std::clamp((-nx * .55 - ny * .83) * bevel, -1., 1.);
        const double v = (y / scale - padding_) / std::max(1., ascent_);
        // Warm metal environment reflection: narrow pale band, dark trough,
        // copper return. Shape-derived normals modulate that reflection.
        const double band = std::exp(-square((v - .32) / .10));
        const double lower = std::exp(-square((v - .78) / .18));
        const double shade =
            std::clamp(.35 + .43 * band + .20 * lower + .55 * light, .12, 1.);
        const double rim = edge_ * std::exp(-std::abs(d) / .24) *
                           std::max(0., .35 + .65 * light);
        r = std::clamp(100 + 155 * shade + 125 * rim, 0., 255.);
        g = std::clamp(24 + 171 * shade + 190 * rim, 0., 255.);
        b = std::clamp(4 + 64 * shade + 160 * rim, 0., 255.);
        if (refine16_) {
          // Narrow bevel: opposed normals give a pale upper-left facet and
          // copper lower-right facet. The environment reflection bends with
          // those normals rather than applying one flat color to the glyph.
          const double facing = std::clamp(-nx * .55 - ny * .83, -1., 1.);
          const double facet = std::exp(-std::max(0., d) / .48);
          const double reflectedV = v + .10 * ny * facet;
          const double reflection =
              std::exp(-square((reflectedV - .35) / .075));
          const double returnLight =
              std::exp(-square((reflectedV - .80) / .14));
          const double lit = std::max(0., facing) * facet;
          const double shadow = std::max(0., -facing) * facet;
          const double contour = edge_ * std::exp(-std::abs(d) / .32) *
                                 (.18 + .82 * std::max(0., facing));
          r = std::clamp(175 + 65 * reflection + 32 * returnLight + 70 * lit -
                             65 * shadow + 100 * contour,
                         0., 255.);
          g = std::clamp(82 + 135 * reflection + 50 * returnLight + 115 * lit -
                             48 * shadow + 150 * contour,
                         0., 255.);
          b = std::clamp(15 + 150 * reflection + 18 * returnLight + 95 * lit -
                             10 * shadow + 155 * contour,
                         0., 255.);
        }
        // Separate glow outside the actual mask; zero glow leaves all
        // pixels outside glyph coverage exactly transparent.
        const double halo =
            glow_ * .22 * std::exp(-square(std::max(0., -d) / 1.35));
        a = coverage + (1 - coverage) * halo;
        if (a > 0) {
          r = (coverage * r + (1 - coverage) * halo * 255) / a;
          g = (coverage * g + (1 - coverage) * halo * 113) / a;
          b = (coverage * b + (1 - coverage) * halo * 10) / a;
        }
      }
      material.setPixel(x, y,
                        qPremultiply(qRgba(int(r), int(g), int(b),
                                           int(std::clamp(a, 0., 1.) * 255))));
    }
  // A space must not acquire the truncated distance field's faint tail.
  if (ch.isSpace())
    material.fill(Qt::transparent);
  QImage result =
      material.scaled(w, h, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
  result.setDevicePixelRatio(dpr_);
  cache_.insert(ch.unicode(), result);
  return cache_[ch.unicode()];
}
void GlyphMaterial::text(QPainter &p, QPointF origin, const QString &text) {
  for (int i = 0; i < text.size(); ++i)
    p.drawImage(origin + QPointF(i * cell_ - padding_, -padding_),
                glyph(text[i]));
}
