/*
  Qt KeePass - Plugin Interface

  Defines the interface that all KeePass plugins must implement.
  Based on MFC KeePassLibCpp/SDK/Details/IKpPlugin.h

  Plugins are loaded using Qt's plugin system (QPluginLoader).
  Each plugin DLL/dylib/so must export a class implementing KpPluginInterface.
*/

#ifndef KPPLUGININTERFACE_H
#define KPPLUGININTERFACE_H

#include <QtPlugin>
#include <QString>
#include <QList>
#include <QIcon>

// Forward declarations
class PwManager;
class QMenu;

// Plugin event codes (matching MFC KPM_* defines)
namespace KpPluginEvent {
    // Initialization/Cleanup
    constexpr quint32 DelayedInit       = 58;   // Main window initialized
    constexpr quint32 Cleanup           = 72;   // Application shutting down

    // Direct commands
    constexpr quint32 DirectExec        = 1;    // Execute menu command
    constexpr quint32 DirectConfig      = 2;    // Show configuration dialog
    constexpr quint32 PluginInfo        = 3;    // Show about/info dialog

    // File operations
    constexpr quint32 FileNewPre        = 10;   // Before creating new database
    constexpr quint32 FileNewPost       = 11;   // After creating new database
    constexpr quint32 OpenDbPre         = 20;   // Before opening database
    constexpr quint32 OpenDbPost        = 21;   // After opening database
    constexpr quint32 OpenDbCommitted   = 22;   // Database open committed
    constexpr quint32 SaveDbPre         = 30;   // Before saving database
    constexpr quint32 SaveDbPost        = 31;   // After saving database
    constexpr quint32 SaveDbAsPre       = 32;   // Before Save As
    constexpr quint32 SaveDbAsPost      = 33;   // After Save As
    constexpr quint32 FileClosePre      = 40;   // Before closing file
    constexpr quint32 FileClosePost     = 41;   // After closing file
    constexpr quint32 FileLockPre       = 50;   // Before locking
    constexpr quint32 FileUnlockFailed  = 51;   // Unlock failed

    // Entry operations
    constexpr quint32 AddEntryPre       = 100;  // Before adding entry
    constexpr quint32 AddEntryPost      = 101;  // After adding entry
    constexpr quint32 EditEntryPre      = 110;  // Before editing entry
    constexpr quint32 EditEntryPost     = 111;  // After editing entry
    constexpr quint32 DeleteEntryPre    = 120;  // Before deleting entry
    constexpr quint32 DeleteEntryPost   = 121;  // After deleting entry

    // Group operations
    constexpr quint32 AddGroupPre       = 200;  // Before adding group
    constexpr quint32 AddGroupPost      = 201;  // After adding group
    constexpr quint32 ModifyGroupPre    = 210;  // Before modifying group
    constexpr quint32 ModifyGroupPost   = 211;  // After modifying group
    constexpr quint32 RemoveGroupPre    = 220;  // Before removing group
    constexpr quint32 RemoveGroupPost   = 221;  // After removing group

    // Context menus
    constexpr quint32 EntryContextMenu  = 300;  // Entry list context menu shown
    constexpr quint32 GroupContextMenu  = 301;  // Group tree context menu shown

    // Custom plugin messages (plugins can use 0x10000+)
    constexpr quint32 CustomBase        = 0x10000;
}

// Menu item flags
namespace KpMenuFlags {
    constexpr quint32 Normal            = 0x00000000;  // Normal menu item
    constexpr quint32 Checkbox          = 0x00000001;  // Checkbox item
    constexpr quint32 Disabled          = 0x00000002;  // Disabled/grayed
    constexpr quint32 Separator         = 0x00000004;  // Separator line
    constexpr quint32 PopupStart        = 0x00000008;  // Start of submenu
    constexpr quint32 PopupEnd          = 0x00000010;  // End of submenu
    constexpr quint32 Checked           = 0x00000020;  // Checkbox is checked
}

// Plugin menu item structure
struct KpMenuItem
{
    QString text;           // Menu item text (empty for separator)
    QString tooltip;        // Status bar tooltip
    QIcon icon;             // Optional icon
    quint32 flags = 0;      // KpMenuFlags
    quint32 commandId = 0;  // Assigned by PluginManager, not by plugin

    // Convenience constructors
    KpMenuItem() = default;

    explicit KpMenuItem(const QString& menuText, quint32 menuFlags = KpMenuFlags::Normal)
        : text(menuText), flags(menuFlags) {}

    static KpMenuItem separator() {
        return KpMenuItem(QString(), KpMenuFlags::Separator);
    }

    static KpMenuItem submenuStart(const QString& menuText) {
        return KpMenuItem(menuText, KpMenuFlags::PopupStart);
    }

    static KpMenuItem submenuEnd() {
        return KpMenuItem(QString(), KpMenuFlags::PopupEnd);
    }
};

// Plugin information structure
struct KpPluginInfo
{
    QString name;           // Plugin display name
    QString version;        // Version string (e.g., "1.0.0")
    QString author;         // Author name
    QString description;    // Short description
    QString website;        // Optional website URL
    QIcon icon;             // Plugin icon (optional)
};

/*
  KpPluginInterface - Main plugin interface

  All KeePass plugins must implement this interface.

  Example plugin implementation:

  class MyPlugin : public QObject, public KpPluginInterface
  {
      Q_OBJECT
      Q_PLUGIN_METADATA(IID KpPluginInterface_iid FILE "myplugin.json")
      Q_INTERFACES(KpPluginInterface)

  public:
      KpPluginInfo pluginInfo() const override;
      bool initialize(PwManager* manager) override;
      void shutdown() override;
      bool onEvent(quint32 eventCode, void* param1, void* param2) override;
      QList<KpMenuItem> menuItems() const override;
  };
*/
class KpPluginInterface
{
public:
    virtual ~KpPluginInterface() = default;

    // Get plugin information (name, version, author, etc.)
    virtual KpPluginInfo pluginInfo() const = 0;

    // Initialize the plugin with access to the database manager
    // Called after plugin is loaded
    // Return true if initialization successful
    virtual bool initialize(PwManager* manager) = 0;

    // Shutdown the plugin (called before unloading)
    virtual void shutdown() = 0;

    // Handle plugin events
    // eventCode: KpPluginEvent code
    // param1, param2: Event-specific parameters
    // Return true if event was handled
    virtual bool onEvent(quint32 eventCode, void* param1 = nullptr, void* param2 = nullptr) = 0;

    // Get menu items provided by this plugin
    // Called during menu building
    virtual QList<KpMenuItem> menuItems() const = 0;

    // Optional: Check if plugin wants to handle a command-line argument
    virtual bool handlesCommandLineArg(const QString& /*arg*/) const { return false; }

    // Optional: Request late unload (unload after other plugins during shutdown)
    virtual bool requestsLateUnload() const { return false; }
};

// Interface ID for Qt plugin system
#define KpPluginInterface_iid "org.keepass.KpPluginInterface/1.0"
Q_DECLARE_INTERFACE(KpPluginInterface, KpPluginInterface_iid)

#endif // KPPLUGININTERFACE_H
