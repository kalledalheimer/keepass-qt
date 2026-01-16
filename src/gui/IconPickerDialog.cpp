/*
  Qt KeePass - Icon Picker Dialog Implementation
*/

#include "IconPickerDialog.h"
#include "IconManager.h"

#include <QListWidget>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QDialogButtonBox>
#include <QLabel>

IconPickerDialog::IconPickerDialog(int currentIcon, QWidget *parent)
    : QDialog(parent)
    , m_listWidget(nullptr)
    , m_okButton(nullptr)
    , m_cancelButton(nullptr)
    , m_currentIcon(currentIcon)
    , m_selectedIcon(currentIcon)
{
    setupUi();
    populateIcons();

    // Pre-select current icon
    if ((m_currentIcon >= 0) && (m_currentIcon < m_listWidget->count())) {
        m_listWidget->setCurrentRow(m_currentIcon);
        m_listWidget->scrollToItem(m_listWidget->item(m_currentIcon));
    }
}

IconPickerDialog::~IconPickerDialog() = default;

void IconPickerDialog::setupUi()
{
    // Reference: MFC/MFC-KeePass/WinGUI/IconPickerDlg.cpp OnInitDialog
    setWindowTitle(tr("Pick an Icon"));
    setMinimumSize(500, 400);

    // Create main layout
    auto *mainLayout = new QVBoxLayout(this);

    // Add description label
    auto *descLabel = new QLabel(tr("Choose an icon from the list of available entry and group icons."), this);
    descLabel->setWordWrap(true);
    mainLayout->addWidget(descLabel);

    // Create list widget for icons
    m_listWidget = new QListWidget(this);
    m_listWidget->setViewMode(QListWidget::IconMode);
    m_listWidget->setIconSize(QSize(16, 16));
    m_listWidget->setGridSize(QSize(40, 40));  // Space for icon + number
    m_listWidget->setMovement(QListWidget::Static);
    m_listWidget->setResizeMode(QListWidget::Adjust);
    m_listWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    mainLayout->addWidget(m_listWidget);

    // Create button box
    auto *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    m_okButton = buttonBox->button(QDialogButtonBox::Ok);
    m_cancelButton = buttonBox->button(QDialogButtonBox::Cancel);

    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    mainLayout->addWidget(buttonBox);

    // Connect signals
    connect(m_listWidget, &QListWidget::itemDoubleClicked,
            this, &IconPickerDialog::onItemDoubleClicked);
    connect(m_listWidget, &QListWidget::itemSelectionChanged,
            this, &IconPickerDialog::onSelectionChanged);
}

void IconPickerDialog::populateIcons()
{
    // Reference: MFC/MFC-KeePass/WinGUI/IconPickerDlg.cpp OnInitDialog (lines 77-82)
    // Load all 69 icons from IconManager
    IconManager &iconMgr = IconManager::instance();

    // KeePass has 69 entry/group icons (indices 0-68)
    for (int i = 0; i < 69; ++i) {
        QIcon icon = iconMgr.getEntryIcon(i);
        QString text = QString::number(i);

        auto *item = new QListWidgetItem(icon, text, m_listWidget);
        item->setData(Qt::UserRole, i);  // Store index
        item->setToolTip(tr("Icon %1").arg(i));
    }
}

void IconPickerDialog::onItemDoubleClicked(QListWidgetItem *item)
{
    // Double-click accepts the dialog
    if (item != nullptr) {
        m_selectedIcon = item->data(Qt::UserRole).toInt();
        accept();
    }
}

void IconPickerDialog::onSelectionChanged()
{
    // Reference: MFC/MFC-KeePass/WinGUI/IconPickerDlg.cpp OnOK (lines 106-114)
    // Update selected icon when selection changes
    QListWidgetItem *currentItem = m_listWidget->currentItem();
    if (currentItem != nullptr) {
        m_selectedIcon = currentItem->data(Qt::UserRole).toInt();
    }
}
