/*
  Qt KeePass - Icon Manager Implementation
*/

#include "IconManager.h"
#include <QImage>
#include <QPixmap>
#include <QDebug>
#include <QFileInfo>
#include <QDir>

IconManager& IconManager::instance()
{
    static IconManager instance;
    return instance;
}

IconManager::IconManager()
{
    loadClientIcons();
    loadToolbarIcons();
}

void IconManager::loadClientIcons()
{
    // Load the main icon bitmap (clienticex.bmp)
    // Format: 1104x16 pixels = 69 icons of 16x16 each
    QString iconPath = ":/icons/clienticex.bmp";

    QImage iconSheet = loadBitmapWithTransparency(iconPath, QColor(TRANSPARENT_R, TRANSPARENT_G, TRANSPARENT_B)).toImage();

    if (iconSheet.isNull()) {
        qWarning() << "Failed to load clienticex.bmp from resources, trying filesystem path";
        // Try filesystem path as fallback
        iconPath = "resources/icons/clienticex.bmp";
        iconSheet = loadBitmapWithTransparency(iconPath, QColor(TRANSPARENT_R, TRANSPARENT_G, TRANSPARENT_B)).toImage();

        if (iconSheet.isNull()) {
            qCritical() << "Failed to load clienticex.bmp icon sheet";
            return;
        }
    }

    // Extract 69 individual 16x16 icons
    const int iconSize = 16;
    const int iconCount = 69;

    m_clientIcons.reserve(iconCount);

    for (int i = 0; i < iconCount; ++i) {
        // Extract icon at position (i * iconSize, 0)
        QImage iconImage = iconSheet.copy(i * iconSize, 0, iconSize, iconSize);

        // Convert to pixmap and create icon
        QPixmap iconPixmap = QPixmap::fromImage(iconImage);
        m_clientIcons.append(QIcon(iconPixmap));
    }

    qDebug() << "Loaded" << m_clientIcons.size() << "client icons from clienticex.bmp";
}

void IconManager::loadToolbarIcons()
{
    // List of toolbar icon files to load
    QStringList toolbarIconNames = {
        "tb_new",
        "tb_open",
        "tb_save",
        "tb_adden",
        "tb_edite",
        "tb_delet",
        "tb_find",
        "tb_about"
    };

    QColor transparentColor(TRANSPARENT_R, TRANSPARENT_G, TRANSPARENT_B);

    for (const QString &name : toolbarIconNames) {
        QString resourcePath = QString(":/icons/%1.bmp").arg(name);
        QPixmap iconPixmap = loadBitmapWithTransparency(resourcePath, transparentColor);

        if (iconPixmap.isNull()) {
            // Try filesystem path as fallback
            QString filePath = QString("resources/icons/%1.bmp").arg(name);
            iconPixmap = loadBitmapWithTransparency(filePath, transparentColor);

            if (iconPixmap.isNull()) {
                qWarning() << "Failed to load toolbar icon:" << name;
                continue;
            }
        }

        m_toolbarIcons[name] = QIcon(iconPixmap);
    }

    qDebug() << "Loaded" << m_toolbarIcons.size() << "toolbar icons";
}

QPixmap IconManager::loadBitmapWithTransparency(const QString &filePath, const QColor &transparentColor)
{
    QImage image(filePath);

    if (image.isNull()) {
        return QPixmap();
    }

    // Convert to ARGB32 format to support transparency
    image = image.convertToFormat(QImage::Format_ARGB32);

    // Make magenta pixels transparent
    for (int y = 0; y < image.height(); ++y) {
        for (int x = 0; x < image.width(); ++x) {
            QColor pixelColor = image.pixelColor(x, y);

            // Check if pixel is magenta (transparent color)
            if (pixelColor.red() == transparentColor.red() &&
                pixelColor.green() == transparentColor.green() &&
                pixelColor.blue() == transparentColor.blue()) {
                // Set alpha to 0 (fully transparent)
                image.setPixelColor(x, y, QColor(0, 0, 0, 0));
            }
        }
    }

    return QPixmap::fromImage(image);
}

QIcon IconManager::getEntryIcon(int index) const
{
    if (index >= 0 && index < m_clientIcons.size()) {
        return m_clientIcons[index];
    }

    // Return default icon (index 0) if invalid
    if (!m_clientIcons.isEmpty()) {
        return m_clientIcons[0];
    }

    return QIcon();
}

QIcon IconManager::getGroupIcon(int index) const
{
    // Groups use the same icon collection as entries
    return getEntryIcon(index);
}

QIcon IconManager::getToolbarIcon(const QString &name) const
{
    return m_toolbarIcons.value(name, QIcon());
}
