#!/usr/bin/env python3
"""Deterministic terminal content; this does not test a terminal renderer itself."""
import argparse
import curses
import sys
import time

RESET = "\x1b[0m"


def static():
    print("PHOSPHOR-001 | Noto Sans Mono 12 pt | inspect at native size")
    print("PHOSPHOR $ printf 'Molten Amber glyph experiment\\n'")
    print("Molten Amber 0123456789  ilI1|  O0Q@  {}[]() <> /\\ _-=+")
    print("\x1b[1mBold\x1b[22m  \x1b[3mItalic\x1b[23m  \x1b[4mUnderline\x1b[24m  \x1b[7mInverse" + RESET)
    print("ANSI foregrounds (colors must remain distinguishable):")
    print("".join(f"\x1b[{c}m{c:3d} Ab " for c in (*range(30, 38), *range(90, 98))) + RESET)
    print("ANSI backgrounds (look for glowing cell rectangles):")
    print("".join(f"\x1b[{c}m {c:3d} " for c in (*range(40, 48), *range(100, 108))) + RESET)
    print("256-color samples:")
    for start in (16, 52, 88, 124, 160, 196, 232):
        print("".join(f"\x1b[38;5;{c}m{c:3d} " for c in range(start, start + 16)) + RESET)
    print("Truecolor gradient:")
    print("".join(f"\x1b[38;2;{255-i*4};{i*4};160m█" for i in range(64)) + RESET)
    print("Deliberate false positive: amber background and amber block glyphs")
    print("\x1b[48;2;255;158;46m                " + RESET + "  \x1b[38;2;255;158;46m████████████████" + RESET)
    print("Amber on blue: \x1b[48;2;25;45;90m\x1b[38;2;255;158;46m EXPECT MASK FAILURE " + RESET)
    print("Unicode: Ångström Ελληνικά 日本語 e\u0301  → ↔ ∑ ∞  🙂")
    print("┌──────────────┬──────────────┐")
    print("│ sharp edges  │ alignment    │")
    print("└──────────────┴──────────────┘")
    print("Select this line, copy it, then paste at the clean shell prompt.")
    print("Inspect small punctuation: .,:;'` !?  Keep cursor visible here.")


def scroll(seconds):
    end = time.monotonic() + seconds
    row = 0
    while time.monotonic() < end:
        sys.stdout.write("".join(f"{i:08d} | Molten Amber rapid scroll | αβγ ── 0123456789\n" for i in range(row, row + 32)))
        sys.stdout.flush()
        row += 32
        time.sleep(1 / 60)


def fullscreen(screen, seconds):
    curses.curs_set(1)
    screen.timeout(50)
    end = time.monotonic() + seconds
    while time.monotonic() < end:
        screen.erase()
        height, width = screen.getmaxyx()
        if height > 3 and width > 20:
            screen.box()
            text = f"PHOSPHOR fullscreen {width}x{height} | resize; q exits"
            screen.addnstr(1, 1, text, width - 2)
            screen.move(2, 1)
        screen.refresh()
        if screen.getch() == ord("q"):
            break


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("mode", choices=("static", "scroll", "fullscreen"), nargs="?", default="static")
    parser.add_argument("--seconds", type=float, default=30)
    args = parser.parse_args()
    if not 0 < args.seconds <= 300:
        parser.error("--seconds must be greater than zero and at most 300")
    try:
        if args.mode == "static":
            static()
        elif args.mode == "scroll":
            scroll(args.seconds)
        else:
            curses.wrapper(fullscreen, args.seconds)
    except KeyboardInterrupt:
        pass
    finally:
        sys.stdout.write(RESET)
        sys.stdout.flush()


if __name__ == "__main__":
    main()
