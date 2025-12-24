/*
  Qt KeePass - Entry Model Implementation
*/

#include "EntryModel.h"
#include "../core/PwManager.h"
#include "../core/PwStructs.h"

#include <QString>
#include <QIcon>

EntryModel::EntryModel(PwManager *pwManager, QObject *parent)
    : QAbstractTableModel(parent)
    , m_pwManager(pwManager)
    , m_filterGroupId(0)
    , m_hasGroupFilter(false)
{
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

    return ColumnCount;
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

    switch (role) {
    case Qt::DisplayRole:
    case Qt::EditRole:
        switch (index.column()) {
        case ColumnTitle:
            return QString::fromUtf8(entry->pszTitle);
        case ColumnUsername:
            return QString::fromUtf8(entry->pszUserName);
        case ColumnURL:
            return QString::fromUtf8(entry->pszURL);
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
        default:
            return QVariant();
        }

    case Qt::DecorationRole:
        if (index.column() == ColumnTitle) {
            // TODO: Return entry icon based on uImageId
            return QVariant();
        }
        return QVariant();

    case Qt::ToolTipRole:
        switch (index.column()) {
        case ColumnTitle:
            return QString::fromUtf8(entry->pszTitle);
        case ColumnUsername:
            return QString::fromUtf8(entry->pszUserName);
        case ColumnURL:
            return QString::fromUtf8(entry->pszURL);
        case ColumnNotes:
            return QString::fromUtf8(entry->pszAdditional);
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

    return getColumnName(static_cast<Column>(section));
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
        return tr("Username");
    case ColumnURL:
        return tr("URL");
    case ColumnNotes:
        return tr("Notes");
    default:
        return QString();
    }
}
