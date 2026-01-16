/*
  Qt KeePass - Auto-Type Entry Selection Dialog

  Shows a list of matching entries when multiple entries match
  the current window title.

  Reference: MFC CEntryListDlg with ELDMODE_LIST_ATITEMS mode
*/

#ifndef AUTOTYPESELECTIONDIALOG_H
#define AUTOTYPESELECTIONDIALOG_H

#include <QDialog>
#include <QList>
#include "../core/PwStructs.h"

class QTableWidget;
class QLabel;
class QPushButton;

class AutoTypeSelectionDialog : public QDialog
{
    Q_OBJECT

public:
    /// Constructor
    /// @param entries List of matching entries to display
    /// @param windowTitle Target window title (for display)
    /// @param sortEntries Whether to sort entries by title
    /// @param parent Parent widget
    explicit AutoTypeSelectionDialog(const QList<PW_ENTRY*>& entries,
                                      const QString& windowTitle,
                                      bool sortEntries = true,
                                      QWidget* parent = nullptr);
    ~AutoTypeSelectionDialog() override = default;

    /// Get the selected entry (after dialog closes with OK)
    /// @return Selected entry, or nullptr if cancelled
    [[nodiscard]] PW_ENTRY* getSelectedEntry() const { return m_selectedEntry; }

private slots:
    void onEntryDoubleClicked(int row, int column);
    void onSelectionChanged();

private:
    void setupUi();
    void populateTable();

    QList<PW_ENTRY*> m_entries;
    QString m_windowTitle;
    bool m_sortEntries;
    PW_ENTRY* m_selectedEntry;

    // UI widgets
    QLabel* m_labelMessage;
    QTableWidget* m_tableEntries;
    QPushButton* m_buttonOk;
    QPushButton* m_buttonCancel;
};

#endif // AUTOTYPESELECTIONDIALOG_H
