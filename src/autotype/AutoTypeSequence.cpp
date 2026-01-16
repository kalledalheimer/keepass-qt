/*
  Qt KeePass - Auto-Type Sequence Parser

  Reference: MFC WinGUI/Util/SprEngine/SprEngine.cpp
*/

#include "AutoTypeSequence.h"
#include "../core/PwManager.h"
#include "../core/SprEngine.h"
#include <QRegularExpression>

AutoTypeSequence::AutoTypeSequence()
{
}

QList<AutoTypeAction> AutoTypeSequence::compile(const QString& sequence,
                                                  PW_ENTRY* entry,
                                                  PwManager* pwManager)
{
    m_lastError.clear();
    QList<AutoTypeAction> actions;

    if (!entry) {
        m_lastError = "Invalid entry";
        return actions;
    }

    // Use SprEngine to resolve data placeholders first
    // SprEngine handles: {USERNAME}, {PASSWORD}, {TITLE}, {URL}, {NOTES},
    //                    {DT_*}, {REF:*}, {CLEARFIELD}, {APPDIR}, etc.
    // Unrecognized placeholders (like {TAB}, {ENTER}) are preserved as-is
    SprEngine sprEngine;
    SprContentFlags sprFlags;
    sprFlags.escapeForAutoType = true;  // Escape special auto-type characters in field values

    QString remaining = sprEngine.compile(sequence, entry, pwManager, sprFlags);
    int pos = 0;

    while (pos < remaining.length()) {
        // Look for placeholder start
        int placeholderStart = remaining.indexOf('{', pos);

        if (placeholderStart == -1) {
            // No more placeholders, add remaining text
            QString text = remaining.mid(pos);
            if (!text.isEmpty()) {
                actions.append(AutoTypeAction::makeText(text));
            }
            break;
        }

        // Add text before placeholder
        if (placeholderStart > pos) {
            QString text = remaining.mid(pos, placeholderStart - pos);
            actions.append(AutoTypeAction::makeText(text));
        }

        // Find placeholder end
        int placeholderEnd = remaining.indexOf('}', placeholderStart);
        if (placeholderEnd == -1) {
            m_lastError = QString("Unclosed placeholder at position %1").arg(placeholderStart);
            return QList<AutoTypeAction>();  // Return empty on error
        }

        // Extract placeholder content
        QString placeholder = remaining.mid(placeholderStart + 1, placeholderEnd - placeholderStart - 1);

        // Parse placeholder
        if (!parsePlaceholder(placeholder, actions, entry, pwManager)) {
            // Error already set in parsePlaceholder
            return QList<AutoTypeAction>();
        }

        pos = placeholderEnd + 1;
    }

    return actions;
}

bool AutoTypeSequence::parsePlaceholder(const QString& placeholder,
                                         QList<AutoTypeAction>& actions,
                                         PW_ENTRY* /* entry */,
                                         PwManager* /* pwManager */)
{
    // Note: Entry field placeholders (USERNAME, PASSWORD, TITLE, URL, NOTES)
    // are now handled by SprEngine before this method is called.
    // This method only handles keyboard/action placeholders.

    QString name = placeholder.trimmed().toUpper();

    // Special key placeholders
    AutoTypeKey key = keyForPlaceholder(name);
    if (key != AutoTypeKey::Tab || name == "TAB") {  // Tab is default, so check explicitly
        actions.append(AutoTypeAction::makeKey(key));
        return true;
    }

    // Delay placeholder: {DELAY X} or {DELAY=X}
    if (name.startsWith("DELAY ") || name.startsWith("DELAY=")) {
        int offset = name.startsWith("DELAY=") ? 6 : 6;
        QString delayStr = name.mid(offset).trimmed();
        bool ok;
        int delayMs = delayStr.toInt(&ok);
        if (ok && delayMs >= 0) {
            actions.append(AutoTypeAction::makeDelay(delayMs));
            return true;
        }
        m_lastError = QString("Invalid delay value: %1").arg(delayStr);
        return false;
    }

    // Modifier keys with content: +{...} (Shift), ^{...} (Ctrl), %{...} (Alt)
    // These should have been handled by the caller, but if we get here
    // with a + or similar, treat as literal text
    if (name == "PLUS") {
        actions.append(AutoTypeAction::makeText("+"));
        return true;
    }
    if (name == "CARET") {
        actions.append(AutoTypeAction::makeText("^"));
        return true;
    }
    if (name == "PERCENT") {
        actions.append(AutoTypeAction::makeText("%"));
        return true;
    }
    if (name == "TILDE") {
        actions.append(AutoTypeAction::makeText("~"));
        return true;
    }

    // Unknown placeholder after SprEngine processing
    // This shouldn't happen normally - SprEngine keeps unrecognized placeholders
    // and we handle all known keyboard placeholders above
    m_lastError = QString("Unknown placeholder: {%1}").arg(placeholder);
    return false;
}

AutoTypeKey AutoTypeSequence::keyForPlaceholder(const QString& name) const
{
    if (name == "TAB") return AutoTypeKey::Tab;
    if (name == "ENTER") return AutoTypeKey::Enter;
    if (name == "SPACE") return AutoTypeKey::Space;
    if (name == "BACKSPACE" || name == "BKSP" || name == "BS") return AutoTypeKey::Backspace;
    if (name == "DELETE" || name == "DEL") return AutoTypeKey::Delete;
    if (name == "INSERT" || name == "INS") return AutoTypeKey::Insert;
    if (name == "HOME") return AutoTypeKey::Home;
    if (name == "END") return AutoTypeKey::End;
    if (name == "PGUP" || name == "PAGEUP") return AutoTypeKey::PageUp;
    if (name == "PGDN" || name == "PAGEDOWN") return AutoTypeKey::PageDown;
    if (name == "LEFT") return AutoTypeKey::Left;
    if (name == "RIGHT") return AutoTypeKey::Right;
    if (name == "UP") return AutoTypeKey::Up;
    if (name == "DOWN") return AutoTypeKey::Down;
    if (name == "ESCAPE" || name == "ESC") return AutoTypeKey::Escape;

    // Function keys
    if (name == "F1") return AutoTypeKey::F1;
    if (name == "F2") return AutoTypeKey::F2;
    if (name == "F3") return AutoTypeKey::F3;
    if (name == "F4") return AutoTypeKey::F4;
    if (name == "F5") return AutoTypeKey::F5;
    if (name == "F6") return AutoTypeKey::F6;
    if (name == "F7") return AutoTypeKey::F7;
    if (name == "F8") return AutoTypeKey::F8;
    if (name == "F9") return AutoTypeKey::F9;
    if (name == "F10") return AutoTypeKey::F10;
    if (name == "F11") return AutoTypeKey::F11;
    if (name == "F12") return AutoTypeKey::F12;

    // Not a recognized key
    return AutoTypeKey::Tab;  // Default, but caller should check
}
