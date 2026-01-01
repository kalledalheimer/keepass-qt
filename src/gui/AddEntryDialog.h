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
    explicit AddEntryDialog(PwManager *pwManager, quint32 selectedGroupId = 0, QWidget *parent = nullptr);
    ~AddEntryDialog() override = default;

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
    quint32 m_selectedGroupId;
};

#endif // ADDENTRYDIALOG_H
