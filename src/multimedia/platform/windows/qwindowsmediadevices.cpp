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

#include "qwindowsmediadevices_p.h"
#include "qmediadevices.h"
#include "qcamerainfo_p.h"
#include "qvarlengtharray.h"

#include "private/qwindowsaudioinput_p.h"
#include "private/qwindowsaudiooutput_p.h"
#include "private/qwindowsaudiodeviceinfo_p.h"
#include "private/qwindowsmultimediautils_p.h"

#include <private/mftvideo_p.h>

#include <mmsystem.h>
#include <mmddk.h>
#include <mfapi.h>
#include <mfobjects.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#include <Mferror.h>
#include <mmdeviceapi.h>
#include "private/qwindowsaudioutils_p.h"

QT_BEGIN_NAMESPACE

class CMMNotificationClient : public IMMNotificationClient
{
    LONG m_cRef;
    QWindowsMediaDevices *m_windowsMediaDevices;
    IMMDeviceEnumerator* m_enumerator;
    QMap<QString, DWORD> m_deviceState;

public:
    CMMNotificationClient(QWindowsMediaDevices *windowsMediaDevices,
                          IMMDeviceEnumerator *enumerator,
                          QMap<QString, DWORD> &&deviceState) :
        m_cRef(1),
        m_windowsMediaDevices(windowsMediaDevices),
        m_enumerator(enumerator),
        m_deviceState(deviceState)
    {}

    virtual ~CMMNotificationClient() {}

    // IUnknown methods -- AddRef, Release, and QueryInterface
    ULONG STDMETHODCALLTYPE AddRef()
    {
        return InterlockedIncrement(&m_cRef);
    }

    ULONG STDMETHODCALLTYPE Release()
    {
        ULONG ulRef = InterlockedDecrement(&m_cRef);
        if (0 == ulRef) {
            delete this;
        }
        return ulRef;
    }

    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, VOID **ppvInterface)
    {
        if (IID_IUnknown == riid) {
            AddRef();
            *ppvInterface = (IUnknown*)this;
        } else if (__uuidof(IMMNotificationClient) == riid) {
            AddRef();
            *ppvInterface = (IMMNotificationClient*)this;
        } else {
            *ppvInterface = NULL;
            return E_NOINTERFACE;
        }
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE OnDefaultDeviceChanged(EDataFlow, ERole, LPCWSTR)
    {
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE OnDeviceAdded(LPCWSTR deviceID)
    {
        auto it = m_deviceState.find(QString::fromWCharArray(deviceID));
        if (it == std::end(m_deviceState)) {
            m_deviceState.insert(QString::fromWCharArray(deviceID), DEVICE_STATE_ACTIVE);
            emitAudioDevicesChanged(deviceID);
        }

        return S_OK;
    };

    HRESULT STDMETHODCALLTYPE OnDeviceRemoved(LPCWSTR deviceID)
    {
        auto key = QString::fromWCharArray(deviceID);
        auto it = m_deviceState.find(key);
        if (it != std::end(m_deviceState)) {
            if (it.value() == DEVICE_STATE_ACTIVE)
                emitAudioDevicesChanged(deviceID);
            m_deviceState.remove(key);
        }

        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE OnDeviceStateChanged(LPCWSTR deviceID, DWORD newState)
    {
        if (auto it = m_deviceState.find(QString::fromWCharArray(deviceID)); it != std::end(m_deviceState)) {
            // If either the old state or the new state is active emit device change
            if ((it.value() == DEVICE_STATE_ACTIVE) != (newState == DEVICE_STATE_ACTIVE)) {
                emitAudioDevicesChanged(deviceID);
            }
            it.value() = newState;
        }

        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE OnPropertyValueChanged(LPCWSTR, const PROPERTYKEY)
    {
        return S_OK;
    }

    void emitAudioDevicesChanged(EDataFlow flow)
    {
        // windowsMediaDevice may be deleted as we are executing the callback
        if (flow == EDataFlow::eCapture) {
            m_windowsMediaDevices->audioInputsChanged();
        } else if (flow == EDataFlow::eRender) {
            m_windowsMediaDevices->audioOutputsChanged();
        }
    }

    void emitAudioDevicesChanged(LPCWSTR deviceID)
    {
        IMMDevice* device = nullptr;
        IMMEndpoint* endpoint = nullptr;
        EDataFlow flow;

        if (SUCCEEDED(m_enumerator->GetDevice(deviceID, &device))) {
            if (SUCCEEDED(device->QueryInterface(__uuidof(IMMEndpoint), (void**)&endpoint))) {
                if (SUCCEEDED(endpoint->GetDataFlow(&flow))) {
                    emitAudioDevicesChanged(flow);
                }
                endpoint->Release();
            }
            device->Release();
        }
    }
};


QWindowsMediaDevices::QWindowsMediaDevices()
    : QPlatformMediaDevices(),
      m_deviceEnumerator(nullptr),
      m_notificationClient(nullptr)

{
    CoInitialize(nullptr);

    auto hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL,
                CLSCTX_INPROC_SERVER,__uuidof(IMMDeviceEnumerator),
                (void**)&m_deviceEnumerator);

    if (SUCCEEDED(hr)) {
        QMap<QString, DWORD> devState;
        IMMDeviceCollection* devColl;

        if (SUCCEEDED(m_deviceEnumerator->EnumAudioEndpoints(EDataFlow::eAll, DEVICE_STATEMASK_ALL, &devColl))) {
            UINT count = 0;
            devColl->GetCount(&count);
            for (UINT i = 0; i < count; i++) {
                IMMDevice* device;

                if (SUCCEEDED(devColl->Item(i, &device))) {
                    DWORD state = 0;
                    LPWSTR id = nullptr;
                    device->GetState(&state);
                    device->GetId(&id);

                    devState.insert(QString::fromWCharArray(id), state);

                    device->Release();
                }
            }
            devColl->Release();
        }

        m_notificationClient = new CMMNotificationClient(this, m_deviceEnumerator, std::move(devState));
        m_deviceEnumerator->RegisterEndpointNotificationCallback(m_notificationClient);

    } else {
        qWarning() << "Audio device change notification disabled";
    }
}

QWindowsMediaDevices::~QWindowsMediaDevices()
{
    if (m_deviceEnumerator) {
        m_deviceEnumerator->UnregisterEndpointNotificationCallback(m_notificationClient);
        m_deviceEnumerator->Release();
    }

    if (m_notificationClient) {
        m_notificationClient->Release();
    }

    CoUninitialize();
}

static QList<QAudioDeviceInfo> availableDevices(QAudio::Mode mode)
{
    Q_UNUSED(mode);

    QList<QAudioDeviceInfo> devices;
    //enumerate device fullnames through directshow api
    auto hrCoInit = CoInitialize(nullptr);
    ICreateDevEnum *pDevEnum = NULL;
    IEnumMoniker *pEnum = NULL;
    // Create the System device enumerator
    HRESULT hr = CoCreateInstance(CLSID_SystemDeviceEnum, NULL,
                 CLSCTX_INPROC_SERVER, IID_ICreateDevEnum,
                 reinterpret_cast<void **>(&pDevEnum));

    unsigned long iNumDevs = mode == QAudio::AudioOutput ? waveOutGetNumDevs() : waveInGetNumDevs();
    if (SUCCEEDED(hr)) {
        // Create the enumerator for the audio input/output category
        if (pDevEnum->CreateClassEnumerator(
             mode == QAudio::AudioOutput ? CLSID_AudioRendererCategory : CLSID_AudioInputDeviceCategory,
             &pEnum, 0) == S_OK) {
            pEnum->Reset();
            // go through and find all audio devices
            IMoniker *pMoniker = NULL;
            while (pEnum->Next(1, &pMoniker, NULL) == S_OK) {
                IPropertyBag *pPropBag;
                hr = pMoniker->BindToStorage(0,0,IID_IPropertyBag,
                     reinterpret_cast<void **>(&pPropBag));
                if (FAILED(hr)) {
                    pMoniker->Release();
                    continue; // skip this one
                }
                // Find if it is a wave device
                VARIANT var;
                VariantInit(&var);
                hr = pPropBag->Read(mode == QAudio::AudioOutput ? L"WaveOutID" : L"WaveInID", &var, 0);
                if (SUCCEEDED(hr)) {
                    LONG waveID = var.lVal;
                    if (waveID >= 0 && waveID < LONG(iNumDevs)) {
                        VariantClear(&var);
                        // Find the description
                        hr = pPropBag->Read(L"FriendlyName", &var, 0);
                        if (!SUCCEEDED(hr)) {
                            pPropBag->Release();
                            continue;
                        }

                        QString description = QString::fromWCharArray(var.bstrVal);

                        // Get the endpoint ID string for this waveOut device. This is required to be able to
                        // identify the device use the WMF APIs
                        auto wave = IntToPtr(waveID);
                        auto waveMessage = [wave, mode](UINT msg, auto p0, auto p1) {
                            return mode == QAudio::AudioOutput
                                    ? waveOutMessage((HWAVEOUT)wave, msg, (DWORD_PTR)p0, (DWORD_PTR)p1)
                                    : waveInMessage((HWAVEIN)wave, msg, (DWORD_PTR)p0, (DWORD_PTR)p1);
                        };

                        size_t len = 0;
                        if (waveMessage(DRV_QUERYFUNCTIONINSTANCEIDSIZE, &len, 0) == MMSYSERR_NOERROR) {
                            QVarLengthArray<WCHAR> id(len);
                            if (waveMessage(DRV_QUERYFUNCTIONINSTANCEID, id.data(), len) == MMSYSERR_NOERROR) {
                                auto strID = QString::fromWCharArray(id.data()).toUtf8();

                                devices.append((new QWindowsAudioDeviceInfo(strID, waveID, description, mode))->create());
                            }
                        }
                    }
                }
                pPropBag->Release();
                pMoniker->Release();
            }
            pEnum->Release();
        }
        pDevEnum->Release();
    }
    if (SUCCEEDED(hrCoInit))
        CoUninitialize();

    return devices;
}

QList<QAudioDeviceInfo> QWindowsMediaDevices::audioInputs() const
{
    return availableDevices(QAudio::AudioInput);
}

QList<QAudioDeviceInfo> QWindowsMediaDevices::audioOutputs() const
{
    return availableDevices(QAudio::AudioOutput);
}

QList<QCameraInfo> QWindowsMediaDevices::videoInputs() const
{
    QList<QCameraInfo> cameras;
    auto hrCoInit = CoInitialize(nullptr);

    IMFAttributes *pAttributes = NULL;
    IMFActivate **ppDevices = NULL;

    // Create an attribute store to specify the enumeration parameters.
    HRESULT hr = MFCreateAttributes(&pAttributes, 1);
    if (SUCCEEDED(hr)) {
        // Source type: video capture devices
        hr = pAttributes->SetGUID(MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE,
                                  MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_GUID);

        if (SUCCEEDED(hr)) {
            // Enumerate devices.
            UINT32 count;
            hr = MFEnumDeviceSources(pAttributes, &ppDevices, &count);
            if (SUCCEEDED(hr)) {
                // Iterate through devices.
                for (int index = 0; index < int(count); index++) {
                    QCameraInfoPrivate *info = new QCameraInfoPrivate;

                    IMFMediaSource *pSource = NULL;
                    IMFSourceReader *reader = NULL;

                    WCHAR *deviceName = NULL;
                    UINT32 deviceNameLength = 0;
                    UINT32 deviceIdLength = 0;
                    WCHAR *deviceId = NULL;

                    hr = ppDevices[index]->GetAllocatedString(MF_DEVSOURCE_ATTRIBUTE_FRIENDLY_NAME,
                                                              &deviceName, &deviceNameLength);
                    if (SUCCEEDED(hr))
                        info->description = QString::fromWCharArray(deviceName);
                    CoTaskMemFree(deviceName);

                    hr = ppDevices[index]->GetAllocatedString(
                            MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_SYMBOLIC_LINK, &deviceId,
                            &deviceIdLength);
                    if (SUCCEEDED(hr))
                        info->id = QString::fromWCharArray(deviceId).toUtf8();
                    CoTaskMemFree(deviceId);

                    // Create the media source object.
                    hr = ppDevices[index]->ActivateObject(
                            IID_PPV_ARGS(&pSource));
                    // Create the media source reader.
                    hr = MFCreateSourceReaderFromMediaSource(pSource, NULL, &reader);
                    if (SUCCEEDED(hr)) {
                        QList<QSize> photoResolutions;
                        QList<QCameraFormat> videoFormats;

                        DWORD dwMediaTypeIndex = 0;
                        IMFMediaType *mediaFormat = NULL;
                        GUID subtype = GUID_NULL;
                        HRESULT mediaFormatResult = S_OK;

                        UINT32 frameRateMin = 0u;
                        UINT32 frameRateMax = 0u;
                        UINT32 denominator = 0u;
                        DWORD index = 0u;
                        UINT32 width = 0u;
                        UINT32 height = 0u;

                        while (SUCCEEDED(mediaFormatResult)) {
                            // Loop through the supported formats for the video device
                            mediaFormatResult = reader->GetNativeMediaType(
                                    (DWORD)MF_SOURCE_READER_FIRST_VIDEO_STREAM, dwMediaTypeIndex,
                                    &mediaFormat);
                            if (mediaFormatResult == MF_E_NO_MORE_TYPES)
                                break;
                            else if (SUCCEEDED(mediaFormatResult)) {
                                QVideoFrameFormat::PixelFormat pixelFormat = QVideoFrameFormat::Format_Invalid;
                                QSize resolution;
                                float minFr = .0;
                                float maxFr = .0;

                                if (SUCCEEDED(mediaFormat->GetGUID(MF_MT_SUBTYPE, &subtype)))
                                    pixelFormat = QWindowsMultimediaUtils::pixelFormatFromMediaSubtype(subtype);

                                if (SUCCEEDED(MFGetAttributeSize(mediaFormat, MF_MT_FRAME_SIZE, &width,
                                                        &height))) {
                                    resolution.rheight() = (int)height;
                                    resolution.rwidth() = (int)width;
                                    photoResolutions << resolution;
                                }

                                if (SUCCEEDED(MFGetAttributeRatio(mediaFormat, MF_MT_FRAME_RATE_RANGE_MIN,
                                                         &frameRateMin, &denominator)))
                                    minFr = qreal(frameRateMin) / denominator;
                                if (SUCCEEDED(MFGetAttributeRatio(mediaFormat, MF_MT_FRAME_RATE_RANGE_MAX,
                                                         &frameRateMax, &denominator)))
                                    maxFr = qreal(frameRateMax) / denominator;

                                auto *f = new QCameraFormatPrivate { QSharedData(), pixelFormat,
                                                                     resolution, minFr, maxFr };
                                videoFormats << f->create();
                            }
                            ++dwMediaTypeIndex;
                        }
                        if (mediaFormat)
                            mediaFormat->Release();

                        info->videoFormats = videoFormats;
                        info->photoResolutions = photoResolutions;
                    }
                    if (reader)
                        reader->Release();
                    cameras.append(info->create());
                }
            }
            for (DWORD i = 0; i < count; i++) {
                if (ppDevices[i])
                    ppDevices[i]->Release();
            }
            CoTaskMemFree(ppDevices);
        }
    }
    if (pAttributes)
        pAttributes->Release();
    if (SUCCEEDED(hrCoInit))
        CoUninitialize();

    return cameras;
}

QAbstractAudioInput *QWindowsMediaDevices::createAudioInputDevice(const QAudioDeviceInfo &deviceInfo)
{
    const auto *devInfo = static_cast<const QWindowsAudioDeviceInfo *>(deviceInfo.handle());
    return new QWindowsAudioInput(devInfo->waveId());
}

QAbstractAudioOutput *QWindowsMediaDevices::createAudioOutputDevice(const QAudioDeviceInfo &deviceInfo)
{
    const auto *devInfo = static_cast<const QWindowsAudioDeviceInfo *>(deviceInfo.handle());
    return new QWindowsAudioOutput(devInfo->waveId());
}

QT_END_NAMESPACE