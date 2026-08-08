// Stella grouped menu

#include "menu.h"
#include <M5Cardputer.h>
#include "display.h"
#include "../audio/sfx.h"
#include "../stella/identity.h"
#include <string.h>

// ============================================================================
// STELLA COPY
// ============================================================================

static const char* const H_ATTACK[] = {
    "WARDOG TOOLS. HANDLE WITH INTENT.",
    "AUTHORIZED RANGE. SHARP TEETH.",
    "CONTROL THE AIR. KEEP YOUR SCOPE."
};
static const char* const H_RECON[] = {
    "NOSE UP. EARS OPEN. TX QUIET.",
    "SNIFF FIRST. BITE ONLY IN SCOPE.",
    "READ THE AIR BEFORE YOU TOUCH IT."
};
static const char* const H_LOOT[] = {
    "CAPTURES, TRACKS AND FINDINGS.",
    "FETCH THE DATA. KEEP THE EVIDENCE.",
    "EVERY GOOD DOG BRINGS SOMETHING HOME."
};
static const char* const H_RANK[] = {
    "WARDOG XP. FIELD CRED. BADGES.",
    "GOOD DOGS LEARN. GREAT DOGS LOG.",
    "PROGRESS SAVED. EGO OPTIONAL."
};
static const char* const H_COMMS[] = {
    "PACK LINK TO W33Z AND FRIENDS.",
    "FETCH, DELIVER, SYNC, REPEAT.",
    "ON LEASH WHEN W33Z IS LISTENING."
};
static const char* const H_SYSTEM[] = {
    "COLLAR, HEALTH, STORAGE, DIAGNOSTICS.",
    "KEEP STELLA SHARP AND FED WITH POWER.",
    "CHECK THE DOG BEFORE BLAMING THE AIR."
};

static const char* const H_OINK[] = {
    "BITE MODE: AUTHORIZED ACTIVE ASSESSMENT.",
    "TEETH OUT ONLY INSIDE YOUR SCOPE.",
    "ACTIVE RF WORK. LOG EVERYTHING."
};
static const char* const H_BLUES[] = {
    "BLUE PAWS: BLE RECON AND TOOLING.",
    "SNIFF THE SHORT-RANGE PACK.",
    "BLE AIRSPACE, NOW WITH MORE PAWS."
};
static const char* const H_DNOHAM[] = {
    "SNIFF: PASSIVE WIFI INTELLIGENCE.",
    "ZERO TX. MAXIMUM NOSE.",
    "QUIET DOG. LOUD DATA."
};
static const char* const H_WARHOG[] = {
    "PATROL: GPS WARDIVING IN MOTION.",
    "TRACK STREETS. MARK RF TERRITORY.",
    "STELLA WALKS. W33Z REMEMBERS."
};
static const char* const H_SPCTRM[] = {
    "AIRWATCH: READ CHANNEL PRESSURE.",
    "SEE THE RF NOISE BEFORE IT BITES.",
    "CHANNELS TALK. STELLA LISTENS."
};
static const char* const H_HASHES[] = {
    "CAPTURE VAULT FEED FOR W33Z.",
    "HANDSHAKES AND PMKIDS, ORGANIZED.",
    "FETCH CLEAN CAPTURES FOR THE LAB."
};
static const char* const H_TRACKS[] = {
    "PATROL TRAILS, GPS AND WIGLE DATA.",
    "FOLLOW THE PAWPRINTS ON THE MAP.",
    "ROUTES BECOME RF HISTORY."
};
static const char* const H_BOUNTY[] = {
    "TARGET NOTES FOR AUTHORIZED PROJECTS.",
    "MARK INTERESTING FINDINGS IN SCOPE.",
    "NO RANDOM PREY. USE A PROJECT SCOPE."
};
static const char* const H_SYNC[] = {
    "PACK LINK: STELLA <-> W33Z.",
    "FETCH DATA. DELIVER CONFIG.",
    "W33Z.GEXZ.BE IS THE HOME KENNEL."
};
static const char* const H_BACONTX[] = {
    "HOWL: RF BEACON AND SIGNAL TOOLING.",
    "MAKE NOISE ONLY WHERE YOU ARE ALLOWED.",
    "LOUD DOG. CONTROLLED RANGE."
};
static const char* const H_XFIL[] = {
    "FETCH FILES OFF STELLA.",
    "DELIVER CAPTURES TO W33Z.",
    "MOVE DATA WITHOUT LOSING THE TRAIL."
};
static const char* const H_FLEX[] = {
    "WARDOG STATS AND FIELD PROGRESS.",
    "SHOW THE RUN. KEEP THE RECEIPTS.",
    "XP IS TEMPORARY. LOGS ARE FOREVER."
};
static const char* const H_BADGES[] = {
    "BADGES FOR ACTUAL FIELD MILESTONES.",
    "GOOD DOG CERTIFICATES, BASICALLY.",
    "UNLOCKED BY WORK, NOT BY OINKING."
};
static const char* const H_SNOUTS[] = {
    "STELLA UNLOCKS AND COLLECTIBLES.",
    "NEW TRICKS FOR A BUSY WARDOG.",
    "THE PAWPRINT ARCHIVE."
};
static const char* const H_SETTINGS[] = {
    "COLLAR SETTINGS FOR YOUR WARDOG.",
    "TUNE IT. TEST IT. KEEP IT STABLE.",
    "PERSONALITY, RADIOS, LINK AND POWER."
};
static const char* const H_BRBRS[] = {
    "FRIENDLY PACK: NEVER BITE THESE.",
    "TRUSTED BSSIDS, SSIDS AND DEVICES.",
    "GOOD DOGS KNOW THEIR FRIENDS."
};
static const char* const H_CRASHES[] = {
    "COREDUMPS: WHEN STELLA FALLS OVER.",
    "POST-MORTEM FOR THE WARDOG BRAIN.",
    "DEBUG THE CRASH, NOT THE DOG."
};
static const char* const H_DIAG[] = {
    "WARDOG HEALTH AND LIVE DIAGNOSTICS.",
    "HEAP, RADIO, STORAGE, LINK, BATTERY.",
    "CHECK VITALS BEFORE A LONG PATROL."
};
static const char* const H_SDFMT[] = {
    "FORMAT THE FIELD STORAGE.",
    "WIPE THE CARD. KEEP THE DOG.",
    "FRESH SD, FRESH PAWPRINTS."
};
static const char* const H_ABOUT[] = {
    "STELLA THE WARDOG. BUILT FOR W33Z.",
    "FOX POMERANIAN ENERGY. RF BRAIN.",
    "WOOF PROTOCOL ACTIVE."
};
static const char* const H_CHARGING[] = {
    "NAP MODE. BATTERY GETS THE BED.",
    "PLUG IN. CURL UP. RECHARGE.",
    "EVEN WARDOGS NEED A NAP."
};

const RootItem Menu::ROOT_ITEMS[] = {
    {"/>",  "ASSESS",  H_ATTACK,  (uint8_t)(sizeof(H_ATTACK)/sizeof(H_ATTACK[0])),  RootType::GROUP,  {.groupId = GroupId::ATTACK}},
    {"o~",  "SNIFF",   H_RECON,   (uint8_t)(sizeof(H_RECON)/sizeof(H_RECON[0])),    RootType::GROUP,  {.groupId = GroupId::RECON}},
    {"[$",  "FETCH",   H_LOOT,    (uint8_t)(sizeof(H_LOOT)/sizeof(H_LOOT[0])),      RootType::GROUP,  {.groupId = GroupId::LOOT}},
    {"^#",  "WARDOG",  H_RANK,    (uint8_t)(sizeof(H_RANK)/sizeof(H_RANK[0])),      RootType::GROUP,  {.groupId = GroupId::RANK}},
    {"))",  "PACK",    H_COMMS,   (uint8_t)(sizeof(H_COMMS)/sizeof(H_COMMS[0])),    RootType::GROUP,  {.groupId = GroupId::COMMS}},
    {"::",  "COLLAR",  H_SYSTEM,  (uint8_t)(sizeof(H_SYSTEM)/sizeof(H_SYSTEM[0])),  RootType::GROUP,  {.groupId = GroupId::SYSTEM}}
};
const uint8_t Menu::ROOT_COUNT = sizeof(ROOT_ITEMS) / sizeof(ROOT_ITEMS[0]);

const MenuItem Menu::GROUP_ATTACK[] = {
    {"/>", StellaLanguage::kBite,      1, H_OINK,  (uint8_t)(sizeof(H_OINK)/sizeof(H_OINK[0]))},
    {"!!", StellaLanguage::kBluePaws,  8, H_BLUES, (uint8_t)(sizeof(H_BLUES)/sizeof(H_BLUES[0]))}
};
const uint8_t Menu::GROUP_ATTACK_SIZE = sizeof(GROUP_ATTACK) / sizeof(GROUP_ATTACK[0]);

const MenuItem Menu::GROUP_RECON[] = {
    {"o~", StellaLanguage::kSniff,    14, H_DNOHAM, (uint8_t)(sizeof(H_DNOHAM)/sizeof(H_DNOHAM[0]))},
    {"<>", StellaLanguage::kPatrol,    2, H_WARHOG, (uint8_t)(sizeof(H_WARHOG)/sizeof(H_WARHOG[0]))},
    {"~~", StellaLanguage::kAirwatch, 10, H_SPCTRM,  (uint8_t)(sizeof(H_SPCTRM)/sizeof(H_SPCTRM[0]))}
};
const uint8_t Menu::GROUP_RECON_SIZE = sizeof(GROUP_RECON) / sizeof(GROUP_RECON[0]);

const MenuItem Menu::GROUP_LOOT[] = {
    {"C#", "CAPTURES", 4,  H_HASHES,  (uint8_t)(sizeof(H_HASHES)/sizeof(H_HASHES[0]))},
    {"~>", "TRAILS",   13, H_TRACKS,  (uint8_t)(sizeof(H_TRACKS)/sizeof(H_TRACKS[0]))},
    {"B$", "MARKS",    17, H_BOUNTY,  (uint8_t)(sizeof(H_BOUNTY)/sizeof(H_BOUNTY[0]))}
};
const uint8_t Menu::GROUP_LOOT_SIZE = sizeof(GROUP_LOOT) / sizeof(GROUP_LOOT[0]);

const MenuItem Menu::GROUP_COMMS[] = {
    {"@)", StellaLanguage::kPackLink, 16, H_SYNC,    (uint8_t)(sizeof(H_SYNC)/sizeof(H_SYNC[0]))},
    {"))", StellaLanguage::kHowl,     18, H_BACONTX, (uint8_t)(sizeof(H_BACONTX)/sizeof(H_BACONTX[0]))},
    {"FX", "FETCH IO",                3, H_XFIL,    (uint8_t)(sizeof(H_XFIL)/sizeof(H_XFIL[0]))}
};
const uint8_t Menu::GROUP_COMMS_SIZE = sizeof(GROUP_COMMS) / sizeof(GROUP_COMMS[0]);

const MenuItem Menu::GROUP_RANK[] = {
    {"^#", StellaLanguage::kWardogStats, 11, H_FLEX,   (uint8_t)(sizeof(H_FLEX)/sizeof(H_FLEX[0]))},
    {"*#", "BADGES",                    9, H_BADGES,  (uint8_t)(sizeof(H_BADGES)/sizeof(H_BADGES[0]))},
    {"?*", "TRICKS",                   15, H_SNOUTS,  (uint8_t)(sizeof(H_SNOUTS)/sizeof(H_SNOUTS[0]))}
};
const uint8_t Menu::GROUP_RANK_SIZE = sizeof(GROUP_RANK) / sizeof(GROUP_RANK[0]);

const MenuItem Menu::GROUP_SYSTEM[] = {
    {"==", "SETTINGS",                     5, H_SETTINGS, (uint8_t)(sizeof(H_SETTINGS)/sizeof(H_SETTINGS[0]))},
    {"[]", StellaLanguage::kFriendlyPack,  12, H_BRBRS,   (uint8_t)(sizeof(H_BRBRS)/sizeof(H_BRBRS[0]))},
    {"!!", "COREDUMP",                     7, H_CRASHES,  (uint8_t)(sizeof(H_CRASHES)/sizeof(H_CRASHES[0]))},
    {"::", "VITALS",                      19, H_DIAG,     (uint8_t)(sizeof(H_DIAG)/sizeof(H_DIAG[0]))},
    {"SD", "FORMAT SD",                   20, H_SDFMT,    (uint8_t)(sizeof(H_SDFMT)/sizeof(H_SDFMT[0]))},
    {"~~", "NAP/CHARGE",                  21, H_CHARGING, (uint8_t)(sizeof(H_CHARGING)/sizeof(H_CHARGING[0]))},
    {":?", StellaLanguage::kAboutStella,    6, H_ABOUT,    (uint8_t)(sizeof(H_ABOUT)/sizeof(H_ABOUT[0]))}
};
const uint8_t Menu::GROUP_SYSTEM_SIZE = sizeof(GROUP_SYSTEM) / sizeof(GROUP_SYSTEM[0]);

uint8_t Menu::rootIdx = 0;
uint8_t Menu::rootScroll = 0;
GroupId Menu::activeGroup = GroupId::NONE;
uint8_t Menu::modalIdx = 0;
uint8_t Menu::modalScroll = 0;
bool Menu::active = false;
MenuCallback Menu::callback = nullptr;
bool Menu::keyWasPressed = false;
uint8_t Menu::rootHintIndex[Menu::ROOT_COUNT] = {0};
uint8_t Menu::attackHintIndex[Menu::GROUP_ATTACK_SIZE] = {0};
uint8_t Menu::reconHintIndex[Menu::GROUP_RECON_SIZE] = {0};
uint8_t Menu::lootHintIndex[Menu::GROUP_LOOT_SIZE] = {0};
uint8_t Menu::commsHintIndex[Menu::GROUP_COMMS_SIZE] = {0};
uint8_t Menu::rankHintIndex[Menu::GROUP_RANK_SIZE] = {0};
uint8_t Menu::systemHintIndex[Menu::GROUP_SYSTEM_SIZE] = {0};

bool Menu::isRootSelectable(uint8_t idx) {
    if (idx >= ROOT_COUNT) return false;
    return ROOT_ITEMS[idx].type != RootType::SEPARATOR;
}

const MenuItem* Menu::getGroupItems(GroupId group) {
    switch (group) {
        case GroupId::ATTACK:  return GROUP_ATTACK;
        case GroupId::RECON:   return GROUP_RECON;
        case GroupId::LOOT:    return GROUP_LOOT;
        case GroupId::COMMS:   return GROUP_COMMS;
        case GroupId::RANK:    return GROUP_RANK;
        case GroupId::SYSTEM:  return GROUP_SYSTEM;
        default: return nullptr;
    }
}

uint8_t Menu::getGroupSize(GroupId group) {
    switch (group) {
        case GroupId::ATTACK:  return GROUP_ATTACK_SIZE;
        case GroupId::RECON:   return GROUP_RECON_SIZE;
        case GroupId::LOOT:    return GROUP_LOOT_SIZE;
        case GroupId::COMMS:   return GROUP_COMMS_SIZE;
        case GroupId::RANK:    return GROUP_RANK_SIZE;
        case GroupId::SYSTEM:  return GROUP_SYSTEM_SIZE;
        default: return 0;
    }
}

const char* Menu::getGroupName(GroupId group) {
    switch (group) {
        case GroupId::ATTACK:  return "ASSESS";
        case GroupId::RECON:   return "SNIFF";
        case GroupId::LOOT:    return "FETCH";
        case GroupId::COMMS:   return "PACK";
        case GroupId::RANK:    return "WARDOG";
        case GroupId::SYSTEM:  return "COLLAR";
        default: return "";
    }
}

void Menu::setCallback(MenuCallback cb) { callback = cb; }

void Menu::init() {
    rootIdx = 0; rootScroll = 0; activeGroup = GroupId::NONE; modalIdx = 0; modalScroll = 0;
    for (uint8_t i = 0; i < ROOT_COUNT && i < sizeof(rootHintIndex)/sizeof(rootHintIndex[0]); i++) {
        rootHintIndex[i] = ROOT_ITEMS[i].hintCount > 0 ? esp_random() % ROOT_ITEMS[i].hintCount : 0;
    }
}

void Menu::show() {
    active = true; rootIdx = 0; rootScroll = 0; activeGroup = GroupId::NONE; modalIdx = 0; modalScroll = 0;
    for (uint8_t i = 0; i < ROOT_COUNT && i < sizeof(rootHintIndex)/sizeof(rootHintIndex[0]); i++) {
        rootHintIndex[i] = ROOT_ITEMS[i].hintCount > 0 ? esp_random() % ROOT_ITEMS[i].hintCount : 0;
    }
}

void Menu::hide() { active = false; activeGroup = GroupId::NONE; }

bool Menu::closeModal() {
    if (activeGroup == GroupId::NONE) return false;
    activeGroup = GroupId::NONE; modalIdx = 0; modalScroll = 0; return true;
}

const char* Menu::getSelectedDescription() {
    if (activeGroup != GroupId::NONE) {
        const MenuItem* items = getGroupItems(activeGroup);
        uint8_t* indices = nullptr;
        switch (activeGroup) {
            case GroupId::ATTACK: indices = attackHintIndex; break;
            case GroupId::RECON:  indices = reconHintIndex;  break;
            case GroupId::LOOT:   indices = lootHintIndex;   break;
            case GroupId::COMMS:  indices = commsHintIndex;  break;
            case GroupId::RANK:   indices = rankHintIndex;   break;
            case GroupId::SYSTEM: indices = systemHintIndex; break;
            default: break;
        }
        uint8_t groupSize = getGroupSize(activeGroup);
        if (items && indices && modalIdx < groupSize && items[modalIdx].hintCount > 0 && indices[modalIdx] < items[modalIdx].hintCount) {
            return items[modalIdx].hintPool[indices[modalIdx]];
        }
        return "";
    }
    if (rootIdx >= ROOT_COUNT || ROOT_ITEMS[rootIdx].hintCount == 0 || rootHintIndex[rootIdx] >= ROOT_ITEMS[rootIdx].hintCount) return "";
    return ROOT_ITEMS[rootIdx].hintPool[rootHintIndex[rootIdx]];
}

void Menu::update() { if (active) handleInput(); }

void Menu::handleInput() {
    bool anyPressed = M5Cardputer.Keyboard.isPressed();
    if (!anyPressed) { keyWasPressed = false; return; }
    if (keyWasPressed) return;
    keyWasPressed = true;
    auto keys = M5Cardputer.Keyboard.keysState();

    if (activeGroup != GroupId::NONE) {
        uint8_t groupSize = getGroupSize(activeGroup);
        if (M5Cardputer.Keyboard.isKeyPressed(';')) {
            if (modalIdx > 0) {
                modalIdx--; SFX::play(SFX::MENU_CLICK);
                if (modalIdx < modalScroll) modalScroll = modalIdx;
            }
        }
        if (M5Cardputer.Keyboard.isKeyPressed('.')) {
            if (modalIdx < groupSize - 1) {
                modalIdx++; SFX::play(SFX::MENU_CLICK);
                if (modalIdx >= modalScroll + MODAL_VISIBLE) modalScroll = modalIdx - MODAL_VISIBLE + 1;
            }
        }
        if (keys.enter) {
            SFX::play(SFX::MENU_CLICK);
            const MenuItem* items = getGroupItems(activeGroup);
            if (items && callback) callback(items[modalIdx].actionId);
            closeModal();
        }
        if (M5Cardputer.Keyboard.isKeyPressed(KEY_BACKSPACE)) closeModal();
    } else {
        if (M5Cardputer.Keyboard.isKeyPressed(';')) {
            int newIdx = rootIdx;
            do { if (newIdx > 0) newIdx--; else break; } while (!isRootSelectable(newIdx) && newIdx > 0);
            if (isRootSelectable(newIdx) && newIdx != rootIdx) {
                rootIdx = newIdx; SFX::play(SFX::MENU_CLICK); if (rootIdx < rootScroll) rootScroll = rootIdx;
            }
        }
        if (M5Cardputer.Keyboard.isKeyPressed('.')) {
            int newIdx = rootIdx;
            do { if (newIdx < ROOT_COUNT - 1) newIdx++; else break; } while (!isRootSelectable(newIdx) && newIdx < ROOT_COUNT - 1);
            if (isRootSelectable(newIdx) && newIdx != rootIdx) {
                rootIdx = newIdx; SFX::play(SFX::MENU_CLICK);
                if (rootIdx >= rootScroll + VISIBLE_ITEMS) rootScroll = rootIdx - VISIBLE_ITEMS + 1;
            }
        }
        if (keys.enter) {
            SFX::play(SFX::MENU_CLICK);
            const RootItem& item = ROOT_ITEMS[rootIdx];
            if (item.type == RootType::GROUP) {
                activeGroup = item.groupId; modalIdx = 0; modalScroll = 0;
                const MenuItem* items = getGroupItems(activeGroup);
                uint8_t groupSize = getGroupSize(activeGroup);
                uint8_t* indices = nullptr;
                switch (activeGroup) {
                    case GroupId::ATTACK: indices = attackHintIndex; break;
                    case GroupId::RECON:  indices = reconHintIndex;  break;
                    case GroupId::LOOT:   indices = lootHintIndex;   break;
                    case GroupId::COMMS:  indices = commsHintIndex;  break;
                    case GroupId::RANK:   indices = rankHintIndex;   break;
                    case GroupId::SYSTEM: indices = systemHintIndex; break;
                    default: break;
                }
                if (items && indices) {
                    uint8_t maxIndex = 0;
                    switch (activeGroup) {
                        case GroupId::ATTACK: maxIndex = sizeof(attackHintIndex)/sizeof(attackHintIndex[0]); break;
                        case GroupId::RECON:  maxIndex = sizeof(reconHintIndex)/sizeof(reconHintIndex[0]); break;
                        case GroupId::LOOT:   maxIndex = sizeof(lootHintIndex)/sizeof(lootHintIndex[0]); break;
                        case GroupId::COMMS:  maxIndex = sizeof(commsHintIndex)/sizeof(commsHintIndex[0]); break;
                        case GroupId::RANK:   maxIndex = sizeof(rankHintIndex)/sizeof(rankHintIndex[0]); break;
                        case GroupId::SYSTEM: maxIndex = sizeof(systemHintIndex)/sizeof(systemHintIndex[0]); break;
                        default: maxIndex = 0; break;
                    }
                    for (uint8_t i = 0; i < groupSize && i < maxIndex; i++) {
                        indices[i] = items[i].hintCount > 0 ? esp_random() % items[i].hintCount : 0;
                    }
                }
            } else if (item.type == RootType::DIRECT && callback) {
                callback(item.actionId);
            }
        }
    }
}

void Menu::draw(M5Canvas& canvas) {
    if (!active) return;
    drawRoot(canvas);
    if (activeGroup != GroupId::NONE) drawModal(canvas);
}

void Menu::drawRoot(M5Canvas& canvas) {
    uint16_t fg = getColorFG();
    uint16_t bg = getColorBG();
    uint16_t accent = fg;
    canvas.fillSprite(bg);
    canvas.setTextColor(fg);
    canvas.setTextDatum(top_center);
    canvas.setTextSize(2);
    char titleBuf[32];
    const RootItem& sel = ROOT_ITEMS[rootIdx];
    if (sel.icon && sel.icon[0] && strlen(sel.icon) < sizeof(titleBuf) - 11) {
        snprintf(titleBuf, sizeof(titleBuf), "%s M5STELLA", sel.icon);
    } else {
        strncpy(titleBuf, "M5STELLA", sizeof(titleBuf) - 1);
        titleBuf[sizeof(titleBuf) - 1] = '\0';
    }
    canvas.drawString(titleBuf, DISPLAY_W / 2, 2);
    canvas.drawLine(10, 20, DISPLAY_W - 10, 20, accent);
    canvas.setTextDatum(top_left);
    canvas.setTextSize(2);
    int yOffset = 25;
    int lineHeight = 18;

    for (uint8_t i = 0; i < VISIBLE_ITEMS && (rootScroll + i) < ROOT_COUNT; i++) {
        uint8_t idx = rootScroll + i;
        int y = yOffset + i * lineHeight;
        const RootItem& item = ROOT_ITEMS[idx];
        if (item.type == RootType::SEPARATOR) {
            canvas.drawLine(20, y + lineHeight/2, DISPLAY_W - 20, y + lineHeight/2, accent);
            continue;
        }
        bool isSelected = (idx == rootIdx) && (activeGroup == GroupId::NONE);
        if (isSelected) { canvas.fillRect(5, y - 2, DISPLAY_W - 10, lineHeight, accent); canvas.setTextColor(bg); }
        else canvas.setTextColor(fg);

        char labelBuf[40];
        const char* icon = (item.icon && item.icon[0]) ? item.icon : ">";
        if (item.type == RootType::GROUP) {
            size_t totalLen = strlen(icon) + strlen(item.label) + 3;
            if (totalLen <= sizeof(labelBuf)) snprintf(labelBuf, sizeof(labelBuf), "%s %s >", icon, item.label);
            else {
                size_t maxLabelLen = sizeof(labelBuf) - strlen(icon) - 3;
                if (maxLabelLen > 0) snprintf(labelBuf, sizeof(labelBuf), "%s %.*s >", icon, (int)maxLabelLen, item.label);
                else { strncpy(labelBuf, icon, sizeof(labelBuf) - 1); labelBuf[sizeof(labelBuf) - 1] = '\0'; }
            }
        } else {
            size_t totalLen = strlen(icon) + strlen(item.label) + 2;
            if (totalLen <= sizeof(labelBuf)) snprintf(labelBuf, sizeof(labelBuf), "%s %s", icon, item.label);
            else {
                size_t maxLabelLen = sizeof(labelBuf) - strlen(icon) - 2;
                if (maxLabelLen > 0) snprintf(labelBuf, sizeof(labelBuf), "%s %.*s", icon, (int)maxLabelLen, item.label);
                else { strncpy(labelBuf, icon, sizeof(labelBuf) - 1); labelBuf[sizeof(labelBuf) - 1] = '\0'; }
            }
        }
        canvas.drawString(labelBuf, 10, y);
    }
    canvas.setTextColor(fg);
    canvas.setTextSize(1);
    if (rootScroll > 0) canvas.drawString("^", DISPLAY_W - 12, 22);
    if (rootScroll + VISIBLE_ITEMS < ROOT_COUNT) canvas.drawString("v", DISPLAY_W - 12, yOffset + (VISIBLE_ITEMS - 1) * lineHeight);
}

void Menu::drawModal(M5Canvas& canvas) {
    uint16_t fg = getColorFG();
    uint16_t bg = getColorBG();
    int boxW = 220, boxH = 90, boxX = (DISPLAY_W - boxW) / 2, boxY = 20;
    canvas.fillRoundRect(boxX, boxY, boxW, boxH, 6, fg);
    canvas.drawRoundRect(boxX, boxY, boxW, boxH, 6, bg);
    canvas.setTextColor(bg);
    canvas.setTextDatum(top_center);
    canvas.setTextSize(2);
    canvas.drawString(getGroupName(activeGroup), boxX + boxW/2, boxY + 4);
    canvas.drawLine(boxX + 10, boxY + 20, boxX + boxW - 10, boxY + 20, bg);
    canvas.setTextDatum(top_left);

    const MenuItem* items = getGroupItems(activeGroup);
    uint8_t groupSize = getGroupSize(activeGroup);
    int itemStartY = boxY + 24, itemHeight = 16, itemPadX = 6, textIndent = 10;
    canvas.setTextSize(2);
    for (int i = 0; i < MODAL_VISIBLE && (modalScroll + i) < groupSize; i++) {
        int idx = modalScroll + i;
        int y = itemStartY + i * itemHeight;
        const MenuItem& item = items[idx];
        bool isSelected = (idx == modalIdx);
        if (isSelected) {
            canvas.fillRect(boxX + itemPadX, y, boxW - (itemPadX * 2), itemHeight - 1, bg);
            canvas.setTextColor(fg); canvas.setCursor(boxX + textIndent, y); canvas.print("> ");
        } else {
            canvas.setTextColor(bg); canvas.setCursor(boxX + textIndent, y); canvas.print("  ");
        }
        if (item.icon && item.icon[0]) { canvas.print(item.icon); canvas.print(" "); }
        else canvas.print("  ");
        constexpr int kMaxLabelChars = 10;
        char shortLabel[kMaxLabelChars + 1];
        if (strlen(item.label) <= kMaxLabelChars) strcpy(shortLabel, item.label);
        else { strncpy(shortLabel, item.label, kMaxLabelChars); shortLabel[kMaxLabelChars] = '\0'; }
        canvas.print(shortLabel);
    }
    canvas.setTextSize(1); canvas.setTextColor(bg);
    if (modalScroll > 0) { canvas.setCursor(boxX + boxW - 12, itemStartY + 4); canvas.print("^"); }
    if (modalScroll + MODAL_VISIBLE < groupSize) {
        canvas.setCursor(boxX + boxW - 12, itemStartY + (MODAL_VISIBLE - 1) * itemHeight + 4); canvas.print("v");
    }
}
