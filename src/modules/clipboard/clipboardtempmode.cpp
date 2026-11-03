/*
 * SPDX-FileCopyrightText: 2026-2026 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */

#include "clipboardtempmode.h"
#include <string_view>
#include "fcitx-utils/key.h"
#include "fcitx-utils/keysym.h"
#include "fcitx/candidatelist.h"
#include "fcitx/event.h"
#include "fcitx/inputcontext.h"
#include "fcitx/inputpanel.h"
#include "fcitx/instance.h"
#include "fcitx/userinterface.h"
#include "clipboard.h"

namespace fcitx {

void ClipboardState::reset(InputContext *inputContext) {
    inputContext->inputPanel().reset();
    inputContext->updatePreedit();
    inputContext->updateUserInterface(UserInterfaceComponent::InputPanel);
}

ClipboardTempMode::ClipboardTempMode(Clipboard *clipboard)
    : clipboard_(clipboard) {}

bool ClipboardTempMode::triggerTempMode(const KeyEvent &keyEvent) {
    if (keyEvent.isRelease()) {
        return false;
    }

    if (keyEvent.checkKeyList(*clipboard_->config().triggerKey,
                              KeyEventMatchingMode::MatchAllNormalizedKeys)) {
        property(keyEvent.inputContext())->setActive(true);
        clipboard_->updateUI(keyEvent.inputContext());
        return true;
    }
    return false;
}

bool ClipboardTempMode::keyEvent(const KeyEvent &keyEvent) {
    auto *inputContext = keyEvent.inputContext();
    if (keyEvent.isRelease()) {
        return false;
    }

    auto candidateList = inputContext->inputPanel().candidateList();
    if (candidateList) {
        int idx = keyEvent.key().digitSelection();
        if (idx >= 0) {
            if (idx < candidateList->size()) {
                candidateList->candidate(idx).select(inputContext);
            }
            return true;
        }
        if (keyEvent.checkKeyList(*clipboard_->config().pasteSelectedKey,
                                  KeyEventMatchingMode::MatchAllNormalizedKeys)) {
            if (!candidateList->empty() && candidateList->cursorIndex() >= 0) {
                candidateList->candidate(candidateList->cursorIndex())
                    .select(inputContext);
            }
            return true;
        }

        if (keyEvent.checkKeyList(*clipboard_->config().previousPageKey,
                                  KeyEventMatchingMode::MatchAllNormalizedKeys)) {
            auto *pageable = candidateList->toPageable();
            if (!pageable->hasPrev()) {
                if (pageable->usedNextBefore()) {
                    return true;
                }
            } else {
                pageable->prev();
                inputContext->updateUserInterface(
                    UserInterfaceComponent::InputPanel);
                return true;
            }
        }

        if (keyEvent.checkKeyList(*clipboard_->config().nextPageKey,
                                  KeyEventMatchingMode::MatchAllNormalizedKeys)) {
            candidateList->toPageable()->next();
            inputContext->updateUserInterface(
                UserInterfaceComponent::InputPanel);
            return true;
        }

        if (keyEvent.checkKeyList(
                *clipboard_->config().previousCandidateKey,
                KeyEventMatchingMode::MatchAllNormalizedKeys)) {
            candidateList->toCursorMovable()->prevCandidate();
            inputContext->updateUserInterface(
                UserInterfaceComponent::InputPanel);
            return true;
        }

        if (keyEvent.checkKeyList(
                *clipboard_->config().nextCandidateKey,
                KeyEventMatchingMode::MatchAllNormalizedKeys)) {
            candidateList->toCursorMovable()->nextCandidate();
            inputContext->updateUserInterface(
                UserInterfaceComponent::InputPanel);
            return true;
        }
    }

    if (keyEvent.checkKeyList(*clipboard_->config().closeMenuKey,
                              KeyEventMatchingMode::MatchAllNormalizedKeys) ||
        keyEvent.checkKeyList(*clipboard_->config().nativeResetKey,
                              KeyEventMatchingMode::MatchAllNormalizedKeys)) {
        reset(inputContext);
        return true;
    }
    if (keyEvent.checkKeyList(*clipboard_->config().clearHistoryKey,
                              KeyEventMatchingMode::MatchAllNormalizedKeys)) {
        clipboard_->clear();
        reset(inputContext);
        return true;
    }

    clipboard_->updateUI(inputContext);
    return true;
}

void ClipboardTempMode::reset(InputContext *inputContext) {
    if (auto *clipboardState = property(inputContext)) {
        Base::reset(inputContext);
        clipboardState->reset(inputContext);
    }
}

std::string_view ClipboardTempMode::name() const { return "clipboardState"; }

} // namespace fcitx
