#pragma once
#include "atlas.h"
#include <QQuickItem>
#include <QQuickPaintedItem>
#include <atomic>
#include <memory>
struct Metrics {
  std::atomic<qint64> cacheNs{0}, workNs{0}, calls{0}, builds{0},
      textureBytes{0}, geometryBytes{0};
};
class CpuView : public QQuickPaintedItem {
public:
  CpuView(QQuickItem *parent, Metrics &metrics, bool effects);
  int revision = 0;
  void paint(QPainter *p) override;

private:
  Metrics &metrics_;
  bool effects_;
  qreal dpr_ = 0;
  std::unique_ptr<GlyphMaterial> material_;
};
class GpuView : public QQuickItem {
public:
  GpuView(QQuickItem *parent, Metrics &metrics, bool effects);
  int revision = 0;

protected:
  QSGNode *updatePaintNode(QSGNode *old, UpdatePaintNodeData *) override;

private:
  Metrics &metrics_;
  bool effects_;
};
