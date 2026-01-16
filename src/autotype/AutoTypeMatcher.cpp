/*
  Qt KeePass - Auto-Type Window Title Matcher Implementation
  Reference: MFC PwSafeDlg.cpp OnHotKey (lines 10403-10491)
*/

#include "AutoTypeMatcher.h"
#include "AutoTypeConfig.h"
#include "../core/PwManager.h"
#include "../core/util/PwUtil.h"
#include <QDateTime>

QList<PW_ENTRY*> AutoTypeMatcher::findMatchingEntries(
    const QString& windowTitle,
    PwManager* pwManager,
    bool normalizeDashes)
{
    QList<PW_ENTRY*> matches;

    if (!pwManager || windowTitle.isEmpty()) {
        return matches;
    }

    // Normalize the target window title once
    QString normalizedTitle = AutoTypeConfig::normalizeWindowTitle(windowTitle, normalizeDashes);

    // Get current time for expiry checking
    PW_TIME tNow;
    PwUtil::getCurrentTime(&tNow);

    // Get backup group IDs to exclude
    quint32 backupGroupId1 = pwManager->getGroupId(PWS_BACKUPGROUP_SRC);
    quint32 backupGroupId2 = pwManager->getGroupId(PWS_BACKUPGROUP);

    // Iterate through all entries
    quint32 entryCount = pwManager->getNumberOfEntries();
    for (quint32 i = 0; i < entryCount; ++i) {
        PW_ENTRY* entry = pwManager->getEntry(i);
        if (!entry) {
            continue;
        }

        // Skip entries in backup groups
        if (entry->uGroupId == backupGroupId1 || entry->uGroupId == backupGroupId2) {
            continue;
        }

        // Skip expired entries
        if (PwUtil::compareTime(&tNow, &entry->tExpire) > 0) {
            continue;
        }

        // Check if entry matches
        if (entryMatches(entry, normalizedTitle, pwManager, normalizeDashes)) {
            matches.append(entry);
        }
    }

    return matches;
}

bool AutoTypeMatcher::entryMatches(
    const PW_ENTRY* entry,
    const QString& windowTitle,
    PwManager* pwManager,
    bool normalizeDashes)
{
    Q_UNUSED(pwManager);  // Reserved for future field substitution (SprCompile)

    if (!entry) {
        return false;
    }

    // Get normalized window title
    QString normalizedTitle = windowTitle;
    if (normalizedTitle != windowTitle) {
        // windowTitle might already be normalized, if not, normalize it
        normalizedTitle = AutoTypeConfig::normalizeWindowTitle(windowTitle, normalizeDashes);
    }

    // Get entry notes
    QString notes;
    if (entry->pszAdditional) {
        notes = QString::fromUtf8(entry->pszAdditional);
    }

    // Extract auto-type-window patterns from notes
    QList<QString> patterns = extractWindowPatterns(notes);

    // Check each pattern
    for (const QString& pattern : patterns) {
        // Normalize the pattern (lowercase + dashes)
        QString normalizedPattern = AutoTypeConfig::normalizeWindowTitle(pattern, normalizeDashes);

        // Match against the pattern
        if (matchPattern(normalizedTitle, normalizedPattern)) {
            return true;
        }
    }

    // If no auto-type-window patterns, fall back to entry title substring match
    // Reference: MFC PwSafeDlg.cpp:10472-10486
    if (patterns.isEmpty()) {
        QString entryTitle;
        if (entry->pszTitle) {
            entryTitle = QString::fromUtf8(entry->pszTitle);
        }

        if (!entryTitle.isEmpty()) {
            // Normalize entry title
            QString normalizedEntryTitle = AutoTypeConfig::normalizeWindowTitle(
                entryTitle, normalizeDashes);

            // Check if window title contains entry title (substring match)
            if (normalizedTitle.contains(normalizedEntryTitle)) {
                return true;
            }
        }
    }

    return false;
}

bool AutoTypeMatcher::matchPattern(const QString& windowTitle, const QString& pattern)
{
    // Reference: MFC PwSafeDlg.cpp:10427-10455
    // Supports wildcards: *text*, text*, *text, exact match

    if (pattern.isEmpty()) {
        return false;
    }

    // Check for wildcard positions
    bool startsWithWildcard = pattern.startsWith('*');
    bool endsWithWildcard = pattern.endsWith('*');

    // Remove wildcards to get the search text
    QString searchText = pattern;
    if (startsWithWildcard) {
        searchText = searchText.mid(1);
    }
    if (endsWithWildcard) {
        searchText.chop(1);
    }

    int searchLen = searchText.length();
    int titleLen = windowTitle.length();

    // Must be able to fit
    if (searchLen > titleLen) {
        return false;
    }

    // Apply matching logic based on wildcard positions
    if (startsWithWildcard && endsWithWildcard) {
        // *text* - contains
        return windowTitle.contains(searchText);
    } else if (startsWithWildcard) {
        // *text - ends with
        return windowTitle.endsWith(searchText);
    } else if (endsWithWildcard) {
        // text* - starts with
        return windowTitle.startsWith(searchText);
    } else {
        // text - exact match
        return windowTitle == searchText;
    }
}

QList<QString> AutoTypeMatcher::extractWindowPatterns(const QString& notes)
{
    // Reference: MFC PwSafeDlg.cpp:10414-10471
    // Extracts patterns like:
    // "auto-type-window: Mozilla Firefox"
    // "auto-type-window-1: Google Chrome"
    // "auto-type-window-2: *Safari*"

    QList<QString> patterns;

    if (notes.isEmpty()) {
        return patterns;
    }

    QStringList lines = notes.split('\n', Qt::KeepEmptyParts);

    for (const QString& line : lines) {
        QString trimmed = line.trimmed();

        // Check for auto-type-window prefix (case-sensitive in MFC)
        if (trimmed.startsWith(AutoTypeConfig::AUTO_TYPE_WINDOW_PREFIX)) {
            QString pattern = trimmed.mid(
                AutoTypeConfig::AUTO_TYPE_WINDOW_PREFIX.length()).trimmed();
            if (!pattern.isEmpty()) {
                patterns.append(pattern);
            }
        }
    }

    return patterns;
}
