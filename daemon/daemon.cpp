/*
  Copyright (C) 2014 Tomasz Sterna

  You may use this file under the terms of BSD license as follows:

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of the authors nor the
      names of its contributors may be used to endorse or promote products
      derived from this software without specific prior written permission.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
  ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR CONTRIBUTORS BE LIABLE FOR
  ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
  (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
  ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "manager.h"

#include <signal.h>
#include <QCoreApplication>
#include <QStandardPaths>
#include <QFile>
#include <QDir>
#include <QFileInfo>
#include <QLoggingCategory>

void signalhandler(int sig)
{
    if (sig == SIGINT) {
        qDebug() << "quit by SIGINT";
        qApp->quit();
    }
    else if (sig == SIGTERM) {
        qDebug() << "quit by SIGTERM";
        qApp->quit();
    }
}

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    QStringList filterRules;

    filterRules << (argc > 1 and QString("-d") == argv[0] ?
                       "*.debug=false" : "*.debug=true");

    // Init logging should be called after app object creation
    QLoggingCategory::setFilterRules(filterRules.join("\n"));

    QLoggingCategory l("main");
    qCDebug(l) << argv[0] << APP_VERSION;

    Settings settings;
    watch::WatchConnector watch;
    DBusConnector dbus;
    VoiceCallManager voice(&settings);
    NotificationManager notifications(&settings);
    Manager manager(&watch, &dbus, &voice, &notifications, &settings);

    signal(SIGINT, signalhandler);
    signal(SIGTERM, signalhandler);
    QObject::connect(&app, SIGNAL(aboutToQuit()), &watch, SLOT(endPhoneCall()));
    QObject::connect(&app, SIGNAL(aboutToQuit()), &watch, SLOT(disconnect()));

    return app.exec();
}
