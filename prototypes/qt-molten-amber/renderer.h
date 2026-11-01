#pragma once
#include <QFont>
#include <QImage>
#include <QMap>
#include <QPainter>

// Disposable, glyph-only material cache. No terminal state or frame recoloring.
class GlyphMaterial {
public:
  GlyphMaterial(int points, qreal dpr, bool effects, double edge, double glow);
  const QImage &glyph(QChar character);
  void text(QPainter &painter, QPointF origin, const QString &text);
  qreal cell() const { return cell_; }
  qreal line() const { return line_; }
  qreal padding() const { return padding_; }
  QFont font() const { return font_; }

private:
  QFont font_;
  qreal dpr_, cell_, line_, ascent_, padding_ = 5;
  bool effects_;
  double edge_, glow_;
  QMap<ushort, QImage> cache_;
};
