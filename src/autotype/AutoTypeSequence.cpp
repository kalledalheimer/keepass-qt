/*
  Qt KeePass - Auto-Type Sequence Parser

  Reference: MFC WinGUI/Util/SprEngine/SprEngine.cpp
*/

#include "AutoTypeSequence.h"
#include "../core/PwManager.h"
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

    QString remaining = sequence;
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
                                         PW_ENTRY* entry,
                                         PwManager* pwManager)
{
    QString name = placeholder.trimmed().toUpper();

    // Entry field placeholders
    if (name == "USERNAME" || name == "USER") {
        if (entry->pszUserName && entry->pszUserName[0] != '\0') {
            actions.append(AutoTypeAction::makeText(QString::fromUtf8(entry->pszUserName)));
        }
        return true;
    }

    if (name == "PASSWORD" || name == "PASS" || name == "PWD") {
        if (entry->pszPassword && pwManager) {
            // Unlock password to access it
            pwManager->unlockEntryPassword(entry);
            QString password = QString::fromUtf8(entry->pszPassword);
            pwManager->lockEntryPassword(entry);

            if (!password.isEmpty()) {
                actions.append(AutoTypeAction::makeText(password));
            }
        }
        return true;
    }

    if (name == "TITLE") {
        if (entry->pszTitle && entry->pszTitle[0] != '\0') {
            actions.append(AutoTypeAction::makeText(QString::fromUtf8(entry->pszTitle)));
        }
        return true;
    }

    if (name == "URL") {
        if (entry->pszURL && entry->pszURL[0] != '\0') {
            actions.append(AutoTypeAction::makeText(QString::fromUtf8(entry->pszURL)));
        }
        return true;
    }

    if (name == "NOTES") {
        if (entry->pszAdditional && entry->pszAdditional[0] != '\0') {
            QString notes = QString::fromUtf8(entry->pszAdditional);
            // Only use first line
            int newlinePos = notes.indexOf('\n');
            if (newlinePos != -1) {
                notes = notes.left(newlinePos);
            }
            if (!notes.isEmpty()) {
                actions.append(AutoTypeAction::makeText(notes));
            }
        }
        return true;
    }

    // Special key placeholders
    AutoTypeKey key = keyForPlaceholder(name);
    if (key != AutoTypeKey::Tab || name == "TAB") {  // Tab is default, so check explicitly
        actions.append(AutoTypeAction::makeKey(key));
        return true;
    }

    // Delay placeholder
    if (name.startsWith("DELAY ")) {
        QString delayStr = name.mid(6).trimmed();
        bool ok;
        int delayMs = delayStr.toInt(&ok);
        if (ok && delayMs >= 0) {
            actions.append(AutoTypeAction::makeDelay(delayMs));
            return true;
        } else {
            m_lastError = QString("Invalid delay value: %1").arg(delayStr);
            return false;
        }
    }

    // Unknown placeholder - could warn but we'll skip it for compatibility
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
