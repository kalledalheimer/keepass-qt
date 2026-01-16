/*
  Qt KeePass - Auto-Type Window Title Matcher

  Matches window titles against entry auto-type-window patterns.
  Supports wildcards and fallback to entry title matching.

  Reference: MFC PwSafeDlg.cpp OnHotKey (lines 10403-10491)
*/

#ifndef AUTOTYPEMATCHER_H
#define AUTOTYPEMATCHER_H

#include <QString>
#include <QList>
#include "../core/PwStructs.h"

class PwManager;

/// Auto-type window pattern matcher
class AutoTypeMatcher
{
public:
    /// Find entries that match the given window title
    /// @param windowTitle Target window title (not normalized)
    /// @param pwManager Password database manager
    /// @param normalizeDashes Apply dash normalization to matching
    /// @return List of matching entries
    [[nodiscard]] static QList<PW_ENTRY*> findMatchingEntries(
        const QString& windowTitle,
        PwManager* pwManager,
        bool normalizeDashes = true);

    /// Check if an entry matches the window title
    /// @param entry Entry to check
    /// @param windowTitle Target window title (not normalized)
    /// @param pwManager Password database manager (for field substitution)
    /// @param normalizeDashes Apply dash normalization to matching
    /// @return true if entry matches
    [[nodiscard]] static bool entryMatches(
        const PW_ENTRY* entry,
        const QString& windowTitle,
        PwManager* pwManager,
        bool normalizeDashes = true);

private:
    /// Match window title against a pattern with wildcards
    /// @param windowTitle Normalized window title
    /// @param pattern Normalized pattern (may contain * wildcards)
    /// @return true if pattern matches
    [[nodiscard]] static bool matchPattern(
        const QString& windowTitle,
        const QString& pattern);

    /// Extract auto-type-window patterns from entry notes
    /// @param notes Entry notes field
    /// @return List of window patterns (may be empty)
    [[nodiscard]] static QList<QString> extractWindowPatterns(const QString& notes);

public:
    AutoTypeMatcher() = delete;  // Static class - no instantiation
};

#endif // AUTOTYPEMATCHER_H
