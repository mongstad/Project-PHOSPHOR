#pragma once
#include "terminal.h"
#include <QKeyEvent>
#include <QQuickItem>
#include <atomic>
struct RenderMetrics {
  std::atomic<qint64> frames{0}, workNs{0}, textureBytes{0}, glyphs{0},
      fallbackCells{0};
};
class TerminalView : public QQuickItem {
public:
  TerminalView(QQuickItem *parent, Terminal &terminal, RenderMetrics &metrics);
  qreal cellWidth() const { return cell_; }
  qreal lineHeight() const { return line_; }
  void handleKey(QKeyEvent *event);

protected:
  QSGNode *updatePaintNode(QSGNode *, UpdatePaintNodeData *) override;
  void keyPressEvent(QKeyEvent *e) override { handleKey(e); }
  void wheelEvent(QWheelEvent *e) override;

private:
  Terminal &terminal_;
  RenderMetrics &metrics_;
  qreal cell_, line_;
};
