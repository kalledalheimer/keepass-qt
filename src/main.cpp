/*
  KeePass Password Safe - Qt Port
  Copyright (C) 2003-2025 Dominik Reichl <dominik.reichl@t-online.de>
  Qt Port Copyright (C) 2025

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

#include <QApplication>
#include "gui/MainWindow.h"
#include "core/PwManager.h"

int main(int argc, char *argv[])
{
    // Initialize Qt resources from static library
    Q_INIT_RESOURCE(resources);

    QApplication app(argc, argv);
    app.setApplicationName("KeePass");
    app.setApplicationVersion(PWM_VERSION_STR);
    app.setOrganizationName("KeePass");
    app.setOrganizationDomain("keepass.info");

    // Create and show main window
    MainWindow mainWindow;
    mainWindow.show();

    return app.exec();
}
