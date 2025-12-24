/*
  Qt KeePass - Group Model

  Model for displaying password groups in a tree view.
  Implements QAbstractItemModel for hierarchical data.
*/

#ifndef GROUPMODEL_H
#define GROUPMODEL_H

#include <QAbstractItemModel>
#include <QModelIndex>
#include <QVariant>

// Forward declarations
class PwManager;
class QObject;

#include "../core/PwStructs.h"

class GroupModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    explicit GroupModel(PwManager *pwManager, QObject *parent = nullptr);
    ~GroupModel() override = default;

    // QAbstractItemModel interface
    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex &child) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    // Custom methods
    PW_GROUP* getGroup(const QModelIndex &index) const;
    QModelIndex indexForGroup(quint32 groupId) const;

    // Data refresh
    void refresh();

private:
    PwManager *m_pwManager;

    // Internal methods
    PW_GROUP* getGroupByIndex(const QModelIndex &index) const;
    int getGroupRow(PW_GROUP *group) const;
    PW_GROUP* getParentGroup(PW_GROUP *group) const;
};

#endif // GROUPMODEL_H
