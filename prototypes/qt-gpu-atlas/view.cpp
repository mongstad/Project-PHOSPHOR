#include "view.h"
#include "fixture.h"
#include <QElapsedTimer>
#include <QQuickWindow>
#include <QSGGeometryNode>
#include <QSGTextureMaterial>
#include <cmath>

CpuView::CpuView(QQuickItem *parent, Metrics &metrics, bool effects)
    : QQuickPaintedItem(parent), metrics_(metrics), effects_(effects) {
  setOpaquePainting(true);
}
void CpuView::paint(QPainter *p) {
  const qreal dpr = window()->devicePixelRatio();
  if (!material_ || dpr_ != dpr) {
    QElapsedTimer cold;
    cold.start();
    material_ = std::make_unique<GlyphMaterial>(16, dpr, effects_, 1, 1, true);
    dpr_ = dpr;
    for (QChar ch : repertoire())
      material_->glyph(ch);
    metrics_.cacheNs += cold.nsecsElapsed();
    ++metrics_.builds;
  }
  QElapsedTimer warm;
  warm.start();
  p->fillRect(boundingRect(), QColor(7, 5, 3));
  const auto lines =
      fixture(size(), material_->cell(), material_->line(), revision);
  for (int row = 0; row < lines.size(); ++row)
    material_->text(*p, QPointF(24, 20 + row * material_->line()), lines[row]);
  metrics_.workNs += warm.nsecsElapsed();
  ++metrics_.calls;
  metrics_.textureBytes +=
      qint64(std::ceil(width() * dpr)) * qint64(std::ceil(height() * dpr)) * 4;
}
namespace {
// Keep each QSG geometry below the renderer's 16-bit batch-index range.
constexpr int glyphsPerBatch = 4096;
static_assert(glyphsPerBatch * 6 < 65536);
class BatchNode : public QSGGeometryNode {
public:
  QSGTextureMaterial material;
  QSGGeometry geometry{QSGGeometry::defaultAttributes_TexturedPoint2D(), 0};
  explicit BatchNode(QSGTexture *texture) {
    material.setTexture(texture);
    material.setFiltering(QSGTexture::Nearest);
    geometry.setDrawingMode(QSGGeometry::DrawTriangles);
    geometry.setVertexDataPattern(QSGGeometry::StreamPattern);
    setGeometry(&geometry);
    setMaterial(&material);
  }
};
class AtlasNode : public QSGNode {
public:
  qreal dpr;
  GlyphMaterial glyphs;
  GlyphAtlas atlas;
  std::unique_ptr<QSGTexture> texture;
  QList<BatchNode *> batches;
  AtlasNode(QQuickWindow *window, bool effects)
      : dpr(window->devicePixelRatio()), glyphs(16, dpr, effects, 1, 1, true),
        atlas(glyphs, dpr) {
    texture.reset(window->createTextureFromImage(atlas.image));
    if (!texture)
      qFatal("GPU atlas texture allocation failed");
  }
};
} // namespace
GpuView::GpuView(QQuickItem *parent, Metrics &metrics, bool effects)
    : QQuickItem(parent), metrics_(metrics), effects_(effects) {
  setFlag(ItemHasContents, true);
}
QSGNode *GpuView::updatePaintNode(QSGNode *old, UpdatePaintNodeData *) {
  auto *node = static_cast<AtlasNode *>(old);
  if (!node || node->dpr != window()->devicePixelRatio()) {
    delete node;
    QElapsedTimer cold;
    cold.start();
    node = new AtlasNode(window(), effects_);
    metrics_.cacheNs += cold.nsecsElapsed();
    ++metrics_.builds;
    metrics_.textureBytes += node->atlas.image.sizeInBytes();
  }
  // Called on the render thread while the GUI thread is blocked: revision and
  // size are coherent here. Node, texture and geometry all live on that thread.
  QElapsedTimer warm;
  warm.start();
  const auto lines =
      fixture(size(), node->glyphs.cell(), node->glyphs.line(), revision);
  int count = 0;
  for (const auto &line : lines)
    for (QChar ch : line)
      if (!ch.isSpace())
        ++count;
  const int required = (count + glyphsPerBatch - 1) / glyphsPerBatch;
  while (node->batches.size() < required) {
    auto *batch = new BatchNode(node->texture.get());
    node->appendChildNode(batch);
    node->batches.append(batch);
  }
  while (node->batches.size() > required) {
    auto *batch = node->batches.takeLast();
    node->removeChildNode(batch);
    delete batch;
  }
  for (int i = 0; i < required; ++i)
    node->batches[i]->geometry.allocate(
        std::min(glyphsPerBatch, count - i * glyphsPerBatch) * 6);
  int emitted = 0;
  const qreal dpr = node->dpr;
  for (int row = 0; row < lines.size(); ++row)
    for (int col = 0; col < lines[row].size(); ++col) {
      const QChar ch = lines[row][col];
      if (ch.isSpace())
        continue;
      const auto rect = node->atlas.entries.value(ch.unicode());
      if (rect.isEmpty())
        qFatal("Fixture character missing from glyph atlas");
      const float x =
          std::round((24 + col * node->glyphs.cell() - node->glyphs.padding()) *
                     dpr) /
          dpr;
      const float y =
          std::round((20 + row * node->glyphs.line() - node->glyphs.padding()) *
                     dpr) /
          dpr;
      const float x2 = x + rect.width() / dpr, y2 = y + rect.height() / dpr;
      const float u = float(rect.x()) / node->atlas.image.width(),
                  u2 = float(rect.x() + rect.width()) /
                       node->atlas.image.width();
      const float t = float(rect.y()) / node->atlas.image.height(),
                  t2 = float(rect.y() + rect.height()) /
                       node->atlas.image.height();
      auto *v = node->batches[emitted / glyphsPerBatch]
                    ->geometry.vertexDataAsTexturedPoint2D() +
                (emitted % glyphsPerBatch) * 6;
      v[0].set(x, y, u, t);
      v[1].set(x2, y, u2, t);
      v[2].set(x, y2, u, t2);
      v[3].set(x, y2, u, t2);
      v[4].set(x2, y, u2, t);
      v[5].set(x2, y2, u2, t2);
      ++emitted;
    }
  for (auto *batch : node->batches) {
    batch->geometry.markVertexDataDirty();
    batch->markDirty(QSGNode::DirtyGeometry);
  }
  metrics_.workNs += warm.nsecsElapsed();
  ++metrics_.calls;
  metrics_.geometryBytes += count * 6 * sizeof(QSGGeometry::TexturedPoint2D);
  return node;
}
