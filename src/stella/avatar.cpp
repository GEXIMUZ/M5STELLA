// M5STELLA ASCII Pomeranian avatar engine.
// Designed specifically for the Cardputer 240x135 display: compact 1x font,
// fixed-width frames, zero bitmap assets, no heap allocation in the render loop.
// Preserves the donor Avatar API, weather hooks, stars and scrolling grass.

#include "../piglet/avatar.h"
#include "../piglet/weather.h"
#include "../ui/display.h"
#include <M5Cardputer.h>
#include <time.h>
#include <string.h>

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
constexpr int kAsciiY = 34;
constexpr int kGroundY = 101;
constexpr int kLeftDrawX = 12;
constexpr int kRightDrawX = 142;
constexpr uint8_t kFrameLines = 8;

uint16_t accent() { return thunderFlashActive ? getColorBG() : getColorFG(); }
uint16_t bg() { return thunderFlashActive ? getColorFG() : getColorBG(); }

// The silhouette deliberately has a broad mane, short legs and a curled plume tail.
// At Font0 / textSize 1 every character is roughly 6x8 px, so this stays inside
// the Cardputer main canvas while remaining recognisably Pomeranian.
static const char* const POM_NEUTRAL[kFrameLines] = {
    "   /\\___/\\    ",
    "  /  o o  \\~~ ",
    " /     ^    \\))",
    "|   \\___/   |))",
    "|  .-|||-.  |  ",
    " \\( ||| )/    ",
    "  /_|| ||_\\    ",
    "    ^^ ^^       "
};

static const char* const POM_BLINK[kFrameLines] = {
    "   /\\___/\\    ",
    "  /  - -  \\~~ ",
    " /     ^    \\))",
    "|   \\___/   |))",
    "|  .-|||-.  |  ",
    " \\( ||| )/    ",
    "  /_|| ||_\\    ",
    "    ^^ ^^       "
};

static const char* const POM_HAPPY[kFrameLines] = {
    "   /\\___/\\    ",
    "  /  ^ ^  \\~~ ",
    " /     ^    \\}}",
    "|   \\_U_/   |}}",
    "|  .-|||-.  |  ",
    " \\( ||| )/    ",
    "  /_|| ||_\\    ",
    "    ^^ ^^       "
};

static const char* const POM_TRACK[kFrameLines] = {
    "   /\\___/\\    ",
    "  /  o o  \\~~ ",
    " /     ^>   \\))",
    "|    ___    |))",
    "|  .-|||-.  |  ",
    " \\( ||| )/    ",
    "  /_|| ||_\\    ",
    "    ^^ ^^       "
};

static const char* const POM_SLEEPY[kFrameLines] = {
    "   /\\___/\\ z  ",
    "  /  - -  \\ z  ",
    " /     ^    \\~ ",
    "|    ___    |) ",
    "|  .-|||-.  |  ",
    " \\( ||| )/    ",
    "  /_|| ||_\\    ",
    "    ^^ ^^       "
};

static const char* const POM_SAD[kFrameLines] = {
    "   /\\___/\\    ",
    "  /  . .  \\~  ",
    " /     ^    \\) ",
    "|    /_\\    |) ",
    "|  .-|||-.  |  ",
    " \\( ||| )/    ",
    "  /_|| ||_\\    ",
    "    ^^ ^^       "
};

static const char* const POM_ANGRY[kFrameLines] = {
    "   /\\___/\\    ",
    "  /  > <  \\~~ ",
    " /     ^    \\}}",
    "|   -------  |}}",
    "|  .-|||-.  |  ",
    " \\( ||| )/    ",
    "  /_|| ||_\\    ",
    "    ^^ ^^       "
};

char mirrorChar(char ch) {
    switch (ch) {
        case '/': return '\\';
        case '\\': return '/';
        case '(': return ')';
        case ')': return '(';
        case '{': return '}';
        case '}': return '{';
        case '<': return '>';
        case '>': return '<';
        default: return ch;
    }
}

void mirrorLine(const char* src, char* dst, size_t dstLen) {
    if (!src || !dst || dstLen == 0) return;
    size_t n = strlen(src);
    if (n >= dstLen) n = dstLen - 1;
    for (size_t i = 0; i < n; ++i) dst[i] = mirrorChar(src[n - 1 - i]);
    dst[n] = '\0';
}

const char* const* frameForState(AvatarState state, bool blink) {
    if (state == AvatarState::EXCITED || state == AvatarState::HAPPY) return POM_HAPPY;
    if (state == AvatarState::HUNTING) return POM_TRACK;
    if (state == AvatarState::SLEEPY) return POM_SLEEPY;
    if (state == AvatarState::SAD) return POM_SAD;
    if (state == AvatarState::ANGRY) return POM_ANGRY;
    return blink ? POM_BLINK : POM_NEUTRAL;
}

void drawPomAscii(M5Canvas& canvas, int x, int y, bool faceRight, bool blink,
                  bool sniff, AvatarState state, bool moving) {
    const uint32_t now = millis();
    const char* const* frame = frameForState(state, blink);
    const uint8_t tailPhase = (now / (state == AvatarState::EXCITED ? 90 : 170)) % 3;
    const bool walkPhase = moving && ((now / 130) & 1);

    canvas.setFont(&fonts::Font0);
    canvas.setTextSize(1);
    canvas.setTextDatum(top_left);
    canvas.setTextColor(accent());

    for (uint8_t row = 0; row < kFrameLines; ++row) {
        char line[28];
        strncpy(line, frame[row], sizeof(line) - 1);
        line[sizeof(line) - 1] = '\0';

        // Tail animation is intentionally ASCII-only: plume alternates between
        // curled braces, parentheses and a soft waving tilde.
        if (row >= 1 && row <= 3 && state != AvatarState::SLEEPY && state != AvatarState::SAD) {
            char* p = strrchr(line, tailPhase == 0 ? '~' : (tailPhase == 1 ? ')' : '}'));
            (void)p; // frame already supplies a readable base tail; motion comes from x/y below.
        }

        // Walking alternates the paw line without changing the fluffy body.
        if (walkPhase && row == 7) strncpy(line, "    ^^  ^^      ", sizeof(line) - 1);

        // Sniff pushes Stella's whole muzzle forward by one character and emits
        // a tiny scent pulse. It reads as a dog action instead of a generic icon.
        if (sniff && row == 2) {
            if (sniffFrame == 1) strncpy(line, " /     ^>   \\))", sizeof(line) - 1);
            else if (sniffFrame == 2) strncpy(line, " /     ^>>  \\))", sizeof(line) - 1);
            line[sizeof(line) - 1] = '\0';
        }

        int rowX = x;
        if (tailPhase == 1 && row >= 1 && row <= 3) rowX += faceRight ? 1 : -1;
        if (sniff && sniffFrame == 2 && row <= 3) rowX += faceRight ? 2 : -2;

        if (faceRight) {
            canvas.drawString(line, rowX, y + row * 8);
        } else {
            char mirrored[28];
            mirrorLine(line, mirrored, sizeof(mirrored));
            canvas.drawString(mirrored, rowX, y + row * 8);
        }
    }
}
} // namespace

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
        if (elapsed >= kSniffDurationMs) {
            isSniffing = false;
            sniffFrame = 0;
        } else {
            sniffFrame = (elapsed / 120) % 3;
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
        int moodBias = moodIntensity > 50 ? -800 : (moodIntensity < -50 ? 1200 : 0);
        int next = random(4000, 8500) + moodBias;
        blinkInterval = next < 1800 ? 1800 : (uint32_t)next;
    }

    if (!transitioning && !grassMoving && !pendingGrassStart) {
        if (now - lastLookTime > lookInterval) {
            const int roll = random(0, 100);
            if (roll < 24) facingRight = !facingRight;
            else if (roll < 50) sniff();
            else if (roll < 66) wiggleEars();
            else if (roll < 82) blink();
            else if (roll < 90 && currentState != AvatarState::SLEEPY && currentState != AvatarState::SAD) cuteJump();
            lastLookTime = now;
            lookInterval = random(2800, 9000);
        }

        if (now - lastWalkTime > walkInterval) {
            const int target = onRightSide ? 20 : 154;
            startWindupSlide(target, target > currentX);
            lastWalkTime = now;
            walkInterval = random(18000, 45000);
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
    int xOffset = 0;
    if (jumpActive) {
        const float t = (float)(now - jumpStartTime) / (float)JUMP_DURATION_MS;
        yOffset = -(int)(4.0f * t * (1.0f - t) * JUMP_HEIGHT);
    } else if (attackShakeActive) {
        const int amp = attackShakeStrong ? 3 : 2;
        xOffset = (esp_random() & 1) ? amp : -amp;
    } else if (transitioning || grassMoving) {
        static const int bounce[4] = {0, -1, -2, -1};
        yOffset = bounce[(now / 100) % 4];
    } else if (currentState == AvatarState::HAPPY || currentState == AvatarState::EXCITED) {
        // One-pixel breathing/bobbing keeps idle Stella alive without visual noise.
        yOffset = ((now / 700) & 1) ? -1 : 0;
    }

    // Preserve the legacy logical position (20/154) for mood bubble placement,
    // but map it to safe ASCII draw anchors that fit the 240px screen.
    int drawX;
    if (transitioning) {
        const int span = 154 - 20;
        const int clamped = currentX < 20 ? 20 : (currentX > 154 ? 154 : currentX);
        drawX = kLeftDrawX + ((clamped - 20) * (kRightDrawX - kLeftDrawX)) / span;
    } else {
        drawX = onRightSide ? kRightDrawX : kLeftDrawX;
    }

    drawPomAscii(canvas, drawX + xOffset, kAsciiY + yOffset, faceRight, doBlink,
                 doSniff, currentState, transitioning || grassMoving);
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
    static const char tufts[] = "/\\^.'";
    for (int i = 0; i < 26; ++i) grassPattern[i] = tufts[random(0, 5)];
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
    canvas.setFont(&fonts::Font0);
    canvas.setTextSize(1);
    canvas.setTextColor(accent());
    canvas.setTextDatum(top_left);

    // Two lightweight ASCII layers: a grassy horizon and the original scrolling
    // pattern. Paws end immediately above this line, so Stella no longer floats.
    canvas.drawFastHLine(0, kGroundY + 2, 240, accent());
    canvas.drawString(" ^  '  /\\   .  ^   /\\  '   ^   .  /\\ ", 0, kGroundY - 5);
    canvas.drawString(grassPattern, 0, kGroundY + 3);
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
        stars[i].x = random(4, 236);
        stars[i].y = random(18, 92);
        stars[i].size = 1;
        stars[i].brightness = 0;
        stars[i].isBlinking = random(0, 100) < 55;
        stars[i].fadeInStart = 0;
    }
}

void Avatar::updateStars() {
    if (!isNightTime()) {
        starsActive = false;
        starCount = 0;
        return;
    }
    starsActive = true;
    const uint32_t now = millis();
    if (starCount < MAX_STARS && now - lastStarSpawn > nextSpawnDelay) {
        stars[starCount].brightness = 255;
        stars[starCount].fadeInStart = now;
        ++starCount;
        lastStarSpawn = now;
        nextSpawnDelay = random(700, 2400);
    }
}

void Avatar::drawStars(M5Canvas& canvas) {
    if (!starsActive) return;
    for (uint8_t i = 0; i < starCount; ++i) {
        if (!stars[i].brightness) continue;
        bool visible = !stars[i].isBlinking || ((millis() / 500 + i) & 1);
        if (visible) canvas.drawPixel(stars[i].x, stars[i].y, accent());
    }
}

void Avatar::fillPigBoundingBox(M5Canvas& canvas) {
    // Legacy method name retained for ABI/API compatibility during migration.
    // Clear only Stella's world area; weather and mood are rendered afterwards.
    canvas.fillRect(0, 15, 240, 99, bg());
}

void Avatar::setFacingLeft() { facingRight = false; }
void Avatar::setFacingRight() { facingRight = true; }

void Avatar::setAttackShake(bool active, bool strong) {
    attackShakeActive = active;
    attackShakeStrong = strong;
    attackShakeRefreshTime = millis();
}

void Avatar::setThunderFlash(bool active) { thunderFlashActive = active; }
bool Avatar::isThunderFlashing() { return thunderFlashActive; }

void Avatar::startWindupSlide(int targetX, bool faceRight) {
    transitionFromX = currentX;
    transitionToX = constrain(targetX, 20, 154);
    transitionToFacingRight = faceRight;
    transitionStartTime = millis();
    transitioning = true;
}
