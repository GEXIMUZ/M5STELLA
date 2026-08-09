#!/usr/bin/env python3
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def replace_many(path, replacements):
    p = ROOT / path
    text = p.read_text(encoding="utf-8")
    original = text
    changed = 0
    missing = []
    for old, new in replacements:
        if old in text:
            text = text.replace(old, new)
            changed += 1
        elif new not in text:
            missing.append(old)
    if text != original:
        p.write_text(text, encoding="utf-8")
    print(f"{path}: {changed} replacement groups applied")
    if missing:
        print(f"  note: {len(missing)} expected markers were not found (possibly already migrated or donor changed)")


replace_many("src/stella/identity.h", [
    ('static constexpr const char* Bite = "BITE";', 'static constexpr const char* Bite = "WIFI ATTACK";'),
    ('static constexpr const char* Sniff = "SNIFF";', 'static constexpr const char* Sniff = "PASSIVE WIFI";'),
    ('static constexpr const char* Patrol = "PATROL";', 'static constexpr const char* Patrol = "WARDRIVING";'),
    ('static constexpr const char* BluePaws = "BLUE PAWS";', 'static constexpr const char* BluePaws = "BLE TOOLS";'),
    ('static constexpr const char* Airwatch = "AIRWATCH";', 'static constexpr const char* Airwatch = "SPECTRUM";'),
    ('static constexpr const char* PackLink = "PACK LINK";', 'static constexpr const char* PackLink = "W33Z SYNC";'),
    ('static constexpr const char* Howl = "HOWL";', 'static constexpr const char* Howl = "RF BEACON";'),
    ('static constexpr const char* FriendlyPack = "FRIENDLY PACK";', 'static constexpr const char* FriendlyPack = "TRUSTED NETS";'),
    ('static constexpr const char* WardogStats = "WARDOG STATS";', 'static constexpr const char* WardogStats = "STATS";'),
])

replace_many("src/ui/menu.cpp", [
    ('"ASSESS"', '"ATTACK"'),
    ('"SNIFF"', '"RECON"'),
    ('"FETCH"', '"CAPTURES"'),
    ('"WARDOG"', '"PROGRESS"'),
    ('"PACK"', '"COMMS"'),
    ('"COLLAR"', '"SYSTEM"'),
    ('"TRAILS"', '"GPS TRACKS"'),
    ('"MARKS"', '"TARGETS"'),
    ('"FETCH IO"', '"FILE TRANSFER"'),
    ('"BADGES"', '"ACHIEVEMENTS"'),
    ('"TRICKS"', '"UNLOCKABLES"'),
    ('"VITALS"', '"DIAGNOSTICS"'),
    ('"NAP/CHARGE"', '"CHARGING"'),
    ('"WARDOG TOOLS. HANDLE WITH INTENT."', '"ACTIVE WIFI AND BLE ASSESSMENT TOOLS."'),
    ('"AUTHORIZED RANGE. SHARP TEETH."', '"USE ONLY ON AUTHORIZED NETWORKS."'),
    ('"CONTROL THE AIR. KEEP YOUR SCOPE."', '"SELECT AN ACTIVE TEST MODE."'),
    ('"NOSE UP. EARS OPEN. TX QUIET."', '"PASSIVE WIFI, GPS AND RF ANALYSIS."'),
    ('"SNIFF FIRST. BITE ONLY IN SCOPE."', '"OBSERVE NETWORKS WITHOUT ACTIVE ATTACKS."'),
    ('"READ THE AIR BEFORE YOU TOUCH IT."', '"SELECT A RECONNAISSANCE MODE."'),
    ('"CAPTURES, TRACKS AND FINDINGS."', '"CAPTURES, GPS TRACKS AND TARGET NOTES."'),
    ('"FETCH THE DATA. KEEP THE EVIDENCE."', '"REVIEW DATA COLLECTED IN THE FIELD."'),
    ('"EVERY GOOD DOG BRINGS SOMETHING HOME."', '"OPEN SAVED RESULTS."'),
    ('"WARDOG XP. FIELD CRED. BADGES."', '"XP, ACHIEVEMENTS AND UNLOCKABLES."'),
    ('"GOOD DOGS LEARN. GREAT DOGS LOG."', '"REVIEW LONG-TERM DEVICE PROGRESS."'),
    ('"PROGRESS SAVED. EGO OPTIONAL."', '"OPEN PROGRESS DATA."'),
    ('"PACK LINK TO W33Z AND FRIENDS."', '"W33Z SYNC, RF BEACON AND FILE TRANSFER."'),
    ('"FETCH, DELIVER, SYNC, REPEAT."', '"MOVE DATA BETWEEN STELLA AND OTHER SYSTEMS."'),
    ('"ON LEASH WHEN W33Z IS LISTENING."', '"SELECT A COMMUNICATION TOOL."'),
    ('"COLLAR, HEALTH, STORAGE, DIAGNOSTICS."', '"SETTINGS, STORAGE AND DIAGNOSTICS."'),
    ('"KEEP STELLA SHARP AND FED WITH POWER."', '"MANAGE DEVICE CONFIGURATION AND HEALTH."'),
    ('"CHECK THE DOG BEFORE BLAMING THE AIR."', '"SELECT A SYSTEM TOOL."'),
    ('"BITE MODE: AUTHORIZED ACTIVE ASSESSMENT."', '"ACTIVE WIFI ASSESSMENT."'),
    ('"TEETH OUT ONLY INSIDE YOUR SCOPE."', '"USE ONLY INSIDE YOUR AUTHORIZED SCOPE."'),
    ('"BLUE PAWS: BLE RECON AND TOOLING."', '"BLUETOOTH LOW ENERGY TOOLS."'),
    ('"SNIFF THE SHORT-RANGE PACK."', '"SCAN AND ANALYZE BLE DEVICES."'),
    ('"BLE AIRSPACE, NOW WITH MORE PAWS."', '"SHORT-RANGE WIRELESS RECON."'),
    ('"SNIFF: PASSIVE WIFI INTELLIGENCE."', '"PASSIVE WIFI INTELLIGENCE."'),
    ('"ZERO TX. MAXIMUM NOSE."', '"NO ACTIVE ATTACK TRAFFIC."'),
    ('"QUIET DOG. LOUD DATA."', '"MONITOR NETWORK ACTIVITY QUIETLY."'),
    ('"PATROL: GPS WARDIVING IN MOTION."', '"GPS-ASSISTED WIFI WARDRIVING."'),
    ('"TRACK STREETS. MARK RF TERRITORY."', '"LOG NETWORKS AND LOCATION DATA."'),
    ('"STELLA WALKS. W33Z REMEMBERS."', '"EXPORT TRACKS FOR LATER ANALYSIS."'),
    ('"AIRWATCH: READ CHANNEL PRESSURE."', '"2.4 GHZ SPECTRUM AND CHANNEL VIEW."'),
    ('"SEE THE RF NOISE BEFORE IT BITES."', '"INSPECT CHANNEL UTILIZATION AND NOISE."'),
    ('"CHANNELS TALK. STELLA LISTENS."', '"ANALYZE RF ACTIVITY."'),
    ('"FOLLOW THE PAWPRINTS ON THE MAP."', '"REVIEW RECORDED WARDRIVING ROUTES."'),
    ('"PACK LINK: STELLA <-> W33Z."', '"SYNC STELLA WITH W33Z."'),
    ('"FETCH DATA. DELIVER CONFIG."', '"UPLOAD DATA AND RECEIVE CONFIGURATION."'),
    ('"W33Z.GEXZ.BE IS THE HOME KENNEL."', '"CONTROL PLANE: W33Z.GEXZ.BE."'),
    ('"HOWL: RF BEACON AND SIGNAL TOOLING."', '"RF BEACON AND SIGNAL TOOLING."'),
    ('"LOUD DOG. CONTROLLED RANGE."', '"CONTROLLED-RANGE RF TESTING."'),
    ('"FETCH FILES OFF STELLA."', '"TRANSFER FILES TO OR FROM STELLA."'),
    ('"DELIVER CAPTURES TO W33Z."', '"MOVE CAPTURES, LOGS AND CONFIG FILES."'),
    ('"MOVE DATA WITHOUT LOSING THE TRAIL."', '"LOCAL FILE MANAGEMENT."'),
    ('"BADGES FOR ACTUAL FIELD MILESTONES."', '"ACHIEVEMENTS FOR FIELD MILESTONES."'),
    ('"GOOD DOG CERTIFICATES, BASICALLY."', '"TRACK COMPLETED OBJECTIVES."'),
    ('"UNLOCKED BY WORK, NOT BY OINKING."', '"UNLOCKED THROUGH DEVICE ACTIVITY."'),
    ('"STELLA UNLOCKS AND COLLECTIBLES."', '"UNLOCKABLE FEATURES AND COLLECTIBLES."'),
    ('"NEW TRICKS FOR A BUSY WARDOG."', '"REVIEW UNLOCKED CONTENT."'),
    ('"THE PAWPRINT ARCHIVE."', '"PROGRESSION REWARDS."'),
    ('"COLLAR SETTINGS FOR YOUR WARDOG."', '"DEVICE SETTINGS."'),
    ('"TUNE IT. TEST IT. KEEP IT STABLE."', '"CONFIGURE RADIOS, SERVICES AND POWER."'),
    ('"PERSONALITY, RADIOS, LINK AND POWER."', '"REVIEW SYSTEM PREFERENCES."'),
    ('"FRIENDLY PACK: NEVER BITE THESE."', '"TRUSTED NETWORK EXCLUSIONS."'),
    ('"TRUSTED BSSIDS, SSIDS AND DEVICES."', '"MANAGE TRUSTED BSSIDS AND SSIDS."'),
    ('"GOOD DOGS KNOW THEIR FRIENDS."', '"EXCLUDED TARGETS ARE NOT ATTACKED."'),
    ('"WARDOG HEALTH AND LIVE DIAGNOSTICS."', '"DEVICE HEALTH AND LIVE DIAGNOSTICS."'),
    ('"CHECK VITALS BEFORE A LONG PATROL."', '"CHECK SYSTEM STATE AND RESOURCES."'),
    ('"NAP MODE. BATTERY GETS THE BED."', '"CHARGING AND LOW-POWER MODE."'),
    ('"PLUG IN. CURL UP. RECHARGE."', '"MONITOR BATTERY AND CHARGE STATE."'),
    ('"EVEN WARDOGS NEED A NAP."', '"REDUCE ACTIVITY WHILE CHARGING."'),
    ('"FOX POMERANIAN ENERGY. RF BRAIN."', '"M5CARDPUTER SECURITY FIELD PLATFORM."'),
    ('"WOOF PROTOCOL ACTIVE."', '"FIRMWARE, BUILD AND PROJECT INFO."'),
])

replace_many("src/core/xp.cpp", [
    ('"SH0AT"', '"SC0UT"'),
    ('"M1TM B0AR"', '"M1TM 0P"'),
    ('"R00T BR1STL3"', '"R00T 0P"'),
    ('"B4C0NM4NC3R"', '"W4RD0G"'),
    ('"SH4D0W_H4M"', '"SH4D0W_0P"'),
    ('"P4C1F1ST_P0RK"', '"P4C1F1ST_0P"'),
    ('"BACON N00B"', '"RF N00B"'),
    ('"0INK Z3R0"', '"S1GN4L Z3R0"'),
    ('"SCRIP7 H4M"', '"SCRIP7 K1D"'),
    ('"P1N6 P1GL3T"', '"P1NG SC0UT"'),
    ('"PR0B3 P0RK"', '"PR0B3 SC0UT"'),
    ('"CH4N CH0P"', '"CH4N H0PP3R"'),
    ('"B34C0N B0AR"', '"B34C0N HUNT3R"'),
    ('"SS1D SN0UT"', '"SS1D SC0UT"'),
    ('"P4CK3T PR0D"', '"P4CK3T PR0"'),
    ('"4SS0C SW1N3"', '"4SS0C 0P"'),
    ('"C4PTUR3 C00K"', '"C4PTUR3 0P"'),
    ('"M1TM MUDP1G"', '"M1TM 0P"'),
    ('"5P00F CH3F"', '"5P00F 0P"'),
    ('"TR4NS1T TR0T"', '"TR4NS1T 0P"'),
    ('"6GHZ GR1NT"', '"6GHZ SC0UT"'),
    ('"0WE 0NK"', '"0WE 0P"'),
    ('"EAP-TLS TUSK"', '"EAP-TLS 0P"'),
    ('"EHT BR15TL3"', '"EHT 0P"'),
    ('"C0R3DUMP P1G"', '"C0R3DUMP 0P"'),
    ('"R00TK1T R1ND"', '"R00TK1T 0P"'),
    ('"PHR4CK P1G"', '"PHR4CK 0P"'),
    ('"2600 B0AR"', '"2600 0P"'),
    ('"BLU3B0X H4M"', '"BLU3B0X 0P"'),
    ('"C0NS0L3 C0W"', '"C0NS0L3 0P"'),
    ('"0xDE4D B4C0N"', '"0xDE4D 0P"'),
    ('"N3UR0 N0S3"', '"N3UR0 0P"'),
    ('"ICEBR34K B0AR"', '"ICEBR34K 0P"'),
    ('"K3RN3L H0G"', '"K3RN3L 0P"'),
    ('"SYSC4LL SW1N"', '"SYSC4LL 0P"'),
    ('"MARATHON PIG"', '"MARATHON SCOUT"'),
    ('"500 P1GS"', '"500 N3TS"'),
    ('"HANDSHAK3 HAM"', '"HANDSHAK3 PR0"'),
    ('"OINK4GEDDON"', '"RF4GEDDON"'),
    ('"snout grew stronger"', '"field skills improved"'),
    ('"new truffle unlocked"', '"new capability unlocked"'),
    ('"oink intensifies"', '"operator level increased"'),
    ('"swine on the rise"', '"wardog on the rise"'),
    ('"BLE MAXED. TRY OINK."', '"BLE XP CAP REACHED."'),
    ('"[XP] Pig immortality confirmed - restored from SD!"', '"[XP] Stella progress restored from SD!"'),
])

print("\nCleanup complete. Internal donor identifiers and persistence keys were intentionally left unchanged for compatibility.")
print("Run: git diff -- src/stella/identity.h src/ui/menu.cpp src/core/xp.cpp")
