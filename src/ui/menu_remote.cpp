#include "menu.h"

#include <string.h>

#include "../audio/sfx.h"

bool Menu::handleRemoteAction(const char* action) {
    if (!active || !action || !*action) return false;

    const bool up = strcmp(action, "up") == 0;
    const bool down = strcmp(action, "down") == 0;
    const bool enter = strcmp(action, "enter") == 0;
    const bool back = strcmp(action, "back") == 0;
    if (!up && !down && !enter && !back) return false;

    if (activeGroup != GroupId::NONE) {
        const uint8_t groupSize = getGroupSize(activeGroup);

        if (up) {
            if (modalIdx > 0) {
                modalIdx--;
                SFX::play(SFX::MENU_CLICK);
                if (modalIdx < modalScroll) modalScroll = modalIdx;
            }
            return true;
        }

        if (down) {
            if (groupSize > 0 && modalIdx < groupSize - 1) {
                modalIdx++;
                SFX::play(SFX::MENU_CLICK);
                if (modalIdx >= modalScroll + MODAL_VISIBLE) {
                    modalScroll = modalIdx - MODAL_VISIBLE + 1;
                }
            }
            return true;
        }

        if (enter) {
            SFX::play(SFX::MENU_CLICK);
            const MenuItem* items = getGroupItems(activeGroup);
            if (items && callback && modalIdx < groupSize) {
                callback(items[modalIdx].actionId);
            }
            closeModal();
            return true;
        }

        if (back) {
            closeModal();
            SFX::play(SFX::MENU_CLICK);
            return true;
        }
    }

    if (up) {
        int newIdx = rootIdx;
        do {
            if (newIdx > 0) newIdx--;
            else break;
        } while (!isRootSelectable(newIdx) && newIdx > 0);

        if (isRootSelectable(newIdx) && newIdx != rootIdx) {
            rootIdx = newIdx;
            SFX::play(SFX::MENU_CLICK);
            if (rootIdx < rootScroll) rootScroll = rootIdx;
        }
        return true;
    }

    if (down) {
        int newIdx = rootIdx;
        do {
            if (newIdx < ROOT_COUNT - 1) newIdx++;
            else break;
        } while (!isRootSelectable(newIdx) && newIdx < ROOT_COUNT - 1);

        if (isRootSelectable(newIdx) && newIdx != rootIdx) {
            rootIdx = newIdx;
            SFX::play(SFX::MENU_CLICK);
            if (rootIdx >= rootScroll + VISIBLE_ITEMS) {
                rootScroll = rootIdx - VISIBLE_ITEMS + 1;
            }
        }
        return true;
    }

    if (enter) {
        SFX::play(SFX::MENU_CLICK);
        const RootItem& item = ROOT_ITEMS[rootIdx];
        if (item.type == RootType::GROUP) {
            activeGroup = item.groupId;
            modalIdx = 0;
            modalScroll = 0;
        } else if (item.type == RootType::DIRECT && callback) {
            callback(item.actionId);
        }
        return true;
    }

    // At root level the caller decides where Back should go (currently IDLE).
    return false;
}
