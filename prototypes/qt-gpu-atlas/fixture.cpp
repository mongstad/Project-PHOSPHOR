#include "fixture.h"
#include <algorithm>
QStringList fixture(QSizeF size, qreal cell, qreal lineHeight, int revision) {
  QStringList lines;
  const QStringList sample = {
      "PHOSPHOR  /  MOLTEN AMBER",
      "",
      "astra@phosphor:~$ ./visual-sample --no-shell",
      "/home/astra/research/PHOSPHOR-002/render.cpp",
      "",
      "ABCDEFGHIJKLMNOPQRSTUVWXYZ  0123456789",
      "abcdefghijklmnopqrstuvwxyz  0O 1Il | ! : ; . ,",
      "!@#$%^&*()_+-=[]{};:'\"<>,.?/\\|~`",
      QString::fromUtf8("┌──────────────────────────────┬───────────────┐"),
      QString::fromUtf8("│ GLYPH MATERIAL               │ STATIC DEMO   │"),
      QString::fromUtf8("├──────────────────────────────┼───────────────┤"),
      QString::fromUtf8("│ Mask → bevel → light → bloom │ 12 / 14 / 16  │"),
      QString::fromUtf8("└──────────────────────────────┴───────────────┘"),
      QString::fromUtf8("Unicode: α β γ Δ Ω λ π ± × ÷ ≤ ≥ ← ↑ → ↓ é ø å"),
      QString::fromUtf8("Solid glyphs: █ ▓ ▒ ░ ■ ◆   Thin glyphs: .,:;!|i"),
      "",
      "No command execution. Synthetic text updates only.",
      ""};
  const int cols = std::max(1, int((size.width() - 48) / cell));
  const int rows = std::max(0, int((size.height() - 40) / lineHeight));
  for (int row = 0; row < rows; ++row) {
    QString line;
    if (row < sample.size())
      line = sample[row];
    else {
      const int n = row + revision;
      line = QString("[%1] material.sample  x=0.125 y=-3.750  "
                     "/src/glyph_%2.cpp  {a+=1;}  OK ")
                 .arg(n, 6, 10, QChar('0'))
                 .arg(n % 97, 2, 10, QChar('0'));
      while (line.size() < cols)
        line += line;
    }
    lines.append(line.left(cols));
  }

  return lines;
}
QString repertoire() {
  QString chars;
  for (int c = 32; c < 127; ++c)
    chars.append(QChar(c));
  for (const auto &line : fixture(QSizeF(1280, 900), 12, 33, 0))
    for (QChar ch : line)
      if (!chars.contains(ch))
        chars.append(ch);
  return chars;
}
