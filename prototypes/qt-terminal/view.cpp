#include "view.h"
#include "fixture.h"
#include "renderer.h"
#include <QElapsedTimer>
#include <QQuickWindow>
#include <QSGGeometryNode>
#include <QSGMaterialShader>
#include <QSGTexture>
#include <QSGVertexColorMaterial>
#include <QTimer>
#include <cmath>
#include <map>
#include <memory>
#include <vector>
namespace {
struct Vertex {
  float x, y, u, v, r, g, b, explicitColor;
};
const QSGGeometry::Attribute attrs[] = {
    QSGGeometry::Attribute::create(0, 2, QSGGeometry::FloatType, true),
    QSGGeometry::Attribute::create(1, 2, QSGGeometry::FloatType),
    QSGGeometry::Attribute::create(2, 4, QSGGeometry::FloatType)};
const QSGGeometry::AttributeSet attributes = {3, sizeof(Vertex), attrs};
class Material;
class Shader : public QSGMaterialShader {
public:
  Shader() {
    setShaderFileName(VertexStage, ":/shaders/glyph.vert.qsb");
    setShaderFileName(FragmentStage, ":/shaders/glyph.frag.qsb");
  }
  bool updateUniformData(RenderState &s, QSGMaterial *,
                         QSGMaterial *) override {
    auto *b = s.uniformData();
    bool changed = false;
    if (s.isMatrixDirty()) {
      memcpy(b->data(), s.combinedMatrix().constData(), 64);
      changed = true;
    }
    if (s.isOpacityDirty()) {
      float opacity = s.opacity();
      memcpy(b->data() + 64, &opacity, 4);
      changed = true;
    }
    return changed;
  }
  void updateSampledImage(RenderState &, int, QSGTexture **, QSGMaterial *,
                          QSGMaterial *) override;
};
class Material : public QSGMaterial {
public:
  QSGTexture *texture;
  explicit Material(QSGTexture *t) : texture(t) { setFlag(Blending, true); }
  QSGMaterialType *type() const override {
    static QSGMaterialType t;
    return &t;
  }
  QSGMaterialShader *
  createShader(QSGRendererInterface::RenderMode) const override {
    return new Shader;
  }
  int compare(const QSGMaterial *o) const override {
    const auto a = texture->comparisonKey(),
               b = static_cast<const Material *>(o)->texture->comparisonKey();
    return a < b ? -1 : a > b ? 1 : 0;
  }
};
void Shader::updateSampledImage(RenderState &state, int binding, QSGTexture **t,
                                QSGMaterial *m, QSGMaterial *) {
  if (binding == 1) {
    *t = static_cast<Material *>(m)->texture;
    (*t)->commitTextureOperations(state.rhi(), state.resourceUpdateBatch());
  }
}
struct Entry {
  int page;
  QRect rect;
};
struct Page {
  QImage image;
  std::unique_ptr<QSGTexture> texture;
  bool dirty = true;
};
class Root : public QSGNode {
public:
  qreal dpr;
  GlyphMaterial material;
  std::map<ushort, Entry> entries;
  std::vector<Page> pages;
  explicit Root(qreal scale)
      : dpr(scale), material(16, scale, true, 1, 1, true) {}
  ~Root() override { clear(); }
  void clear() {
    while (firstChild()) {
      auto *n = firstChild();
      removeChildNode(n);
      delete n;
    }
  }
  void add(QChar ch) {
    if (entries.contains(ch.unicode()) || entries.size() >= 512)
      return;
    const auto &glyph = material.glyph(ch);
    int index = int(entries.size());
    int page = index / 128, slot = index % 128;
    const int w = glyph.width() + 2, h = glyph.height() + 2;
    if (page == int(pages.size())) {
      Page p;
      p.image = QImage(w * 16, h * 8, QImage::Format_ARGB32_Premultiplied);
      p.image.fill(Qt::transparent);
      pages.push_back(std::move(p));
    }
    QRect rect((slot % 16) * w + 1, (slot / 16) * h + 1, glyph.width(),
               glyph.height());
    for (int y = 0; y < glyph.height(); ++y)
      memcpy(pages[page].image.scanLine(rect.y() + y) + rect.x() * 4,
             glyph.constScanLine(y), glyph.width() * 4);
    pages[page].dirty = true;
    entries.emplace(ch.unicode(), Entry{page, rect});
  }
};
void solid(QSGNode *root, const std::vector<std::pair<QRectF, QColor>> &rects) {
  for (size_t start = 0; start < rects.size(); start += 4096) {
    const size_t count = std::min(size_t(4096), rects.size() - start);
    auto *n = new QSGGeometryNode;
    auto *g = new QSGGeometry(QSGGeometry::defaultAttributes_ColoredPoint2D(),
                              int(count * 6));
    g->setDrawingMode(QSGGeometry::DrawTriangles);
    auto *v = g->vertexDataAsColoredPoint2D();
    for (size_t i = 0; i < count; ++i) {
      const auto &[r, c] = rects[start + i];
      const QPointF p[] = {r.topLeft(),    r.topRight(), r.bottomLeft(),
                           r.bottomLeft(), r.topRight(), r.bottomRight()};
      for (auto point : p)
        (v++)->set(point.x(), point.y(), c.red(), c.green(), c.blue(), 255);
    }
    n->setGeometry(g);
    n->setFlag(QSGNode::OwnsGeometry);
    n->setMaterial(new QSGVertexColorMaterial);
    n->setFlag(QSGNode::OwnsMaterial);
    root->appendChildNode(n);
  }
}
} // namespace
TerminalView::TerminalView(QQuickItem *p, Terminal &t, RenderMetrics &m)
    : QQuickItem(p), terminal_(t), metrics_(m) {
  setFlag(ItemHasContents);
  setFlag(ItemIsFocusScope);
  setClip(true);
  setAcceptedMouseButtons(Qt::AllButtons);
  setAcceptHoverEvents(true);
  setFocus(true);
  GlyphMaterial metrics(16, 1, true, 1, 1, true);
  cell_ = metrics.cell();
  line_ = metrics.line();
}
// The existing 24 px right gutter holds the control without taking a column.
QRectF TerminalView::scrollTrack() const {
  return QRectF(width() - 14, 20, 10, std::max(qreal(0), height() - 40));
}
QRectF TerminalView::scrollThumb() const {
  const auto track = scrollTrack();
  const int history = terminal_.alternateScreen() ? 0 : terminal_.historySize();
  const qreal size = std::min(track.height(), std::max(qreal(24),
      track.height() * terminal_.rows() / (terminal_.rows() + history)));
  const qreal position = history ? qreal(history - terminal_.offset()) / history : 1;
  return QRectF(track.x(), track.y() + position * (track.height() - size),
                track.width(), size);
}
void TerminalView::mousePressEvent(QMouseEvent *e) {
  if (e->button() != Qt::LeftButton || !scrollTrack().contains(e->position())) {
    e->ignore();
    return;
  }
  pressed_ = true;
  forceActiveFocus(Qt::MouseFocusReason);
  const auto thumb = scrollThumb();
  if (!terminal_.alternateScreen() && terminal_.historySize()) {
    if (thumb.contains(e->position())) {
      dragging_ = true;
      grabFraction_ = (e->position().y() - thumb.y()) / thumb.height();
    } else {
      terminal_.scroll((e->position().y() < thumb.y() ? 1 : -1) *
                       std::max(1, terminal_.rows() - 1));
    }
  }
  update();
  e->accept();
}
void TerminalView::mouseMoveEvent(QMouseEvent *e) {
  if (!dragging_) { e->ignore(); return; }
  const auto track = scrollTrack(), thumb = scrollThumb();
  const qreal travel = track.height() - thumb.height();
  if (travel > 0) {
    const qreal position = std::clamp(
        (e->position().y() - track.y() - grabFraction_ * thumb.height()) / travel,
        qreal(0), qreal(1));
    terminal_.setOffset(qRound((1 - position) * terminal_.historySize()));
  }
  update();
  e->accept();
}
void TerminalView::mouseReleaseEvent(QMouseEvent *e) {
  dragging_ = pressed_ = false;
  update();
  e->accept();
}
void TerminalView::mouseUngrabEvent() {
  dragging_ = pressed_ = false;
  update();
}
void TerminalView::hoverMoveEvent(QHoverEvent *e) {
  const bool oldTrack = hovering_ && scrollTrack().contains(hoverPosition_);
  const bool oldThumb = hovering_ && scrollThumb().contains(hoverPosition_);
  hovering_ = true;
  hoverPosition_ = e->position();
  if (oldTrack != scrollTrack().contains(hoverPosition_) ||
      oldThumb != scrollThumb().contains(hoverPosition_))
    update();
}
void TerminalView::hoverLeaveEvent(QHoverEvent *) {
  hovering_ = false;
  update();
}
void TerminalView::wheelEvent(QWheelEvent *e) {
  terminal_.scroll(e->angleDelta().y() / 120 * 3);
  update();
  e->accept();
}
void TerminalView::handleKey(QKeyEvent *e) {
  if (e->modifiers() == Qt::ShiftModifier &&
      (e->key() == Qt::Key_PageUp || e->key() == Qt::Key_PageDown)) {
    terminal_.scroll((e->key() == Qt::Key_PageUp ? 1 : -1) *
                     std::max(1, terminal_.rows() - 1));
    update();
    e->accept();
    return;
  }
  int mod = 0;
  if (e->modifiers() & Qt::ShiftModifier)
    mod |= VTERM_MOD_SHIFT;
  if (e->modifiers() & Qt::AltModifier)
    mod |= VTERM_MOD_ALT;
  if (e->modifiers() & Qt::ControlModifier)
    mod |= VTERM_MOD_CTRL;
  const auto m = VTermModifier(mod);
  VTermKey k = VTERM_KEY_NONE;
  switch (e->key()) {
  case Qt::Key_Return:
  case Qt::Key_Enter:
    k = VTERM_KEY_ENTER;
    break;
  case Qt::Key_Tab:
  case Qt::Key_Backtab:
    k = VTERM_KEY_TAB;
    break;
  case Qt::Key_Backspace:
    k = VTERM_KEY_BACKSPACE;
    break;
  case Qt::Key_Escape:
    k = VTERM_KEY_ESCAPE;
    break;
  case Qt::Key_Left:
    k = VTERM_KEY_LEFT;
    break;
  case Qt::Key_Right:
    k = VTERM_KEY_RIGHT;
    break;
  case Qt::Key_Up:
    k = VTERM_KEY_UP;
    break;
  case Qt::Key_Down:
    k = VTERM_KEY_DOWN;
    break;
  case Qt::Key_Home:
    k = VTERM_KEY_HOME;
    break;
  case Qt::Key_End:
    k = VTERM_KEY_END;
    break;
  case Qt::Key_Delete:
    k = VTERM_KEY_DEL;
    break;
  case Qt::Key_Insert:
    k = VTERM_KEY_INS;
    break;
  case Qt::Key_PageUp:
    k = VTERM_KEY_PAGEUP;
    break;
  case Qt::Key_PageDown:
    k = VTERM_KEY_PAGEDOWN;
    break;
  default:
    if (e->key() >= Qt::Key_F1 && e->key() <= Qt::Key_F12)
      k = VTermKey(VTERM_KEY_FUNCTION_0 + 1 + e->key() - Qt::Key_F1);
  }
  if (k != VTERM_KEY_NONE)
    terminal_.key(k, m);
  else if ((mod & VTERM_MOD_CTRL) && e->key() >= Qt::Key_A &&
           e->key() <= Qt::Key_Z)
    terminal_.text('a' + e->key() - Qt::Key_A, m);
  else
    for (char32_t cp : e->text().toUcs4())
      terminal_.text(cp, m);
  update();
  e->accept();
}
QSGNode *TerminalView::updatePaintNode(QSGNode *old, UpdatePaintNodeData *) {
  QElapsedTimer timer;
  timer.start();
  auto *root = static_cast<Root *>(old);
  const auto dpr = window()->devicePixelRatio();
  if (!root || root->dpr != dpr) {
    delete root;
    root = new Root(dpr);
    root->add(QChar(0xfffd));
    for (QChar ch : repertoire())
      root->add(ch);
  }
  root->clear();
  const auto cells = terminal_.cells();
  int added = 0;
  bool more = false;
  for (const auto &c : cells)
    if (c.width && c.glyph != ' ' &&
        !root->entries.contains(c.glyph.unicode())) {
      if (added < 2 && root->entries.size() < 512) {
        root->add(c.glyph);
        ++added;
      } else if (root->entries.size() < 512)
        more = true;
    }
  for (auto &page : root->pages)
    if (page.dirty) {
      page.texture.reset(window()->createTextureFromImage(page.image));
      if (!page.texture)
        qFatal("Atlas allocation failed");
      page.texture->setFiltering(QSGTexture::Nearest);
      metrics_.textureBytes += page.image.sizeInBytes();
      page.dirty = false;
    }
  metrics_.glyphs = root->entries.size();
  std::vector<std::pair<QRectF, QColor>> backgrounds, lines;
  for (int row = 0; row < terminal_.rows(); ++row)
    for (int col = 0; col < terminal_.columns(); ++col) {
      const auto &c = cells[row * terminal_.columns() + col];
      QRectF r(24 + col * cell_, 20 + row * line_, cell_, line_);
      if (c.background != QColor(7, 5, 3))
        backgrounds.emplace_back(r, c.background);
      if (c.underline)
        lines.emplace_back(QRectF(r.x(), r.bottom() - 3, r.width(), 1),
                           c.foreground);
      if (c.strike)
        lines.emplace_back(QRectF(r.x(), r.y() + line_ * .5, r.width(), 1),
                           c.foreground);
    }
  solid(root, backgrounds);
  // Opaque, preblended amber layers stay inside the existing right gutter.
  // No blur pass, glyph-material change, animation or additional texture upload.
  const auto track = scrollTrack(), thumb = scrollThumb();
  const bool enabled = !terminal_.alternateScreen() && terminal_.historySize();
  const bool overTrack = hovering_ && track.contains(hoverPosition_);
  const bool overThumb = hovering_ && thumb.contains(hoverPosition_);
  const bool active = enabled && (dragging_ || pressed_);
  const qreal light = !enabled ? .35 : active ? 1.25 : overThumb ? 1.1 : .85;
  auto amber = [light](int r, int g, int b) {
    return QColor(std::min(255, qRound(r * light)),
                  std::min(255, qRound(g * light)),
                  std::min(255, qRound(b * light)));
  };
  std::vector<std::pair<QRectF, QColor>> bar = {
      {track, QColor(20, 14, 7)},
      {QRectF(track.x(), track.y(), 1, track.height()),
       enabled && (overTrack || active) ? QColor(72, 43, 14) : QColor(39, 25, 11)},
      {QRectF(track.right() - 1, track.y(), 1, track.height()), QColor(35, 23, 10)}};
  if (enabled) {
    // Two restrained halo steps; lateral extent is only two extra pixels.
    bar.emplace_back(thumb.adjusted(-2, 0, 2, 0), amber(22, 13, 5));
    bar.emplace_back(thumb.adjusted(-1, 0, 1, 0), amber(46, 27, 8));
  }
  bar.emplace_back(thumb, amber(130, 76, 22));
  // Narrow metal bevel: lit left edge, gold core, copper shaded right edge.
  const QColor bands[] = {amber(244, 181, 78), amber(199, 125, 37),
                          amber(166, 91, 23), amber(119, 61, 15)};
  for (int band = 0; band < 4; ++band)
    bar.emplace_back(QRectF(thumb.x() + 1 + band * 2, thumb.y() + 1,
                           2, std::max(qreal(0), thumb.height() - 2)), bands[band]);
  bar.emplace_back(QRectF(thumb.x() + 1, thumb.y(), 8, 1), amber(255, 202, 110));
  bar.emplace_back(QRectF(thumb.x() + 1, thumb.bottom() - 1, 8, 1), amber(84, 43, 12));
  solid(root, bar);
  // Preserve row/glyph order across page switches and bounded batches.
  int current = -1;
  std::vector<Vertex> vertices;
  auto flush = [&] {
    if (vertices.empty())
      return;
    auto *n = new QSGGeometryNode;
    auto *g = new QSGGeometry(attributes, int(vertices.size()));
    g->setDrawingMode(QSGGeometry::DrawTriangles);
    memcpy(g->vertexData(), vertices.data(), vertices.size() * sizeof(Vertex));
    n->setGeometry(g);
    n->setFlag(QSGNode::OwnsGeometry);
    n->setMaterial(new Material(root->pages[current].texture.get()));
    n->setFlag(QSGNode::OwnsMaterial);
    root->appendChildNode(n);
    vertices.clear();
  };
  qint64 fallback = 0;
  for (int row = 0; row < terminal_.rows(); ++row)
    for (int col = 0; col < terminal_.columns(); ++col) {
      const auto &c = cells[row * terminal_.columns() + col];
      if (!c.width || c.glyph == ' ')
        continue;
      auto it = root->entries.find(c.glyph.unicode());
      if (it == root->entries.end()) {
        it = root->entries.find(0xfffd);
        ++fallback;
      }
      const auto &entry = it->second;
      if (entry.page != current || vertices.size() >= 4096 * 6) {
        flush();
        current = entry.page;
      }
      const auto &rect = entry.rect;
      const auto &im = root->pages[current].image;
      const float x = std::round((24 + col * cell_ - 5) * dpr) / dpr,
                  y = std::round((20 + row * line_ - 5) * dpr) / dpr;
      const float x2 = x + rect.width() / dpr, y2 = y + rect.height() / dpr;
      const float u = float(rect.x()) / im.width(),
                  v = float(rect.y()) / im.height(),
                  u2 = float(rect.x() + rect.width()) / im.width(),
                  v2 = float(rect.y() + rect.height()) / im.height();
      auto add = [&](float px, float py, float tu, float tv) {
        vertices.push_back({px, py, tu, tv, float(c.foreground.redF()),
                            float(c.foreground.greenF()),
                            float(c.foreground.blueF()), c.amber ? 0.f : 1.f});
      };
      add(x, y, u, v);
      add(x2, y, u2, v);
      add(x, y2, u, v2);
      add(x, y2, u, v2);
      add(x2, y, u2, v);
      add(x2, y2, u2, v2);
    }
  flush();
  metrics_.fallbackCells = fallback;
  solid(root, lines);
  if (terminal_.cursorVisible()) {
    const auto p = terminal_.cursor();
    solid(root,
          {{QRectF(24 + p.col * cell_, 20 + (p.row + 1) * line_ - 3, cell_, 2),
            QColor(255, 224, 160)}});
  }
  ++metrics_.frames;
  metrics_.workNs += timer.nsecsElapsed();
  if (more)
    QMetaObject::invokeMethod(this, [this] { update(); }, Qt::QueuedConnection);
  return root;
}
