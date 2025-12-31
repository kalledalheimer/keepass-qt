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
