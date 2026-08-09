// M5STELLA compact Stella avatar engine.
// Keeps the original Porkchop presentation model: 3-line mascot, large terminal
// font, sliding/bounce movement, scrolling grass, weather hooks and night stars.
// Only the pig-specific avatar art/behavior is replaced with Stella the Pom.

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
uint32_t Avatar::blinkInterval = 3000;
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
uint32_t Avatar::nextSpawnDelay = 2000;
bool Avatar::starsActive = false;
uint32_t Avatar::lastNightCheck = 0;
bool Avatar::cachedNightMode = false;

namespace {
bool facingRight = true;
uint32_t lastLookTime = 0;
uint32_t lookInterval = 5000;
uint32_t lastWalkTime = 0;
uint32_t walkInterval = 30000;
uint32_t sniffStartTime = 0;
uint8_t sniffFrame = 0;
bool tailWagActive = false;
uint32_t tailWagStartTime = 0;
bool attackShakeActive = false;
bool attackShakeStrong = false;
uint32_t attackShakeRefreshTime = 0;
bool thunderFlashActive = false;
uint32_t lastGrassStopTime = 0;

constexpr uint32_t kSniffDurationMs = 600;
constexpr uint32_t kTailWagDurationMs = 720;
constexpr uint32_t kGrassCooldownMs = 3000;
constexpr uint32_t kTransitionMs = 1200;
constexpr int kLeftEdge = 20;
constexpr int kRightEdge = 108;
constexpr uint8_t kFrameLines = 3;

uint16_t drawColor() { return thunderFlashActive ? getColorBG() : getColorFG(); }
uint16_t bgColor() { return thunderFlashActive ? getColorFG() : getColorBG(); }

// ---------------------------------------------------------------------------
// STELLA DERPY POM SPRITES
// User-designed 3-line footprint, intentionally matching the original Porkchop
// scale. _L means Stella looks left; _R means Stella looks right.
// ---------------------------------------------------------------------------

static const char* const STELLA_NEUTRAL_L[kFrameLines] = {
    " ^ ^  ",
    "(oo ) ",
    "(   )>"
};
static const char* const STELLA_NEUTRAL_R[kFrameLines] = {
    "  ^ ^ ",
    " ( oo)",
    "<(   )"
};

static const char* const STELLA_HAPPY_L[kFrameLines] = {
    " ^ ^  ",
    "(^^ ) ",
    "(   )>"
};
static const char* const STELLA_HAPPY_R[kFrameLines] = {
    "  ^ ^ ",
    " ( ^^)",
    "<(   )"
};

static const char* const STELLA_EXCITED_L[kFrameLines] = {
    " ^ ^  ",
    "($$ ) ",
    "(   )>"
};
static const char* const STELLA_EXCITED_R[kFrameLines] = {
    "  ^ ^ ",
    " ( $$)",
    "<(   )"
};

static const char* const STELLA_HUNTING_L[kFrameLines] = {
    " ^ ^  ",
    "(== ) ",
    "(   )>"
};
static const char* const STELLA_HUNTING_R[kFrameLines] = {
    "  ^ ^ ",
    " ( ==)",
    "<(   )"
};

static const char* const STELLA_SLEEPY_L[kFrameLines] = {
    " ^ ^  ",
    "(-- ) ",
    "(   )>"
};
static const char* const STELLA_SLEEPY_R[kFrameLines] = {
    "  ^ ^ ",
    " ( --)",
    "<(   )"
};

static const char* const STELLA_SAD_L[kFrameLines] = {
    " ^ ^  ",
    "(~~') ",
    "(   )>"
};
static const char* const STELLA_SAD_R[kFrameLines] = {
    "  ^ ^ ",
    " ('~~)",
    "<(   )"
};

static const char* const STELLA_ANGRY_L[kFrameLines] = {
    " ^ ^  ",
    "(@@ ) ",
    "(   )>"
};
static const char* const STELLA_ANGRY_R[kFrameLines] = {
    "  ^ ^ ",
    " ( @@)",
    "<(   )"
};

static const char* const STELLA_BLINK_L[kFrameLines] = {
    " ^ ^  ",
    "(>< ) ",
    "(   )>"
};
static const char* const STELLA_BLINK_R[kFrameLines] = {
    "  ^ ^ ",
    " ( ><)",
    "<(   )"
};

static const char* const STELLA_SNIFF_1_L[kFrameLines] = {
    " ^ ^  ",
    "(o.o) ",
    "(   )>"
};
static const char* const STELLA_SNIFF_1_R[kFrameLines] = {
    "  ^ ^ ",
    " (o.o)",
    "<(   )"
};
static const char* const STELLA_SNIFF_2_L[kFrameLines] = {
    " ^ ^  ",
    "(>.<) ",
    "(   )>"
};
static const char* const STELLA_SNIFF_2_R[kFrameLines] = {
    "  ^ ^ ",
    " (>.<)",
    "<(   )"
};

static const char* const STELLA_TAIL_WAG_1_L[kFrameLines] = {
    " ^ ^  ",
    "(oo )/",
    "(   )\\"
};
static const char* const STELLA_TAIL_WAG_1_R[kFrameLines] = {
    "  ^ ^ ",
    "\\( oo)",
    "/(   )"
};
static const char* const STELLA_TAIL_WAG_2_L[kFrameLines] = {
    " ^ ^  ",
    "(oo )\\",
    "(   )/"
};
static const char* const STELLA_TAIL_WAG_2_R[kFrameLines] = {
    "  ^ ^ ",
    "/( oo)",
    "\\(   )"
};

const char* const* moodFrame(AvatarState state, bool right) {
    switch (state) {
        case AvatarState::HAPPY:   return right ? STELLA_HAPPY_R : STELLA_HAPPY_L;
        case AvatarState::EXCITED: return right ? STELLA_EXCITED_R : STELLA_EXCITED_L;
        case AvatarState::HUNTING: return right ? STELLA_HUNTING_R : STELLA_HUNTING_L;
        case AvatarState::SLEEPY:  return right ? STELLA_SLEEPY_R : STELLA_SLEEPY_L;
        case AvatarState::SAD:     return right ? STELLA_SAD_R : STELLA_SAD_L;
        case AvatarState::ANGRY:   return right ? STELLA_ANGRY_R : STELLA_ANGRY_L;
        default:                   return right ? STELLA_NEUTRAL_R : STELLA_NEUTRAL_L;
    }
}

const char* const* renderFrame(AvatarState state, bool right, bool blink, bool sniff) {
    if (sniff) {
        if (sniffFrame == 0) return right ? STELLA_SNIFF_1_R : STELLA_SNIFF_1_L;
        if (sniffFrame == 1) return right ? STELLA_SNIFF_2_R : STELLA_SNIFF_2_L;
        return right ? STELLA_SNIFF_1_R : STELLA_SNIFF_1_L;
    }
    if (blink && state != AvatarState::SLEEPY) {
        return right ? STELLA_BLINK_R : STELLA_BLINK_L;
    }

    // Happy/excited Stella wags continuously; neutral Stella occasionally wags
    // when the organic behavior engine triggers a short wag event.
    const bool stateWag = state == AvatarState::HAPPY || state == AvatarState::EXCITED;
    if ((stateWag || tailWagActive) && state != AvatarState::SLEEPY && state != AvatarState::SAD) {
        bool phase = ((millis() / (state == AvatarState::EXCITED ? 90 : 150)) & 1) != 0;
        if (phase) return right ? STELLA_TAIL_WAG_1_R : STELLA_TAIL_WAG_1_L;
        return right ? STELLA_TAIL_WAG_2_R : STELLA_TAIL_WAG_2_L;
    }

    return moodFrame(state, right);
}
} // namespace

void Avatar::init() {
    currentState = AvatarState::NEUTRAL;
    isBlinking = false;
    isSniffing = false;
    earsUp = true;
    lastBlinkTime = millis();
    blinkInterval = random(4000, 8000);

    bool startRight = random(0, 2) == 0;
    onRightSide = startRight;
    currentX = startRight ? kRightEdge : kLeftEdge;
    facingRight = !startRight; // face toward centre

    transitioning = false;
    grassMoving = false;
    pendingGrassStart = false;
    grassDirection = true;
    grassSpeed = 80;
    lastGrassUpdate = millis();
    lastGrassStopTime = 0;

    lastLookTime = millis();
    lookInterval = random(3000, 8000);
    lastWalkTime = millis();
    walkInterval = random(25000, 50000);

    resetGrassPattern();
    starsActive = false;
    starCount = 0;
    lastStarSpawn = 0;
    nextSpawnDelay = 2000;
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
            sniffFrame = (elapsed / 100) % 3;
        }
    }

    if (tailWagActive && now - tailWagStartTime >= kTailWagDurationMs) {
        tailWagActive = false;
    }

    if (transitioning) {
        const uint32_t elapsed = now - transitionStartTime;
        if (elapsed >= kTransitionMs) {
            transitioning = false;
            currentX = transitionToX;
            facingRight = transitionToFacingRight;
            onRightSide = currentX > 60;

            if (pendingGrassStart) {
                pendingGrassStart = false;
                grassMoving = true;
                facingRight = !grassDirection;
            } else {
                const int arrivalRoll = random(0, 100);
                if (arrivalRoll < 20) facingRight = !facingRight;
                else if (arrivalRoll < 35) sniff();
                else if (arrivalRoll < 48) {
                    tailWagActive = true;
                    tailWagStartTime = now;
                }
            }
            lastLookTime = now;
            lookInterval = random(1500, 6000);
        } else {
            const float t = (float)elapsed / (float)kTransitionMs;
            const float smooth = t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f);
            currentX = transitionFromX + (int)((transitionToX - transitionFromX) * smooth);
        }
    }

    float blinkMod = 1.0f - (moodIntensity / 200.0f);
    uint32_t minBlink = (uint32_t)(4000 * blinkMod);
    uint32_t maxBlink = (uint32_t)(8000 * blinkMod);
    if (now - lastBlinkTime > blinkInterval) {
        isBlinking = true;
        lastBlinkTime = now;
        blinkInterval = random(minBlink, maxBlink);
    }

    float moveMod = 1.0f - (moodIntensity / 300.0f);
    uint32_t minWalk = (uint32_t)(30000 * moveMod);
    uint32_t maxWalk = (uint32_t)(75000 * moveMod);
    uint32_t minLook = (uint32_t)(4000 * moveMod);
    uint32_t maxLook = (uint32_t)(15000 * moveMod);

    if (!transitioning && !grassMoving && !pendingGrassStart) {
        if (now - lastLookTime > lookInterval) {
            const int roll = random(0, 100);
            if (roll < 30) facingRight = !facingRight;
            else if (roll < 50) sniff();
            else if (roll < 64) {
                tailWagActive = true;
                tailWagStartTime = now;
            } else if (roll < 76) wiggleEars();
            else if (roll < 88) blink();
            else if (currentState != AvatarState::SLEEPY && currentState != AvatarState::SAD) cuteJump();

            lastLookTime = now;
            lookInterval = random(minLook, maxLook);
        }

        if (now - lastWalkTime > walkInterval) {
            int targetX;
            int walkRoll = random(0, 100);
            if (walkRoll < 60) targetX = onRightSide ? kLeftEdge : kRightEdge;
            else targetX = random(0, 2) == 0 ? kLeftEdge : kRightEdge;

            if (abs(targetX - currentX) > 15) {
                startWindupSlide(targetX, targetX > currentX);
            } else {
                facingRight = !facingRight;
            }
            lastWalkTime = now;
            walkInterval = random(minWalk, maxWalk);
        }
    }

    const bool doBlink = isBlinking && currentState != AvatarState::SLEEPY;
    isBlinking = false;
    drawFrame(canvas, nullptr, 0, doBlink, facingRight, isSniffing);
}

void Avatar::drawFrame(M5Canvas& canvas, const char**, uint8_t, bool doBlink, bool faceRight, bool doSniff) {
    updateStars();
    drawStars(canvas);
    fillPigBoundingBox(canvas); // legacy API name; only clears mascot footprint

    const uint32_t now = millis();
    if (attackShakeRefreshTime == 0 || now - attackShakeRefreshTime > 250) {
        attackShakeActive = false;
        attackShakeStrong = false;
    }
    if (jumpActive && now - jumpStartTime > JUMP_DURATION_MS) jumpActive = false;

    int shakeY = 0;
    if (jumpActive) {
        const uint32_t elapsed = now - jumpStartTime;
        const float t = (float)elapsed / (float)JUMP_DURATION_MS;
        const float arc = 4.0f * t * (1.0f - t);
        shakeY = -(int)(arc * JUMP_HEIGHT);
    } else if (attackShakeActive) {
        const int amp = attackShakeStrong ? 6 : 4;
        shakeY = (esp_random() & 1) ? amp : -amp;
    } else if (transitioning || grassMoving) {
        static const int bouncePattern[4] = {0, -3, -1, -2};
        shakeY = bouncePattern[(now / 80) % 4];
    }

    const char* const* frame = renderFrame(currentState, faceRight, doBlink, doSniff);

    canvas.setTextDatum(top_left);
    canvas.setTextSize(3);
    canvas.setTextColor(drawColor());

    const int startX = currentX;
    const int startY = 23 + shakeY;
    const int lineHeight = 22;
    for (uint8_t i = 0; i < kFrameLines; ++i) {
        canvas.drawString(frame[i], startX, startY + i * lineHeight);
    }

    drawGrass(canvas);
}

void Avatar::setGrassMoving(bool moving, bool directionRight) {
    if (moving && (grassMoving || pendingGrassStart)) return;
    if (!moving && !grassMoving && !pendingGrassStart) return;

    if (moving) {
        const uint32_t now = millis();
        if (lastGrassStopTime && now - lastGrassStopTime < kGrassCooldownMs) return;
        grassDirection = directionRight;
        const int targetX = directionRight ? kRightEdge : kLeftEdge;

        if (transitioning) {
            if (transitionToX == kLeftEdge) return;
            pendingGrassStart = true;
            grassMoving = false;
        } else if (currentX != targetX) {
            startWindupSlide(targetX, directionRight);
            pendingGrassStart = true;
            grassMoving = false;
        } else {
            facingRight = !directionRight;
            grassMoving = true;
            pendingGrassStart = false;
        }
        lastGrassStopTime = 0;
    } else {
        grassMoving = false;
        pendingGrassStart = false;
        lastGrassStopTime = millis();
        lastWalkTime = millis();
        startWindupSlide(kLeftEdge, false);
    }
}

void Avatar::setGrassSpeed(uint16_t ms) { grassSpeed = ms; }

void Avatar::setGrassPattern(const char* pattern) {
    if (!pattern) return;
    strncpy(grassPattern, pattern, 26);
    grassPattern[26] = '\0';
}

void Avatar::resetGrassPattern() {
    for (int i = 0; i < 26; ++i) grassPattern[i] = random(0, 2) == 0 ? '/' : '\\';
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

    if (random(0, 30) == 0) {
        const int pos = random(0, 26);
        grassPattern[pos] = random(0, 2) == 0 ? '/' : '\\';
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
        const uint8_t hour = dt.time.hours;
        cachedNightMode = hour >= 20 || hour < 6;
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

    const bool nightNow = isNightTime();
    if (nightNow && !starsActive) {
        starsActive = true;
        starCount = 0;
        lastStarSpawn = now;
        nextSpawnDelay = random(800, 4001);
        initStarPositions();
    } else if (!nightNow && starsActive) {
        starsActive = false;
        starCount = 0;
    }
    if (!starsActive) return;

    if (starCount < MAX_STARS && now - lastStarSpawn >= nextSpawnDelay) {
        stars[starCount].fadeInStart = now;
        stars[starCount].brightness = 0;
        ++starCount;
        lastStarSpawn = now;
        nextSpawnDelay = random(800, 4001);
    }

    for (uint8_t i = 0; i < starCount; ++i) {
        const uint32_t age = now - stars[i].fadeInStart;
        stars[i].brightness = age < 500 ? (age * 255) / 500 : 255;
    }
}

void Avatar::drawStars(M5Canvas& canvas) {
    if (!starsActive || starCount == 0) return;
    canvas.setTextSize(1);
    canvas.setTextColor(drawColor());
    canvas.setTextDatum(top_left);

    const uint32_t now = millis();
    for (uint8_t i = 0; i < starCount; ++i) {
        if (stars[i].brightness < 128 || stars[i].y >= 88) continue;
        char star = '.';
        if (stars[i].isBlinking) {
            const uint32_t phase = (now + i * 700) % 4000;
            if (phase >= 1700 && phase < 2300) star = '*';
        }
        canvas.drawChar(star, stars[i].x, stars[i].y);
    }
}

void Avatar::fillPigBoundingBox(M5Canvas& canvas) {
    // Keep legacy API name for compatibility. Clear only the moving mascot area,
    // preserving the original environment instead of repainting the whole world.
    int boxX = currentX - 20;
    int boxW = 155;
    int boxY = 11;
    int boxH = 82;
    if (boxX < 0) { boxW += boxX; boxX = 0; }
    if (boxX + boxW > 240) boxW = 240 - boxX;
    canvas.fillRect(boxX, boxY, boxW, boxH, bgColor());
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
    if (currentX != targetX) {
        transitioning = true;
        transitionFromX = currentX;
        transitionToX = constrain(targetX, kLeftEdge, kRightEdge);
        transitionStartTime = millis();
        transitionToFacingRight = faceRight;
    }
    facingRight = faceRight;
}
