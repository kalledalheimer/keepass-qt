/*
  Qt KeePass - Add Entry Dialog

  Dialog for creating new password entries.
  Matches MFC KeePass AddEntryDlg functionality.
*/

#ifndef ADDENTRYDIALOG_H
#define ADDENTRYDIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QTextEdit>
#include <QComboBox>
#include <QSpinBox>
#include <QCheckBox>
#include <QDateTimeEdit>
#include <QPushButton>
#include "../core/PwStructs.h"

// Forward declarations
class PwManager;

class AddEntryDialog : public QDialog
{
    Q_OBJECT

public:
    enum Mode {
        AddMode,
        EditMode
    };

    // Constructor
    // For AddMode: idValue is the selectedGroupId
    // For EditMode: idValue is the entryIndex
    explicit AddEntryDialog(PwManager *pwManager, Mode mode, quint32 idValue = 0, QWidget *parent = nullptr);

    ~AddEntryDialog() override = default;

    // Get mode
    Mode getMode() const { return m_mode; }
    quint32 getEntryIndex() const { return m_entryIndex; }

    // Getters for dialog results
    QString getTitle() const;
    QString getUsername() const;
    QString getPassword() const;
    QString getUrl() const;
    QString getNotes() const;
    quint32 getGroupId() const;
    quint32 getIconId() const;
    PW_TIME getExpirationTime() const;
    bool hasExpiration() const;

private slots:
    void onPasswordChanged();
    void onShowPasswordToggled(bool checked);
    void onExpiresToggled(bool checked);
    void validateAndAccept();

private:
    void setupUi();
    bool validateInput();
    void populateGroupCombo();
    void populateFromEntry(PW_ENTRY *entry);
    QString generateRandomPassword();

    // UI components
    QComboBox *m_groupCombo;
    QSpinBox *m_iconIdSpin;
    QLineEdit *m_titleEdit;
    QLineEdit *m_usernameEdit;
    QLineEdit *m_passwordEdit;
    QLineEdit *m_repeatPasswordEdit;
    QCheckBox *m_showPasswordCheck;
    QLineEdit *m_urlEdit;
    QTextEdit *m_notesEdit;
    QCheckBox *m_expiresCheck;
    QDateTimeEdit *m_expirationDateTime;
    QPushButton *m_okButton;
    QPushButton *m_cancelButton;

    // Data
    PwManager *m_pwManager;
    Mode m_mode;
    quint32 m_selectedGroupId;  // For Add mode
    quint32 m_entryIndex;       // For Edit mode
};

#endif // ADDENTRYDIALOG_H
