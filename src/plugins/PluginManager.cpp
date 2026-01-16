/*
  Qt KeePass - Plugin Manager Implementation

  Reference: MFC WinGUI/Plugins/PluginMgr.cpp
*/

#include "PluginManager.h"
#include "../core/PwManager.h"
#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QMenu>
#include <QAction>
#include <QDebug>

PluginManager& PluginManager::instance()
{
    static PluginManager singleton;
    return singleton;
}

PluginManager::PluginManager()
    : QObject(nullptr)
{
}

PluginManager::~PluginManager()
{
    unloadAllPlugins(false);
}

QStringList PluginManager::pluginDirectories() const
{
    QStringList dirs;

    // Primary: {AppDir}/plugins/
    QString appDir = QCoreApplication::applicationDirPath();
    dirs.append(appDir + "/plugins");

    // Secondary: {AppDir}/
    dirs.append(appDir);

    // On macOS, also check inside bundle
#ifdef Q_OS_MACOS
    QString bundlePlugins = appDir + "/../PlugIns";
    if (QDir(bundlePlugins).exists()) {
        dirs.append(bundlePlugins);
    }
#endif

    return dirs;
}

QString PluginManager::primaryPluginDirectory() const
{
    QString dir = QCoreApplication::applicationDirPath() + "/plugins";

    // Create if doesn't exist
    QDir().mkpath(dir);

    return dir;
}

QStringList PluginManager::findPluginCandidates() const
{
    QStringList candidates;
    QStringList dirs = pluginDirectories();

    // Platform-specific plugin extensions
#ifdef Q_OS_WIN
    QStringList filters = {"*.dll"};
#elif defined(Q_OS_MACOS)
    QStringList filters = {"*.dylib", "*.so"};
#else
    QStringList filters = {"*.so"};
#endif

    for (const QString& dirPath : dirs) {
        QDir dir(dirPath);
        if (!dir.exists()) {
            continue;
        }

        QFileInfoList files = dir.entryInfoList(filters, QDir::Files | QDir::Readable);
        for (const QFileInfo& fileInfo : files) {
            QString filePath = fileInfo.absoluteFilePath();
            if (!candidates.contains(filePath)) {
                candidates.append(filePath);
            }
        }
    }

    return candidates;
}

bool PluginManager::isValidPlugin(const QString& filePath) const
{
    // Try to load as Qt plugin and check for our interface
    QPluginLoader loader(filePath);

    // Check metadata first (faster than loading)
    QJsonObject metaData = loader.metaData();
    if (metaData.isEmpty()) {
        return false;
    }

    // Check IID
    QString iid = metaData.value("IID").toString();
    return (iid == KpPluginInterface_iid);
}

void PluginManager::loadAllPlugins()
{
    qDebug() << "PluginManager: Loading all plugins...";

    QStringList candidates = findPluginCandidates();
    qDebug() << "PluginManager: Found" << candidates.size() << "plugin candidates";

    for (const QString& filePath : candidates) {
        loadPlugin(filePath);
    }

    qDebug() << "PluginManager: Loaded" << m_plugins.size() << "plugins";
}

bool PluginManager::loadPlugin(const QString& filePath)
{
    qDebug() << "PluginManager: Attempting to load:" << filePath;

    // Check if already loaded
    for (const auto& plugin : m_plugins) {
        if (plugin->filePath == filePath) {
            qDebug() << "PluginManager: Plugin already loaded:" << filePath;
            return false;
        }
    }

    // Create plugin instance
    auto instance = std::make_unique<PluginInstance>();
    instance->filePath = filePath;
    instance->loader = std::make_unique<QPluginLoader>(filePath);

    // Try to load
    QObject* pluginObject = instance->loader->instance();
    if (pluginObject == nullptr) {
        qWarning() << "PluginManager: Failed to load plugin:" << filePath;
        qWarning() << "  Error:" << instance->loader->errorString();
        return false;
    }

    // Cast to our interface
    instance->interface = qobject_cast<KpPluginInterface*>(pluginObject);
    if (instance->interface == nullptr) {
        qWarning() << "PluginManager: Plugin does not implement KpPluginInterface:" << filePath;
        instance->loader->unload();
        return false;
    }

    // Get plugin info
    instance->info = instance->interface->pluginInfo();
    instance->pluginId = m_nextPluginId++;

    qDebug() << "PluginManager: Loaded plugin:" << instance->info.name
             << "v" << instance->info.version
             << "by" << instance->info.author;

    // Initialize plugin if we have a database manager
    if (m_databaseManager != nullptr) {
        if (instance->interface->initialize(m_databaseManager)) {
            instance->initialized = true;
            qDebug() << "PluginManager: Plugin initialized:" << instance->info.name;
        } else {
            qWarning() << "PluginManager: Plugin initialization failed:" << instance->info.name;
        }
    }

    // Cache menu items and assign command IDs
    instance->menuItems = instance->interface->menuItems();
    assignCommandIds(*instance);

    // Store plugin
    quint32 pluginId = instance->pluginId;
    m_plugins.push_back(std::move(instance));

    emit pluginLoaded(pluginId);
    return true;
}

void PluginManager::assignCommandIds(PluginInstance& plugin)
{
    for (KpMenuItem& item : plugin.menuItems) {
        // Skip separators and submenu markers
        if ((item.flags & KpMenuFlags::Separator) != 0 ||
            (item.flags & KpMenuFlags::PopupEnd) != 0) {
            continue;
        }

        // Assign command ID
        if (m_nextCommandId <= m_commandRangeLast) {
            item.commandId = m_nextCommandId++;
            m_commandToPlugin[item.commandId] = &plugin;
        } else {
            qWarning() << "PluginManager: Command ID range exhausted!";
        }
    }
}

bool PluginManager::unloadPlugin(quint32 pluginId)
{
    for (size_t i = 0; i < m_plugins.size(); ++i) {
        if (m_plugins[i]->pluginId == pluginId) {
            auto& plugin = m_plugins[i];

            qDebug() << "PluginManager: Unloading plugin:" << plugin->info.name;

            // Remove command mappings
            for (const KpMenuItem& item : plugin->menuItems) {
                m_commandToPlugin.remove(item.commandId);
            }

            // Shutdown plugin
            if (plugin->initialized && plugin->interface != nullptr) {
                plugin->interface->shutdown();
            }

            // Unload library
            if (plugin->loader != nullptr) {
                plugin->loader->unload();
            }

            m_plugins.erase(m_plugins.begin() + static_cast<long>(i));
            emit pluginUnloaded(pluginId);
            return true;
        }
    }
    return false;
}

void PluginManager::unloadAllPlugins(bool skipLateUnload)
{
    qDebug() << "PluginManager: Unloading all plugins...";

    // First pass: unload plugins that don't request late unload
    if (skipLateUnload) {
        for (size_t i = m_plugins.size(); i > 0; --i) {
            auto& plugin = m_plugins[i - 1];
            if (plugin->interface != nullptr && !plugin->interface->requestsLateUnload()) {
                unloadPlugin(plugin->pluginId);
            }
        }
    }

    // Second pass: unload remaining plugins
    while (!m_plugins.empty()) {
        unloadPlugin(m_plugins.front()->pluginId);
    }

    // Reset command ID counter
    m_nextCommandId = m_commandRangeFirst;
    m_commandToPlugin.clear();
}

void PluginManager::setDatabaseManager(PwManager* manager)
{
    m_databaseManager = manager;

    // Initialize any plugins that haven't been initialized yet
    for (auto& plugin : m_plugins) {
        if (!plugin->initialized && plugin->interface != nullptr && m_databaseManager != nullptr) {
            if (plugin->interface->initialize(m_databaseManager)) {
                plugin->initialized = true;
            }
        }
    }
}

const PluginInstance* PluginManager::getPlugin(int index) const
{
    if (index >= 0 && index < static_cast<int>(m_plugins.size())) {
        return m_plugins[static_cast<size_t>(index)].get();
    }
    return nullptr;
}

const PluginInstance* PluginManager::getPluginById(quint32 pluginId) const
{
    for (const auto& plugin : m_plugins) {
        if (plugin->pluginId == pluginId) {
            return plugin.get();
        }
    }
    return nullptr;
}

void PluginManager::broadcastEvent(quint32 eventCode, void* param1, void* param2)
{
    for (auto& plugin : m_plugins) {
        if (plugin->initialized && plugin->interface != nullptr) {
            plugin->interface->onEvent(eventCode, param1, param2);
        }
    }
    emit pluginEvent(eventCode);
}

bool PluginManager::sendEventToPlugin(quint32 pluginId, quint32 eventCode, void* param1, void* param2)
{
    for (auto& plugin : m_plugins) {
        if (plugin->pluginId == pluginId && plugin->initialized && plugin->interface != nullptr) {
            return plugin->interface->onEvent(eventCode, param1, param2);
        }
    }
    return false;
}

void PluginManager::buildPluginMenu(QMenu* menu)
{
    if (menu == nullptr) {
        return;
    }

    // Clear existing items (if rebuilding)
    menu->clear();

    // Track submenu stack for nested menus
    QList<QMenu*> menuStack;
    menuStack.append(menu);

    for (const auto& plugin : m_plugins) {
        if (plugin->menuItems.isEmpty()) {
            continue;
        }

        // Add separator between plugins
        if (menuStack.last()->actions().count() > 0) {
            menuStack.last()->addSeparator();
        }

        for (const KpMenuItem& item : plugin->menuItems) {
            QMenu* currentMenu = menuStack.last();

            if ((item.flags & KpMenuFlags::Separator) != 0) {
                currentMenu->addSeparator();
            }
            else if ((item.flags & KpMenuFlags::PopupStart) != 0) {
                // Start submenu
                QMenu* submenu = currentMenu->addMenu(item.icon, item.text);
                menuStack.append(submenu);
            }
            else if ((item.flags & KpMenuFlags::PopupEnd) != 0) {
                // End submenu
                if (menuStack.size() > 1) {
                    menuStack.removeLast();
                }
            }
            else {
                // Normal menu item
                QAction* action = currentMenu->addAction(item.icon, item.text);
                action->setStatusTip(item.tooltip);
                action->setData(item.commandId);

                if ((item.flags & KpMenuFlags::Checkbox) != 0) {
                    action->setCheckable(true);
                    action->setChecked((item.flags & KpMenuFlags::Checked) != 0);
                }

                if ((item.flags & KpMenuFlags::Disabled) != 0) {
                    action->setEnabled(false);
                }

                // Connect to our handler
                connect(action, &QAction::triggered, this, [this, commandId = item.commandId]() {
                    handleMenuCommand(commandId);
                });
            }
        }
    }

    // Add "Manage Plugins..." item if there's content
    if (menu->actions().count() > 0) {
        menu->addSeparator();
    }
}

void PluginManager::handleMenuCommand(quint32 commandId)
{
    PluginInstance* plugin = findPluginByCommandId(commandId);
    if (plugin != nullptr && plugin->initialized && plugin->interface != nullptr) {
        plugin->interface->onEvent(KpPluginEvent::DirectExec, reinterpret_cast<void*>(commandId), nullptr);
    }
}

PluginInstance* PluginManager::findPluginByCommandId(quint32 commandId)
{
    auto it = m_commandToPlugin.find(commandId);
    if (it != m_commandToPlugin.end()) {
        return it.value();
    }
    return nullptr;
}

void PluginManager::setCommandRange(quint32 first, quint32 last)
{
    m_commandRangeFirst = first;
    m_commandRangeLast = last;
    m_nextCommandId = first;
}

bool PluginManager::pluginHandlesArg(const QString& arg) const
{
    for (const auto& plugin : m_plugins) {
        if (plugin->interface != nullptr && plugin->interface->handlesCommandLineArg(arg)) {
            return true;
        }
    }
    return false;
}
