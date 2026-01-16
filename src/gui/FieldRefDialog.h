/*
  Qt KeePass - Field Reference Dialog

  Helps users create field references using {REF:Field@SearchType:Value} syntax.
  Allows selecting an entry and field to reference.

  Reference: MFC WinGUI/FieldRefDlg.h
*/

#ifndef FIELDREFDIALOG_H
#define FIELDREFDIALOG_H

#include <QDialog>
#include <QTableWidget>
#include <QRadioButton>
#include <QButtonGroup>
#include <QPushButton>
#include <QLabel>
#include "../core/PwStructs.h"

// Forward declarations
class PwManager;

class FieldRefDialog : public QDialog
{
    Q_OBJECT

public:
    explicit FieldRefDialog(PwManager *pwManager, QWidget *parent = nullptr);
    ~FieldRefDialog() override = default;

    /// Get the generated field reference string
    [[nodiscard]] QString getFieldReference() const { return m_fieldReference; }

    /// Default reference field selection
    enum class DefaultRef : quint8 {
        Title = 0,
        Username,
        Password,
        URL,
        Notes
    };

    void setDefaultRef(DefaultRef ref);

private slots:
    void onEntrySelectionChanged();
    void validateAndAccept();
    void onHelp();

private:
    void setupUi();
    void populateEntryList();
    void updateControls();
    PW_ENTRY* getSelectedEntry();
    bool idMatchesMultipleTimes(const QString& value, QChar searchType);

    // UI components
    QTableWidget *m_entryTable;

    // Field to reference (what to retrieve)
    QButtonGroup *m_refFieldGroup;
    QRadioButton *m_radioRefTitle;
    QRadioButton *m_radioRefUsername;
    QRadioButton *m_radioRefPassword;
    QRadioButton *m_radioRefUrl;
    QRadioButton *m_radioRefNotes;

    // Identify by (how to find the entry)
    QButtonGroup *m_idFieldGroup;
    QRadioButton *m_radioIdTitle;
    QRadioButton *m_radioIdUsername;
    QRadioButton *m_radioIdPassword;
    QRadioButton *m_radioIdUrl;
    QRadioButton *m_radioIdNotes;
    QRadioButton *m_radioIdUuid;

    QLabel *m_statusLabel;
    QPushButton *m_okButton;
    QPushButton *m_cancelButton;
    QPushButton *m_helpButton;

    // Data
    PwManager *m_pwManager;
    QString m_fieldReference;

    // Table column indices
    enum Column : int {
        ColGroup = 0,
        ColTitle,
        ColUsername,
        ColUrl,
        ColNotes,
        ColUuid,
        ColCount
    };
};

#endif // FIELDREFDIALOG_H
