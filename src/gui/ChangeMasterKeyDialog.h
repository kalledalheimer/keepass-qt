/*
  Qt KeePass - Change Master Key Dialog

  Dialog for changing the database master password.
  Allows user to set a new master password for an existing database.
  The database will be re-encrypted with the new password.

  MFC Reference: WinGUI/ChangeMasterKeyDlg.h
*/

#ifndef CHANGEMASTERKEYDIALOG_H
#define CHANGEMASTERKEYDIALOG_H

#include <QDialog>

class QLineEdit;
class QCheckBox;
class QLabel;
class QPushButton;

class ChangeMasterKeyDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ChangeMasterKeyDialog(QWidget *parent = nullptr);
    ~ChangeMasterKeyDialog();

    // Get the new password
    QString getNewPassword() const;

private slots:
    void onShowPasswordToggled(bool checked);
    void onPasswordChanged();
    void onOkClicked();

private:
    void setupUi();
    void updateOkButton();

    QLineEdit *m_newPasswordEdit;
    QLineEdit *m_confirmPasswordEdit;
    QCheckBox *m_showPasswordCheckBox;
    QLabel *m_newPasswordLabel;
    QLabel *m_confirmPasswordLabel;
    QLabel *m_warningLabel;
    QLabel *m_infoLabel;
    QPushButton *m_okButton;
    QPushButton *m_cancelButton;
};

#endif // CHANGEMASTERKEYDIALOG_H
