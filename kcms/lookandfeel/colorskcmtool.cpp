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
#include "../krdb/krdb.h"

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
    KAboutData aboutData("colorskcmtool",
                         i18n("Colour Scheme Tool"),
                         version,
                         i18n("Command line tool to apply a Colour Scheme."),
                         KAboutLicense::GPL,
                         i18n("Copyright 2017, Marco Martin, 2020, The Feren OS Dev"));
    aboutData.addAuthor(i18n("The Feren OS Dev"), i18n("Maintainer"), QStringLiteral("ferenosdev@outlook.com"));
    aboutData.setDesktopFileName("org.feren.qtstyletool");
    KAboutData::setApplicationData(aboutData);

    const static auto _a = QStringLiteral("apply");

    QCommandLineOption _apply = QCommandLineOption(QStringList() << QStringLiteral("a") << _a, i18n("Apply a Colour Scheme"), i18n("packagename"));

    QCommandLineParser parser;
    parser.addOption(_apply);
    aboutData.setupCommandLine(&parser);

    parser.process(app);
    aboutData.processCommandLine(&parser);

    if (!parser.isSet(_apply)) {
        parser.showHelp();
    }

    if (parser.isSet(_apply)) {
        KCMLookandFeel *kcm = new KCMLookandFeel(nullptr, QVariantList());
        kcm->load();
        QString colorScheme = parser.value(_apply);
        colorScheme.replace(0, 1, colorScheme.at(0).toUpper());
        
        colorScheme.remove(QLatin1Char('\'')); // So Foo's does not become FooS
        QRegExp fixer(QStringLiteral("[\\W,.-]+(.?)"));
        int offset;
        while ((offset = fixer.indexIn(colorScheme)) >= 0) {
            colorScheme.replace(offset, fixer.matchedLength(), fixer.cap(1).toUpper());
        }
        colorScheme.replace(0, 1, colorScheme.at(0).toUpper());

        // NOTE: why this loop trough all the scheme files?
        // the scheme theme name is an heuristic, there is no plugin metadata whatsoever.
        // is based on the file name stripped from weird characters or the
        // eventual id- prefix store.kde.org puts, so we can just find a
        // theme that ends as the specified name
        bool schemeFound = false;
        const QStringList schemeDirs =
            QStandardPaths::locateAll(QStandardPaths::GenericDataLocation, QStringLiteral("color-schemes"), QStandardPaths::LocateDirectory);
        for (const QString &dir : schemeDirs) {
            const QStringList fileNames = QDir(dir).entryList(QStringList() << QStringLiteral("*.colors"));
            for (const QString &file : fileNames) {
                if (file.endsWith(colorScheme + QStringLiteral(".colors"))) {
                    kcm->setColors(colorScheme, dir + QLatin1Char('/') + file);
                    schemeFound = true;
                    break;
                }
            }
            if (schemeFound) {
                break;
            }
        }
        runRdb(KRdbExportQtColors | KRdbExportGtkTheme | KRdbExportColors | KRdbExportQtSettings | KRdbExportXftSettings);
        delete kcm;
    }

    return 0;
}
