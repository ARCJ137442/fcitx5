/*
 * SPDX-FileCopyrightText: 2017-2017 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#ifndef _FCITX_MODULES_CLIPBOARD_CLIPBOARD_H_
#define _FCITX_MODULES_CLIPBOARD_CLIPBOARD_H_

#include <cstddef>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include "fcitx-config/configuration.h"
#include "fcitx-config/iniparser.h"
#include "fcitx-config/option.h"
#include "fcitx-config/rawconfig.h"
#include "fcitx-utils/eventloopinterface.h"
#include "fcitx-utils/handlertable.h"
#include "fcitx-utils/i18n.h"
#include "fcitx-utils/key.h"
#include "fcitx-utils/log.h"
#include "fcitx-utils/misc.h"
#include "fcitx-utils/misc_p.h"
#include "fcitx/addoninstance.h"
#include "fcitx/addonmanager.h"
#include "fcitx/inputcontextproperty.h"
#include "fcitx/instance.h"
#include "clipboard_public.h"
#include "clipboardentry.h"

#ifdef ENABLE_X11
#include "xcb_public.h"
#include "xcbclipboard.h"
#endif
#ifdef WAYLAND_FOUND
#include "wayland_public.h"
#include "waylandclipboard.h"
#endif

namespace fcitx {

constexpr size_t MAX_CLIPBOARD_SIZE = 4000;
constexpr char PASSWORD_MIME_TYPE[] = "x-kde-passwordManagerHint";

FCITX_CONFIGURATION(
    ClipboardConfig, KeyListOption triggerKey{this,
                                              "TriggerKey",
                                              _("Clipboard trigger keys"),
                                              {Key("Control+semicolon")},
                                              KeyListConstrain()};
    KeyListOption favoritesTriggerKey{
        this,
        "FavoritesTriggerKey",
        _("Pinned menu trigger keys"),
        {Key("Alt+Shift+v")},
        KeyListConstrain()};
    KeyListOption searchTriggerKey{
        this,
        "SearchTriggerKey",
        _("Clipboard history search trigger keys"),
        {Key("Alt+Shift+f")},
        KeyListConstrain()};
    KeyListOption searchTriggerKeySecondary{
        this,
        "SearchTriggerKeySecondary",
        _("Additional clipboard history search trigger keys"),
        {},
        KeyListConstrain()};
    Option<bool> showShortcutHints{
        this, "ShowShortcutHints", _("Show shortcut hints"), true};
    KeyListOption favoritesFilterKey{
        this,
        "FavoritesFilterKey",
        _("Show pinned clipboard items"),
        {Key("f")},
        KeyListConstrain({KeyConstrainFlag::AllowModifierLess})};
    KeyListOption searchFilterKey{
        this,
        "SearchFilterKey",
        _("Search clipboard history"),
        {Key("s")},
        KeyListConstrain({KeyConstrainFlag::AllowModifierLess})};
    KeyListOption pinToggleKey{
        this,
        "PinToggleKey",
        _("Pin or unpin selected clipboard item"),
        {Key("d")},
        KeyListConstrain({KeyConstrainFlag::AllowModifierLess})};
    KeyListOption deleteEntryKey{
        this,
        "DeleteEntryKey",
        _("Delete selected clipboard item"),
        {Key("x")},
        KeyListConstrain({KeyConstrainFlag::AllowModifierLess})};
    KeyListOption closeMenuKey{
        this,
        "CloseMenuKey",
        _("Close clipboard menu"),
        {Key("q")},
        KeyListConstrain({KeyConstrainFlag::AllowModifierLess})};
    KeyListOption nativeResetKey{
        this,
        "NativeResetKey",
        _("Close native clipboard panel"),
        {Key("Escape")},
        KeyListConstrain({KeyConstrainFlag::AllowModifierLess})};
    KeyListOption clearHistoryKey{
        this,
        "ClearHistoryKey",
        _("Clear clipboard history"),
        {Key("BackSpace"), Key("Delete")},
        KeyListConstrain({KeyConstrainFlag::AllowModifierLess})};
    KeyListOption selectionKeys{
        this,
        "SelectionKeys",
        _("Select clipboard item by position"),
        {Key("1"), Key("2"), Key("3"), Key("4"), Key("5"),
         Key("6"), Key("7"), Key("8"), Key("9"), Key("0")},
        KeyListConstrain({KeyConstrainFlag::AllowModifierLess})};
    KeyListOption pasteSelectedKey{
        this,
        "PasteSelectedKey",
        _("Paste selected clipboard item"),
        {Key("space"), Key("Return"), Key("KP_Enter")},
        KeyListConstrain({KeyConstrainFlag::AllowModifierLess})};
    KeyListOption previousCandidateKey{
        this,
        "PreviousCandidateKey",
        _("Select previous clipboard item"),
        {Key("Up")},
        KeyListConstrain({KeyConstrainFlag::AllowModifierLess})};
    KeyListOption nextCandidateKey{
        this,
        "NextCandidateKey",
        _("Select next clipboard item"),
        {Key("Down")},
        KeyListConstrain({KeyConstrainFlag::AllowModifierLess})};
    KeyListOption movePinnedUpKey{
        this,
        "MovePinnedUpKey",
        _("Move pinned clipboard item up"),
        {Key("Control+Up")},
        KeyListConstrain()};
    KeyListOption movePinnedDownKey{
        this,
        "MovePinnedDownKey",
        _("Move pinned clipboard item down"),
        {Key("Control+Down")},
        KeyListConstrain()};
    KeyListOption previousPageKey{
        this,
        "PreviousPageKey",
        _("Previous clipboard page"),
        {Key("bracketleft"), Key("Page_Up")},
        KeyListConstrain({KeyConstrainFlag::AllowModifierLess})};
    KeyListOption nextPageKey{
        this,
        "NextPageKey",
        _("Next clipboard page"),
        {Key("bracketright"), Key("Page_Down")},
        KeyListConstrain({KeyConstrainFlag::AllowModifierLess})};
    KeyListOption pastePrimaryKey{
        this, "PastePrimaryKey", _("Paste Primary"), {}, KeyListConstrain()};
    Option<int, IntConstrain> numOfEntries{this, "Number of entries",
                                           _("Number of entries"), 5,
                                           IntConstrain(3, 30)};
    Option<int, IntConstrain> candidatePageSize{
        this,
        "CandidatePageSize",
        _("Clipboard candidates per page"),
        10,
        IntConstrain(1, 30)};
    ConditionalHidden<isAndroid(),
                      OptionWithAnnotation<bool, ToolTipAnnotation>>
        ignorePasswordFromPasswordManager{
            this,
            "IgnorePasswordFromPasswordManager",
            _("Do not show password from password managers"),
            false,
            {},
            {},
            {_("When copying password from a password manager, if the password "
               "manager supports marking the clipboard content as password, "
               "this clipboard update will be ignored.")}};
    ConditionalHidden<isAndroid(), Option<bool>> showPassword{
        this, "ShowPassword", _("Display passwords as plain text"), false};
    ConditionalHidden<
        isAndroid(),
        Option<int, IntConstrain, DefaultMarshaller<int>, ToolTipAnnotation>>
        clearPasswordAfter{this,
                           "ClearPasswordAfter",
                           _("Seconds before clearing password"),
                           30,
                           IntConstrain(0, 300),
                           {},
                           {_("0 means never clear password.")}};);

class ClipboardTempMode;
class Clipboard final : public AddonInstance {
    static constexpr char configFile[] = "conf/clipboard.conf";

public:
    Clipboard(Instance *instance);
    ~Clipboard();

    Instance *instance() { return instance_; }

    void clear();
    void reset(InputContext *inputContext);
    void updateUI(InputContext *inputContext);
    ClipboardCandidateInfo currentEntry(const InputContext *inputContext) const;
    ClipboardCandidateInfo removeCurrentEntry(InputContext *inputContext);

    void reloadConfig() override;

    const Configuration *getConfig() const override { return &config_; }
    void setConfig(const RawConfig &config) override {
        config_.load(config, true);
        safeSaveAsIni(config_, configFile);
    }

    std::string primary(const InputContext *ic) const;
    std::string clipboard(const InputContext *ic) const;

    void setPrimary(const std::string &name, const std::string &str);
    void setClipboard(const std::string &name, const std::string &str);
    void setPrimaryV2(const std::string &name, const std::string &str,
                      bool password);
    void setClipboardV2(const std::string &name, const std::string &str,
                        bool password);
    const auto &config() const { return config_; }

#ifdef ENABLE_X11
    FCITX_ADDON_DEPENDENCY_LOADER(xcb, instance_->addonManager());
#endif

private:
    FCITX_ADDON_EXPORT_FUNCTION(Clipboard, primary);
    FCITX_ADDON_EXPORT_FUNCTION(Clipboard, clipboard);
    FCITX_ADDON_EXPORT_FUNCTION(Clipboard, currentEntry);
    FCITX_ADDON_EXPORT_FUNCTION(Clipboard, removeCurrentEntry);
    FCITX_ADDON_EXPORT_FUNCTION(Clipboard, setPrimary);
    FCITX_ADDON_EXPORT_FUNCTION(Clipboard, setClipboard);
    FCITX_ADDON_EXPORT_FUNCTION(Clipboard, setPrimaryV2);
    FCITX_ADDON_EXPORT_FUNCTION(Clipboard, setClipboardV2);
#ifdef WAYLAND_FOUND
    FCITX_ADDON_DEPENDENCY_LOADER(wayland, instance_->addonManager());
#endif

    void refreshPasswordTimer();
    void setPrimaryEntry(const std::string &name, ClipboardEntry entry);
    void setClipboardEntry(const std::string &name,
                           const ClipboardEntry &entry);

    Instance *instance_;
    std::vector<std::unique_ptr<fcitx::HandlerTableEntry<fcitx::EventHandler>>>
        eventHandlers_;
    ClipboardConfig config_;
    std::unique_ptr<ClipboardTempMode> tempMode_;

#ifdef ENABLE_X11
    std::unique_ptr<HandlerTableEntry<XCBConnectionCreated>>
        xcbCreatedCallback_;
    std::unique_ptr<HandlerTableEntry<XCBConnectionClosed>> xcbClosedCallback_;
    std::unordered_map<std::string, std::unique_ptr<XcbClipboard>>
        xcbClipboards_;
#endif

#ifdef WAYLAND_FOUND
    std::unique_ptr<HandlerTableEntry<WaylandConnectionCreated>>
        waylandCreatedCallback_;
    std::unique_ptr<HandlerTableEntry<WaylandConnectionClosed>>
        waylandClosedCallback_;
    std::unordered_map<std::string, std::unique_ptr<WaylandClipboard>>
        waylandClipboards_;
#endif
    OrderedSet<ClipboardEntry> history_;
    ClipboardEntry primary_;
    std::unique_ptr<EventSourceTime> clearPasswordTimer_;
};

FCITX_DECLARE_LOG_CATEGORY(clipboard_log);

#define FCITX_CLIPBOARD_DEBUG() FCITX_LOGC(::fcitx::clipboard_log, Debug)
} // namespace fcitx

#endif // _FCITX_MODULES_CLIPBOARD_CLIPBOARD_H_
