/*
  Qt KeePass - TAN Wizard Dialog
  Reference: MFC/MFC-KeePass/WinGUI/TanWizardDlg.h
*/

#ifndef TANWIZARDDIALOG_H
#define TANWIZARDDIALOG_H

#include <QDialog>

class QLineEdit;
class QPlainTextEdit;
class QCheckBox;
class QSpinBox;
class QPushButton;

// Default acceptable characters for TANs (MFC: TW_DEFAULTCHARS)
#define TAN_DEFAULT_CHARS "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-"

class TanWizardDialog : public QDialog
{
    Q_OBJECT

public:
    explicit TanWizardDialog(const QString& groupName, QWidget* parent = nullptr);

    // Results
    [[nodiscard]] QStringList getTanList() const { return m_tanList; }
    [[nodiscard]] bool useSequentialNumbering() const { return m_useNumbering; }
    [[nodiscard]] int getStartNumber() const { return m_startNumber; }

private:
    void setupUI();
    void parseTans();

private slots:
    void onNumberingToggled(bool checked);
    void onAccept();

private:
    // UI Components
    QPlainTextEdit* m_tansEdit;
    QLineEdit* m_charsEdit;
    QCheckBox* m_numberingCheck;
    QSpinBox* m_startNumberSpin;
    QPushButton* m_okButton;
    QPushButton* m_cancelButton;

    // Data
    QString m_groupName;
    QStringList m_tanList;
    bool m_useNumbering;
    int m_startNumber;
};

#endif // TANWIZARDDIALOG_H
