/*
  Qt KeePass - Icon Manager

  Manages the KeePass icon collection matching MFC implementation.

  Icon System:
  - clienticex.bmp: 69 icons (16x16 each) arranged horizontally (1104x16 pixels)
  - Toolbar icons: Individual 16x16 BMP files
  - Transparent color: Magenta RGB(255, 0, 255)
*/

#ifndef ICONMANAGER_H
#define ICONMANAGER_H

#include <QIcon>
#include <QPixmap>
#include <QVector>
#include <QMap>
#include <QString>

class IconManager
{
public:
    // Singleton access
    static IconManager& instance();

    // Icon access
    QIcon getEntryIcon(int index) const;
    QIcon getGroupIcon(int index) const;
    QIcon getToolbarIcon(const QString &name) const;

    // Icon constants (matching MFC PWM_STD_ICON_* definitions)
    static constexpr int ICON_GROUP = 48;          // Standard folder (closed)
    static constexpr int ICON_GROUP_OPEN = 49;     // Folder (open)
    static constexpr int ICON_GROUP_PKG = 50;      // Package/box
    static constexpr int ICON_EMAIL = 19;          // Email
    static constexpr int ICON_WINDOWS = 38;        // Windows
    static constexpr int ICON_NETWORK = 3;         // Network
    static constexpr int ICON_INTERNET = 1;        // Internet/world
    static constexpr int ICON_HOMEBANKING = 37;    // Homebanking/finance

private:
    IconManager();
    ~IconManager() = default;

    // Prevent copying
    IconManager(const IconManager&) = delete;
    IconManager& operator=(const IconManager&) = delete;

    // Icon loading
    void loadClientIcons();
    void loadToolbarIcons();
    QPixmap loadBitmapWithTransparency(const QString &filePath, const QColor &transparentColor);

    // Icon storage
    QVector<QIcon> m_clientIcons;     // 69 entry/group icons from clienticex.bmp
    QMap<QString, QIcon> m_toolbarIcons; // Toolbar icons by name

    // Transparent color (magenta)
    static constexpr int TRANSPARENT_R = 255;
    static constexpr int TRANSPARENT_G = 0;
    static constexpr int TRANSPARENT_B = 255;
};

#endif // ICONMANAGER_H
