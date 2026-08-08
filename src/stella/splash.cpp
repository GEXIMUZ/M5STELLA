#include "splash.h"

#include <M5Unified.h>
#include "../ui/display.h"
#include "../audio/sfx.h"

namespace StellaSplash {
namespace {

void waitWithAudio(uint32_t ms) {
    const uint32_t start = millis();
    while (millis() - start < ms) {
        SFX::update();
        delay(5);
        yield();
    }
}

void drawPomMark(uint16_t fg, uint16_t bg) {
    // Lightweight placeholder mark: pointed fox-Pom ears, fluffy head and muzzle.
    // The dedicated Stella art pass will replace this with proper sprite frames.
    const int cx = DISPLAY_W / 2;
    const int cy = DISPLAY_H / 2 - 6;

    M5.Display.fillScreen(bg);
    M5.Display.fillTriangle(cx - 34, cy - 18, cx - 15, cy - 40, cx - 8, cy - 10, fg);
    M5.Display.fillTriangle(cx + 34, cy - 18, cx + 15, cy - 40, cx + 8, cy - 10, fg);
    M5.Display.fillCircle(cx, cy, 31, fg);
    M5.Display.fillCircle(cx - 11, cy - 3, 4, bg);
    M5.Display.fillCircle(cx + 11, cy - 3, 4, bg);
    M5.Display.fillCircle(cx, cy + 10, 7, bg);
    M5.Display.fillTriangle(cx - 5, cy + 8, cx + 5, cy + 8, cx, cy + 14, fg);
}

} // namespace

void show() {
    const uint16_t fg = COLOR_FG;
    const uint16_t bg = COLOR_BG;

    M5.Display.setColorDepth(8);
    M5.Display.setTextColor(fg, bg);
    M5.Display.setTextDatum(middle_center);

    // First audible/visual identity break from Porkchop: Stella barks, never oinks.
    M5.Display.fillScreen(bg);
    M5.Display.setTextSize(4);
    M5.Display.drawString("WOOF", DISPLAY_W / 2, DISPLAY_H / 2 - 20);
    M5.Display.drawString("WOOF", DISPLAY_W / 2, DISPLAY_H / 2 + 20);
    SFX::play(SFX::BOOT);
    waitWithAudio(800);

    M5.Display.fillScreen(bg);
    M5.Display.setTextSize(3);
    M5.Display.drawString("MY NAME IS", DISPLAY_W / 2, DISPLAY_H / 2);
    waitWithAudio(650);

    drawPomMark(fg, bg);
    M5.Display.setTextColor(fg, bg);
    M5.Display.setTextSize(3);
    M5.Display.drawString("STELLA", DISPLAY_W / 2, DISPLAY_H - 17);
    waitWithAudio(950);

    M5.Display.fillScreen(bg);
    M5.Display.setTextDatum(top_left);
    M5.Display.setTextSize(1);
}

} // namespace StellaSplash
