/*
  Qt KeePass - Icon Picker Dialog

  Allows user to select an icon from the KeePass icon collection.
  Matches MFC CIconPickerDlg implementation.
*/

#ifndef ICONPICKERDIALOG_H
#define ICONPICKERDIALOG_H

#include <QDialog>

class QListWidget;
class QListWidgetItem;
class QPushButton;

class IconPickerDialog : public QDialog
{
    Q_OBJECT

public:
    explicit IconPickerDialog(int currentIcon = 0, QWidget *parent = nullptr);
    ~IconPickerDialog() override;

    // Get the selected icon index
    [[nodiscard]] int selectedIcon() const { return m_selectedIcon; }

private slots:
    void onItemDoubleClicked(QListWidgetItem *item);
    void onSelectionChanged();

private:
    void setupUi();
    void populateIcons();

    QListWidget *m_listWidget;
    QPushButton *m_okButton;
    QPushButton *m_cancelButton;

    int m_currentIcon;
    int m_selectedIcon;
};

#endif // ICONPICKERDIALOG_H
