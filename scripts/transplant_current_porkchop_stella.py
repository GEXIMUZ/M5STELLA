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


# ---------------------------------------------------------------------------
# 1) TAKE CURRENT UPSTREAM PORKCHOP ENVIRONMENT VERBATIM
# ---------------------------------------------------------------------------
weather = fetch("src/piglet/weather.cpp")
write("src/piglet/weather.cpp", weather)

# ---------------------------------------------------------------------------
# 2) TAKE CURRENT UPSTREAM PORKCHOP AVATAR ENGINE VERBATIM
#    THEN REPLACE ONLY THE MASCOT ART/RENDERING WITH STELLA.
# ---------------------------------------------------------------------------
avatar = fetch("src/piglet/avatar.cpp")

stella_frames = r'''// --- STELLA THE WARDOG: exact user-supplied 3-line frames ---
// Environment, motion, bounce, grass, stars, weather hooks and timing below
// remain from current upstream M5PORKCHOP.
const char* AVATAR_NEUTRAL_L[] = {
    " ^^   ",
    "(oo)  ",
    "(   )>"
};
const char* AVATAR_NEUTRAL_R[] = {
    "   ^^ ",
    "  (oo)",
    "<(   )"
};

const char* AVATAR_HAPPY_L[] = {
    " ^^   ",
    "(^^)  ",
    "(   )>"
};
const char* AVATAR_HAPPY_R[] = {
    "   ^^ ",
    "  (^^)",
    "<(   )"
};

const char* AVATAR_EXCITED_L[] = {
    " ^^   ",
    "($$)  ",
    "(   )>"
};
const char* AVATAR_EXCITED_R[] = {
    "   ^^ ",
    "  ($$)",
    "<(   )"
};

const char* AVATAR_HUNTING_L[] = {
    " ^^   ",
    "(==)  ",
    "(   )>"
};
const char* AVATAR_HUNTING_R[] = {
    "   ^^ ",
    "  (==)",
    "<(   )"
};

const char* AVATAR_SLEEPY_L[] = {
    " ^^   ",
    "(--)  ",
    "(   )>"
};
const char* AVATAR_SLEEPY_R[] = {
    "   ^^ ",
    "  (--)",
    "<(   )"
};

const char* AVATAR_SAD_L[] = {
    " ^^   ",
    "(**)  ",
    "(   )>"
};
const char* AVATAR_SAD_R[] = {
    "   ^^ ",
    "  (**)",
    "<(   )"
};

const char* AVATAR_ANGRY_L[] = {
    " ^^   ",
    "(@@)  ",
    "(   )>"
};
const char* AVATAR_ANGRY_R[] = {
    "   ^^ ",
    "  (@@)",
    "<(   )"
};

static const char* STELLA_BLINK_L[] = {
    " ^^   ",
    "(><)  ",
    "(   )>"
};
static const char* STELLA_BLINK_R[] = {
    "   ^^ ",
    "  (><)",
    "<(   )"
};

static const char* STELLA_SNIFF_1_L[] = {
    " ^ ^  ",
    "(o.o) ",
    "(   )>"
};
static const char* STELLA_SNIFF_1_R[] = {
    "  ^ ^ ",
    " (o.o)",
    "<(   )"
};
static const char* STELLA_SNIFF_2_L[] = {
    " ^ ^  ",
    "(>.<) ",
    "(   )>"
};
static const char* STELLA_SNIFF_2_R[] = {
    "  ^ ^ ",
    " (>.<)",
    "<(   )"
};

static const char* STELLA_TAIL_WAG_1_L[] = {
    " ^^   ",
    "(oo) /",
    "(   )\\"
};
static const char* STELLA_TAIL_WAG_1_R[] = {
    "   ^^ ",
    "\\ (oo)",
    "/(   )"
};
static const char* STELLA_TAIL_WAG_2_L[] = {
    " ^^   ",
    "(oo) \\",
    "(   )/"
};
static const char* STELLA_TAIL_WAG_2_R[] = {
    "   ^^ ",
    "/ (oo)",
    "\\(   )"
};
'''

# Replace only the donor ASCII frame declarations.
pattern = re.compile(
    r"// --- DERPY STYLE with direction ---.*?(?=void Avatar::init\(\))",
    re.S,
)
if not pattern.search(avatar):
    raise SystemExit("ERROR: donor sprite block not found; upstream layout changed")
avatar = pattern.sub(stella_frames + "\n", avatar, count=1)

# Keep Porkchop's sniff event duration/triggering, but render the two exact Stella
# sniff images instead of mutating pig nose characters. The cadence remains donor
# 100 ms; no environment/motion timing is changed.

# Replace donor frame-selection tail with Stella-aware selection while preserving
# all preceding upstream behavior and state updates.
select_pattern = re.compile(
    r"    // Select frame based on state and direction \(blink modifies eye only, not ears\).*?"
    r"    drawFrame\(canvas, frame, 3, shouldBlink, facingRight, isSniffing\);",
    re.S,
)
select_replacement = r'''    // Select Stella artwork only. All movement/state logic above is donor Porkchop.
    const char** frame;
    bool shouldBlink = isBlinking && currentState != AvatarState::SLEEPY;

    if (isBlinking) {
        isBlinking = false;
    }

    if (isSniffing) {
        // Exact supplied sniff frames; donor still owns when/why sniffing happens.
        bool second = ((sniffFrame & 1U) != 0U);
        if (facingRight) {
            frame = second ? STELLA_SNIFF_2_R : STELLA_SNIFF_1_R;
        } else {
            frame = second ? STELLA_SNIFF_2_L : STELLA_SNIFF_1_L;
        }
        shouldBlink = false;
    } else if (shouldBlink) {
        frame = facingRight ? STELLA_BLINK_R : STELLA_BLINK_L;
    } else if ((currentState == AvatarState::HAPPY || currentState == AvatarState::EXCITED) &&
               !transitioning && !grassMoving) {
        // Use only the exact supplied wag frames. This changes mascot art only.
        bool wag2 = ((millis() / 220U) & 1U) != 0U;
        if (facingRight) {
            frame = wag2 ? STELLA_TAIL_WAG_2_R : STELLA_TAIL_WAG_1_R;
        } else {
            frame = wag2 ? STELLA_TAIL_WAG_2_L : STELLA_TAIL_WAG_1_L;
        }
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
    raise SystemExit("ERROR: donor frame-selection block not found; upstream layout changed")
avatar = select_pattern.sub(select_replacement, avatar, count=1)

# Replace ONLY the pig-specific per-line drawing code inside drawFrame. We leave
# donor star update, bounding-box clearing, thunder colors, jump, attack shake,
# walk bounce, X/Y placement and every environment function untouched.
loop_pattern = re.compile(
    r"    for \(uint8_t i = 0; i < lines; i\+\+\) \{.*?\n    \}\n(?=\})",
    re.S,
)
loop_replacement = r'''    // Stella frames already contain the complete head/body/tail.
    // Do not apply Porkchop's pig-snout or z-tail mutations.
    for (uint8_t i = 0; i < lines; i++) {
        canvas.drawString(frame[i], startX, startY + i * lineHeight);
    }
'''
# Target the loop occurring after drawFrame, not arbitrary earlier loops.
draw_pos = avatar.find("void Avatar::drawFrame(")
if draw_pos < 0:
    raise SystemExit("ERROR: donor drawFrame not found")
prefix, draw_tail = avatar[:draw_pos], avatar[draw_pos:]
if not loop_pattern.search(draw_tail):
    raise SystemExit("ERROR: donor pig draw loop not found; upstream layout changed")
draw_tail = loop_pattern.sub(loop_replacement, draw_tail, count=1)
avatar = prefix + draw_tail

write("src/piglet/avatar.cpp", avatar)

# ---------------------------------------------------------------------------
# 3) UI-ONLY LEGACY SYNC LABEL. INTERNAL PIGSYNC PROTOCOL NAMES STAY UNCHANGED.
# ---------------------------------------------------------------------------
sync_path = ROOT / "src/modes/pigsync_client.cpp"
if sync_path.exists():
    s = sync_path.read_text(encoding="utf-8")
    s2 = s.replace('"PIGSYNC OFFLINE"', '"W33Z SYNC OFFLINE"')
    if s2 != s:
        sync_path.write_text(s2, encoding="utf-8")
        print("Updated visible PIGSYNC OFFLINE label -> W33Z SYNC OFFLINE")

print("\nDONE")
print("- src/piglet/weather.cpp = exact current upstream Porkchop")
print("- src/piglet/avatar.cpp = exact current upstream Porkchop engine + Stella art only")
print("- Stella frames = exact supplied Stella_Animations(4).txt artwork")
print("- no custom grass/cloud/weather/movement/timing transplant")
print("\nNext: pio run -e m5cardputer")
