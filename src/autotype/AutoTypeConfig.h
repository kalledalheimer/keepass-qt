/*
  Qt KeePass - Auto-Type Configuration Helper

  Manages parsing and formatting of auto-type configuration
  stored in entry notes field.

  MFC KeePass stores auto-type config in notes using special prefixes:
  - "Auto-Type: <sequence>" - Custom auto-type sequence
  - "Auto-Type-Window: <window_title>" - Target window title pattern
*/

#ifndef AUTOTYPECONFIG_H
#define AUTOTYPECONFIG_H

#include <QString>

class AutoTypeConfig
{
public:
    // Parse auto-type configuration from notes text
    static void parseFromNotes(const QString& notes,
                               QString& outSequence,
                               QString& outWindowTitle);

    // Format auto-type configuration into notes text
    // Preserves other note content while updating auto-type lines
    static QString formatToNotes(const QString& existingNotes,
                                const QString& sequence,
                                const QString& windowTitle);

    // Get notes without auto-type configuration lines
    static QString stripAutoTypeConfig(const QString& notes);

    // Check if notes contain auto-type configuration
    static bool hasAutoTypeConfig(const QString& notes);

    // Window title normalization (for matching)
    // Reference: MFC StrUtil.cpp SU_NormalizeDashes, PwSafeDlg.cpp _AutoTypeNormalizeWindowText
    static QString normalizeWindowTitle(const QString& title, bool normalizeDashes = true);

    // Normalize various Unicode dash characters to standard hyphen (U+002D)
    // Replaces: U+2010-2015, U+2212 with '-'
    static QString normalizeDashes(const QString& text);

    // Apply Internet Explorer fix to auto-type sequence
    // Prepends {DELAY 50}1{DELAY 50}{BACKSPACE} if window title contains IE/Maxthon
    // Reference: MFC PwSafeDlg.cpp:10033-10038
    static QString applyIEFix(const QString& sequence, const QString& windowTitle, bool iefixEnabled);

    // Constants for note prefixes
    static const QString AUTO_TYPE_PREFIX;
    static const QString AUTO_TYPE_WINDOW_PREFIX;
};

#endif // AUTOTYPECONFIG_H
