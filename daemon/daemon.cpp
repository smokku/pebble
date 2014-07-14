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

#include "LogManager"
#include "SystemlogAppender"
#include "FileAppender"
#include "helpers/factory.h"
#include "Appender"
#include "PropertyConfigurator"

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

// For some reason exactly SystemLogAppender doesn't have a factory registered in log4qt,
// so in order for it to be instantiatable from .conf file (or even in general?) we declare factory
// right here and will register it in initLoggin()
Log4Qt::Appender *create_system_log_appender() {
    return new Log4Qt::SystemLogAppender;
}

void initLogging()
{
    // Should really be done in log4qt, but somehow it's missing these
    Log4Qt::Factory::registerAppender("org.apache.log4j.SystemLogAppender", create_system_log_appender);

    // Sailfish OS-specific locations for the app settings files and app's own files
    const QString logConfigFilePath(QStandardPaths::standardLocations(QStandardPaths::ConfigLocation).at(0)
                                    + "pebble/log4qt.conf");
    const QString fallbackLogConfigPath("/usr/share/pebble/log4qt.conf");

    const QString& usedConfigFile = QFile::exists(logConfigFilePath) ? logConfigFilePath : fallbackLogConfigPath;
    Log4Qt::PropertyConfigurator::configure(usedConfigFile);

    // Uglyish hack for replacing $XDG_CACHE_HOME with the proper cache directory
    // TODO: Implement replacing of $XDG_CACHE_HOME (and other vars?) with the proper values before configuring log4qt

    // Iterate all appenders attached to root logger and whenever a FileAppender (or its descender found), replace
    // $XDG_CACHE_HOME with the proper folder name
    QList<Log4Qt::Appender *> appenders = Log4Qt::LogManager::rootLogger()->appenders();
    QList<Log4Qt::Appender *>::iterator i;
    QDir pathCreator;
    for (i = appenders.begin(); i != appenders.end(); ++i) {
          Log4Qt::FileAppender* fa = qobject_cast<Log4Qt::FileAppender*>(*i);
          if(fa) {
              QString filename = fa->file();

              // As per March 2014 on emulator QStandardPaths::CacheLocation is /home/nemo/.cache
              // while on device it is /home/nemo/.cache/app-name
              // both things are fine, logging path will just be a little deeper on device
              filename.replace("$XDG_CACHE_HOME",
                              QStandardPaths::standardLocations(QStandardPaths::CacheLocation).at(0)
                              );
              // make sure loggin dir exists
              QFileInfo fi(filename);
              if(!pathCreator.mkpath(fi.path())) {
                  Log4Qt::LogManager::rootLogger()->error("Failed to create dir for logging: %1", fi.path());
              }

              fa->setFile(filename);
              fa->activateOptions();
          }
    }

    // For capturing qDebug() and console.log() messages
    // Note that console.log() might fail in Sailfish OS device builds. Not sure why, but it seems like
    // console.log() exactly in Sailfish OS device release builds doesn't go through the same qDebug() channel
    Log4Qt::LogManager::setHandleQtMessages(true);

    qDebug() << "Using following log config file: " << usedConfigFile;

    Log4Qt::Logger::logger(QLatin1String("Main Logger"))->info("Logging started");
}

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    // Init logging should be called after app object creation as initLogging() will examine
    // QCoreApplication for determining the .conf files locations
    initLogging();

    watch::WatchConnector watch;
    DBusConnector dbus;
    VoiceCallManager voice;
    NotificationManager notifications;

    Manager manager(&watch, &dbus, &voice, &notifications);

    signal(SIGINT, signalhandler);
    signal(SIGTERM, signalhandler);
    QObject::connect(&app, SIGNAL(aboutToQuit()), &watch, SLOT(endPhoneCall()));

    return app.exec();
}
