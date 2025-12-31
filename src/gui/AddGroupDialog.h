/*
  Qt KeePass - Add Group Dialog

  Dialog for creating new password groups.
  Matches MFC KeePass AddGroupDlg functionality.
*/

#ifndef ADDGROUPDIALOG_H
#define ADDGROUPDIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QPushButton>
#include <QSpinBox>

class AddGroupDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AddGroupDialog(QWidget *parent = nullptr);
    ~AddGroupDialog() override = default;

    // Getters for dialog results
    QString getGroupName() const;
    quint32 getIconId() const;

private slots:
    void onGroupNameChanged(const QString &text);
    void validateAndAccept();

private:
    void setupUi();
    bool validateInput();

    // UI components
    QLineEdit *m_groupNameEdit;
    QSpinBox *m_iconIdSpin;
    QPushButton *m_okButton;
    QPushButton *m_cancelButton;

    // Reserved group names (cannot be used)
    static const QStringList ReservedNames;
};

#endif // ADDGROUPDIALOG_H
