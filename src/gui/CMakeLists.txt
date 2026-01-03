# KeePass GUI Library
add_library(keepass-gui STATIC
    # Main window
    MainWindow.cpp
    MainWindow.h

    # Models
    GroupModel.cpp
    GroupModel.h
    EntryModel.cpp
    EntryModel.h

    # Dialogs
    MasterKeyDialog.cpp
    MasterKeyDialog.h
    AddGroupDialog.cpp
    AddGroupDialog.h
    AddEntryDialog.cpp
    AddEntryDialog.h
    FindDialog.cpp
    FindDialog.h
    PasswordGeneratorDialog.cpp
    PasswordGeneratorDialog.h
    DatabaseSettingsDialog.cpp
    DatabaseSettingsDialog.h

    # Icon management
    IconManager.cpp
    IconManager.h

    # Resources
    ${CMAKE_SOURCE_DIR}/resources/resources.qrc
)

target_link_libraries(keepass-gui
    PUBLIC
        keepass-core
        Qt${QT_VERSION_MAJOR}::Widgets
)

target_include_directories(keepass-gui
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/..
)
