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

QString AutoTypeConfig::normalizeWindowTitle(const QString& title, bool normalizeDashes)
{
    // Reference: MFC PwSafeDlg.cpp:10126-10136 (_AutoTypeNormalizeWindowText)
    if (title.isEmpty()) {
        return {};
    }

    QString normalized = title.toLower();

    if (normalizeDashes) {
        normalized = AutoTypeConfig::normalizeDashes(normalized);
    }

    return normalized;
}

QString AutoTypeConfig::normalizeDashes(const QString& text)
{
    // Reference: MFC StrUtil.cpp:992-1048 (SU_NormalizeDashes, SU_GetNormDashes)
    // Replaces various Unicode dash characters with standard hyphen-minus (U+002D)

    if (text.isEmpty()) {
        return {};
    }

    QString result = text;

    // List of Unicode dash characters to normalize (from MFC SU_GetNormDashes)
    // All of these are replaced with standard hyphen '-' (U+002D)
    static const QList<QChar> dashChars = {
        QChar(0x2010),  // Hyphen
        QChar(0x2011),  // Non-breaking hyphen
        QChar(0x2012),  // Figure dash
        QChar(0x2013),  // En dash
        QChar(0x2014),  // Em dash
        QChar(0x2015),  // Horizontal bar
        QChar(0x2212)   // Minus sign
    };

    // Replace all dash variants with standard hyphen
    for (const QChar& dashChar : dashChars) {
        result.replace(dashChar, '-');
    }

    return result;
}

QString AutoTypeConfig::applyIEFix(const QString& sequence, const QString& windowTitle,
                                     bool iefixEnabled)
{
    // Reference: MFC PwSafeDlg.cpp:10033-10038
    // If IE fix is enabled and window title contains IE or Maxthon,
    // prepend {DELAY 50}1{DELAY 50}{BACKSPACE} to work around auto-complete issues

    if (!iefixEnabled || windowTitle.isEmpty()) {
        return sequence;
    }

    // Check if window title contains Internet Explorer or Maxthon (case-insensitive)
    QString lowerTitle = windowTitle.toLower();
    if (lowerTitle.contains(QStringLiteral("internet explorer")) ||
        lowerTitle.contains(QStringLiteral("maxthon"))) {
        // Prepend IE fix sequence
        return QStringLiteral("{DELAY 50}1{DELAY 50}{BACKSPACE}") + sequence;
    }

    return sequence;
}
