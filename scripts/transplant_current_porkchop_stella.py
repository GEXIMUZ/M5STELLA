#!/usr/bin/env python3
from pathlib import Path
import re
import urllib.request

ROOT = Path(__file__).resolve().parents[1]
DONOR_BASE = "https://raw.githubusercontent.com/0ct0sec/M5PORKCHOP/main"


def fetch(path: str) -> str:
    url = f"{DONOR_BASE}/{path}"
    print(f"Fetching donor: {url}")
    with urllib.request.urlopen(url, timeout=20) as r:
        return r.read().decode("utf-8")


def write(path: str, text: str):
    p = ROOT / path
    p.parent.mkdir(parents=True, exist_ok=True)
    p.write_text(text, encoding="utf-8")
    print(f"Wrote {path}")


def cpp_string(value: str) -> str:
    return '"' + value.replace('\\', '\\\\').replace('"', '\\"') + '"'


def emit_frame(name: str, lines) -> str:
    body = ",\n".join(f"    {cpp_string(line)}" for line in lines)
    return f"static const char* {name}[] = {{\n{body}\n}};\n"


# Exact artwork supplied in Stella_Animations(4).txt.
FRAMES = {
    "AVATAR_NEUTRAL_L": [" ^^   ", "(oo)  ", "(   )>"],
    "AVATAR_NEUTRAL_R": ["   ^^ ", "  (oo)", "<(   )"],
    "AVATAR_HAPPY_L": [" ^^   ", "(^^)  ", "(   )>"],
    "AVATAR_HAPPY_R": ["   ^^ ", "  (^^)", "<(   )"],
    "AVATAR_EXCITED_L": [" ^^   ", "($$)  ", "(   )>"],
    "AVATAR_EXCITED_R": ["   ^^ ", "  ($$)", "<(   )"],
    "AVATAR_HUNTING_L": [" ^^   ", "(==)  ", "(   )>"],
    "AVATAR_HUNTING_R": ["   ^^ ", "  (==)", "<(   )"],
    "AVATAR_SLEEPY_L": [" ^^   ", "(--)  ", "(   )>"],
    "AVATAR_SLEEPY_R": ["   ^^ ", "  (--)", "<(   )"],
    "AVATAR_SAD_L": [" ^^   ", "(**)  ", "(   )>"],
    "AVATAR_SAD_R": ["   ^^ ", "  (**)", "<(   )"],
    "AVATAR_ANGRY_L": [" ^^   ", "(@@)  ", "(   )>"],
    "AVATAR_ANGRY_R": ["   ^^ ", "  (@@)", "<(   )"],
    "STELLA_BLINK_L": [" ^^   ", "(><)  ", "(   )>"],
    "STELLA_BLINK_R": ["   ^^ ", "  (><)", "<(   )"],
    "STELLA_SNIFF_1_L": [" ^ ^  ", "(o.o) ", "(   )>"],
    "STELLA_SNIFF_1_R": ["  ^ ^ ", " (o.o)", "<(   )"],
    "STELLA_SNIFF_2_L": [" ^ ^  ", "(>.<) ", "(   )>"],
    "STELLA_SNIFF_2_R": ["  ^ ^ ", " (>.<)", "<(   )"],
    "STELLA_TAIL_WAG_1_L": [" ^^   ", "(oo) /", "(   )\\"],
    "STELLA_TAIL_WAG_1_R": ["   ^^ ", "\\ (oo)", "/(   )"],
    "STELLA_TAIL_WAG_2_L": [" ^^   ", "(oo) \\", "(   )/"],
    "STELLA_TAIL_WAG_2_R": ["   ^^ ", "/ (oo)", "\\(   )"],
}


# 1) Current upstream environment, verbatim.
weather = fetch("src/piglet/weather.cpp")
write("src/piglet/weather.cpp", weather)

# 2) Current upstream avatar engine, verbatim base.
avatar = fetch("src/piglet/avatar.cpp")

frame_order = [
    "AVATAR_NEUTRAL_R", "AVATAR_HAPPY_R", "AVATAR_EXCITED_R",
    "AVATAR_HUNTING_R", "AVATAR_SLEEPY_R", "AVATAR_SAD_R", "AVATAR_ANGRY_R",
    "AVATAR_NEUTRAL_L", "AVATAR_HAPPY_L", "AVATAR_EXCITED_L",
    "AVATAR_HUNTING_L", "AVATAR_SLEEPY_L", "AVATAR_SAD_L", "AVATAR_ANGRY_L",
    "STELLA_BLINK_R", "STELLA_BLINK_L",
    "STELLA_SNIFF_1_R", "STELLA_SNIFF_1_L",
    "STELLA_SNIFF_2_R", "STELLA_SNIFF_2_L",
    "STELLA_TAIL_WAG_1_R", "STELLA_TAIL_WAG_1_L",
    "STELLA_TAIL_WAG_2_R", "STELLA_TAIL_WAG_2_L",
]

stella_frames = (
    "// --- STELLA THE WARDOG: exact user-supplied 3-line artwork ---\n"
    "// Only mascot artwork changes. Porkchop owns environment, motion and timing.\n"
    + "\n".join(emit_frame(name, FRAMES[name]) for name in frame_order)
)

sprite_block = re.compile(
    r"// --- DERPY STYLE with direction ---.*?(?=void Avatar::init\(\))",
    re.S,
)
if not sprite_block.search(avatar):
    raise SystemExit("ERROR: donor sprite block not found; upstream avatar layout changed")
avatar = sprite_block.sub(lambda _: stella_frames + "\n", avatar, count=1)

# Only replace artwork selection. Donor decides when every state/animation occurs.
select_pattern = re.compile(
    r"    // Select frame based on state and direction \(blink modifies eye only, not ears\).*?"
    r"    drawFrame\(canvas, frame, 3, shouldBlink, facingRight, isSniffing\);",
    re.S,
)
select_replacement = '''    // Select Stella artwork; state/timing above remains exact donor Porkchop.
    const char** frame;
    bool shouldBlink = isBlinking && currentState != AvatarState::SLEEPY;

    if (isBlinking) {
        isBlinking = false;
    }

    if (isSniffing) {
        // Preserve donor sniff cadence; map the donor phases onto the two supplied Stella frames.
        const bool second = ((sniffFrame & 1U) != 0U);
        frame = facingRight
            ? (second ? STELLA_SNIFF_2_R : STELLA_SNIFF_1_R)
            : (second ? STELLA_SNIFF_2_L : STELLA_SNIFF_1_L);
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

    drawFrame(canvas, frame, 3, false, facingRight, false);'''
if not select_pattern.search(avatar):
    raise SystemExit("ERROR: donor frame-selection block not found; upstream avatar layout changed")
avatar = select_pattern.sub(lambda _: select_replacement, avatar, count=1)

# Replace exactly drawFrame(), bounded by the next donor function. This avoids
# deleting setGrassMoving(), drawGrass(), star/weather helpers, etc.
drawframe_pattern = re.compile(
    r"void Avatar::drawFrame\(M5Canvas& canvas, const char\*\* frame, uint8_t lines, bool blink, bool faceRight, bool sniff\) \{.*?\n\}\n\n(?=void Avatar::setGrassMoving)",
    re.S,
)
drawframe_replacement = '''void Avatar::drawFrame(M5Canvas& canvas, const char** frame, uint8_t lines, bool blink, bool faceRight, bool sniff) {
    // Donor Porkchop environment/animation mechanics preserved exactly.
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

    if (jumpActive && (now - jumpStartTime > JUMP_DURATION_MS)) {
        jumpActive = false;
    }

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

    // Only this part differs from Porkchop: draw the supplied Stella artwork
    // directly instead of mutating a pig snout/tail string.
    for (uint8_t i = 0; i < lines; i++) {
        canvas.drawString(frame[i], startX, startY + i * lineHeight);
    }

    // Keep donor grass rendering exactly where it originally happens.
    drawGrass(canvas);
}

'''
if not drawframe_pattern.search(avatar):
    raise SystemExit("ERROR: donor drawFrame boundary not found; upstream avatar layout changed")
avatar = drawframe_pattern.sub(lambda _: drawframe_replacement, avatar, count=1)

# Sanity guards: these donor functions MUST still exist after transplant.
required = [
    "void Avatar::setGrassMoving(bool moving, bool directionRight)",
    "void Avatar::setGrassSpeed(uint16_t ms)",
    "void Avatar::drawGrass(M5Canvas& canvas)",
    "bool Avatar::isNightTime()",
]
for marker in required:
    if marker not in avatar:
        raise SystemExit(f"ERROR: transplant removed donor function: {marker}")

write("src/piglet/avatar.cpp", avatar)

# 3) UI-only sync label; protocol/class names remain untouched.
sync_path = ROOT / "src/modes/pigsync_client.cpp"
if sync_path.exists():
    s = sync_path.read_text(encoding="utf-8")
    s2 = s.replace('"PIGSYNC OFFLINE"', '"W33Z SYNC OFFLINE"')
    if s2 != s:
        sync_path.write_text(s2, encoding="utf-8")
        print("Updated visible PIGSYNC OFFLINE -> W33Z SYNC OFFLINE")

print("\nDONE")
print("- weather.cpp: exact current upstream M5PORKCHOP")
print("- avatar.cpp: current upstream engine/environment + Stella artwork only")
print("- Stella artwork: exact Stella_Animations(4).txt")
print("- donor grass/weather/stars/bounce/movement/timing preserved")
print("- donor API sanity checks passed")
print("\nNext: pio run -e m5cardputer -t clean && pio run -e m5cardputer")
