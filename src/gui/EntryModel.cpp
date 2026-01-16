/*
  Qt KeePass - Entry Model Implementation
*/

#include "EntryModel.h"
#include "../core/PwManager.h"
#include "../core/PwStructs.h"
#include "../core/platform/PwSettings.h"
#include "../core/util/PwUtil.h"
#include "IconManager.h"

#include <QString>
#include <QIcon>
#include <QDateTime>

EntryModel::EntryModel(PwManager *pwManager, QObject *parent)
    : QAbstractTableModel(parent)
    , m_pwManager(pwManager)
    , m_filterGroupId(0)
    , m_hasGroupFilter(false)
    , m_hasIndexFilter(false)
{
    // Initialize all columns as visible by default (matching MFC defaults)
    for (int i = 0; i < ColumnCount; ++i) {
        m_columnVisible[i] = true;
    }

    // Load saved visibility preferences
    loadColumnVisibility();
}

int EntryModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid() || !m_pwManager) {
        return 0;
    }

    return getFilteredEntries().count();
}

int EntryModel::columnCount(const QModelIndex &parent) const
{
    if (parent.isValid()) {
        return 0;
    }

    // Count only visible columns
    int visibleCount = 0;
    for (int i = 0; i < ColumnCount; ++i) {
        if (m_columnVisible[i]) {
            ++visibleCount;
        }
    }
    return visibleCount;
}

QVariant EntryModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || !m_pwManager) {
        return QVariant();
    }

    QList<PW_ENTRY*> entries = getFilteredEntries();
    if (index.row() < 0 || index.row() >= entries.count()) {
        return QVariant();
    }

    PW_ENTRY *entry = entries.at(index.row());
    if (!entry) {
        return QVariant();
    }

    // Map visible column index to logical column
    int logicalCol = visibleToLogicalColumn(index.column());
    if (logicalCol < 0 || logicalCol >= ColumnCount) {
        return QVariant();
    }

    Column column = static_cast<Column>(logicalCol);

    switch (role) {
    case Qt::DisplayRole:
    case Qt::EditRole:
        switch (column) {
        case ColumnTitle:
            return QString::fromUtf8(entry->pszTitle);
        case ColumnUsername:
            // Check if usernames should be hidden
            // Reference: MFC m_bUserStars (PWMKEY_HIDEUSERS)
            if (PwSettings::instance().getHideUsernameStars()) {
                if (entry->pszUserName && *entry->pszUserName != '\0') {
                    return QStringLiteral("***");
                }
            }
            return QString::fromUtf8(entry->pszUserName);
        case ColumnURL:
            return QString::fromUtf8(entry->pszURL);
        case ColumnPassword:
            // Check if passwords should be hidden
            // Reference: MFC m_bPasswordStars (PWMKEY_HIDESTARS)
            if (PwSettings::instance().getHidePasswordStars()) {
                // Show masked password (asterisks)
                if (entry->pszPassword && entry->uPasswordLen > 0) {
                    return QString("*").repeated(qMin(entry->uPasswordLen, 16u));
                }
            } else {
                // Show actual password (for when user disables masking)
                return QString::fromUtf8(entry->pszPassword);
            }
            return QString();
        case ColumnNotes:
            // Show truncated notes (first line only)
            {
                QString notes = QString::fromUtf8(entry->pszAdditional);
                int newlinePos = notes.indexOf('\n');
                if (newlinePos > 0) {
                    return notes.left(newlinePos) + "...";
                }
                return notes;
            }
        case ColumnCreationTime:
            return PwUtil::pwTimeToDateTime(&entry->tCreation).toString("yyyy-MM-dd hh:mm:ss");
        case ColumnLastModification:
            return PwUtil::pwTimeToDateTime(&entry->tLastMod).toString("yyyy-MM-dd hh:mm:ss");
        case ColumnLastAccess:
            return PwUtil::pwTimeToDateTime(&entry->tLastAccess).toString("yyyy-MM-dd hh:mm:ss");
        case ColumnExpires:
            if (entry->tExpire.shYear == 2999) {
                return tr("Never");
            }
            return PwUtil::pwTimeToDateTime(&entry->tExpire).toString("yyyy-MM-dd hh:mm:ss");
        case ColumnUUID:
            {
                QString uuid;
                for (int i = 0; i < 16; ++i) {
                    uuid += QString("%1").arg(entry->uuid[i], 2, 16, QChar('0'));
                }
                return uuid.toUpper();
            }
        case ColumnAttachment:
            if (entry->pBinaryData && entry->uBinaryDataLen > 0 && entry->pszBinaryDesc) {
                return QString::fromUtf8(entry->pszBinaryDesc);
            }
            return QString();
        default:
            return QVariant();
        }

    case Qt::DecorationRole:
        if (column == ColumnTitle) {
            return IconManager::instance().getEntryIcon(entry->uImageId);
        }
        return QVariant();

    case Qt::ToolTipRole:
        switch (column) {
        case ColumnTitle:
            return QString::fromUtf8(entry->pszTitle);
        case ColumnUsername:
            return QString::fromUtf8(entry->pszUserName);
        case ColumnURL:
            return QString::fromUtf8(entry->pszURL);
        case ColumnPassword:
            return tr("Password (hidden)");
        case ColumnNotes:
            return QString::fromUtf8(entry->pszAdditional);
        case ColumnCreationTime:
            return tr("Created: %1").arg(PwUtil::pwTimeToDateTime(&entry->tCreation).toString("yyyy-MM-dd hh:mm:ss"));
        case ColumnLastModification:
            return tr("Modified: %1").arg(PwUtil::pwTimeToDateTime(&entry->tLastMod).toString("yyyy-MM-dd hh:mm:ss"));
        case ColumnLastAccess:
            return tr("Accessed: %1").arg(PwUtil::pwTimeToDateTime(&entry->tLastAccess).toString("yyyy-MM-dd hh:mm:ss"));
        case ColumnExpires:
            if (entry->tExpire.shYear == 2999) {
                return tr("Never expires");
            }
            return tr("Expires: %1").arg(PwUtil::pwTimeToDateTime(&entry->tExpire).toString("yyyy-MM-dd hh:mm:ss"));
        case ColumnUUID:
            {
                QString uuid;
                for (int i = 0; i < 16; ++i) {
                    uuid += QString("%1").arg(entry->uuid[i], 2, 16, QChar('0'));
                }
                return tr("UUID: %1").arg(uuid.toUpper());
            }
        case ColumnAttachment:
            if (entry->pBinaryData && entry->uBinaryDataLen > 0) {
                return tr("Attachment: %1 (%2 bytes)").arg(QString::fromUtf8(entry->pszBinaryDesc)).arg(entry->uBinaryDataLen);
            }
            return tr("No attachment");
        default:
            return QVariant();
        }

    default:
        return QVariant();
    }
}

QVariant EntryModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation != Qt::Horizontal || role != Qt::DisplayRole) {
        return QVariant();
    }

    // Map visible column index to logical column
    int logicalCol = visibleToLogicalColumn(section);
    if (logicalCol < 0 || logicalCol >= ColumnCount) {
        return QVariant();
    }

    return getColumnName(static_cast<Column>(logicalCol));
}

PW_ENTRY* EntryModel::getEntry(const QModelIndex &index) const
{
    if (!index.isValid() || !m_pwManager) {
        return nullptr;
    }

    QList<PW_ENTRY*> entries = getFilteredEntries();
    if (index.row() < 0 || index.row() >= entries.count()) {
        return nullptr;
    }

    return entries.at(index.row());
}

QModelIndex EntryModel::indexForEntry(quint32 entryIndex) const
{
    if (!m_pwManager) {
        return QModelIndex();
    }

    QList<PW_ENTRY*> entries = getFilteredEntries();
    PW_ENTRY *targetEntry = m_pwManager->getEntry(entryIndex);

    for (int i = 0; i < entries.count(); ++i) {
        if (entries.at(i) == targetEntry) {
            return createIndex(i, 0);
        }
    }

    return QModelIndex();
}

void EntryModel::setGroupFilter(quint32 groupId)
{
    beginResetModel();
    m_filterGroupId = groupId;
    m_hasGroupFilter = true;
    endResetModel();
}

void EntryModel::clearGroupFilter()
{
    beginResetModel();
    m_hasGroupFilter = false;
    endResetModel();
}

void EntryModel::setIndexFilter(const QList<quint32>& entryIndices)
{
    beginResetModel();
    m_filterIndices = entryIndices;
    m_hasIndexFilter = true;
    endResetModel();
}

void EntryModel::clearIndexFilter()
{
    beginResetModel();
    m_hasIndexFilter = false;
    m_filterIndices.clear();
    endResetModel();
}

void EntryModel::refresh()
{
    beginResetModel();
    endResetModel();
}

QList<PW_ENTRY*> EntryModel::getFilteredEntries() const
{
    QList<PW_ENTRY*> result;

    if (!m_pwManager) {
        return result;
    }

    // If index filter is active, use it (takes precedence over group filter)
    if (m_hasIndexFilter) {
        for (quint32 idx : m_filterIndices) {
            PW_ENTRY *entry = m_pwManager->getEntry(idx);
            if (entry) {
                result.append(entry);
            }
        }
        return result;
    }

    // Otherwise, use group filter if active
    quint32 numEntries = m_pwManager->getNumberOfEntries();
    for (quint32 i = 0; i < numEntries; ++i) {
        PW_ENTRY *entry = m_pwManager->getEntry(i);
        if (!entry) continue;

        // Apply group filter if active
        if (m_hasGroupFilter) {
            if (entry->uGroupId != m_filterGroupId) {
                continue;
            }
        }

        result.append(entry);
    }

    return result;
}

QString EntryModel::getColumnName(Column column) const
{
    switch (column) {
    case ColumnTitle:
        return tr("Title");
    case ColumnUsername:
        return tr("User Name");
    case ColumnURL:
        return tr("URL");
    case ColumnPassword:
        return tr("Password");
    case ColumnNotes:
        return tr("Notes");
    case ColumnCreationTime:
        return tr("Creation Time");
    case ColumnLastModification:
        return tr("Last Modification");
    case ColumnLastAccess:
        return tr("Last Access");
    case ColumnExpires:
        return tr("Expires");
    case ColumnUUID:
        return tr("UUID");
    case ColumnAttachment:
        return tr("Attachment");
    default:
        return QString();
    }
}

// Column visibility methods

bool EntryModel::isColumnVisible(Column column) const
{
    if (column < 0 || column >= ColumnCount) {
        return false;
    }
    return m_columnVisible[column];
}

void EntryModel::setColumnVisible(Column column, bool visible)
{
    if (column < 0 || column >= ColumnCount) {
        return;
    }

    if (m_columnVisible[column] != visible) {
        beginResetModel();
        m_columnVisible[column] = visible;
        endResetModel();
        saveColumnVisibility();
    }
}

void EntryModel::loadColumnVisibility()
{
    // Load from settings (matching MFC key names)
    PwSettings& settings = PwSettings::instance();

    m_columnVisible[ColumnTitle] = settings.get("ViewOptions/ShowTitle", true).toBool();
    m_columnVisible[ColumnUsername] = settings.get("ViewOptions/ShowUsername", true).toBool();
    m_columnVisible[ColumnURL] = settings.get("ViewOptions/ShowURL", true).toBool();
    m_columnVisible[ColumnPassword] = settings.get("ViewOptions/ShowPassword", false).toBool();  // Default: hidden
    m_columnVisible[ColumnNotes] = settings.get("ViewOptions/ShowNotes", true).toBool();
    m_columnVisible[ColumnCreationTime] = settings.get("ViewOptions/ShowCreation", false).toBool();
    m_columnVisible[ColumnLastModification] = settings.get("ViewOptions/ShowLastMod", false).toBool();
    m_columnVisible[ColumnLastAccess] = settings.get("ViewOptions/ShowLastAccess", false).toBool();
    m_columnVisible[ColumnExpires] = settings.get("ViewOptions/ShowExpires", false).toBool();
    m_columnVisible[ColumnUUID] = settings.get("ViewOptions/ShowUUID", false).toBool();
    m_columnVisible[ColumnAttachment] = settings.get("ViewOptions/ShowAttachment", false).toBool();
}

void EntryModel::saveColumnVisibility()
{
    // Save to settings
    PwSettings& settings = PwSettings::instance();

    settings.set("ViewOptions/ShowTitle", m_columnVisible[ColumnTitle]);
    settings.set("ViewOptions/ShowUsername", m_columnVisible[ColumnUsername]);
    settings.set("ViewOptions/ShowURL", m_columnVisible[ColumnURL]);
    settings.set("ViewOptions/ShowPassword", m_columnVisible[ColumnPassword]);
    settings.set("ViewOptions/ShowNotes", m_columnVisible[ColumnNotes]);
    settings.set("ViewOptions/ShowCreation", m_columnVisible[ColumnCreationTime]);
    settings.set("ViewOptions/ShowLastMod", m_columnVisible[ColumnLastModification]);
    settings.set("ViewOptions/ShowLastAccess", m_columnVisible[ColumnLastAccess]);
    settings.set("ViewOptions/ShowExpires", m_columnVisible[ColumnExpires]);
    settings.set("ViewOptions/ShowUUID", m_columnVisible[ColumnUUID]);
    settings.set("ViewOptions/ShowAttachment", m_columnVisible[ColumnAttachment]);

    settings.sync();
}

// Column mapping methods

int EntryModel::logicalToVisibleColumn(int logicalColumn) const
{
    if (logicalColumn < 0 || logicalColumn >= ColumnCount) {
        return -1;
    }

    // If column is not visible, return -1
    if (!m_columnVisible[logicalColumn]) {
        return -1;
    }

    // Count how many visible columns come before this one
    int visibleIndex = 0;
    for (int i = 0; i < logicalColumn; ++i) {
        if (m_columnVisible[i]) {
            ++visibleIndex;
        }
    }

    return visibleIndex;
}

int EntryModel::visibleToLogicalColumn(int visibleColumn) const
{
    if (visibleColumn < 0) {
        return -1;
    }

    // Find the Nth visible column
    int visibleCount = 0;
    for (int i = 0; i < ColumnCount; ++i) {
        if (m_columnVisible[i]) {
            if (visibleCount == visibleColumn) {
                return i;
            }
            ++visibleCount;
        }
    }

    return -1;  // Visible column index out of range
}
