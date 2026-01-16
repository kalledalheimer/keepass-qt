/*
  Qt KeePass - Languages Dialog

  Dialog for selecting the application language.
  Reference: MFC KeePass LanguagesDlg
*/

#ifndef LANGUAGESDIALOG_H
#define LANGUAGESDIALOG_H

#include <QDialog>

class QListWidget;
class QListWidgetItem;
class QPushButton;
class QLabel;

class LanguagesDialog : public QDialog
{
    Q_OBJECT

public:
    explicit LanguagesDialog(QWidget *parent = nullptr);
    ~LanguagesDialog() override = default;

    // Get selected language code
    QString selectedLanguage() const;

private slots:
    void onLanguageSelectionChanged();
    void onLanguageDoubleClicked(QListWidgetItem *item);
    void onOkClicked();

private:
    void setupUi();
    void populateLanguages();

    QListWidget *m_languageList;
    QLabel *m_infoLabel;
    QPushButton *m_okButton;
    QPushButton *m_cancelButton;

    QString m_selectedLanguage;
};

#endif // LANGUAGESDIALOG_H
