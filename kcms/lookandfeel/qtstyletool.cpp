/*
 *   Copyright 2017 Marco MArtin <mart@kde.org>
 *   Copyright 2020 Dominic Hayes <ferenosdev@outlook.com>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as
 *   published by the Free Software Foundation; either version 2,
 *   or (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details
 *
 *   You should have received a copy of the GNU Library General Public
 *   License along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include "kcm.h"

#include <iostream>

#include <QApplication>
#include <QCommandLineParser>
#include <QDebug>

// Frameworks
#include <KAboutData>
#include <KLocalizedString>

#include <KPackage/Package>
#include <KPackage/PackageLoader>

#include "lookandfeelsettings.h"

int main(int argc, char **argv)
{
    QApplication app(argc, argv);

    const char version[] = "1.0";

    // About data
    KAboutData aboutData("plasma-apply-appstyle",
                         i18n("Application Style Tool"),
                         version,
                         i18n("Command line tool to apply a Qt Application Style."),
                         KAboutLicense::GPL,
                         i18n("Copyright 2017, Marco Martin"));
    aboutData.addAuthor(i18n("Marco Martin"), i18n("Maintainer"), QStringLiteral("mart@kde.org"));
    aboutData.addAuthor(i18n("Dominic Hayes"), i18n("Maintainer"), QStringLiteral("ferenosdev@outlook.com"));
    aboutData.setDesktopFileName("org.kde.plasma-apply-appstyle");
    KAboutData::setApplicationData(aboutData);

    const static auto _a = QStringLiteral("apply");

    QCommandLineOption _apply =
        QCommandLineOption(QStringList() << QStringLiteral("a") << _a,
                           i18n("Apply an Application Style"),
                           i18n("packagename"));

    QCommandLineParser parser;
    parser.addOption(_apply);
    aboutData.setupCommandLine(&parser);

    parser.process(app);
    aboutData.processCommandLine(&parser);

    if (!parser.isSet(_apply)) {
        parser.showHelp();
    }
    
    if (parser.isSet(_apply)) {
        QString requestedTheme{parser.value(_apply)};

        KCMLookandFeel *kcm = new KCMLookandFeel(nullptr, QVariantList());
        kcm->load();
        kcm->setWidgetStyle(requestedTheme);
        delete kcm;
    }

    return 0;
}
