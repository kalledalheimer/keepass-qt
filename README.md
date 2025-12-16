# Qt-KeePass - Cross-Platform Port of KeePass Password Safe

## Overview

Qt-KeePass is a cross-platform port of [KeePass Password Safe v1.x](https://keepass.info/) from MFC (Microsoft Foundation Classes) to Qt framework. This project aims to bring the robust password management capabilities of KeePass to Windows, macOS, and Linux while maintaining 100% compatibility with the KDB v1.x database format.

## Project Status

**Current Phase: Phase 1 - Core Library Foundation**

This is an active migration project. The following features are planned:

### Completed
- ✅ Project structure and build system
- ✅ CMake configuration

### In Progress
- 🔄 Core library porting (KeePassLibCpp)
- 🔄 Cryptography layer (AES, Twofish, SHA-256)
- 🔄 KDB v1.x format I/O

### Planned
- ⏳ Cross-platform GUI (Qt Widgets)
- ⏳ Auto-type functionality
- ⏳ Plugin system
- ⏳ Import/Export (CSV, XML, HTML, etc.)

## Features (Target)

- **Full KDB v1.x Compatibility**: Open and save databases created with the original KeePass
- **Strong Encryption**: AES-256 and Twofish support with 600,000+ key transformation rounds
- **Cross-Platform**: Native support for Windows, macOS, and Linux
- **Secure Memory**: Memory locking and secure erasure of sensitive data
- **Auto-Type**: Platform-specific keyboard simulation for password entry
- **Import/Export**: Multiple format support (CSV, XML, HTML, TXT, legacy formats)
- **Plugin System**: Qt-based plugin architecture
- **Localization**: Multi-language support via Qt Linguist

## Building

### Requirements

- **CMake** 3.16 or later
- **Qt** 5.15 LTS or Qt 6.2+ LTS
  - Components: Core, Gui, Widgets
- **OpenSSL** 1.1.1+ or 3.x
- **C++17** compatible compiler
  - GCC 8+, Clang 10+, or MSVC 2019+

### Platform-Specific Dependencies

**Windows:**
- Visual Studio 2019 or later
- Windows SDK 10+

**macOS:**
- Xcode 12+
- macOS 10.13+

**Linux:**
- X11 development libraries: `libx11-dev`, `libxtst-dev`
- Install on Ubuntu/Debian: `sudo apt install libx11-dev libxtst-dev`

### Build Instructions

```bash
# Clone or navigate to the Qt-KeePass directory
cd Qt/Qt-KeePass

# Create build directory
mkdir build && cd build

# Configure with CMake
cmake ..

# Build
cmake --build .

# Run (once implemented)
./keepass
```

### Build Options

```bash
# Use Qt6 instead of Qt5
cmake -DQT_VERSION_MAJOR=6 ..

# Release build with optimizations
cmake -DCMAKE_BUILD_TYPE=Release ..

# Debug build with symbols
cmake -DCMAKE_BUILD_TYPE=Debug ..
```

## KDB Format Compatibility

This port maintains byte-perfect compatibility with the KDB v1.x format used by KeePass 1.x:

- **Database Signatures**: 0x9AA2D903, 0xB54BFB65
- **Version**: 0x00030004
- **Encryption**: AES-256 CBC or Twofish
- **Key Derivation**: AES-KDF with configurable rounds (default: 600,000)
- **Hash Function**: SHA-256
- **String Encoding**: UTF-8

## Project Structure

```
Qt-KeePass/
├── src/
│   ├── core/           # Core password management library
│   │   ├── crypto/     # Cryptographic implementations
│   │   ├── io/         # KDB file I/O
│   │   ├── generators/ # Password generators
│   │   └── platform/   # Platform abstractions
│   ├── gui/            # Qt-based GUI
│   │   ├── dialogs/    # Application dialogs
│   │   ├── widgets/    # Custom widgets
│   │   └── models/     # Qt Model/View classes
│   └── autotype/       # Auto-type system
│       └── platform/   # Platform-specific implementations
├── tests/              # Unit and integration tests
│   └── compatibility/  # KDB format compatibility tests
└── resources/          # Application resources
    ├── icons/
    └── translations/
```

## License

Copyright (C) 2003-2025 Dominik Reichl

This program is free software; you can redistribute it and/or modify it under the terms of the **GNU General Public License** version 2 or later as published by the Free Software Foundation.

See [LICENSE.txt](LICENSE.txt) for the full license text.

## Original KeePass

This is a port of KeePass Password Safe v1.x. The original project is maintained by Dominik Reichl:

- **Website**: https://keepass.info/
- **Original Source**: https://keepass.info/download.html

## Migration Plan

This project follows a phased migration approach:

1. **Phase 1** (Weeks 1-6): Core Library Foundation
   - Port data structures and crypto implementations
   - Achieve KDB format compatibility

2. **Phase 2** (Weeks 7-8): Platform Abstraction
   - Create cross-platform interface layer
   - Implement platform-specific code

3. **Phase 3** (Weeks 9-12): Basic GUI
   - Main window with entry/group management
   - Essential dialogs

4. **Phase 4** (Weeks 13-16): Advanced GUI
   - Complete all 25 dialogs
   - Custom widgets

5. **Phase 5** (Weeks 17-19): Auto-Type System
   - Cross-platform keyboard simulation

6. **Phase 6** (Weeks 20-22): Import/Export & Plugins
   - All format support
   - Qt plugin system

7. **Phase 7** (Weeks 23-26): Polish & Release
   - Testing, localization, documentation
   - Production v1.0 release

## Contributing

This project is currently in early development. Contributions are welcome once the core infrastructure is established.

## Acknowledgments

- **Dominik Reichl** - Original KeePass Password Safe author
- **Qt Project** - Cross-platform application framework
- **OpenSSL** - Cryptography library

## Disclaimer

This is an independent port project. While we strive for full compatibility with KeePass 1.x databases, always maintain backups of your password databases.

**⚠️ IMPORTANT**: This software is provided "AS IS" without warranty of any kind. Use at your own risk.
