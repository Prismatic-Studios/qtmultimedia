/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qwasmmediaintegration_p.h"
#include "qwasmmediadevices_p.h"
#include <QLoggingCategory>

#include <private/qplatformmediaformatinfo_p.h>
#include <private/qplatformmediaplugin_p.h>

QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(qtWasmMediaPlugin, "qt.multimedia.wasm")

class QWasmMediaPlugin : public QPlatformMediaPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID QPlatformMediaPlugin_iid FILE "wasm.json")

public:
    QWasmMediaPlugin()
      : QPlatformMediaPlugin()
    {}

    QPlatformMediaIntegration* create(const QString &name) override
    {
        if (name == QLatin1String("wasm"))
            return new QWasmMediaIntegration;
        return nullptr;
    }
};

QWasmMediaIntegration::QWasmMediaIntegration()
{

}

QWasmMediaIntegration::~QWasmMediaIntegration()
{
    delete m_devices;
    delete m_formatInfo;
}

QPlatformMediaFormatInfo *QWasmMediaIntegration::formatInfo()
{
     if (!m_formatInfo)
         m_formatInfo = new QPlatformMediaFormatInfo();
     return m_formatInfo;
}

QPlatformMediaDevices *QWasmMediaIntegration::devices()
{
    if (!m_devices)
        m_devices = new QWasmMediaDevices(this);
    return m_devices;
}

QT_END_NAMESPACE

#include "qwasmmediaintegration.moc"
