/*
  Qt KeePass - String Placeholder Replacement Engine

  Resolves placeholders in strings:
  - Entry fields: {USERNAME}, {PASSWORD}, {TITLE}, {URL}, {NOTES}
  - DateTime: {DT_SIMPLE}, {DT_YEAR}, {DT_MONTH}, etc.
  - Field references: {REF:Field@SearchType:SearchValue}
  - Special: {CLEARFIELD}, {APPDIR}

  Reference: MFC WinGUI/Util/SprEngine/SprEngine.cpp
*/

#ifndef SPRENGINE_H
#define SPRENGINE_H

#include <QString>
#include <QMap>
#include <QDateTime>

// Forward declarations
struct _PW_ENTRY;
typedef struct _PW_ENTRY PW_ENTRY;
class PwManager;

// Content transformation flags
struct SprContentFlags
{
    bool escapeForAutoType = false;     // Apply auto-type encoding
    bool escapeForCommandLine = false;  // Quote for command line execution
};

class SprEngine
{
public:
    SprEngine();

    /// Compile a string by resolving all placeholders
    /// @param text Input string with placeholders
    /// @param entry Current entry for field values (can be null)
    /// @param database Database for field references (can be null)
    /// @param flags Content transformation flags
    /// @return String with all placeholders resolved
    QString compile(const QString& text,
                    PW_ENTRY* entry,
                    PwManager* database,
                    const SprContentFlags& flags = SprContentFlags());

    /// Transform content with escaping (after placeholder resolution)
    /// @param content Resolved content
    /// @param flags Transformation flags
    /// @return Transformed content
    static QString transformContent(const QString& content, const SprContentFlags& flags);

    /// Get the default auto-type CLEARFIELD sequence
    static QString clearFieldSequence();

    /// Maximum recursion depth for nested placeholders
    static constexpr int MaxRecursionDepth = 12;

    /// Maximum iterations for field reference resolution
    static constexpr int MaxRefIterations = 20;

private:
    // Internal compile with recursion tracking
    QString compileInternal(const QString& text,
                            PW_ENTRY* entry,
                            PwManager* database,
                            const SprContentFlags& flags,
                            int recursionLevel,
                            QMap<QString, QString>& refCache);

    // Resolve a single placeholder
    QString resolvePlaceholder(const QString& placeholder,
                               PW_ENTRY* entry,
                               PwManager* database,
                               const SprContentFlags& flags,
                               int recursionLevel,
                               QMap<QString, QString>& refCache);

    // Entry field placeholders
    QString resolveEntryField(const QString& fieldName,
                              PW_ENTRY* entry,
                              PwManager* database);

    // DateTime placeholders
    QString resolveDateTime(const QString& placeholder);

    // Field references: {REF:Field@SearchType:Value}
    QString resolveFieldReference(const QString& refSpec,
                                  PwManager* database,
                                  const SprContentFlags& flags,
                                  int recursionLevel,
                                  QMap<QString, QString>& refCache);

    // Parse field reference specification
    bool parseFieldReference(const QString& refSpec,
                             QChar& targetField,
                             QChar& searchType,
                             QString& searchValue);

    // Find entry by search criteria
    PW_ENTRY* findEntryByField(PwManager* database,
                               QChar searchType,
                               const QString& searchValue);

    // Get field value from entry
    QString getEntryField(PW_ENTRY* entry,
                          PwManager* database,
                          QChar fieldType);

    // Remove metadata from notes
    static QString removeMetadata(const QString& notes);

    // Auto-type encoding
    static QString encodeForAutoType(const QString& text);

    // Command line escaping
    static QString escapeForCommandLine(const QString& text);
};

#endif // SPRENGINE_H
