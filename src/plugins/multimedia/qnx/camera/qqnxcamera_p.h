/****************************************************************************
**
** Copyright (C) 2016 Research In Motion
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
#ifndef QQnxCamera_H
#define QQnxCamera_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <private/qplatformcamera_p.h>

#include <camera/camera_api.h>

QT_BEGIN_NAMESPACE

class QQnxMediaCaptureSession;

class QQnxCamera : public QPlatformCamera
{
    Q_OBJECT
public:
    explicit QQnxCamera(QCamera *parent);

    bool isActive() const override;
    void setActive(bool active) override;

    void setCamera(const QCameraDevice &camera) override;

//    bool setCameraFormat(const QCameraFormat &/*format*/) override;

    void setCaptureSession(QPlatformMediaCaptureSession *session) override;

    bool isFocusModeSupported(QCamera::FocusMode mode) const override;
    void setFocusMode(QCamera::FocusMode mode) override;

    void setCustomFocusPoint(const QPointF &point) override;

//    void setFocusDistance(float) override;

//    // smaller 0: zoom instantly, rate in power-of-two/sec
    void zoomTo(float /*newZoomFactor*/, float /*rate*/ = -1.) override;

//    void setExposureCompensation(float) override;
//    int isoSensitivity() const override;
//    void setManualIsoSensitivity(int) override;
//    void setManualExposureTime(float) override;
//    float exposureTime() const override;

    bool isWhiteBalanceModeSupported(QCamera::WhiteBalanceMode mode) const override;
    void setWhiteBalanceMode(QCamera::WhiteBalanceMode /*mode*/) override;
    void setColorTemperature(int /*temperature*/) override;

    camera_handle_t handle() const;

private:
    void updateCameraFeatures();
    void setColorTemperatureInternal(unsigned temp);

    QQnxMediaCaptureSession *m_session;

    QCameraDevice m_camera;
    camera_unit_t m_cameraUnit = CAMERA_UNIT_NONE;
    camera_handle_t m_handle = CAMERA_HANDLE_INVALID;
    uint minZoom = 1;
    uint maxZoom = 1;
    mutable bool whiteBalanceModesChecked = false;
    mutable bool continuousColorTemperatureSupported = false;
    mutable int minColorTemperature = 0;
    mutable int maxColorTemperature = 0;
    mutable QList<unsigned> manualColorTemperatureValues;
};

QT_END_NAMESPACE

#endif
