/*
  Qt KeePass - Password Import

  Imports password data from various formats (CodeWallet, PwSafe, KeePass merge).
  Matches MFC CPwImport functionality for format compatibility.
*/

#ifndef PWIMPORT_H
#define PWIMPORT_H

#include <QString>
#include <QStringList>
#include "../PwManager.h"
#include "../PwStructs.h"

// Import format constants
constexpr quint32 PWIMP_CWALLET = 1;     // CodeWallet TXT
constexpr quint32 PWIMP_PWSAFE = 2;      // Password Safe v3 TXT
constexpr quint32 PWIMP_KEEPASS = 3;     // KeePass KDB merge

// KeePass merge modes
enum class KdbMergeMode : quint8 {
    CREATE_NEW_UUIDS = 0,      // Generate new UUIDs for all items
    OVERWRITE_ALWAYS = 1,      // Replace existing items unconditionally
    OVERWRITE_IF_NEWER = 2     // Replace only if source is newer
};

class PwImport
{
public:
    // Import from file
    static bool importFromFile(PwManager *manager, const QString &filePath,
                              quint32 format, QString *errorMsg = nullptr);

    // Import/Merge from another KeePass database
    static bool mergeDatabase(PwManager *targetManager,
                             const QString &sourceFile,
                             const QString &masterPassword,
                             KdbMergeMode mergeMode,
                             QString *errorMsg = nullptr);

private:
    // Format-specific import methods
    static bool importCodeWallet(PwManager *manager, const QString &filePath,
                                QString *errorMsg);

    static bool importPwSafe(PwManager *manager, const QString &filePath,
                           QString *errorMsg);

    // Helper methods
    static QString detectEncoding(const QByteArray &data);
    static QStringList readLines(const QString &filePath, QString *errorMsg);
    static quint32 findOrCreateGroup(PwManager *manager, const QString &groupName);
    static quint32 getPreferredIcon(const QString &groupName);
    static QString extractField(const QString &line, const QString &prefix);
    static bool isFieldPrefix(const QString &line, const QStringList &prefixes,
                             QString *value);

    // Password Safe specific
    static void splitPwSafeTitle(const QString &combined, QString &group,
                                QString &title);
    static bool isDomainExtension(const QString &ext);
};

#endif // PWIMPORT_H
