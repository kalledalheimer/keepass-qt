/*
  Qt KeePass - Group Model Implementation
*/

#include "GroupModel.h"
#include "../core/PwManager.h"
#include "../core/PwStructs.h"

#include <QString>
#include <QIcon>

GroupModel::GroupModel(PwManager *pwManager, QObject *parent)
    : QAbstractItemModel(parent)
    , m_pwManager(pwManager)
{
}

QModelIndex GroupModel::index(int row, int column, const QModelIndex &parent) const
{
    if (!hasIndex(row, column, parent) || !m_pwManager) {
        return QModelIndex();
    }

    PW_GROUP *parentGroup = nullptr;
    if (parent.isValid()) {
        parentGroup = getGroupByIndex(parent);
    }

    // Find the child group at the specified row
    quint32 numGroups = m_pwManager->getNumberOfGroups();
    int currentRow = 0;

    for (quint32 i = 0; i < numGroups; ++i) {
        PW_GROUP *group = m_pwManager->getGroup(i);
        if (!group) continue;

        // Check if this group's parent matches
        PW_GROUP *groupParent = getParentGroup(group);
        if (groupParent == parentGroup) {
            if (currentRow == row) {
                return createIndex(row, column, group);
            }
            currentRow++;
        }
    }

    return QModelIndex();
}

QModelIndex GroupModel::parent(const QModelIndex &child) const
{
    if (!child.isValid() || !m_pwManager) {
        return QModelIndex();
    }

    PW_GROUP *childGroup = getGroupByIndex(child);
    if (!childGroup) {
        return QModelIndex();
    }

    PW_GROUP *parentGroup = getParentGroup(childGroup);
    if (!parentGroup) {
        return QModelIndex();  // Top-level group
    }

    int row = getGroupRow(parentGroup);
    if (row >= 0) {
        return createIndex(row, 0, parentGroup);
    }

    return QModelIndex();
}

int GroupModel::rowCount(const QModelIndex &parent) const
{
    if (!m_pwManager) {
        return 0;
    }

    PW_GROUP *parentGroup = nullptr;
    if (parent.isValid()) {
        parentGroup = getGroupByIndex(parent);
    }

    // Count children of this parent
    quint32 numGroups = m_pwManager->getNumberOfGroups();
    int count = 0;

    for (quint32 i = 0; i < numGroups; ++i) {
        PW_GROUP *group = m_pwManager->getGroup(i);
        if (!group) continue;

        PW_GROUP *groupParent = getParentGroup(group);
        if (groupParent == parentGroup) {
            count++;
        }
    }

    return count;
}

int GroupModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return 1;  // Single column (group name)
}

QVariant GroupModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || !m_pwManager) {
        return QVariant();
    }

    PW_GROUP *group = getGroupByIndex(index);
    if (!group) {
        return QVariant();
    }

    switch (role) {
    case Qt::DisplayRole:
    case Qt::EditRole:
        return QString::fromUtf8(group->pszGroupName);

    case Qt::DecorationRole:
        // TODO: Return group icon based on uImageId
        return QVariant();

    case Qt::ToolTipRole:
        return QString::fromUtf8(group->pszGroupName);

    default:
        return QVariant();
    }
}

QVariant GroupModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole && section == 0) {
        return tr("Groups");
    }

    return QVariant();
}

PW_GROUP* GroupModel::getGroup(const QModelIndex &index) const
{
    return getGroupByIndex(index);
}

QModelIndex GroupModel::indexForGroup(quint32 groupId) const
{
    if (!m_pwManager) {
        return QModelIndex();
    }

    quint32 numGroups = m_pwManager->getNumberOfGroups();
    for (quint32 i = 0; i < numGroups; ++i) {
        PW_GROUP *group = m_pwManager->getGroup(i);
        if (group && group->uGroupId == groupId) {
            int row = getGroupRow(group);
            if (row >= 0) {
                return createIndex(row, 0, group);
            }
        }
    }

    return QModelIndex();
}

void GroupModel::refresh()
{
    beginResetModel();
    endResetModel();
}

PW_GROUP* GroupModel::getGroupByIndex(const QModelIndex &index) const
{
    if (!index.isValid()) {
        return nullptr;
    }

    return static_cast<PW_GROUP*>(index.internalPointer());
}

int GroupModel::getGroupRow(PW_GROUP *group) const
{
    if (!group || !m_pwManager) {
        return -1;
    }

    PW_GROUP *parentGroup = getParentGroup(group);

    // Find the row of this group among its siblings
    quint32 numGroups = m_pwManager->getNumberOfGroups();
    int row = 0;

    for (quint32 i = 0; i < numGroups; ++i) {
        PW_GROUP *g = m_pwManager->getGroup(i);
        if (!g) continue;

        PW_GROUP *gParent = getParentGroup(g);
        if (gParent == parentGroup) {
            if (g == group) {
                return row;
            }
            row++;
        }
    }

    return -1;
}

PW_GROUP* GroupModel::getParentGroup(PW_GROUP *group) const
{
    if (!group || !m_pwManager) {
        return nullptr;
    }

    // In KDB format, groups are stored flat with usLevel indicating hierarchy
    // Level 0 = top-level, level 1 = child of previous level 0, etc.
    // We need to find the parent by walking backwards until we find a group
    // with level = (current level - 1)

    if (group->usLevel == 0) {
        return nullptr;  // Top-level group
    }

    quint32 numGroups = m_pwManager->getNumberOfGroups();
    quint16 parentLevel = group->usLevel - 1;

    // Find this group's index
    quint32 groupIndex = 0;
    for (quint32 i = 0; i < numGroups; ++i) {
        if (m_pwManager->getGroup(i) == group) {
            groupIndex = i;
            break;
        }
    }

    // Walk backwards from this group to find parent
    for (int i = groupIndex - 1; i >= 0; --i) {
        PW_GROUP *candidate = m_pwManager->getGroup(static_cast<quint32>(i));
        if (candidate && candidate->usLevel == parentLevel) {
            return candidate;
        }
    }

    return nullptr;
}
