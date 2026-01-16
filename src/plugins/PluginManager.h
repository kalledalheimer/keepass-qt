/*
  Qt KeePass - Plugin Manager

  Manages plugin loading, unloading, and lifecycle.
  Singleton class that handles all plugin operations.

  Reference: MFC WinGUI/Plugins/PluginMgr.h
*/

#ifndef PLUGINMANAGER_H
#define PLUGINMANAGER_H

#include "KpPluginInterface.h"
#include <QObject>
#include <QString>
#include <QMap>
#include <QPluginLoader>
#include <vector>
#include <memory>

class PwManager;
class QMenu;

// Loaded plugin instance information
struct PluginInstance
{
    quint32 pluginId = 0;               // Unique ID assigned by manager
    QString filePath;                   // Full path to plugin file
    std::unique_ptr<QPluginLoader> loader;  // Plugin loader
    KpPluginInterface* interface = nullptr; // Plugin interface pointer
    KpPluginInfo info;                  // Cached plugin info
    bool initialized = false;           // Whether initialize() was called
    QList<KpMenuItem> menuItems;        // Cached menu items with assigned IDs
};

class PluginManager : public QObject
{
    Q_OBJECT

public:
    // Singleton access
    static PluginManager& instance();

    // Prevent copying
    PluginManager(const PluginManager&) = delete;
    PluginManager& operator=(const PluginManager&) = delete;

    // Plugin loading
    void loadAllPlugins();
    void unloadAllPlugins(bool skipLateUnload = false);
    bool loadPlugin(const QString& filePath);
    bool unloadPlugin(quint32 pluginId);

    // Database manager access (for plugin initialization)
    void setDatabaseManager(PwManager* manager);
    [[nodiscard]] PwManager* databaseManager() const { return m_databaseManager; }

    // Plugin enumeration
    [[nodiscard]] int pluginCount() const { return static_cast<int>(m_plugins.size()); }
    [[nodiscard]] const PluginInstance* getPlugin(int index) const;
    [[nodiscard]] const PluginInstance* getPluginById(quint32 pluginId) const;

    // Event broadcasting
    void broadcastEvent(quint32 eventCode, void* param1 = nullptr, void* param2 = nullptr);
    bool sendEventToPlugin(quint32 pluginId, quint32 eventCode, void* param1 = nullptr, void* param2 = nullptr);

    // Menu integration
    void buildPluginMenu(QMenu* menu);
    void handleMenuCommand(quint32 commandId);

    // Command range for plugin menu items
    void setCommandRange(quint32 first, quint32 last);
    [[nodiscard]] quint32 commandRangeFirst() const { return m_commandRangeFirst; }
    [[nodiscard]] quint32 commandRangeLast() const { return m_commandRangeLast; }

    // Plugin directories
    [[nodiscard]] QStringList pluginDirectories() const;
    [[nodiscard]] QString primaryPluginDirectory() const;

    // Command-line argument handling
    [[nodiscard]] bool pluginHandlesArg(const QString& arg) const;

signals:
    void pluginLoaded(quint32 pluginId);
    void pluginUnloaded(quint32 pluginId);
    void pluginEvent(quint32 eventCode);

private:
    PluginManager();
    ~PluginManager() override;

    // Find plugin files in directories
    QStringList findPluginCandidates() const;

    // Validate plugin file
    bool isValidPlugin(const QString& filePath) const;

    // Assign command IDs to plugin menu items
    void assignCommandIds(PluginInstance& plugin);

    // Find plugin by command ID
    PluginInstance* findPluginByCommandId(quint32 commandId);

    // Members
    std::vector<std::unique_ptr<PluginInstance>> m_plugins;
    PwManager* m_databaseManager = nullptr;
    quint32 m_nextPluginId = 1;
    quint32 m_commandRangeFirst = 0x9000;
    quint32 m_commandRangeLast = 0x9FFF;
    quint32 m_nextCommandId = 0x9000;
    QMap<quint32, PluginInstance*> m_commandToPlugin;  // Command ID to plugin mapping
};

#endif // PLUGINMANAGER_H
