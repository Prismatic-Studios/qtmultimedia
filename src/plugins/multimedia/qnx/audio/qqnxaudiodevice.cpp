/****************************************************************************
**
** Copyright (C) 2016 Research In Motion
** Copyright (C) 2021 The Qt Company
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

#include "qqnxaudiodevice_p.h"

#include "qqnxaudioutils_p.h"

#include <sys/asoundlib.h>

QT_BEGIN_NAMESPACE

QnxAudioDeviceInfo::QnxAudioDeviceInfo(const QByteArray &deviceName, QAudioDevice::Mode mode)
    : QAudioDevicePrivate(deviceName, mode)
{
    preferredFormat.setSampleRate(44100);
    preferredFormat.setSampleFormat(QAudioFormat::Int16);
    preferredFormat.setChannelCount(mode == QAudioDevice::Input ? 1 : 2);

    description = QString::fromUtf8(id);

    minimumSampleRate = 8000;
    maximumSampleRate = 48000;
    minimumChannelCount = 1;
    maximumChannelCount = 2;
    supportedSampleFormats << QAudioFormat::UInt8 << QAudioFormat::Int16 << QAudioFormat::Int32;
}

QnxAudioDeviceInfo::~QnxAudioDeviceInfo()
{
}

bool QnxAudioDeviceInfo::isFormatSupported(const QAudioFormat &format) const
{
    const int pcmMode = (mode == QAudioDevice::Output) ? SND_PCM_OPEN_PLAYBACK : SND_PCM_OPEN_CAPTURE;
    snd_pcm_t *handle;

    int card = 0;
    int device = 0;
    if (snd_pcm_open_preferred(&handle, &card, &device, pcmMode) < 0)
        return false;

    snd_pcm_channel_info_t info;
    memset (&info, 0, sizeof(info));
    info.channel = (mode == QAudioDevice::Output) ? SND_PCM_CHANNEL_PLAYBACK : SND_PCM_CHANNEL_CAPTURE;

    if (snd_pcm_plugin_info(handle, &info) < 0) {
        qWarning("QAudioDevice: couldn't get channel info");
        snd_pcm_close(handle);
        return false;
    }

    snd_pcm_channel_params_t params = QnxAudioUtils::formatToChannelParams(format, mode, info.max_fragment_size);
    const int errorCode = snd_pcm_plugin_params(handle, &params);
    snd_pcm_close(handle);

    return errorCode == 0;
}

QT_END_NAMESPACE
