/* Copyright (C) 2014 Dan Chapman <dpniel@ubuntu.com>

   This file is part of the Trojita Qt IMAP e-mail client,
   http://trojita.flaska.net/

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2 of
   the License or (at your option) version 3 or any later version
   accepted by the membership of KDE e.V. (or its successor approved
   by the membership of KDE e.V.), which shall act as a proxy
   defined in Section 14 of version 3 of the license.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#include <QtGui/QGuiApplication>
#include <QtQuick/QQuickView>
#include <QtQml/QtQml>
#include <QQmlContext>
#include <QDebug>
#include <QSettings>
#include "AppVersion/SetCoreApplication.h"
#include "Common/Application.h"
#include "Common/MetaTypes.h"
#include "Imap/Model/ImapAccess.h"
#include "config.h"


static QString fullPath(const QString &fileName)
{
    QString result;
    QString appPath = QCoreApplication::applicationDirPath();
    if (appPath == UBUNTU_BINDIR) {
        result = QString(UBUNTU_INSTALL_DATADIR) + fileName;
        qDebug() << result;
    } else {
        result = QString(UBUNTU_DEV_DATADIR) + fileName;
        qDebug() << result;
    }
    return result;
}
int main(int argc, char *argv[])
{

    QGuiApplication app(argc, argv);
    QStringList args = app.arguments();
    if (args.contains("-h") || args.contains("--help")) {
        qDebug() << "usage: " + args.at(0) + " [-p|--phone] [-t|--tablet] [-h|--help] [-I <path>]";
        qDebug() << "    -p|--phone    If running on Desktop, start in a phone sized window.";
        qDebug() << "    -t|--tablet   If running on Desktop, start in a tablet sized window.";
        qDebug() << "    -h|--help     Print this help.";
        return 0;
    }

    QQuickView viewer;
    viewer.setResizeMode(QQuickView::SizeRootObjectToView);

    viewer.engine()->rootContext()->setContextProperty("tablet", QVariant(false));
    viewer.engine()->rootContext()->setContextProperty("phone", QVariant(false));
    if (args.contains("-t") || args.contains("--tablet")) {
        qDebug() << "running in tablet mode";
        viewer.engine()->rootContext()->setContextProperty("tablet", QVariant(true));
    } else if (args.contains("-p") || args.contains("--phone")){
        qDebug() << "running in phone mode";
        viewer.engine()->rootContext()->setContextProperty("phone", QVariant(true));
    } else if (qgetenv("QT_QPA_PLATFORM") != "ubuntumirclient") {
        // Default to tablet size on X11
        viewer.engine()->rootContext()->setContextProperty("tablet", QVariant(true));
    }

    Common::registerMetaTypes();
    Common::Application::name = QString::fromLatin1("trojita-tp");
    AppVersion::setGitVersion();
    AppVersion::setCoreApplicationData();


    QSettings s;
    Imap::ImapAccess imapAccess(0, &s, QLatin1String("defaultAccount"));
    viewer.engine()->rootContext()->setContextProperty(QLatin1String("imapAccess"), &imapAccess);

    viewer.setTitle("Trojita");
    viewer.setSource(fullPath("qml/trojita/main.qml"));
    viewer.show();
    return app.exec();
}
