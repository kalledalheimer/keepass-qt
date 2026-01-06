/*
  Qt KeePass - Auto-Type Sequence Parser

  Parses auto-type sequence strings and compiles them with entry data.

  Sequence format (KeePass v1.x compatible):
    {USERNAME}  - Entry username
    {PASSWORD}  - Entry password
    {TITLE}     - Entry title
    {URL}       - Entry URL
    {NOTES}     - Entry notes (first line)
    {TAB}       - Tab key
    {ENTER}     - Enter key
    {SPACE}     - Space key
    {BACKSPACE} - Backspace key
    {DELETE}    - Delete key
    {INSERT}    - Insert key
    {HOME}      - Home key
    {END}       - End key
    {PGUP}      - Page Up key
    {PGDN}      - Page Down key
    {LEFT}      - Left arrow
    {RIGHT}     - Right arrow
    {UP}        - Up arrow
    {DOWN}      - Down arrow
    {DELAY X}   - Wait X milliseconds
    Plain text  - Type as-is

  Default sequence: {USERNAME}{TAB}{PASSWORD}{ENTER}

  Reference: MFC WinGUI/Util/SprEngine/SprEngine.cpp
*/

#ifndef AUTOTYPESEQUENCE_H
#define AUTOTYPESEQUENCE_H

#include "platform/AutoTypePlatform.h"
#include "../core/PwStructs.h"
#include <QString>
#include <QList>

class PwManager;

class AutoTypeSequence
{
public:
    AutoTypeSequence();

    /// Parse and compile auto-type sequence
    /// @param sequence Sequence string (e.g., "{USERNAME}{TAB}{PASSWORD}{ENTER}")
    /// @param entry Entry to get field values from
    /// @param pwManager Password manager (for unlocking password)
    /// @return List of auto-type actions ready to execute
    QList<AutoTypeAction> compile(const QString& sequence,
                                   PW_ENTRY* entry,
                                   PwManager* pwManager);

    /// Get last parse error (if compile returns empty list)
    QString lastError() const { return m_lastError; }

    /// Get default auto-type sequence
    static QString defaultSequence() {
        return "{USERNAME}{TAB}{PASSWORD}{ENTER}";
    }

private:
    QString m_lastError;

    // Parse a single placeholder like "{USERNAME}" or "{DELAY 100}"
    bool parsePlaceholder(const QString& placeholder,
                          QList<AutoTypeAction>& actions,
                          PW_ENTRY* entry,
                          PwManager* pwManager);

    // Convert placeholder name to special key
    AutoTypeKey keyForPlaceholder(const QString& name) const;
};

#endif // AUTOTYPESEQUENCE_H
