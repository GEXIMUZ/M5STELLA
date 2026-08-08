// Native Stella fox-Pomeranian avatar engine.
// Replaces the donor pig renderer while preserving the Avatar API used by the
// rest of the firmware. Internal file/folder names remain compatible for now.

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
int Avatar::transitionFromX = 18;
int Avatar::transitionToX = 18;
bool Avatar::transitionToFacingRight = true;
int Avatar::currentX = 18;

bool Avatar::grassMoving = false;
bool Avatar::grassDirection = true;
bool Avatar::pendingGrassStart = false;
bool Avatar::onRightSide = false;
uint32_t Avatar::lastGrassUpdate = 0;
uint16_t Avatar::grassSpeed = 90;
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
constexpr uint32_t kSniffDurationMs = 650;
constexpr uint32_t kGrassCooldownMs = 2500;

uint16_t drawColor() { return thunderFlashActive ? getColorBG() : getColorFG(); }
uint16_t bgColor() { return thunderFlashActive ? getColorFG() : getColorBG(); }

// Three-line Pomeranian frames. Pointed ears + fox muzzle are fixed while the
// body line gets a dynamic plume tail in drawFrame().
const char* STELLA_NEUTRAL_R[] = {" /\\_/\\", "(o .>)", "(    )"};
const char* STELLA_HAPPY_R[]   = {" /\\_/\\", "(^ .>)", "(    )"};
const char* STELLA_EXCITED_R[] = {" /\\!/\\", "(@ .>)", "(    )"};
const char* STELLA_TRACK_R[]   = {" /\\^/\\", "(= .>)", "(    )"};
const char* STELLA_SLEEP_R[]   = {" /\\_/\\", "(- .>)", "(    )"};
const char* STELLA_SAD_R[]     = {" /\\_/\\", "(T .>)", "(    )"};
const char* STELLA_ANGRY_R[]   = {" /\\^/\\", "(# .>)", "(    )"};

const char* STELLA_NEUTRAL_L[] = {" /\\_/\\", "(<. o)", "(    )"};
const char* STELLA_HAPPY_L[]   = {" /\\_/\\", "(<. ^)", "(    )"};
const char* STELLA_EXCITED_L[] = {" /\\!/\\", "(<. @)", "(    )"};
const char* STELLA_TRACK_L[]   = {" /\\^/\\", "(<. =)", "(    )"};
const char* STELLA_SLEEP_L[]   = {" /\\_/\\", "(<. -)", "(    )"};
const char* STELLA_SAD_L[]     = {" /\\_/\\", "(<. T)", "(    )"};
const char* STELLA_ANGRY_L[]   = {" /\\^/\\", "(<. #)", "(    )"};
}

void Avatar::init() {
    currentState = AvatarState::NEUTRAL;
    isBlinking = false;
    isSniffing = false;
    earsUp = true;
    lastBlinkTime = millis();
    blinkInterval = random(4000, 8000);
    facingRight = true;
    currentX = 18;
    onRightSide = false;
    transitioning = false;
    grassMoving = false;
    pendingGrassStart = false;
    grassDirection = true;
    grassSpeed = 90;
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

void Avatar::sniff() {
    isSniffing = true;
    sniffStartTime = millis();
    sniffFrame = 0;
}

void Avatar::cuteJump() {
    jumpActive = true;
    jumpStartTime = millis();
}

void Avatar::draw(M5Canvas& canvas) {
    const uint32_t now = millis();

    if (isSniffing) {
        const uint32_t elapsed = now - sniffStartTime;
        if (elapsed >= kSniffDurationMs) {
            isSniffing = false;
            sniffFrame = 0;
        } else {
            sniffFrame = (elapsed / 110) % 3;
        }
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
            const int target = onRightSide ? 18 : 112;
            startWindupSlide(target, target > currentX);
            lastWalkTime = now;
            walkInterval = random(20000, 50000);
        }
    }

    const char** frame = facingRight ? STELLA_NEUTRAL_R : STELLA_NEUTRAL_L;
    switch (currentState) {
        case AvatarState::HAPPY: frame = facingRight ? STELLA_HAPPY_R : STELLA_HAPPY_L; break;
        case AvatarState::EXCITED: frame = facingRight ? STELLA_EXCITED_R : STELLA_EXCITED_L; break;
        case AvatarState::HUNTING: frame = facingRight ? STELLA_TRACK_R : STELLA_TRACK_L; break;
        case AvatarState::SLEEPY: frame = facingRight ? STELLA_SLEEP_R : STELLA_SLEEP_L; break;
        case AvatarState::SAD: frame = facingRight ? STELLA_SAD_R : STELLA_SAD_L; break;
        case AvatarState::ANGRY: frame = facingRight ? STELLA_ANGRY_R : STELLA_ANGRY_L; break;
        default: break;
    }

    const bool doBlink = isBlinking && currentState != AvatarState::SLEEPY;
    isBlinking = false;
    drawFrame(canvas, frame, 3, doBlink, facingRight, isSniffing);
}

void Avatar::drawFrame(M5Canvas& canvas, const char** frame, uint8_t lines, bool doBlink, bool faceRight, bool doSniff) {
    updateStars();
    drawStars(canvas);
    fillPigBoundingBox(canvas); // legacy method name; now clears Stella's bounds

    canvas.setTextDatum(top_left);
    canvas.setTextSize(3);
    canvas.setTextColor(drawColor());

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
        static const int bounce[4] = {0, -2, -1, -3};
        yOffset = bounce[(now / 90) % 4];
    }

    const int startX = currentX;
    const int startY = 23 + yOffset;
    const int lineHeight = 22;

    for (uint8_t i = 0; i < lines; ++i) {
        if (i == 2) {
            const char* body = faceRight ? "~(    )" : "(    )~";
            const int bodyX = faceRight ? startX - 18 : startX;
            canvas.drawString(body, bodyX, startY + i * lineHeight);
            continue;
        }

        char line[16];
        strncpy(line, frame[i], sizeof(line) - 1);
        line[sizeof(line) - 1] = '\0';

        if (i == 0 && !earsUp) {
            // Fold the ear tips for a tiny listening animation.
            for (size_t j = 0; line[j]; ++j) {
                if (line[j] == '/') line[j] = '-';
                else if (line[j] == '\\') line[j] = '-';
            }
        }

        if (i == 1) {
            if (doBlink) {
                if (faceRight) line[1] = '-';
                else line[4] = '-';
            }
            if (doSniff) {
                char a = '.', b = '>';
                if (sniffFrame == 1) a = 'o';
                else if (sniffFrame == 2) a = 'O';
                if (faceRight) { line[3] = a; line[4] = b; }
                else { line[1] = '<'; line[2] = a; }
            }
        }

        canvas.drawString(line, startX, startY + i * lineHeight);
    }

    drawGrass(canvas);
}

void Avatar::setGrassMoving(bool moving, bool directionRight) {
    if (moving == grassMoving && !pendingGrassStart) return;

    if (moving) {
        if (lastGrassStopTime && millis() - lastGrassStopTime < kGrassCooldownMs) return;
        grassDirection = directionRight;
        const int target = directionRight ? 112 : 18;
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
        startWindupSlide(18, false);
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
    canvas.setTextColor(drawColor());
    canvas.setTextDatum(top_left);
    canvas.drawString(grassPattern, 0, 91);
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
        struct tm info;
        localtime_r(&unixNow, &info);
        cachedNightMode = info.tm_hour >= 20 || info.tm_hour < 6;
        return cachedNightMode;
    }

    cachedNightMode = false;
    return false;
}

bool Avatar::areStarsActive() { return starsActive; }

void Avatar::initStarPositions() {
    for (uint8_t i = 0; i < MAX_STARS; ++i) {
        stars[i].x = random(5, 235);
        stars[i].y = random(20, 88);
        stars[i].size = 1;
        stars[i].brightness = 0;
        stars[i].isBlinking = random(0, 100) < 20;
        stars[i].fadeInStart = 0;
    }
}

void Avatar::updateStars() {
    const uint32_t now = millis();
    if (Weather::isRaining()) {
        starsActive = false;
        starCount = 0;
        return;
    }

    const bool night = isNightTime();
    if (night && !starsActive) {
        starsActive = true;
        starCount = 0;
        lastStarSpawn = now;
        nextSpawnDelay = random(800, 3000);
        initStarPositions();
    } else if (!night && starsActive) {
        starsActive = false;
        starCount = 0;
    }

    if (!starsActive) return;
    if (starCount < MAX_STARS && now - lastStarSpawn >= nextSpawnDelay) {
        stars[starCount].fadeInStart = now;
        stars[starCount].brightness = 255;
        ++starCount;
        lastStarSpawn = now;
        nextSpawnDelay = random(800, 3500);
    }
}

void Avatar::drawStars(M5Canvas& canvas) {
    if (!starsActive) return;
    canvas.setTextSize(1);
    canvas.setTextColor(drawColor());
    canvas.setTextDatum(top_left);
    const uint32_t now = millis();
    for (uint8_t i = 0; i < starCount; ++i) {
        const bool twinkle = stars[i].isBlinking && ((now + i * 650) % 3600) > 2900;
        canvas.drawChar(twinkle ? '*' : '.', stars[i].x, stars[i].y);
    }
}

void Avatar::fillPigBoundingBox(M5Canvas& canvas) {
    if (!starsActive || starCount == 0) return;
    int x = currentX - 28;
    int w = 155;
    if (x < 0) { w += x; x = 0; }
    if (x + w > 240) w = 240 - x;
    canvas.fillRect(x, 11, w, 84, bgColor());
}

void Avatar::setFacingLeft() { facingRight = false; }
void Avatar::setFacingRight() { facingRight = true; }

void Avatar::setAttackShake(bool active, bool strong) {
    attackShakeActive = active;
    attackShakeStrong = strong;
    attackShakeRefreshTime = active ? millis() : 0;
}

void Avatar::setThunderFlash(bool active) { thunderFlashActive = active; }
bool Avatar::isThunderFlashing() { return thunderFlashActive; }

void Avatar::startWindupSlide(int targetX, bool faceRight) {
    if (currentX == targetX) {
        facingRight = faceRight;
        return;
    }
    transitioning = true;
    transitionStartTime = millis();
    transitionFromX = currentX;
    transitionToX = targetX;
    transitionToFacingRight = faceRight;
    facingRight = faceRight;
}
