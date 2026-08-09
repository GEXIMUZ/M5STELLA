// M5STELLA avatar implementation.
// IMPORTANT: the Porkchop movement/environment engine below is intentionally
// kept equivalent to the original donor implementation. Only the mascot art
// and pig-specific face/tail rendering are replaced with Stella sprites.

#include "../piglet/avatar.h"
#include "../piglet/weather.h"
#include "../ui/display.h"
#include <time.h>

AvatarState Avatar::currentState = AvatarState::NEUTRAL;
bool Avatar::isBlinking = false;
bool Avatar::earsUp = true;
uint32_t Avatar::lastBlinkTime = 0;
uint32_t Avatar::blinkInterval = 3000;
int Avatar::moodIntensity = 0;

bool Avatar::jumpActive = false;
uint32_t Avatar::jumpStartTime = 0;

bool Avatar::transitioning = false;
uint32_t Avatar::transitionStartTime = 0;
int Avatar::transitionFromX = 2;
int Avatar::transitionToX = 2;
bool Avatar::transitionToFacingRight = true;
int Avatar::currentX = 2;

bool Avatar::isSniffing = false;
static uint32_t sniffStartTime = 0;
static const uint32_t SNIFF_DURATION_MS = 600;
static uint8_t sniffFrame = 0;

static const uint32_t TRANSITION_DURATION_MS = 1200;
static uint32_t lastGrassStopTime = 0;
static const uint32_t GRASS_REST_COOLDOWN_MS = 3000;

static bool attackShakeActive = false;
static bool attackShakeStrong = false;
static uint32_t attackShakeRefreshTime = 0;
static bool thunderFlashActive = false;

Avatar::Star Avatar::stars[15] = {{0}};
uint8_t Avatar::starCount = 0;
uint32_t Avatar::lastStarSpawn = 0;
uint32_t Avatar::nextSpawnDelay = 2000;
bool Avatar::starsActive = false;
uint32_t Avatar::lastNightCheck = 0;
bool Avatar::cachedNightMode = false;

static uint16_t getDrawColor() {
    if (thunderFlashActive) return getColorBG();
    return getColorFG();
}

static uint16_t getBGColor() {
    if (thunderFlashActive) return getColorFG();
    return getColorBG();
}

bool Avatar::grassMoving = false;
bool Avatar::grassDirection = true;
bool Avatar::pendingGrassStart = false;
uint32_t Avatar::lastGrassUpdate = 0;
uint16_t Avatar::grassSpeed = 80;
char Avatar::grassPattern[32] = {0};

static bool facingRight = true;
static uint32_t lastFlipTime = 0;
static uint32_t flipInterval = 5000;
static uint32_t lastLookTime = 0;
static uint32_t lookInterval = 2000;
bool Avatar::onRightSide = false;

// ---------------------------------------------------------------------------
// Stella art only. Same compact 3-line footprint as original Porkchop.
// ---------------------------------------------------------------------------
const char* AVATAR_NEUTRAL_R[] = {
    "  ^ ^ ",
    " ( oo)",
    "<(   )"
};
const char* AVATAR_HAPPY_R[] = {
    "  ^ ^ ",
    " ( ^^)",
    "<(   )"
};
const char* AVATAR_EXCITED_R[] = {
    "  ^ ^ ",
    " ( $$)",
    "<(   )"
};
const char* AVATAR_HUNTING_R[] = {
    "  ^ ^ ",
    " ( ==)",
    "<(   )"
};
const char* AVATAR_SLEEPY_R[] = {
    "  ^ ^ ",
    " ( --)",
    "<(   )"
};
const char* AVATAR_SAD_R[] = {
    "  ^ ^ ",
    " ('~~)",
    "<(   )"
};
const char* AVATAR_ANGRY_R[] = {
    "  ^ ^ ",
    " ( @@)",
    "<(   )"
};

const char* AVATAR_NEUTRAL_L[] = {
    " ^ ^  ",
    "(oo ) ",
    "(   )>"
};
const char* AVATAR_HAPPY_L[] = {
    " ^ ^  ",
    "(^^ ) ",
    "(   )>"
};
const char* AVATAR_EXCITED_L[] = {
    " ^ ^  ",
    "($$ ) ",
    "(   )>"
};
const char* AVATAR_HUNTING_L[] = {
    " ^ ^  ",
    "(== ) ",
    "(   )>"
};
const char* AVATAR_SLEEPY_L[] = {
    " ^ ^  ",
    "(-- ) ",
    "(   )>"
};
const char* AVATAR_SAD_L[] = {
    " ^ ^  ",
    "(~~') ",
    "(   )>"
};
const char* AVATAR_ANGRY_L[] = {
    " ^ ^  ",
    "(@@ ) ",
    "(   )>"
};

static const char* STELLA_BLINK_R[] = {
    "  ^ ^ ",
    " ( ><)",
    "<(   )"
};
static const char* STELLA_BLINK_L[] = {
    " ^ ^  ",
    "(>< ) ",
    "(   )>"
};
static const char* STELLA_SNIFF_1_R[] = {
    "  ^ ^ ",
    " (o.o)",
    "<(   )"
};
static const char* STELLA_SNIFF_1_L[] = {
    " ^ ^  ",
    "(o.o) ",
    "(   )>"
};
static const char* STELLA_SNIFF_2_R[] = {
    "  ^ ^ ",
    " (>.<)",
    "<(   )"
};
static const char* STELLA_SNIFF_2_L[] = {
    " ^ ^  ",
    "(>.<) ",
    "(   )>"
};

void Avatar::init() {
    currentState = AvatarState::NEUTRAL;
    isBlinking = false;
    isSniffing = false;
    earsUp = true;
    lastBlinkTime = millis();
    blinkInterval = random(4000, 8000);

    bool startRight = random(0, 2) == 0;
    onRightSide = startRight;
    currentX = startRight ? 108 : 20;
    facingRight = !startRight;
    lastFlipTime = millis();
    flipInterval = random(25000, 50000);
    lastLookTime = millis();
    lookInterval = random(3000, 8000);

    grassMoving = false;
    grassDirection = true;
    pendingGrassStart = false;
    grassSpeed = 80;
    lastGrassUpdate = millis();
    lastGrassStopTime = 0;
    for (int i = 0; i < 26; i++) {
        grassPattern[i] = (random(0, 2) == 0) ? '/' : '\\';
    }
    grassPattern[26] = '\0';

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
    if (!isSniffing) sniffFrame = 0;
    isSniffing = true;
    sniffStartTime = millis();
}

void Avatar::cuteJump() {
    jumpActive = true;
    jumpStartTime = millis();
}

void Avatar::draw(M5Canvas& canvas) {
    uint32_t now = millis();

    if (isSniffing) {
        if (now - sniffStartTime > SNIFF_DURATION_MS) {
            isSniffing = false;
            sniffFrame = 0;
        } else {
            sniffFrame = ((now - sniffStartTime) / 100) % 3;
        }
    }

    if (transitioning) {
        uint32_t elapsed = now - transitionStartTime;
        if (elapsed >= TRANSITION_DURATION_MS) {
            transitioning = false;
            currentX = transitionToX;
            facingRight = transitionToFacingRight;
            onRightSide = (currentX > 60);

            if (pendingGrassStart) {
                grassMoving = true;
                pendingGrassStart = false;
                facingRight = !grassDirection;
            } else {
                int arrivalRoll = random(0, 100);
                if (arrivalRoll < 20) {
                    facingRight = !facingRight;
                } else if (arrivalRoll < 35) {
                    sniff();
                } else if (arrivalRoll < 45) {
                    wiggleEars();
                } else if (arrivalRoll < 55) {
                    facingRight = !transitionToFacingRight;
                }
            }
            lastLookTime = now;
            lookInterval = random(1500, 6000);
        } else {
            float t = (float)elapsed / TRANSITION_DURATION_MS;
            float smoothT = t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f);
            currentX = transitionFromX + (int)((transitionToX - transitionFromX) * smoothT);
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

    float flipMod = 1.0f - (moodIntensity / 300.0f);
    uint32_t minWalk = (uint32_t)(30000 * flipMod);
    uint32_t maxWalk = (uint32_t)(75000 * flipMod);
    uint32_t minLook = (uint32_t)(4000 * flipMod);
    uint32_t maxLook = (uint32_t)(15000 * flipMod);

    if (!transitioning && !grassMoving && !pendingGrassStart) {
        if (now - lastLookTime > lookInterval) {
            int lookRoll = random(0, 100);
            if (lookRoll < 35) {
                facingRight = !facingRight;
            } else if (lookRoll < 55) {
                facingRight = !facingRight;
                lookInterval = random(800, 1500);
                lastLookTime = now;
                goto skip_look_reset;
            } else if (lookRoll < 70) {
                facingRight = random(0, 2) == 0;
                sniff();
            } else if (lookRoll < 82) {
                wiggleEars();
            } else if (lookRoll < 90) {
                blink();
            }

            lastLookTime = now;
            if (random(0, 5) == 0) lookInterval = random(1500, 4000);
            else lookInterval = random(minLook, maxLook);
        }
        skip_look_reset:

        if (now - lastFlipTime > flipInterval) {
            int walkRoll = random(0, 100);
            int targetX;
            const int LEFT_EDGE = 20;
            const int RIGHT_EDGE = 108;

            if (walkRoll < 50) {
                targetX = onRightSide ? LEFT_EDGE : RIGHT_EDGE;
            } else if (walkRoll < 85) {
                targetX = random(0, 2) == 0 ? LEFT_EDGE : RIGHT_EDGE;
            } else if (walkRoll < 95) {
                if (onRightSide) targetX = random(85, 108);
                else targetX = random(20, 45);
            } else {
                facingRight = !facingRight;
                lastFlipTime = now;
                flipInterval = random(minWalk / 2, maxWalk / 2);
                goto skip_walk;
            }

            if (abs(targetX - currentX) > 15) {
                bool goingRight = targetX > currentX;
                transitioning = true;
                transitionStartTime = now;
                transitionFromX = currentX;
                transitionToX = targetX;
                transitionToFacingRight = goingRight;
                lastFlipTime = now;
                if (random(0, 4) == 0) flipInterval = random(15000, 30000);
                else flipInterval = random(minWalk, maxWalk);
            } else {
                facingRight = targetX > currentX;
                lastFlipTime = now;
                flipInterval = random(minWalk / 3, minWalk);
            }
        }
        skip_walk:;
    }

    const char** frame;
    bool shouldBlink = isBlinking && currentState != AvatarState::SLEEPY;
    if (isBlinking) isBlinking = false;

    if (isSniffing) {
        if (sniffFrame == 1) frame = facingRight ? STELLA_SNIFF_2_R : STELLA_SNIFF_2_L;
        else frame = facingRight ? STELLA_SNIFF_1_R : STELLA_SNIFF_1_L;
        shouldBlink = false;
    } else if (shouldBlink) {
        frame = facingRight ? STELLA_BLINK_R : STELLA_BLINK_L;
    } else {
        switch (currentState) {
            case AvatarState::HAPPY:
                frame = facingRight ? AVATAR_HAPPY_R : AVATAR_HAPPY_L; break;
            case AvatarState::EXCITED:
                frame = facingRight ? AVATAR_EXCITED_R : AVATAR_EXCITED_L; break;
            case AvatarState::HUNTING:
                frame = facingRight ? AVATAR_HUNTING_R : AVATAR_HUNTING_L; break;
            case AvatarState::SLEEPY:
                frame = facingRight ? AVATAR_SLEEPY_R : AVATAR_SLEEPY_L; break;
            case AvatarState::SAD:
                frame = facingRight ? AVATAR_SAD_R : AVATAR_SAD_L; break;
            case AvatarState::ANGRY:
                frame = facingRight ? AVATAR_ANGRY_R : AVATAR_ANGRY_L; break;
            default:
                frame = facingRight ? AVATAR_NEUTRAL_R : AVATAR_NEUTRAL_L; break;
        }
    }

    drawFrame(canvas, frame, 3, false, facingRight, false);
}

void Avatar::drawFrame(M5Canvas& canvas, const char** frame, uint8_t lines, bool, bool, bool) {
    // Exact original environment ordering: stars -> avatar clear -> mascot -> grass.
    updateStars();
    drawStars(canvas);
    fillPigBoundingBox(canvas);

    canvas.setTextDatum(top_left);
    canvas.setTextSize(3);
    canvas.setTextColor(getDrawColor());

    uint32_t now = millis();
    if (attackShakeRefreshTime == 0 || (now - attackShakeRefreshTime) > 250) {
        attackShakeActive = false;
        attackShakeStrong = false;
    }
    if (jumpActive && (now - jumpStartTime > JUMP_DURATION_MS)) jumpActive = false;

    int shakeY = 0;
    if (jumpActive) {
        uint32_t elapsed = now - jumpStartTime;
        float t = (float)elapsed / (float)JUMP_DURATION_MS;
        float arc = 4.0f * t * (1.0f - t);
        shakeY = -(int)(arc * JUMP_HEIGHT);
    } else if (attackShakeActive) {
        const int amp = attackShakeStrong ? 6 : 4;
        shakeY = (esp_random() % 2 == 0) ? amp : -amp;
    } else if (transitioning || grassMoving) {
        static const int bouncePattern[4] = {0, -3, -1, -2};
        int phase = (now / 80) % 4;
        shakeY = bouncePattern[phase];
    }

    int startX = currentX;
    int startY = 23 + shakeY;
    int lineHeight = 22;

    // Mascot-only change: draw Stella's supplied three lines verbatim.
    // No pig snout rewriting and no generated z-pigtail.
    for (uint8_t i = 0; i < lines; i++) {
        canvas.drawString(frame[i], startX, startY + i * lineHeight);
    }

    drawGrass(canvas);
}

void Avatar::setGrassMoving(bool moving, bool directionRight) {
    if (moving && (grassMoving || pendingGrassStart)) return;
    if (!moving && !grassMoving && !pendingGrassStart) return;

    if (moving) {
        uint32_t now = millis();
        if (lastGrassStopTime > 0 && (now - lastGrassStopTime) < GRASS_REST_COOLDOWN_MS) return;
        grassDirection = directionRight;
        int targetX = directionRight ? 108 : 20;

        if (transitioning) {
            if (transitionToX == 20) return;
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
        lastFlipTime = millis();
        startWindupSlide(20, false);
    }
}

void Avatar::setGrassSpeed(uint16_t ms) { grassSpeed = ms; }

void Avatar::setGrassPattern(const char* pattern) {
    strncpy(grassPattern, pattern, 26);
    grassPattern[26] = '\0';
}

void Avatar::resetGrassPattern() {
    for (int i = 0; i < 26; i++) {
        grassPattern[i] = (random(0, 2) == 0) ? '/' : '\\';
    }
    grassPattern[26] = '\0';
}

void Avatar::updateGrass() {
    if (!grassMoving) return;
    uint32_t now = millis();
    if (now - lastGrassUpdate < grassSpeed) return;
    lastGrassUpdate = now;

    if (grassDirection) {
        char last = grassPattern[25];
        for (int i = 25; i > 0; i--) grassPattern[i] = grassPattern[i - 1];
        grassPattern[0] = last;
    } else {
        char first = grassPattern[0];
        for (int i = 0; i < 25; i++) grassPattern[i] = grassPattern[i + 1];
        grassPattern[25] = first;
    }

    if (random(0, 30) == 0) {
        int pos = random(0, 26);
        grassPattern[pos] = (random(0, 2) == 0) ? '/' : '\\';
    }
}

void Avatar::drawGrass(M5Canvas& canvas) {
    updateGrass();
    canvas.setTextSize(2);
    canvas.setTextColor(getDrawColor());
    canvas.setTextDatum(top_left);
    int grassY = 91;
    canvas.drawString(grassPattern, 0, grassY);
}

bool Avatar::isNightTime() {
    uint32_t now = millis();
    if (now - lastNightCheck < 60000 && lastNightCheck != 0) return cachedNightMode;
    lastNightCheck = now;

    auto dt = M5.Rtc.getDateTime();
    if (dt.date.year >= 2024) {
        uint8_t hour = dt.time.hours;
        cachedNightMode = (hour >= 20 || hour < 6);
        return cachedNightMode;
    }

    time_t unixNow = time(nullptr);
    if (unixNow >= 1700000000) {
        struct tm timeinfo;
        localtime_r(&unixNow, &timeinfo);
        uint8_t hour = (uint8_t)timeinfo.tm_hour;
        cachedNightMode = (hour >= 20 || hour < 6);
        return cachedNightMode;
    }

    cachedNightMode = false;
    return false;
}

bool Avatar::areStarsActive() { return starsActive; }

void Avatar::initStarPositions() {
    for (uint8_t i = 0; i < MAX_STARS; i++) {
        stars[i].x = random(5, 235);
        stars[i].y = random(20, 88);
        stars[i].size = 1;
        stars[i].brightness = 0;
        stars[i].fadeInStart = 0;
        stars[i].isBlinking = (random(0, 100) < 20);
    }
}

void Avatar::updateStars() {
    uint32_t now = millis();

    if (Weather::isRaining()) {
        if (starsActive) {
            starsActive = false;
            starCount = 0;
        }
        return;
    }

    bool nightNow = isNightTime();
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

    if (starCount < MAX_STARS && (now - lastStarSpawn >= nextSpawnDelay)) {
        stars[starCount].fadeInStart = now;
        stars[starCount].brightness = 0;
        starCount++;
        lastStarSpawn = now;
        nextSpawnDelay = random(800, 4001);
    }

    for (uint8_t i = 0; i < starCount; i++) {
        uint32_t age = now - stars[i].fadeInStart;
        if (age < 500) stars[i].brightness = (age * 255) / 500;
        else stars[i].brightness = 255;
    }
}

void Avatar::fillPigBoundingBox(M5Canvas& canvas) {
    if (!starsActive || starCount == 0) return;

    int boxX = currentX - 25;
    int boxW = 155;
    int boxY = 11;
    int boxH = 84;

    if (boxX < 0) { boxW += boxX; boxX = 0; }
    if (boxX + boxW > 240) boxW = 240 - boxX;
    canvas.fillRect(boxX, boxY, boxW, boxH, getBGColor());
}

void Avatar::drawStars(M5Canvas& canvas) {
    if (!starsActive || starCount == 0) return;

    uint32_t now = millis();
    uint16_t fg = getDrawColor();
    canvas.setTextSize(1);
    canvas.setTextColor(fg);
    canvas.setTextDatum(top_left);

    for (uint8_t i = 0; i < starCount; i++) {
        if (stars[i].brightness < 128) continue;
        if (stars[i].y >= 88) continue;

        char starChar = '.';
        if (stars[i].isBlinking) {
            uint32_t phase = (now + i * 700) % 4000;
            if (phase >= 1700 && phase < 2300) starChar = '*';
        }
        canvas.drawChar(starChar, stars[i].x, stars[i].y);
    }
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
        transitionToX = targetX;
        transitionStartTime = millis();
        transitionToFacingRight = faceRight;
    }
    facingRight = faceRight;
}
