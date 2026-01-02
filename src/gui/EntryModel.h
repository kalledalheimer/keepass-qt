/*
  Qt KeePass - Entry Model

  Model for displaying password entries in a table view.
  Implements QAbstractTableModel for tabular data.
*/

#ifndef ENTRYMODEL_H
#define ENTRYMODEL_H

#include <QAbstractTableModel>
#include <QModelIndex>
#include <QVariant>

// Forward declarations
class PwManager;
class QObject;

#include "../core/PwStructs.h"

class EntryModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    // Column definitions
    enum Column {
        ColumnTitle = 0,
        ColumnUsername,
        ColumnURL,
        ColumnNotes,
        ColumnCount
    };

    explicit EntryModel(PwManager *pwManager, QObject *parent = nullptr);
    ~EntryModel() override = default;

    // QAbstractTableModel interface
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    // Custom methods
    PW_ENTRY* getEntry(const QModelIndex &index) const;
    QModelIndex indexForEntry(quint32 entryIndex) const;

    // Filter by group
    void setGroupFilter(quint32 groupId);
    void clearGroupFilter();

    // Filter by entry indices (for search results)
    void setIndexFilter(const QList<quint32>& entryIndices);
    void clearIndexFilter();

    // Data refresh
    void refresh();

private:
    PwManager *m_pwManager;
    quint32 m_filterGroupId;
    bool m_hasGroupFilter;
    QList<quint32> m_filterIndices;
    bool m_hasIndexFilter;

    // Internal methods
    QList<PW_ENTRY*> getFilteredEntries() const;
    QString getColumnName(Column column) const;
};

#endif // ENTRYMODEL_H
