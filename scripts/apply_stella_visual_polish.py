#!/usr/bin/env python3
from pathlib import Path
import re
import urllib.request

ROOT = Path(__file__).resolve().parents[1]
AVATAR = ROOT / "src/piglet/avatar.cpp"
SYNC = ROOT / "src/modes/pigsync_client.cpp"
WEATHER = ROOT / "src/piglet/weather.cpp"


def replace_once(text: str, old: str, new: str, label: str) -> str:
    if old in text:
        return text.replace(old, new, 1)
    if new in text:
        print(f"avatar: {label} already applied")
        return text
    raise RuntimeError(f"avatar: expected marker not found for {label}")


def replace_sprite(text: str, name: str, rows) -> str:
    pattern = rf'(?:static\s+)?const char\*\s+{re.escape(name)}\[\]\s*=\s*\{{.*?\n\}};'
    prefix = "static const char*" if name.startswith("STELLA_") else "const char*"
    body = ",\n".join(f'    "{row}"' for row in rows)
    repl = f"{prefix} {name}[] = {{\n{body}\n}};"
    new_text, count = re.subn(pattern, repl, text, count=1, flags=re.S)
    if count == 0:
        raise RuntimeError(f"avatar: sprite block {name} not found")
    return new_text


text = AVATAR.read_text(encoding="utf-8")

sprites = {
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
}

for name, rows in sprites.items():
    text = replace_sprite(text, name, rows)

# Add Stella's user-designed wag frames plus a tiny walk-squash frame.
if "STELLA_TAIL_WAG_1_L" not in text:
    insert_after = re.search(r'static const char\* STELLA_SNIFF_2_L\[\]\s*=\s*\{.*?\n\};', text, flags=re.S)
    if not insert_after:
        raise RuntimeError("avatar: could not locate sniff frame insertion point")
    extra = r'''

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

// Subtle one-character body compression while moving. This keeps Stella's
// supplied head art intact and recreates the compact Porkchop walk feel.
static const char* STELLA_WALK_L[] = {
    " ^^   ",
    "(oo)  ",
    "(  )> "
};

static const char* STELLA_WALK_R[] = {
    "   ^^ ",
    "  (oo)",
    " <(  )"
};'''
    pos = insert_after.end()
    text = text[:pos] + extra + text[pos:]

# Slow sniff animation: 180 ms per pose instead of the current hyper-fast 50 ms.
text = text.replace("((now - sniffStartTime) / 50) & 1", "((now - sniffStartTime) / 180) & 1")

# Restore the current upstream Porkchop scene geometry used by its weather layer.
text = re.sub(r'int startY = \d+ \+ shakeY;[^\n]*', 'int startY = 23 + shakeY;  // Porkchop weather-compatible baseline', text, count=1)
text = re.sub(r'int grassY = \d+;[^\n]*', 'int grassY = 91;  // Porkchop upstream grass baseline', text, count=1)

# Restore the original lightweight movement bounce if the local v0.1.6 pass removed it.
if "bouncePattern[4]" not in text:
    marker = """    } else if (attackShakeActive) {\n        const int amp = attackShakeStrong ? 6 : 4;\n        shakeY = (esp_random() % 2 == 0) ? amp : -amp;\n    }\n"""
    replacement = """    } else if (attackShakeActive) {\n        const int amp = attackShakeStrong ? 6 : 4;\n        shakeY = (esp_random() % 2 == 0) ? amp : -amp;\n    } else if (transitioning || grassMoving) {\n        static const int bouncePattern[4] = {0, -3, -1, -2};\n        int phase = (now / 80) % 4;\n        shakeY = bouncePattern[phase];\n    }\n"""
    text = replace_once(text, marker, replacement, "movement bounce")

# Select special animation overlays before the normal mood state.
old_select = """    } else if (shouldBlink) {\n        frame = facingRight ? STELLA_BLINK_R : STELLA_BLINK_L;\n    } else {\n        switch (currentState) {\n"""
new_select = """    } else if (shouldBlink) {\n        frame = facingRight ? STELLA_BLINK_R : STELLA_BLINK_L;\n    } else if ((transitioning || grassMoving) && (((now / 120) & 1) != 0)) {\n        frame = facingRight ? STELLA_WALK_R : STELLA_WALK_L;\n    } else if ((currentState == AvatarState::HAPPY || currentState == AvatarState::EXCITED) && !transitioning && !grassMoving) {\n        bool wagPhase = ((now / 220) & 1) != 0;\n        frame = wagPhase\n            ? (facingRight ? STELLA_TAIL_WAG_2_R : STELLA_TAIL_WAG_2_L)\n            : (facingRight ? STELLA_TAIL_WAG_1_R : STELLA_TAIL_WAG_1_L);\n    } else {\n        switch (currentState) {\n"""
if old_select in text:
    text = text.replace(old_select, new_select, 1)
elif "STELLA_WALK_R" in text and "wagPhase" in text:
    print("avatar: animation overlay selection already applied")
else:
    raise RuntimeError("avatar: frame selection marker not found")

AVATAR.write_text(text, encoding="utf-8")
print("avatar: compact Stella sprites + slower sniff + wag + walk squash + bounce applied")

# User-facing sync label only. Keep PigSyncMode/protocol identifiers for compatibility.
sync = SYNC.read_text(encoding="utf-8")
sync = sync.replace('"PIGSYNC OFFLINE"', '"W33Z SYNC OFFLINE"')
SYNC.write_text(sync, encoding="utf-8")
print("pigsync_client: visible PIGSYNC OFFLINE label changed to W33Z SYNC OFFLINE")

# Restore the real current upstream Porkchop weather implementation verbatim.
weather_url = "https://raw.githubusercontent.com/0ct0sec/M5PORKCHOP/main/src/piglet/weather.cpp"
print(f"weather: downloading upstream donor from {weather_url}")
with urllib.request.urlopen(weather_url, timeout=15) as response:
    weather = response.read().decode("utf-8")
if "namespace Weather" not in weather or "drawClouds" not in weather or "RAIN_DROP_COUNT" not in weather:
    raise RuntimeError("weather: downloaded donor file failed sanity checks")
WEATHER.write_text(weather, encoding="utf-8")
print("weather: restored exact upstream Porkchop clouds/rain/thunder/wind implementation")

print("\nPolish pass complete.")
print("Review with:")
print("  git diff -- src/piglet/avatar.cpp src/piglet/weather.cpp src/modes/pigsync_client.cpp")
print("Then build with:")
print("  pio run -e m5cardputer")
