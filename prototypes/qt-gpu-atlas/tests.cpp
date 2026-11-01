#include "atlas.h"
#include "fixture.h"
#include <QDebug>
#include <QGuiApplication>
#include <QSet>
int main(int argc, char **argv) {
  QGuiApplication app(argc, argv);
  auto require = [](bool ok, const char *s) {
    if (!ok)
      qFatal("%s", s);
  };
  for (bool effects : {false, true})
    for (qreal dpr : {1., 1.5, 2.}) {
      GlyphMaterial material(16, dpr, effects, 1, 1, true);
      GlyphAtlas atlas(material, dpr);
      for (QChar ch : repertoire()) {
        const auto &im = material.glyph(ch);
        const QRect r = atlas.entries.value(ch.unicode());
        require(atlas.image.rect().contains(r), "glyph outside atlas");
        require(im.size() == r.size(), "glyph geometry changed");
        for (int y = 0; y < im.height(); ++y)
          for (int x = 0; x < im.width(); ++x)
            require(im.pixel(x, y) == atlas.image.pixel(r.x() + x, r.y() + y),
                    "atlas changed premultiplied texels");
        require(qAlpha(atlas.image.pixel(r.x() - 1, r.y())) == 0,
                "atlas guard not transparent");
        require(qAlpha(im.pixel(0, 0)) == 0,
                "material has rectangular halo (known GCC failure)");
      }
      for (int revision : {0, 1, 97, 123456})
        for (QSizeF size : {QSizeF(1280, 900), QSizeF(3840, 2160)})
          for (const auto &line :
               fixture(size, material.cell(), material.line(), revision))
            for (QChar ch : line)
              require(atlas.entries.contains(ch.unicode()),
                      "fixture missing atlas glyph");
      QSet<QRgb> colors;
      const auto block = material.glyph(QChar(0x2588));
      for (int y = 0; y < block.height(); ++y)
        for (int x = 0; x < block.width(); ++x)
          if (qAlpha(block.pixel(x, y)) == 255)
            colors.insert(block.pixel(x, y));
      if (effects)
        require(colors.size() > 3, "flat material");
      qInfo() << "PASS atlas texels/guards/repertoire" << effects << dpr;
    }
}
