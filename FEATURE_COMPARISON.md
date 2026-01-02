# KeePass Qt vs MFC Feature Comparison

**Last Updated:** 2026-01-01
**Status:** Phase 3 (Basic GUI) Complete

## ✅ Implemented Features

### Core Library
- [x] KDB v1.x file format (100% compatible)
- [x] Master password authentication
- [x] AES-256 and Twofish encryption
- [x] SHA-256 hashing
- [x] Key transformation (600,000+ rounds)
- [x] Session key (in-memory password encryption)
- [x] Groups and entries management
- [x] UUID generation
- [x] Timestamps (creation, modification, access, expiration)
- [x] Binary attachments support

### File Operations
- [x] File > New (create new database)
- [x] File > Open (open existing database)
- [x] File > Save
- [x] File > Save As
- [x] File > Close (implicit via New/Open)

### Group Management
- [x] Add Group dialog
- [x] Edit Group (via double-click)
- [x] Delete Group (with backup option)
- [x] Group tree view with icons
- [x] Default group structure (General + 5 subgroups)
- [x] Group hierarchy (levels)

### Entry Management
- [x] Add Entry dialog (all 10 fields)
- [x] Edit Entry dialog
- [x] Delete Entry
- [x] Entry table view with icons
- [x] Entry filtering by group
- [x] Auto-generated random passwords (16 chars)
- [x] Password show/hide toggle
- [x] Password match validation
- [x] Expiration date/time picker

### Icon System
- [x] 69 entry/group icons (clienticex.bmp)
- [x] 8 toolbar icons
- [x] Magenta transparency support
- [x] Icons in toolbar, tree view, and table view

### User Interface
- [x] Main window with splitter
- [x] Group tree view (left panel)
- [x] Entry table view (right panel)
- [x] Menu bar
- [x] Toolbar with icons
- [x] Status bar with messages
- [x] Keyboard shortcuts (Return, Delete, Insert)
- [x] Confirmation dialogs

---

## ❌ Missing Features (Prioritized)

### HIGH PRIORITY - Phase 4 (Advanced GUI)

#### Search/Find Functionality
- [ ] Edit > Find dialog
  - Search in: Title, URL, UUID, Username, Notes, Password, Group name
  - Regular expression support
  - Case sensitive option
  - Exclude backups option
  - Exclude expired entries option
- [ ] Quick find combo box (toolbar)
- [ ] Find next/previous

#### Password Generator
- [ ] Password Generator dialog (simple mode)
  - Character sets: uppercase, lowercase, digits, special chars
  - Password length
  - Exclude look-alike characters
  - Exclude specific characters
- [ ] Password Generator (advanced mode)
  - Pattern-based generation
  - Pronounceable passwords
  - Hex passwords
- [ ] Password quality indicator (entropy calculation)
- [ ] Password generation profiles/presets

#### Entry Management Advanced
- [ ] Copy Username (Edit menu + toolbar)
- [ ] Copy Password (Edit menu + toolbar)
- [ ] Entry properties dialog (read-only view)
- [ ] Entry history/backup
- [ ] Duplicate entry
- [ ] Move entry to different group
- [ ] Sort entries (by title, username, password, URL, etc.)

#### Group Management Advanced
- [ ] Edit Group (dedicated dialog)
- [ ] Move group
- [ ] Sort groups
- [ ] Expand/collapse all groups

#### View Options
- [ ] View > Change Language
- [ ] View > Show Toolbar (toggle)
- [ ] View > Show Details View (toggle)
- [ ] View > Always on Top
- [ ] View > Hide Usernames Behind Asterisks
- [ ] View > Hide Passwords Behind Asterisks
- [ ] View > Auto-Sort Entry List (by various fields)
- [ ] View > TAN View Options
  - Simple List View for TAN-Only Groups
  - Show TAN Indices in Entry Titles

#### Database Settings
- [ ] File > Database Settings dialog
  - Database name
  - Database description
  - Default username
  - Database color
  - Key transformation rounds (adjustable)
  - Encryption algorithm selection (AES/Twofish)
  - Compression settings

#### Tools Menu
- [ ] Tools > Options dialog
  - **Security tab:**
    - Lock workspace when minimizing
    - Lock workspace on Windows lock/sleep
    - Lock workspace after idle time
    - Disable unsafe operations
    - Use secure password edit controls
  - **Interface tab:**
    - Display images on buttons
    - Show grid lines in entry list
    - Auto-resize table columns
    - Font settings
    - Main window position/size
  - **Advanced tab:**
    - Start minimized
    - Minimize to tray
    - Close button minimizes to tray
    - Remember last used file
    - Automatically save database
    - Backup on save

### MEDIUM PRIORITY - Phase 5

#### Import/Export
- [ ] File > Import submenu
  - CSV File
  - CodeWallet TXT File
  - PwSafe v2 TXT File
  - Personal Vault TXT File
  - KeePass Database
  - Plugin support
- [ ] File > Export submenu
  - TXT File
  - HTML File
  - XML File
  - CSV File
  - KeePass Database

#### Auto-Type
- [ ] Auto-Type functionality
  - Global hot key
  - Window title matching
  - Custom sequences
  - Two-channel auto-type obfuscation
- [ ] Tools > Options > Auto-Type tab
  - Enable/disable auto-type
  - Global hot key configuration
  - Default auto-type sequence

#### Entry Advanced Features
- [ ] Field references (insert references to other entries)
- [ ] URL field dropdown with history
- [ ] Notes field with rich text editing
- [ ] Attachment management
  - Set attachment
  - Save attachment
  - Remove attachment

#### Printing
- [ ] File > Print Preview
- [ ] File > Print

### LOW PRIORITY - Phase 6+

#### Plugin System
- [ ] Plugin framework
- [ ] Plugin management dialog
- [ ] Import/export plugins
- [ ] KPScript support

#### Advanced Dialogs
- [ ] Icon Picker dialog (custom icon selection)
- [ ] Get Random dialog (mouse/keyboard entropy collection)
- [ ] TAN Wizard
- [ ] Entry List dialog (show all entries flat)
- [ ] Permission dialog
- [ ] Help Source dialog
- [ ] Update Info dialog

#### System Integration
- [ ] Lock Workspace functionality
- [ ] System tray integration
- [ ] Global hot keys
- [ ] File associations (.kdb files)
- [ ] Command-line arguments
- [ ] Drag & drop support

#### Localization
- [ ] Multi-language support
- [ ] Language selection dialog
- [ ] Translation loading from LNG files

---

## 📊 Feature Completion Summary

**Phase 1 (Core Library):** ✅ 100% Complete
**Phase 2 (Platform Abstraction):** ✅ 100% Complete (lite version)
**Phase 3 (Basic GUI):** ✅ 100% Complete

**Overall Progress:**
- **Implemented:** ~30% of all MFC features
- **Phase 4 (Advanced GUI):** 0% (next priority)
- **Phase 5 (Auto-Type):** 0%
- **Phase 6+ (Plugins, Advanced):** 0%

---

## 🎯 Next Steps (Phase 4 Priority Order)

1. **Search/Find** - Essential for usability
2. **Password Generator** - Core password management feature
3. **Copy Username/Password** - Common operations
4. **Database Settings** - Important configuration
5. **Tools > Options** - User preferences and security settings
6. **View Options** - UI customization
7. **Sort/Move operations** - Entry/group organization

---

## 📝 Notes

- All Phase 3 features are MFC-compatible
- KDB file format is 100% compatible with MFC KeePass v1.43
- Icon system matches MFC exactly (69 icons + 8 toolbar)
- Dialog layouts and behavior match MFC version

**Testing Status:**
- Core library: ✅ Validated with MFC round-trip tests
- Basic GUI: ⏳ Manual testing in progress

