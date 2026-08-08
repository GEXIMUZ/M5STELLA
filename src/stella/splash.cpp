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

// Purpose-built fox-Pomeranian boot mark. This deliberately avoids the old
// circular pig-head silhouette: narrow fox face, pointed ears, cheek fluff,
// black nose, tiny tongue and a plume-tail/paw identity cue.
void drawPomMark(uint16_t fg, uint16_t bg) {
    const int cx = DISPLAY_W / 2;
    const int cy = DISPLAY_H / 2 - 8;

    M5.Display.fillScreen(bg);

    // Ears + inner ears.
    M5.Display.fillTriangle(cx - 31, cy - 8, cx - 18, cy - 42, cx - 7, cy - 7, fg);
    M5.Display.fillTriangle(cx + 31, cy - 8, cx + 18, cy - 42, cx + 7, cy - 7, fg);
    M5.Display.fillTriangle(cx - 24, cy - 10, cx - 18, cy - 32, cx - 12, cy - 9, bg);
    M5.Display.fillTriangle(cx + 24, cy - 10, cx + 18, cy - 32, cx + 12, cy - 9, bg);

    // Fluffy fox-Pom head: layered cheeks instead of one pig-like circle.
    M5.Display.fillCircle(cx, cy, 26, fg);
    M5.Display.fillCircle(cx - 20, cy + 9, 13, fg);
    M5.Display.fillCircle(cx + 20, cy + 9, 13, fg);
    M5.Display.fillTriangle(cx - 25, cy + 14, cx - 9, cy + 33, cx - 3, cy + 12, fg);
    M5.Display.fillTriangle(cx + 25, cy + 14, cx + 9, cy + 33, cx + 3, cy + 12, fg);

    // Eyes: smaller, dog-like and widely spaced.
    M5.Display.fillCircle(cx - 10, cy - 3, 3, bg);
    M5.Display.fillCircle(cx + 10, cy - 3, 3, bg);
    M5.Display.drawPixel(cx - 9, cy - 4, fg);
    M5.Display.drawPixel(cx + 11, cy - 4, fg);

    // Cream muzzle cutout + pointed black nose. No round pig snout.
    M5.Display.fillEllipse(cx, cy + 10, 14, 9, bg);
    M5.Display.fillTriangle(cx - 4, cy + 6, cx + 4, cy + 6, cx, cy + 12, fg);
    M5.Display.drawLine(cx, cy + 12, cx, cy + 16, fg);
    M5.Display.drawArc(cx, cy + 15, 7, 4, 20, 160, fg);

    // Tiny chest tuft/paw signature under the face.
    M5.Display.fillTriangle(cx, cy + 22, cx - 9, cy + 34, cx + 9, cy + 34, fg);
    M5.Display.fillCircle(cx - 5, cy + 33, 3, fg);
    M5.Display.fillCircle(cx + 5, cy + 33, 3, fg);
}

} // namespace

void show() {
    const uint16_t fg = COLOR_FG;
    const uint16_t bg = COLOR_BG;

    M5.Display.setColorDepth(8);
    M5.Display.setTextColor(fg, bg);
    M5.Display.setTextDatum(middle_center);

    M5.Display.fillScreen(bg);
    M5.Display.setTextSize(4);
    M5.Display.drawString("WOOF", DISPLAY_W / 2, DISPLAY_H / 2 - 20);
    M5.Display.drawString("WOOF", DISPLAY_W / 2, DISPLAY_H / 2 + 20);
    SFX::play(SFX::BOOT);
    waitWithAudio(800);

    M5.Display.fillScreen(bg);
    M5.Display.setTextSize(3);
    M5.Display.drawString("THIS IS", DISPLAY_W / 2, DISPLAY_H / 2 - 15);
    M5.Display.drawString("STELLA", DISPLAY_W / 2, DISPLAY_H / 2 + 17);
    waitWithAudio(650);

    drawPomMark(fg, bg);
    M5.Display.setTextColor(fg, bg);
    M5.Display.setTextSize(2);
    M5.Display.drawString("STELLA THE WARDOG", DISPLAY_W / 2, DISPLAY_H - 13);
    waitWithAudio(1050);

    M5.Display.fillScreen(bg);
    M5.Display.setTextDatum(top_left);
    M5.Display.setTextSize(1);
}

} // namespace StellaSplash
