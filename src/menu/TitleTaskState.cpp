#include <coreinit/debug.h>
#include <cstring>
#include <menu/TitleOptionsState.h>
#include <menu/TitleTaskState.h>
#include <savemng.h>
#include <utils/InputUtils.h>
#include <utils/LanguageUtils.h>

static int cursorPos = 0;
static int entrycount;
static uint8_t slot = 0;
static int8_t allusers = -1, allusers_d = -1, sdusers = -1;
static bool common = true;

void TitleTaskState::render() {
    if (this->state == STATE_DO_SUBSTATE) {
        if (this->subState == nullptr) {
            OSFatal("SubState was null");
        }
        this->subState->render();
        return;
    }
    if (this->state == STATE_TITLE_TASKS) {
        this->isWiiUTitle = (this->title.highID == 0x00050000) || (this->title.highID == 0x00050002);
        entrycount = 3 + 2 * static_cast<int>(this->isWiiUTitle) + 1 * static_cast<int>(this->isWiiUTitle && (this->title.isTitleDupe));
        consolePrintPos(M_OFF, 2, "   [%08X-%08X] [%s]", this->title.highID, this->title.lowID,
                        this->title.productCode);
        consolePrintPos(M_OFF, 3, "   %s", this->title.shortName);
        consolePrintPos(M_OFF, 5, LanguageUtils::gettext("   Backup savedata"));
        consolePrintPos(M_OFF, 6, LanguageUtils::gettext("   Restore savedata"));
        consolePrintPos(M_OFF, 7, LanguageUtils::gettext("   Wipe savedata"));
        if (this->isWiiUTitle) {
            consolePrintPos(M_OFF, 8, LanguageUtils::gettext("   Import from loadiine"));
            consolePrintPos(M_OFF, 9, LanguageUtils::gettext("   Export to loadiine"));
            if (this->title.isTitleDupe)
                consolePrintPos(M_OFF, 10, LanguageUtils::gettext("   Copy Savedata to Title in %s"),
                                this->title.isTitleOnUSB ? "NAND" : "USB");
            if (this->title.iconBuf != nullptr)
                DrawUtils::drawTGA(660, 80, 1, this->title.iconBuf);
        } else if (this->title.iconBuf != nullptr)
            DrawUtils::drawRGB5A3(645, 80, 1, this->title.iconBuf);
        consolePrintPos(M_OFF, 2 + 3 + cursorPos, "\u2192");
        consolePrintPosAligned(17, 4, 2, LanguageUtils::gettext("\ue000: Select Task  \ue001: Back"));
    }
}

ApplicationState::eSubState TitleTaskState::update(Input *input) {
    if (this->state == STATE_TITLE_TASKS) {
        if (input->get(TRIGGER, PAD_BUTTON_B))
            return SUBSTATE_RETURN;

        if (input->get(TRIGGER, PAD_BUTTON_A)) {
            bool noError = true;
            this->task = (Task) cursorPos;

            if (this->task == backup) {
                if (!this->title.saveInit) {
                    promptError(LanguageUtils::gettext("No save to Backup."));
                    noError = false;
                }
            }

            if (this->task == restore) {
                getAccountsSD(&this->title, slot);
                allusers = ((sdusers == -1) ? -1 : allusers);
                sdusers = ((allusers == -1) ? -1 : sdusers);
            }

            if (this->task == wipe) {
                if (!this->title.saveInit) {
                    promptError(LanguageUtils::gettext("No save to Wipe."));
                    noError = false;
                }
            }

            if ((this->task == importLoadiine) || (this->task == exportLoadiine)) {
                char gamePath[PATH_SIZE];
                memset(versionList, 0, 0x100 * sizeof(int));
                if (!getLoadiineGameSaveDir(gamePath, this->title.productCode, this->title.longName, this->title.highID, this->title.lowID)) {
                    return SUBSTATE_RUNNING;
                }
                getLoadiineSaveVersionList(versionList, gamePath);
                if (this->task == importLoadiine) {
                    importFromLoadiine(&this->title, common, versionList != nullptr ? versionList[slot] : 0);
                }
                if (this->task == exportLoadiine) {
                    if (!this->title.saveInit) {
                        promptError(LanguageUtils::gettext("No save to Export."));
                        noError = false;
                    }
                    exportToLoadiine(&this->title, common, versionList != nullptr ? versionList[slot] : 0);
                }
            }

            if (this->task == copytoOtherDevice) {
                if (!this->title.saveInit) {
                    promptError(LanguageUtils::gettext("No save to Copy."));
                    noError = false;
                }
            }
            if (noError) {
                DrawUtils::setRedraw(true);
                this->state = STATE_DO_SUBSTATE;
                this->subState = std::make_unique<TitleOptionsState>(this->title, this->task, this->versionList, sdusers, allusers, common, allusers_d, this->titles, this->titlesCount);
            }
        }
        if (cursorPos > entrycount)
            cursorPos = entrycount;
        if (input->get(TRIGGER, PAD_BUTTON_DOWN)) {
            if (entrycount <= 14)
                cursorPos = (cursorPos + 1) % entrycount;
        } else if (input->get(TRIGGER, PAD_BUTTON_UP)) {
            if (cursorPos > 0)
                --cursorPos;
        }
    } else if (this->state == STATE_DO_SUBSTATE) {
        auto retSubState = this->subState->update(input);
        if (retSubState == SUBSTATE_RUNNING) {
            // keep running.
            return SUBSTATE_RUNNING;
        } else if (retSubState == SUBSTATE_RETURN) {
            this->subState.reset();
            this->state = STATE_TITLE_TASKS;
        }
    }
    return SUBSTATE_RUNNING;
}