#include "terminal.h"
#include <algorithm>
const VTermScreenCallbacks Terminal::callbacks_ = {
    nullptr,
    nullptr,
    nullptr,
    [](VTermProp prop, VTermValue *value, void *user) {
      auto &t = *static_cast<Terminal *>(user);
      if (prop == VTERM_PROP_CURSORVISIBLE)
        t.visible_ = value->boolean;
      if (prop == VTERM_PROP_ALTSCREEN) {
        t.alternate_ = value->boolean;
        t.offset_ = 0;
      }
      // Deliberately ignore OSC titles, clipboard, hyperlinks and desktop
      // actions.
      return 1;
    },
    nullptr,
    nullptr,
    [](int cols, const VTermScreenCell *cells, void *user) {
      auto &t = *static_cast<Terminal *>(user);
      if (t.alternate_)
        return 1;
      t.history_.emplace_back(cells, cells + cols);
      if (t.offset_)
        ++t.offset_;
      if (t.history_.size() > 1000)
        t.history_.pop_front();
      t.offset_ = std::min(t.offset_, int(t.history_.size()));
      return 1;
    },
    nullptr,
    [](void *user) {
      auto &t = *static_cast<Terminal *>(user);
      t.history_.clear();
      t.offset_ = 0;
      return 1;
    }};
Terminal::Terminal(int rows, int columns)
    : vt_(vterm_new(rows, columns)), screen_(vterm_obtain_screen(vt_)),
      rows_(rows), columns_(columns) {
  VTERM_CHECK_VERSION;
  vterm_set_utf8(vt_, 1);
  vterm_screen_enable_altscreen(screen_, 1);
  vterm_screen_set_damage_merge(screen_, VTERM_DAMAGE_SCREEN);
  vterm_screen_set_callbacks(screen_, &callbacks_, this);
  vterm_output_set_callback(
      vt_,
      [](const char *s, size_t n, void *u) {
        auto &t = *static_cast<Terminal *>(u);
        if (t.output)
          t.output(QByteArray(s, qsizetype(n)));
      },
      this);
  VTermColor fg, bg;
  vterm_color_rgb(&fg, 255, 179, 66);
  vterm_color_rgb(&bg, 7, 5, 3);
  vterm_screen_set_default_colors(screen_, &fg, &bg);
  vterm_screen_reset(screen_, 1);
}
Terminal::~Terminal() { vterm_free(vt_); }
void Terminal::feed(const QByteArray &bytes) {
  vterm_input_write(vt_, bytes.constData(), size_t(bytes.size()));
  vterm_screen_flush_damage(screen_);
}
void Terminal::resize(int rows, int columns) {
  rows_ = std::clamp(rows, 1, 256);
  columns_ = std::clamp(columns, 1, 512);
  vterm_set_size(vt_, rows_, columns_);
  vterm_screen_flush_damage(screen_);
}
void Terminal::key(VTermKey k, VTermModifier m) {
  offset_ = 0;
  vterm_keyboard_key(vt_, k, m);
}
void Terminal::text(uint32_t c, VTermModifier m) {
  offset_ = 0;
  vterm_keyboard_unichar(vt_, c, m);
}
void Terminal::setOffset(int lines) {
  offset_ = alternate_ ? 0 : std::clamp(lines, 0, historySize());
}
void Terminal::scroll(int lines) { setOffset(offset_ + lines); }
VTermPos Terminal::cursor() const {
  VTermPos p{};
  vterm_state_get_cursorpos(vterm_obtain_state(vt_), &p);
  return p;
}
Cell Terminal::convert(VTermScreenCell c) const {
  Cell result;
  result.width = c.width;
  if (c.chars[0] == uint32_t(-1)) {
    result.width = 0;
    c.chars[0] = 0;
  }
  QString text;
  for (uint32_t ch : c.chars) {
    if (!ch)
      break;
    const char32_t cp = ch;
    text += QString::fromUcs4(&cp, 1);
  }
  text = text.normalized(QString::NormalizationForm_C);
  if (!text.isEmpty())
    result.glyph = (text.size() == 1 && c.width == 1 && !text[0].isSurrogate())
                       ? text[0]
                       : QChar(0xfffd);
  result.amber = VTERM_COLOR_IS_DEFAULT_FG(&c.fg) && !c.attrs.reverse;
  vterm_screen_convert_color_to_rgb(screen_, &c.fg);
  vterm_screen_convert_color_to_rgb(screen_, &c.bg);
  result.foreground = QColor(c.fg.rgb.red, c.fg.rgb.green, c.fg.rgb.blue);
  result.background = QColor(c.bg.rgb.red, c.bg.rgb.green, c.bg.rgb.blue);
  if (c.attrs.reverse)
    std::swap(result.foreground, result.background);
  if (c.attrs.conceal)
    result.glyph = ' ';
  result.underline = c.attrs.underline;
  result.strike = c.attrs.strike;
  return result;
}
std::vector<Cell> Terminal::cells() const {
  std::vector<Cell> out;
  out.reserve(rows_ * columns_);
  for (int row = 0; row < rows_; ++row)
    for (int col = 0; col < columns_; ++col) {
      VTermScreenCell c{};
      const int index = int(history_.size()) - offset_ + row;
      if (index < int(history_.size())) {
        const auto &line = history_[index];
        if (col < int(line.size()))
          c = line[col];
        else {
          Cell blank;
          blank.background = QColor(7, 5, 3);
          out.push_back(blank);
          continue;
        }
      } else
        vterm_screen_get_cell(screen_, {row - offset_, col}, &c);
      auto cell = convert(c);
      if (cell.width == 0 && col > 0) {
        // libvterm marks continuation cells but may retain their old pen.
        // The leading wide cell owns the style across its complete advance.
        cell = out.back();
        cell.width = 0;
        cell.glyph = ' ';
      }
      out.push_back(cell);
    }
  return out;
}
QString Terminal::textDump() const {
  QString out;
  auto grid = cells();
  for (int r = 0; r < rows_; ++r) {
    for (int c = 0; c < columns_; ++c)
      out += grid[r * columns_ + c].glyph;
    out += '\n';
  }
  return out;
}
