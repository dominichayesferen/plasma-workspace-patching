/*
   Copyright (C) 2020 David Edmundson <davidedmundson@kde.org>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the Lesser GNU General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

   You should have received a copy of the Lesser GNU General Public License
   along with this program; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "waylandclipboard.h"

#include <QDebug>
#include <QFile>
#include <QFutureWatcher>
#include <QGuiApplication>
#include <QPointer>

#include <QtWaylandClient/QWaylandClientExtension>

#include <qpa/qplatformnativeinterface.h>

#include <unistd.h>

#include "datacontrol.h"

#include <sys/select.h>

WaylandClipboard::WaylandClipboard(QObject *parent)
    : SystemClipboard(parent)
    , m_manager(new DataControlDeviceManager)
{
    connect(m_manager.get(), &DataControlDeviceManager::activeChanged, this, [this]() {
        if (m_manager->isActive()) {
            QPlatformNativeInterface *native = qApp->platformNativeInterface();
            if (!native) {
                return;
            }
            auto seat = static_cast<struct ::wl_seat *>(native->nativeResourceForIntegration("wl_seat"));
            if (!seat) {
                return;
            }

            m_device.reset(new DataControlDevice(m_manager->get_data_device(seat)));

            connect(m_device.get(), &DataControlDevice::receivedSelectionChanged, this, [this]() {
                emit changed(QClipboard::Clipboard);
            });
            connect(m_device.get(), &DataControlDevice::selectionChanged, this, [this]() {
                emit changed(QClipboard::Clipboard);
            });
        } else {
            m_device.reset();
        }
    });
}

void WaylandClipboard::setMimeData(QMimeData *mime, QClipboard::Mode mode)
{
    if (!m_device) {
        return;
    }
    auto source = std::make_unique<DataControlSource>(m_manager->create_data_source(), mime);
    if (mode == QClipboard::Clipboard) {
        m_device->setSelection(std::move(source));
    }
}

void WaylandClipboard::clear(QClipboard::Mode mode)
{
    if (!m_device) {
        return;
    }
    if (mode == QClipboard::Clipboard) {
        m_device->set_selection(nullptr);
    } else if (mode == QClipboard::Selection) {
        if (zwlr_data_control_device_v1_get_version(m_device->object()) >= ZWLR_DATA_CONTROL_DEVICE_V1_SET_PRIMARY_SELECTION_SINCE_VERSION) {
            m_device->set_primary_selection(nullptr);
        }
    }
}

const QMimeData *WaylandClipboard::mimeData(QClipboard::Mode mode) const
{
    if (!m_device) {
        return nullptr;
    }
    if (mode == QClipboard::Clipboard) {
        // return our locally set selection if it's not cancelled to avoid copying data to ourselves
        return m_device->selection() ? m_device->selection() : m_device->receivedSelection();
    }
    return nullptr;
}

#include "waylandclipboard.moc"
