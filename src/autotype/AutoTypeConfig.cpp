/*
  Qt KeePass - Auto-Type Configuration Helper Implementation
  Reference: MFC/MFC-KeePass/WinGUI/AddEntryDlg.cpp (auto-type methods)
*/

#include "AutoTypeConfig.h"
#include <QStringList>

// Define constants
const QString AutoTypeConfig::AUTO_TYPE_PREFIX = QStringLiteral("Auto-Type: ");
const QString AutoTypeConfig::AUTO_TYPE_WINDOW_PREFIX = QStringLiteral("Auto-Type-Window: ");

void AutoTypeConfig::parseFromNotes(const QString& notes,
                                     QString& outSequence,
                                     QString& outWindowTitle)
{
    // Reference: MFC stores these as lines in the notes field
    // "Auto-Type: {USERNAME}{TAB}{PASSWORD}{ENTER}"
    // "Auto-Type-Window: Mozilla Firefox"

    outSequence.clear();
    outWindowTitle.clear();

    if (notes.isEmpty()) {
        return;
    }

    QStringList lines = notes.split('\n', Qt::KeepEmptyParts);

    for (const QString& line : lines) {
        QString trimmedLine = line.trimmed();

        // Check for auto-type sequence
        if (trimmedLine.startsWith(AUTO_TYPE_PREFIX)) {
            outSequence = trimmedLine.mid(AUTO_TYPE_PREFIX.length()).trimmed();
        }
        // Check for auto-type window
        else if (trimmedLine.startsWith(AUTO_TYPE_WINDOW_PREFIX)) {
            outWindowTitle = trimmedLine.mid(AUTO_TYPE_WINDOW_PREFIX.length()).trimmed();
        }
    }
}

QString AutoTypeConfig::formatToNotes(const QString& existingNotes,
                                     const QString& sequence,
                                     const QString& windowTitle)
{
    // Reference: MFC appends these lines to notes
    // We'll preserve existing content and update/add auto-type lines

    // First, remove any existing auto-type configuration
    QString cleanNotes = stripAutoTypeConfig(existingNotes);

    // Build result
    QString result = cleanNotes;

    // Add new auto-type configuration if provided
    if (!sequence.isEmpty()) {
        if (!result.isEmpty() && !result.endsWith('\n')) {
            result += '\n';
        }
        result += AUTO_TYPE_PREFIX + sequence;
    }

    if (!windowTitle.isEmpty()) {
        if (!result.isEmpty() && !result.endsWith('\n')) {
            result += '\n';
        }
        result += AUTO_TYPE_WINDOW_PREFIX + windowTitle;
    }

    return result;
}

QString AutoTypeConfig::stripAutoTypeConfig(const QString& notes)
{
    if (notes.isEmpty()) {
        return {};
    }

    QStringList lines = notes.split('\n', Qt::KeepEmptyParts);
    QStringList cleanedLines;

    for (const QString& line : lines) {
        QString trimmedLine = line.trimmed();

        // Skip auto-type configuration lines
        if (trimmedLine.startsWith(AUTO_TYPE_PREFIX) ||
            trimmedLine.startsWith(AUTO_TYPE_WINDOW_PREFIX)) {
            continue;
        }

        cleanedLines.append(line);
    }

    // Join back and trim trailing empty lines
    QString result = cleanedLines.join('\n');
    while (result.endsWith('\n')) {
        result.chop(1);
    }

    return result;
}

bool AutoTypeConfig::hasAutoTypeConfig(const QString& notes)
{
    if (notes.isEmpty()) {
        return false;
    }

    return notes.contains(AUTO_TYPE_PREFIX) ||
           notes.contains(AUTO_TYPE_WINDOW_PREFIX);
}
