/* This file is part of the KDE Project
   Copyright (c) 2014 Marco Martin <mart@kde.org>
   Copyright (c) 2014 Vishesh Handa <me@vhanda.in>
   Copyright (c) 2019 Cyril Rossi <cyril.rossi@enioka.com>
   Copyright (c) 2020-2021 Dominic Hayes <ferenosdev@outlook.com>
   Copyright (c) 2021 Benjamin Port <benjamin.port@enioka.com>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "kcm.h"
#include "../kcms-common_p.h"
#include "config-kcm.h"
#include "config-workspace.h"
#include "krdb.h"

#include <KAboutData>
#include <KAutostart>
#include <KDialogJobUiDelegate>
#include <KIO/ApplicationLauncherJob>
#include <KIconLoader>
#include <KMessageBox>
#include <KService>
#include <KSharedConfig>

#include <QDBusConnection>
#include <QDBusMessage>
#include <QDebug>
#include <QProcess>
#include <QQuickItem>
#include <QQuickWindow>
#include <QStandardItemModel>
#include <QStandardPaths>
#include <QStyle>
#include <QStyleFactory>
#include <QX11Info>

#include <KLocalizedString>
#include <KPackage/PackageLoader>

#include <X11/Xlib.h>

#include <updatelaunchenvjob.h>

#include "desktoplayoutdata.h"
#include "desktoplayoutsettings.h"

#ifdef HAVE_XCURSOR
#include "../cursortheme/xcursor/xcursortheme.h"
#include <X11/Xcursor/Xcursor.h>
#endif

#ifdef HAVE_XFIXES
#include <X11/extensions/Xfixes.h>
#endif

KCMDesktopLayout::KCMDesktopLayout(QObject *parent, const QVariantList &args)
    : KQuickAddons::ManagedConfigModule(parent, args)
    , m_data(new DesktopLayoutData(this))
    , m_config(QStringLiteral("kdeglobals"))
    , m_configGroup(m_config.group("KDE"))
{
    qmlRegisterType<DesktopLayoutSettings>();
    qmlRegisterType<QStandardItemModel>();
    qmlRegisterType<KCMDesktopLayout>();

    KAboutData *about = new KAboutData(QStringLiteral("kcm_desktoplayout"), i18n("Desktop Layout"), QStringLiteral("0.1"), QString(), KAboutLicense::LGPL);
    about->addAuthor(i18n("Dominic Hayes"), QString(), QStringLiteral("ferenosdev@outlook.com"));
    setAboutData(about);
    setButtons(Apply | Default);

    m_model = new QStandardItemModel(this);
    QHash<int, QByteArray> roles = m_model->roleNames();
    roles[PluginNameRole] = "pluginName";
    roles[DescriptionRole] = "description";
    roles[ScreenshotRole] = "screenshot";
    roles[FullScreenPreviewRole] = "fullScreenPreview";
    m_model->setItemRoleNames(roles);
    loadModel();
}

KCMDesktopLayout::~KCMDesktopLayout()
{
}

void KCMDesktopLayout::reloadModel()
{
    loadModel();
}

QStandardItemModel *KCMDesktopLayout::desktopLayoutModel() const
{
    return m_model;
}

int KCMDesktopLayout::pluginIndex(const QString &pluginName) const
{
    const auto results = m_model->match(m_model->index(0, 0), PluginNameRole, pluginName, 1, Qt::MatchExactly);
    if (results.count() == 1) {
        return results.first().row();
    }

    return -1;
}

QList<KPackage::Package> KCMDesktopLayout::availablePackages(const QStringList &components)
{
    QList<KPackage::Package> packages;
    QStringList paths;
    const QStringList dataPaths = QStandardPaths::standardLocations(QStandardPaths::GenericDataLocation);

    paths.reserve(dataPaths.count());
    for (const QString &path : dataPaths) {
        QDir dir(path + QStringLiteral("/plasma/look-and-feel"));
        paths << dir.entryList(QDir::AllDirs | QDir::NoDotAndDotDot);
    }

    for (const QString &path : paths) {
        KPackage::Package pkg = KPackage::PackageLoader::self()->loadPackage(QStringLiteral("Plasma/LookAndFeel"));
        pkg.setPath(path);
        pkg.setFallbackPackage(KPackage::Package());
        if (components.isEmpty()) {
            packages << pkg;
        } else {
            for (const auto &component : components) {
                if (!pkg.filePath(component.toUtf8()).isEmpty()) {
                    packages << pkg;
                    break;
                }
            }
        }
    }

    return packages;
}

DesktopLayoutSettings *KCMDesktopLayout::desktopLayoutSettings() const
{
    return m_data->settings();
}

void KCMDesktopLayout::loadModel()
{
    m_model->clear();

    QList<KPackage::Package> pkgs = availablePackages({"defaults", "layouts"});

    // Sort case-insensitively
    QCollator collator;
    collator.setCaseSensitivity(Qt::CaseInsensitive);
    std::sort(pkgs.begin(), pkgs.end(), [&collator](const KPackage::Package &a, const KPackage::Package &b) {
        return collator.compare(a.metadata().name(), b.metadata().name()) < 0;
    });

    for (const KPackage::Package &pkg : pkgs) {
        if (!pkg.metadata().isValid()) {
            continue;
        }
        QFileInfo check_file(pkg.filePath("layouts") + "/org.kde.plasma.desktop-layout.js");
        if (!check_file.isFile()) {
            continue;
        }
        QStandardItem *row = new QStandardItem(pkg.metadata().name());
        row->setData(pkg.metadata().pluginId(), PluginNameRole);
        row->setData(pkg.metadata().description(), DescriptionRole);
        row->setData(pkg.filePath("preview"), ScreenshotRole);
        row->setData(pkg.filePath("fullscreenpreview"), FullScreenPreviewRole);

        m_model->appendRow(row);
    }

    // Model has been cleared so pretend the selected look and fell changed to force view update
    emit desktopLayoutSettings()->lookAndFeelPackageChanged();
}

bool KCMDesktopLayout::isSaveNeeded() const
{
    return desktopLayoutSettings()->isSaveNeeded();
}

void KCMDesktopLayout::load()
{
    ManagedConfigModule::load();

    m_package = KPackage::PackageLoader::self()->loadPackage(QStringLiteral("Plasma/LookAndFeel"));
    m_package.setPath(desktopLayoutSettings()->lookAndFeelPackage());
}

void KCMDesktopLayout::save()
{
    KPackage::Package package = KPackage::PackageLoader::self()->loadPackage(QStringLiteral("Plasma/LookAndFeel"));
    package.setPath(desktopLayoutSettings()->lookAndFeelPackage());

    if (!package.isValid()) {
        return;
    }

    ManagedConfigModule::save();
    

    QDBusMessage message = QDBusMessage::createMethodCall(QStringLiteral("org.kde.plasmashell"),
                                                            QStringLiteral("/PlasmaShell"),
                                                            QStringLiteral("org.kde.PlasmaShell"),
                                                            QStringLiteral("loadLookAndFeelDefaultLayout"));

    QList<QVariant> args;
    args << desktopLayoutSettings()->lookAndFeelPackage();
    message.setArguments(args);

    QDBusConnection::sessionBus().call(message, QDBus::NoBlock);

    if (!package.filePath("defaults").isEmpty()) {
        KSharedConfigPtr conf = KSharedConfig::openConfig(package.filePath("defaults"));
        KConfigGroup cg(conf, "FerenThemer");
        cg = KConfigGroup(&cg, "Options");
        setFilesLayout(cg.readEntry("FilesStyle", QString()));
        
        cg = KConfigGroup(conf, "kwinrc");
        cg = KConfigGroup(&cg, "org.kde.kdecoration2");
        setWindowButtonsLayout(cg.readEntry("ButtonsOnLeft", QString()), cg.readEntry("ButtonsOnRight", QString()));
        
        cg = KConfigGroup(conf, "kwinrc");
        cg = KConfigGroup(&cg, "Windows");
        setBorderlessMaximised(cg.readEntry("BorderlessMaximizedWindows", QString()));
        
        cg = KConfigGroup(conf, "kdeglobals");
        cg = KConfigGroup(&cg, "KDE");
        
        // If the Desktop Layout is not org.feren.default and the Plasma Theme specified is 'feren', set it to 'feren-alt' instead - same for 'feren-light' -> 'feren-light-alt', and if it is org.feren.default switch it back if on -alt
        KConfig config2(QStringLiteral("plasmarc"));
        KConfigGroup cg2(&config2, "Theme");

        if ((cg2.readEntry("name", QString()) == "feren") && (desktopLayoutSettings()->lookAndFeelPackage() != "org.feren.default")) {
            cg2.writeEntry("name", QString("feren-alt"));
        } else if ((cg2.readEntry("name", QString()) == "feren-light") && (desktopLayoutSettings()->lookAndFeelPackage() != "org.feren.default")) {
            cg2.writeEntry("name", QString("feren-light-alt"));
        } else if ((cg2.readEntry("name", QString()) == "feren-alt") && (desktopLayoutSettings()->lookAndFeelPackage() == "org.feren.default")) {
            cg2.writeEntry("name", QString("feren"));
        } else if ((cg2.readEntry("name", QString()) == "feren-light-alt") && (desktopLayoutSettings()->lookAndFeelPackage() == "org.feren.default")) {
            cg2.writeEntry("name", QString("feren-light"));
        }
        cg2.sync();

        // autostart
        // remove all the old package to autostart
        {
            KSharedConfigPtr oldConf = KSharedConfig::openConfig(m_package.filePath("defaults"));
            cg = KConfigGroup(oldConf, QStringLiteral("Autostart"));
            const QStringList autostartServices = cg.readEntry("Services", QStringList());

            if (qEnvironmentVariableIsSet("KDE_FULL_SESSION")) {
                for (const QString &serviceFile : autostartServices) {
                    KService service(serviceFile + QStringLiteral(".desktop"));
                    KAutostart as(serviceFile);
                    as.setAutostarts(false);
                    // FIXME: quite ugly way to stop things, and what about non KDE things?
                    QProcess::startDetached(QStringLiteral("kquitapp5"),
                                            {QStringLiteral("--service"), service.property(QStringLiteral("X-DBUS-ServiceName")).toString()});
                }
            }
        }
        // Set all the stuff in the new lnf to autostart
        {
            cg = KConfigGroup(conf, QStringLiteral("Autostart"));
            const QStringList autostartServices = cg.readEntry("Services", QStringList());

            for (const QString &serviceFile : autostartServices) {
                KService::Ptr service{new KService(serviceFile + QStringLiteral(".desktop"))};
                KAutostart as(serviceFile);
                as.setCommand(service->exec());
                as.setAutostarts(true);
                if (qEnvironmentVariableIsSet("KDE_FULL_SESSION")) {
                    auto *job = new KIO::ApplicationLauncherJob(service);
                    job->setUiDelegate(new KDialogJobUiDelegate(KJobUiDelegate::AutoHandlingEnabled, nullptr));
                    job->start();
                }
            }
        }
        
        // Reload KWin if something changed, but only once.
        message = QDBusMessage(QDBusMessage::createSignal(QStringLiteral("/KWin"), QStringLiteral("org.kde.KWin"), QStringLiteral("reloadConfig")));
        QDBusConnection::sessionBus().send(message);
        
        //Set Latte Dock layout
        QFileInfo check_file(package.filePath("layouts") + "/latte-layout.layout.latte");
        if (check_file.isFile()) {
            setLatteLayout(package.filePath("layouts") + "/latte-layout.layout.latte");
        } else {
            KAutostart as("org.kde.latte-dock");
            as.setAutostarts(false);
            // Cleanly quit Latte Dock
            std::system("/usr/bin/killall feren-latte-launch");
            QDBusMessage message1 = QDBusMessage::createMethodCall(QStringLiteral("org.kde.lattedock"), QStringLiteral("/MainApplication"),
                                                        QStringLiteral("org.qtproject.Qt.QCoreApplication"), QStringLiteral("quit"));
            QDBusConnection::sessionBus().call(message1, QDBus::NoBlock);
        }
    }

    m_configGroup.sync();
    m_package.setPath(desktopLayoutSettings()->lookAndFeelPackage());
    runRdb(KRdbExportQtColors | KRdbExportGtkTheme | KRdbExportColors | KRdbExportQtSettings | KRdbExportXftSettings);
}

void KCMDesktopLayout::defaults()
{
    ManagedConfigModule::defaults();
}

void KCMDesktopLayout::revertKeyIfNeeded(KConfigGroup &group, KConfigGroup &home, KConfigGroup &defaults)
{
    for (const QString &key : group.keyList()) {
        home.revertToDefault(key);
        if (m_data->isDefaults()) {
            defaults.revertToDefault(key);
        }
    }
    for (const QString &subgroupname : group.groupList()) {
        KConfigGroup subgroup(group.group(subgroupname));
        KConfigGroup subgroupHome(home.group(subgroupname));
        KConfigGroup subgroupDefaults(defaults.group(subgroupname));
        revertKeyIfNeeded(subgroup, subgroupHome, subgroupDefaults);
    }
}

void KCMDesktopLayout::setBorderlessMaximised(const QString &value)
{

    KConfig config(QStringLiteral("kwinrc"));
    KConfigGroup cg(&config, QStringLiteral("Windows"));
    KConfig configDefault(configDefaults(QStringLiteral("kwinrc")));
    KConfigGroup cgd(&configDefault, QStringLiteral("Windows"));
    

    if (value.isEmpty()) {
        writeNewDefaults(cg, cgd, QStringLiteral("BorderlessMaximizedWindows"), "false");
        return;
    }

    writeNewDefaults(cg, cgd, QStringLiteral("BorderlessMaximizedWindows"), value);
}

void KCMDesktopLayout::setLatteLayout(const QString &theme)
{
    // FIXME: quite ugly way to do this
    QProcess::startDetached(QStringLiteral("killall -9 latte-dock feren-latte-launch"));
    
    QProcess::startDetached(QStringLiteral("/usr/bin/latte-dock"),
                                        {QStringLiteral("--import-layout"), theme});
    QProcess::startDetached(QStringLiteral("/usr/bin/feren-latte-launch"));
}

void KCMDesktopLayout::setFilesLayout(const QString &theme)
{
    if (theme.isEmpty()) {
        return;
    }

    std::string selectedtheme = theme.toStdString();
    std::system(("/usr/bin/feren-theme-tool-plasma setnemo "+selectedtheme).c_str());
}

void KCMDesktopLayout::setWindowButtonsLayout(const QString &leftbtns, const QString &rightbtns)
{
    if (leftbtns.isEmpty() && rightbtns.isEmpty()) {
        return;
    }

    KConfig config(QStringLiteral("kwinrc"));
    KConfigGroup cg(&config, QStringLiteral("org.kde.kdecoration2"));
    KConfig configDefault(configDefaults(QStringLiteral("kwinrc")));
    KConfigGroup cgd(&configDefault, QStringLiteral("org.kde.kdecoration2"));
    
    writeNewDefaults(cg, cgd, QStringLiteral("ButtonsOnLeft"), leftbtns);
    writeNewDefaults(cg, cgd, QStringLiteral("ButtonsOnRight"), rightbtns);
}

void KCMDesktopLayout::writeNewDefaults(const QString &filename,
                                      const QString &group,
                                      const QString &key,
                                      const QString &value,
                                      KConfig::WriteConfigFlags writeFlags)
{
    KConfig config(filename);
    KConfigGroup cg(&config, group);

    KConfig configDefault(configDefaults(filename));
    KConfigGroup cgd(&configDefault, group);

    writeNewDefaults(cg, cgd, key, value, writeFlags);
}

void KCMDesktopLayout::writeNewDefaults(KConfig &config,
                                      KConfig &configDefault,
                                      const QString &group,
                                      const QString &key,
                                      const QString &value,
                                      KConfig::WriteConfigFlags writeFlags)
{
    KConfigGroup cg(&config, group);
    KConfigGroup cgd(&configDefault, group);

    writeNewDefaults(cg, cgd, key, value, writeFlags);
}

void KCMDesktopLayout::writeNewDefaults(KConfigGroup &cg, KConfigGroup &cgd, const QString &key, const QString &value, KConfig::WriteConfigFlags writeFlags)
{
    if (m_data->isDefaults()) {
        cgd.revertToDefault(key);
    } else {
        cgd.writeEntry(key, value, writeFlags);
    }
    cgd.sync();

    cg.revertToDefault(key, writeFlags);
    cg.sync();
}

KConfig KCMDesktopLayout::configDefaults(const QString &filename)
{
    return KConfig(QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation) + "/kdedefaults/" + filename, KConfig::SimpleConfig);
}
