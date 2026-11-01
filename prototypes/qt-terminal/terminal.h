#pragma once
#include <QByteArray>
#include <QColor>
#include <QString>
#include <deque>
#include <functional>
#include <vector>
#include <vterm.h>
struct Cell {
  QChar glyph = ' ';
  QColor foreground, background;
  bool amber = true, underline = false, strike = false;
  int width = 1;
};
// Only this adapter knows libvterm; PTY and presentation do not parse bytes.
class Terminal {
public:
  Terminal(int rows, int columns);
  ~Terminal();
  Terminal(const Terminal &) = delete;
  Terminal &operator=(const Terminal &) = delete;
  void feed(const QByteArray &bytes);
  void resize(int rows, int columns);
  void key(VTermKey key, VTermModifier modifiers);
  void text(uint32_t codepoint, VTermModifier modifiers);
  std::vector<Cell> cells() const;
  QString textDump() const;
  void scroll(int lines);
  int rows() const { return rows_; }
  int columns() const { return columns_; }
  int historySize() const { return int(history_.size()); }
  int offset() const { return offset_; }
  VTermPos cursor() const;
  bool cursorVisible() const { return visible_ && offset_ == 0; }
  std::function<void(const QByteArray &)> output;

private:
  VTerm *vt_;
  VTermScreen *screen_;
  int rows_, columns_, offset_ = 0;
  bool visible_ = true, alternate_ = false;
  std::deque<std::vector<VTermScreenCell>> history_;
  static const VTermScreenCallbacks callbacks_;
  Cell convert(VTermScreenCell cell) const;
};
