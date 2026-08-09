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
    """Return one safe C++ string literal."""
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


# ---------------------------------------------------------------------------
# 1) CURRENT UPSTREAM PORKCHOP ENVIRONMENT, VERBATIM
# ---------------------------------------------------------------------------
weather = fetch("src/piglet/weather.cpp")
write("src/piglet/weather.cpp", weather)

# ---------------------------------------------------------------------------
# 2) CURRENT UPSTREAM PORKCHOP AVATAR ENGINE, VERBATIM BASE
# ---------------------------------------------------------------------------
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

# Replace only Porkchop's artwork selection. Donor code still controls WHEN blink,
# sniff, movement, mood changes, bounce, jump and all random behavior happen.
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
        // Donor sniffFrame cadence is untouched. Map its phases to the two supplied Stella frames.
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

# Replace only the pig-specific line mutation in drawFrame. Everything before this
# loop (stars, clear box, thunder color, jump, shake, donor walk bounce, X/Y) remains.
loop_pattern = re.compile(
    r"    for \(uint8_t i = 0; i < lines; i\+\+\) \{.*?\n    \}\n(?=\})",
    re.S,
)
loop_replacement = '''    // Stella frames already contain complete head/body/tail artwork.
    for (uint8_t i = 0; i < lines; i++) {
        canvas.drawString(frame[i], startX, startY + i * lineHeight);
    }
'''
draw_pos = avatar.find("void Avatar::drawFrame(")
if draw_pos < 0:
    raise SystemExit("ERROR: donor drawFrame not found")
prefix, draw_tail = avatar[:draw_pos], avatar[draw_pos:]
if not loop_pattern.search(draw_tail):
    raise SystemExit("ERROR: donor pig draw loop not found; upstream avatar layout changed")
draw_tail = loop_pattern.sub(lambda _: loop_replacement, draw_tail, count=1)
avatar = prefix + draw_tail

write("src/piglet/avatar.cpp", avatar)

# ---------------------------------------------------------------------------
# 3) UI-ONLY LABEL. INTERNAL PIGSYNC PROTOCOL IS LEFT UNTOUCHED.
# ---------------------------------------------------------------------------
sync_path = ROOT / "src/modes/pigsync_client.cpp"
if sync_path.exists():
    s = sync_path.read_text(encoding="utf-8")
    s2 = s.replace('"PIGSYNC OFFLINE"', '"W33Z SYNC OFFLINE"')
    if s2 != s:
        sync_path.write_text(s2, encoding="utf-8")
        print("Updated visible PIGSYNC OFFLINE -> W33Z SYNC OFFLINE")

print("\nDONE")
print("- weather.cpp: exact current upstream M5PORKCHOP")
print("- avatar.cpp: exact current upstream engine; only pig artwork renderer replaced")
print("- Stella artwork: exact Stella_Animations(4).txt")
print("- no custom grass, clouds, weather, movement, bounce or timing")
print("\nNext: pio run -e m5cardputer -t clean && pio run -e m5cardputer")
