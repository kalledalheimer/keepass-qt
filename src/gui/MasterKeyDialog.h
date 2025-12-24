/*
  Qt KeePass - Master Key Dialog Header

  Dialog for entering/setting the database master key (password)
*/

#ifndef MASTERKEYDIALOG_H
#define MASTERKEYDIALOG_H

#include <QDialog>

class QLineEdit;
class QCheckBox;
class QLabel;
class QPushButton;

class MasterKeyDialog : public QDialog
{
    Q_OBJECT

public:
    enum Mode {
        CreateNew,  // Creating new database (requires password confirmation)
        OpenExisting  // Opening existing database (no confirmation needed)
    };

    explicit MasterKeyDialog(Mode mode, QWidget *parent = nullptr);
    ~MasterKeyDialog();

    // Get the entered password
    QString getPassword() const;

private slots:
    void onShowPasswordToggled(bool checked);
    void onPasswordChanged();
    void onOkClicked();

private:
    void setupUi();
    void updateOkButton();

    Mode m_mode;
    QLineEdit *m_passwordEdit;
    QLineEdit *m_confirmPasswordEdit;
    QCheckBox *m_showPasswordCheckBox;
    QLabel *m_passwordLabel;
    QLabel *m_confirmPasswordLabel;
    QLabel *m_warningLabel;
    QPushButton *m_okButton;
    QPushButton *m_cancelButton;
};

#endif // MASTERKEYDIALOG_H
