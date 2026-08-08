// Native Stella fox-Pomeranian avatar engine.
// Code-drawn visuals: no baked image assets. Preserves the donor Avatar API,
// weather hooks and the clean scrolling grass system that made Oink feel alive.

#include "../piglet/avatar.h"
#include "../piglet/weather.h"
#include "../ui/display.h"
#include <M5Cardputer.h>
#include <time.h>

AvatarState Avatar::currentState = AvatarState::NEUTRAL;
bool Avatar::isBlinking = false;
bool Avatar::isSniffing = false;
bool Avatar::earsUp = true;
uint32_t Avatar::lastBlinkTime = 0;
uint32_t Avatar::blinkInterval = 5000;
int Avatar::moodIntensity = 0;

bool Avatar::jumpActive = false;
uint32_t Avatar::jumpStartTime = 0;
bool Avatar::transitioning = false;
uint32_t Avatar::transitionStartTime = 0;
int Avatar::transitionFromX = 20;
int Avatar::transitionToX = 20;
bool Avatar::transitionToFacingRight = true;
int Avatar::currentX = 20;

bool Avatar::grassMoving = false;
bool Avatar::grassDirection = true;
bool Avatar::pendingGrassStart = false;
bool Avatar::onRightSide = false;
uint32_t Avatar::lastGrassUpdate = 0;
uint16_t Avatar::grassSpeed = 80;
char Avatar::grassPattern[32] = {0};

Avatar::Star Avatar::stars[15] = {{0}};
uint8_t Avatar::starCount = 0;
uint32_t Avatar::lastStarSpawn = 0;
uint32_t Avatar::nextSpawnDelay = 1500;
bool Avatar::starsActive = false;
uint32_t Avatar::lastNightCheck = 0;
bool Avatar::cachedNightMode = false;

namespace {
bool facingRight = true;
uint32_t lastLookTime = 0;
uint32_t lookInterval = 4500;
uint32_t lastWalkTime = 0;
uint32_t walkInterval = 24000;
uint32_t sniffStartTime = 0;
uint8_t sniffFrame = 0;
bool attackShakeActive = false;
bool attackShakeStrong = false;
uint32_t attackShakeRefreshTime = 0;
bool thunderFlashActive = false;
uint32_t lastGrassStopTime = 0;
constexpr uint32_t kSniffDurationMs = 720;
constexpr uint32_t kGrassCooldownMs = 2500;

uint16_t accent() { return thunderFlashActive ? getColorBG() : getColorFG(); }
uint16_t bg() { return thunderFlashActive ? getColorFG() : getColorBG(); }
uint16_t fur() { return thunderFlashActive ? getColorBG() : 0xFD20; }
uint16_t furDark() { return thunderFlashActive ? getColorBG() : 0xA240; }
uint16_t cream() { return thunderFlashActive ? getColorBG() : 0xFF9C; }
uint16_t ink() { return thunderFlashActive ? getColorBG() : 0x0000; }
uint16_t blush() { return thunderFlashActive ? getColorBG() : 0xF9B2; }

void drawEye(M5Canvas& c, int x, int y, bool closed, bool angry) {
    if (closed) { c.drawFastHLine(x - 2, y, 5, ink()); return; }
    if (angry) c.drawLine(x - 3, y - 3, x + 2, y - 1, ink());
    c.fillCircle(x, y, 3, ink());
    c.fillCircle(x + 1, y - 1, 1, 0xFFFF);
}

void drawPlumeTail(M5Canvas& c, int x, int y, bool right, uint32_t now, AvatarState state) {
    int wag = ((now / 120) % 3) - 1;
    if (state == AvatarState::SLEEPY || state == AvatarState::SAD) wag = 0;
    int tx = right ? x - 13 : x + 52;
    int dir = right ? -1 : 1;
    c.fillCircle(tx, y - 3 + wag, 10, fur());
    c.fillCircle(tx + dir * 7, y - 10 + wag, 9, fur());
    c.fillCircle(tx + dir * 3, y - 17 + wag, 8, cream());
    c.drawCircle(tx, y - 3 + wag, 10, furDark());
}

void drawPom(M5Canvas& c, int x, int y, bool right, bool blink, bool sniff, int yOff,
             AvatarState state, bool walking, bool grassMovingNow, bool earsUpNow) {
    const uint32_t now = millis();
    const bool angry = state == AvatarState::ANGRY;
    const bool sleepy = state == AvatarState::SLEEPY;
    const bool sad = state == AvatarState::SAD;
    const bool happy = state == AvatarState::HAPPY || state == AvatarState::EXCITED;
    const bool tracking = state == AvatarState::HUNTING;

    y += yOff;
    drawPlumeTail(c, x, y + 36, right, now, state);

    c.fillCircle(x + 20, y + 36, 17, fur());
    c.fillCircle(x + 35, y + 35, 18, fur());
    c.fillCircle(x + 27, y + 27, 17, fur());
    c.fillCircle(x + 27, y + 39, 11, cream());
    c.drawCircle(x + 20, y + 36, 17, furDark());
    c.drawCircle(x + 35, y + 35, 18, furDark());

    int step = (walking || grassMovingNow) ? ((now / 120) & 1) : 0;
    c.fillRoundRect(x + 15, y + 48 + step, 8, 12 - step, 3, furDark());
    c.fillRoundRect(x + 37, y + 49 - step, 8, 11 + step, 3, furDark());
    c.fillRoundRect(x + 14, y + 57, 11, 4, 2, cream());
    c.fillRoundRect(x + 36, y + 57, 11, 4, 2, cream());

    c.fillCircle(x + 31, y + 17, 19, fur());
    c.fillCircle(x + 20, y + 21, 9, fur());
    c.fillCircle(x + 42, y + 21, 9, fur());

    if (earsUpNow) {
        c.fillTriangle(x + 16, y + 8, x + 21, y - 8, x + 27, y + 8, fur());
        c.fillTriangle(x + 35, y + 7, x + 42, y - 9, x + 47, y + 9, fur());
        c.fillTriangle(x + 19, y + 5, x + 22, y - 3, x + 25, y + 6, blush());
        c.fillTriangle(x + 38, y + 5, x + 42, y - 4, x + 44, y + 7, blush());
    } else {
        c.fillTriangle(x + 17, y + 7, x + 15, y - 2, x + 28, y + 8, fur());
        c.fillTriangle(x + 35, y + 8, x + 47, y - 1, x + 45, y + 9, fur());
    }

    c.fillCircle(x + 25, y + 21, 9, cream());
    c.fillCircle(x + 37, y + 21, 9, cream());
    c.fillEllipse(x + 31, y + 26, 12, 8, cream());

    int lx = right ? x + 24 : x + 38;
    int rx = right ? x + 38 : x + 24;
    drawEye(c, lx, y + 15, blink || sleepy, angry);
    drawEye(c, rx, y + 15, blink || sleepy, angry);

    int nosePush = 0;
    if (sniff) nosePush = sniffFrame == 1 ? 2 : (sniffFrame == 2 ? 4 : 1);
    int noseX = x + 31 + (right ? nosePush : -nosePush);
    c.fillCircle(noseX, y + 24, tracking ? 4 : 3, ink());

    if (happy) {
        c.drawArc(x + 31, y + 29, 7, 4, 15, 165, ink());
        if (state == AvatarState::EXCITED) c.fillCircle(x + 31, y + 32, 2, blush());
    } else if (sad) {
        c.drawArc(x + 31, y + 34, 7, 4, 195, 345, ink());
    } else if (angry) {
        c.drawFastHLine(x + 27, y + 31, 9, ink());
    } else if (sleepy) {
        c.drawFastHLine(x + 28, y + 30, 7, ink());
    } else {
        c.drawArc(x + 31, y + 29, 5, 3, 20, 160, ink());
    }

    c.drawFastHLine(x + 19, y + 33, 24, accent());
    c.fillCircle(x + 31, y + 36, 3, accent());

    if (tracking) {
        int sx = right ? x + 50 : x + 12;
        c.drawCircle(sx, y + 24, 3, accent());
        c.drawCircle(sx, y + 24, 6, accent());
    }
}
}

void Avatar::init() {
    currentState = AvatarState::NEUTRAL;
    isBlinking = false;
    isSniffing = false;
    earsUp = true;
    lastBlinkTime = millis();
    blinkInterval = random(4000, 8000);
    facingRight = true;
    currentX = 20;
    onRightSide = false;
    transitioning = false;
    grassMoving = false;
    pendingGrassStart = false;
    grassDirection = true;
    grassSpeed = 80;
    lastLookTime = millis();
    lookInterval = random(3500, 9000);
    lastWalkTime = millis();
    walkInterval = random(18000, 42000);
    lastGrassStopTime = 0;
    resetGrassPattern();
    starsActive = false;
    starCount = 0;
    lastNightCheck = 0;
    cachedNightMode = false;
    initStarPositions();
}

void Avatar::setState(AvatarState state) { currentState = state; }
void Avatar::setMoodIntensity(int intensity) { moodIntensity = constrain(intensity, -100, 100); }
bool Avatar::isFacingRight() { return facingRight; }
bool Avatar::isOnRightSide() { return onRightSide; }
bool Avatar::isTransitioning() { return transitioning; }
int Avatar::getCurrentX() { return currentX; }
void Avatar::blink() { isBlinking = true; }
void Avatar::wiggleEars() { earsUp = !earsUp; }

void Avatar::sniff() { isSniffing = true; sniffStartTime = millis(); sniffFrame = 0; }
void Avatar::cuteJump() { jumpActive = true; jumpStartTime = millis(); }

void Avatar::draw(M5Canvas& canvas) {
    const uint32_t now = millis();

    if (isSniffing) {
        const uint32_t elapsed = now - sniffStartTime;
        if (elapsed >= kSniffDurationMs) { isSniffing = false; sniffFrame = 0; }
        else sniffFrame = (elapsed / 110) % 3;
    }

    if (transitioning) {
        const uint32_t elapsed = now - transitionStartTime;
        if (elapsed >= TRANSITION_DURATION_MS) {
            transitioning = false;
            currentX = transitionToX;
            facingRight = transitionToFacingRight;
            onRightSide = currentX > 70;
            if (pendingGrassStart) {
                pendingGrassStart = false;
                grassMoving = true;
                facingRight = !grassDirection;
            }
        } else {
            const float t = (float)elapsed / (float)TRANSITION_DURATION_MS;
            const float smooth = t * t * (3.0f - 2.0f * t);
            currentX = transitionFromX + (int)((transitionToX - transitionFromX) * smooth);
        }
    }

    if (now - lastBlinkTime > blinkInterval) {
        isBlinking = true;
        lastBlinkTime = now;
        blinkInterval = random(4000, 8500);
    }

    if (!transitioning && !grassMoving && !pendingGrassStart) {
        if (now - lastLookTime > lookInterval) {
            const int roll = random(0, 100);
            if (roll < 35) facingRight = !facingRight;
            else if (roll < 58) sniff();
            else if (roll < 75) wiggleEars();
            else if (roll < 88) blink();
            lastLookTime = now;
            lookInterval = random(3000, 10000);
        }

        if (now - lastWalkTime > walkInterval) {
            const int target = onRightSide ? 20 : 154;
            startWindupSlide(target, target > currentX);
            lastWalkTime = now;
            walkInterval = random(20000, 50000);
        }
    }

    const bool doBlink = isBlinking && currentState != AvatarState::SLEEPY;
    isBlinking = false;
    drawFrame(canvas, nullptr, 0, doBlink, facingRight, isSniffing);
}

void Avatar::drawFrame(M5Canvas& canvas, const char**, uint8_t, bool doBlink, bool faceRight, bool doSniff) {
    updateStars();
    drawStars(canvas);
    fillPigBoundingBox(canvas);

    const uint32_t now = millis();
    if (attackShakeRefreshTime == 0 || now - attackShakeRefreshTime > 250) {
        attackShakeActive = false;
        attackShakeStrong = false;
    }
    if (jumpActive && now - jumpStartTime > JUMP_DURATION_MS) jumpActive = false;

    int yOffset = 0;
    if (jumpActive) {
        const float t = (float)(now - jumpStartTime) / (float)JUMP_DURATION_MS;
        yOffset = -(int)(4.0f * t * (1.0f - t) * JUMP_HEIGHT);
    } else if (attackShakeActive) {
        const int amp = attackShakeStrong ? 6 : 3;
        yOffset = (esp_random() & 1) ? amp : -amp;
    } else if (transitioning || grassMoving) {
        static const int bounce[4] = {0, -1, -2, -1};
        yOffset = bounce[(now / 100) % 4];
    }

    drawPom(canvas, currentX, 24, faceRight, doBlink, doSniff, yOffset,
            currentState, transitioning, grassMoving, earsUp);
    drawGrass(canvas);
}

void Avatar::setGrassMoving(bool moving, bool directionRight) {
    if (moving == grassMoving && !pendingGrassStart) return;
    if (moving) {
        if (lastGrassStopTime && millis() - lastGrassStopTime < kGrassCooldownMs) return;
        grassDirection = directionRight;
        const int target = directionRight ? 154 : 20;
        if (currentX != target) {
            startWindupSlide(target, target > currentX);
            pendingGrassStart = true;
            grassMoving = false;
        } else {
            grassMoving = true;
            pendingGrassStart = false;
            facingRight = !directionRight;
        }
    } else {
        grassMoving = false;
        pendingGrassStart = false;
        lastGrassStopTime = millis();
        startWindupSlide(20, false);
    }
}

void Avatar::setGrassSpeed(uint16_t ms) { grassSpeed = ms; }
void Avatar::setGrassPattern(const char* pattern) {
    if (!pattern) return;
    strncpy(grassPattern, pattern, 26);
    grassPattern[26] = '\0';
}
void Avatar::resetGrassPattern() {
    for (int i = 0; i < 26; ++i) grassPattern[i] = (random(0, 2) == 0) ? '/' : '\\';
    grassPattern[26] = '\0';
}
void Avatar::updateGrass() {
    if (!grassMoving) return;
    const uint32_t now = millis();
    if (now - lastGrassUpdate < grassSpeed) return;
    lastGrassUpdate = now;
    if (grassDirection) {
        const char last = grassPattern[25];
        for (int i = 25; i > 0; --i) grassPattern[i] = grassPattern[i - 1];
        grassPattern[0] = last;
    } else {
        const char first = grassPattern[0];
        for (int i = 0; i < 25; ++i) grassPattern[i] = grassPattern[i + 1];
        grassPattern[25] = first;
    }
}
void Avatar::drawGrass(M5Canvas& canvas) {
    updateGrass();
    canvas.setTextSize(2);
    canvas.setTextColor(accent());
    canvas.setTextDatum(top_left);
    canvas.drawString(grassPattern, 0, 111);
}

bool Avatar::isNightTime() {
    const uint32_t now = millis();
    if (lastNightCheck && now - lastNightCheck < 60000) return cachedNightMode;
    lastNightCheck = now;
    auto dt = M5.Rtc.getDateTime();
    if (dt.date.year >= 2024) {
        cachedNightMode = dt.time.hours >= 20 || dt.time.hours < 6;
        return cachedNightMode;
    }
    const time_t unixNow = time(nullptr);
    if (unixNow >= 1700000000) {
        struct tm info; localtime_r(&unixNow, &info);
        cachedNightMode = info.tm_hour >= 20 || info.tm_hour < 6;
        return cachedNightMode;
    }
    cachedNightMode = false;
    return false;
}
bool Avatar::areStarsActive() { return starsActive; }
void Avatar::initStarPositions() {
    for (uint8_t i = 0; i < MAX_STARS; ++i) {
        stars[i].x = random(4, 236); stars[i].y = random(20, 100); stars[i].size = random(1, 3);
        stars[i].brightness = 0; stars[i].isBlinking = random(0, 100) < 50; stars[i].fadeInStart = 0;
    }
}
void Avatar::updateStars() {
    if (!isNightTime()) { starsActive = false; starCount = 0; return; }
    starsActive = true;
    const uint32_t now = millis();
    if (starCount < MAX_STARS && now - lastStarSpawn > nextSpawnDelay) {
        stars[starCount].brightness = 255; stars[starCount].fadeInStart = now; ++starCount;
        lastStarSpawn = now; nextSpawnDelay = random(700, 2400);
    }
}
void Avatar::drawStars(M5Canvas& canvas) {
    if (!starsActive) return;
    for (uint8_t i = 0; i < starCount; ++i) {
        if (!stars[i].brightness) continue;
        bool visible = !stars[i].isBlinking || ((millis() / 500 + i) & 1);
        if (visible) canvas.fillCircle(stars[i].x, stars[i].y, stars[i].size, accent());
    }
}
void Avatar::fillPigBoundingBox(M5Canvas& canvas) {
    canvas.fillRect(0, 15, 240, 100, bg());
}
void Avatar::setFacingLeft() { facingRight = false; }
void Avatar::setFacingRight() { facingRight = true; }
void Avatar::setAttackShake(bool active, bool strong) {
    attackShakeActive = active; attackShakeStrong = strong; attackShakeRefreshTime = millis();
}
void Avatar::setThunderFlash(bool active) { thunderFlashActive = active; }
bool Avatar::isThunderFlashing() { return thunderFlashActive; }
void Avatar::startWindupSlide(int targetX, bool faceRight) {
    transitionFromX = currentX; transitionToX = constrain(targetX, 20, 154);
    transitionToFacingRight = faceRight; transitionStartTime = millis(); transitioning = true;
}
