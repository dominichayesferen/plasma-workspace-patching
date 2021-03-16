/*
 *   Copyright 2017 Marco Martin <mart@kde.org>
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

#include "colors.h"

#include <iostream>

#include <qcommandlineparser.h>
#include <QApplication>
#include <QDebug>

// Frameworks
#include <KAboutData>
#include <klocalizedstring.h>

#include <KPackage/Package>
#include <KPackage/PackageLoader>

#include "../krdb/krdb.h"

#include "colorsmodel.h"
#include "filterproxymodel.h"
#include "colorssettings.h"
#include "colorsdata.h"

int main(int argc, char **argv)
{
    QApplication app(argc, argv);

    const char version[] = "1.0";

    // About data
    KAboutData aboutData("colorskcmtool", i18n("Colours KCM Tool"), version, i18n("Temporary command line tool to apply colour schemes, as to work around the current issue where GTK3 applications' colours do not automatically reload outside of applying from Colours KCM manually."), KAboutLicense::GPL, i18n("Copyright 2020, Dominic Hayes"));
    aboutData.addAuthor(i18n("Dominic Hayes"), i18n("Maintainer"), QStringLiteral("ferenosdev@outlook.com"));
    aboutData.setDesktopFileName("org.kde.colorskcmtool");
    KAboutData::setApplicationData(aboutData);

    const static auto _a = QStringLiteral("apply");
    
    QCommandLineOption _apply = QCommandLineOption(QStringList() << QStringLiteral("a") << _a,
                                i18n("Apply a colour scheme"), i18n("packagename"));

    QCommandLineParser parser;
    parser.addOption(_apply);
    aboutData.setupCommandLine(&parser);

    parser.process(app);
    aboutData.processCommandLine(&parser);

    if (!parser.isSet(_apply)) {
        parser.showHelp();
    }

    if (parser.isSet(_apply)) {
        KCMColors *kcm = new KCMColors(nullptr, QVariantList());
        kcm->load();
        kcm->colorsSettings()->setColorScheme(parser.value(_apply));
        kcm->save();
        // TODO: figure out why setGlobalThemePackage isn't setting GlobalThemePackage
        KConfig config(QStringLiteral("kdeglobals"));
        KConfigGroup cg(&config, "KDE");
        //cg.writeEntry("e", parser.value(_apply));
        cg.sync();
        delete kcm;
    }

    return 0;
}
