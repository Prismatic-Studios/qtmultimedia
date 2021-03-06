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
#ifndef QFFMPEGDECODER_P_H
#define QFFMPEGDECODER_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API. It exists purely as an
// implementation detail. This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include "qffmpegthread_p.h"
#include "qffmpeg_p.h"
#include "qffmpegmediaplayer_p.h"
#include "qffmpeghwaccel_p.h"
#include "qffmpegclock_p.h"

#include <qshareddata.h>
#include <qtimer.h>
#include <qqueue.h>

QT_BEGIN_NAMESPACE

class QAudioSink;

namespace QFFmpeg
{

// queue up max 16M of encoded data, that should always be enough
// (it's around 2 secs of 4K HDR video, longer for almost all other formats)
enum { MaxQueueSize = 16*1024*1024 };

inline qint64 timeStamp(qint64 ts, AVRational base)
{
    return (1000*ts*base.num + 500)/base.den;
}

struct Packet
{
    struct Data {
        Data(AVPacket *p)
            : packet(p)
        {}
        ~Data() {
            if (packet)
                av_packet_free(&packet);
        }
        QAtomicInt ref;
        AVPacket *packet = nullptr;
    };
    Packet() = default;
    Packet(AVPacket *p)
        : d(new Data(p))
    {}

    bool isValid() const { return !!d; }
    AVPacket *avPacket() const { return d->packet; }
private:
    QExplicitlySharedDataPointer<Data> d;
};

struct Codec
{
    struct Data {
        Data(AVCodecContext *context, AVStream *stream, const QFFmpeg::HWAccel &hwAccel);
        ~Data();
        QAtomicInt ref;
        AVCodecContext *context = nullptr;
        AVStream *stream = nullptr;
        QFFmpeg::HWAccel hwAccel;
        int streamIndex = -1;
    };

    Codec() = default;
    Codec(AVFormatContext *format, int streamIndex);
    bool isValid() const { return !!d; }

    AVCodecContext *context() const { return d->context; }
    AVStream *stream() const { return d->stream; }
    uint streamIndex() const { return d->stream->index; }
    HWAccel hwAccel() const { return d->hwAccel; }
    qint64 toMs(qint64 ts) const { return timeStamp(ts, d->stream->time_base); }

private:
    QExplicitlySharedDataPointer<Data> d;
};


struct Frame
{
    struct Data {
        Data(AVFrame *f, const Codec &codec, qint64 pts)
            : codec(codec)
            , frame(f)
            , pts(pts)
        {}
        Data(const QString &text, qint64 pts, qint64 duration)
            : text(text), pts(pts), duration(duration)
        {}
        ~Data() {
            if (frame)
                av_frame_unref(frame);
        }
        QAtomicInt ref;
        Codec codec;
        AVFrame *frame = nullptr;
        QString text;
        qint64 pts = -1;
        qint64 duration = -1;
    };
    Frame() = default;
    Frame(AVFrame *f, const Codec &codec, qint64 pts)
        : d(new Data(f, codec, pts))
    {}
    Frame(const QString &text, qint64 pts, qint64 duration)
        : d(new Data(text, pts, duration))
    {}
    bool isValid() const { return !!d; }

    AVFrame *avFrame() const { return d->frame; }
    AVFrame *takeAVFrame() const {
        AVFrame *f = d->frame;
        d->frame = nullptr;
        return f;
    }
    const Codec *codec() const { return &d->codec; }
    qint64 pts() const { return d->pts; }
    qint64 duration() const { return d->duration; }
    qint64 end() const { return d->pts + d->duration; }
    QString text() const { return d->text; }
private:
    QExplicitlySharedDataPointer<Data> d;
};

class Demuxer;
class StreamDecoder;
class Renderer;
class AudioRenderer;
class VideoRenderer;

class Decoder : public QObject
{
    Q_OBJECT
public:
    Decoder();
    ~Decoder();

    void setUrl(const QUrl &media);

    void init();
    void play() {
        setPaused(false);
    }
    void pause() {
        setPaused(true);
    }
    void stop();
    void setPaused(bool b);
    void triggerStep();

    void setVideoSink(QVideoSink *sink);
    void setAudioSink(QPlatformAudioOutput *output);

    void changeAVTrack(QPlatformMediaPlayer::TrackType type, int index);

    void seek(qint64 pos);
    void setPlaybackRate(float rate);

    void setMediaPlayer(QFFmpegMediaPlayer *p) { player = p; }

    void checkStreams();

    int activeTrack(QPlatformMediaPlayer::TrackType type);
    void setActiveTrack(QPlatformMediaPlayer::TrackType type, int streamNumber);

    bool isSeekable() const
    {
        Q_ASSERT(context);
        return !(context->ctx_flags & AVFMTCTX_UNSEEKABLE);
    }

public Q_SLOTS:
    void updateCurrentTime(qint64 time);

public:
    QFFmpegMediaPlayer *player = nullptr;

    bool paused = true;

    Demuxer *demuxer = nullptr;

    AVFormatContext *context = nullptr;
    int m_currentAVStreamIndex[QPlatformMediaPlayer::NTrackTypes] = { -1, -1, -1 };

    QVideoSink *videoSink = nullptr;
    Renderer *videoRenderer = nullptr;

    QPlatformAudioOutput *audioOutput = nullptr;
    Renderer *audioRenderer = nullptr;

    ClockController clockController;
    bool playing = false;

    struct StreamInfo {
        int avStreamIndex = -1;
        bool isDefault = false;
        QMediaMetaData metaData;
    };

    QList<StreamInfo> m_streamMap[QPlatformMediaPlayer::NTrackTypes];
    int m_requestedStreams[3] = { -1, -1, -1 };
    qint64 m_duration = 0;
    QMediaMetaData m_metaData;
};

class Demuxer : public Thread
{
    Q_OBJECT
public:
    Demuxer(Decoder *decoder);

    StreamDecoder *addStream(int streamIndex);
    void removeStream(int streamIndex);

    bool isStopped() const
    {
        return m_isStopped.loadRelaxed();
    }
    void startDecoding()
    {
        m_isStopped.storeRelaxed(false);
        updateEnabledStreams();
        wake();
    }
    void stopDecoding();

    int seek(qint64 pos);

Q_SIGNALS:
    void atEnd();

private:
    void updateEnabledStreams();
    void sendFinalPacketToStreams();

    void init() override;
    void cleanup() override;
    bool shouldWait() const override;
    void loop() override;

    Decoder *decoder;
    QList<StreamDecoder *> streamDecoders;

    QAtomicInteger<bool> m_isStopped = true;
    qint64 last_pts = -1;
};


class StreamDecoder : public Thread
{
    Q_OBJECT
protected:
    Decoder *decoder = nullptr;
    Renderer *m_renderer = nullptr;

    struct PacketQueue {
        mutable QMutex mutex;
        QQueue<Packet> queue;
        qint64 size = 0;
        qint64 duration = 0;
    };
    PacketQueue packetQueue;

    struct FrameQueue {
        mutable QMutex mutex;
        QQueue<Frame> queue;
        int maxSize = 3;
    };
    FrameQueue frameQueue;

public:
    StreamDecoder(Decoder *decoder, const Codec &codec);

    void addPacket(AVPacket *packet);

    qint64 queuedPacketSize() const {
        QMutexLocker locker(&packetQueue.mutex);
        return packetQueue.size;
    }
    qint64 queuedDuration() const {
        QMutexLocker locker(&packetQueue.mutex);
        return packetQueue.duration;
    }

    const Frame *lockAndPeekFrame()
    {
        frameQueue.mutex.lock();
        return frameQueue.queue.isEmpty() ? nullptr : &frameQueue.queue.first();
    }
    void removePeekedFrame()
    {
        frameQueue.queue.takeFirst();
        wake();
    }
    void unlockAndReleaseFrame()
    {
        frameQueue.mutex.unlock();
    }
    Frame takeFrame();

    void flush();

    Codec codec;

    void setRenderer(Renderer *r);
    Renderer *renderer() const { return m_renderer; }

private:
    Packet takePacket();
    void addFrame(const Frame &f);

    bool hasEnoughFrames() const
    {
        QMutexLocker locker(&frameQueue.mutex);
        return frameQueue.queue.size() >= frameQueue.maxSize;
    }
    bool hasNoPackets() const
    {
        QMutexLocker locker(&packetQueue.mutex);
        return packetQueue.queue.isEmpty();
    }

    void init() override;
    bool shouldWait() const override;
    void loop() override;

    void decode();
    void decodeSubtitle();

    QPlatformMediaPlayer::TrackType type() const;
};

class Renderer : public Thread
{
    Q_OBJECT
protected:
    Decoder *decoder = nullptr;
    QPlatformMediaPlayer::TrackType type;

    mutable bool step = false;
    bool paused = true;
    StreamDecoder *streamDecoder = nullptr;

public:
    Renderer(Decoder *decoder, QPlatformMediaPlayer::TrackType type);

    void pause() {
        QMutexLocker locker(&mutex);
        paused = true;
    }
    void unPause() {
        QMutexLocker locker(&mutex);
        paused = false;
        condition.wakeAll();
        if (streamDecoder)
            streamDecoder->condition.wakeAll();
    }
    void singleStep() {
        QMutexLocker locker(&mutex);
        if (!paused)
            return;
        step = true;
        condition.wakeAll();
    }
    void doneStep() {
        step = false;
    }

    void setStream(StreamDecoder *stream);
    virtual void setSubtitleStream(StreamDecoder *) {}

    void kill() override;

    virtual void streamChanged() {}

protected:
    bool shouldWait() const override;

public:
};

class ClockedRenderer : public Renderer, public Clock
{
public:
    ClockedRenderer(Decoder *decoder, QPlatformMediaPlayer::TrackType type)
        : Renderer(decoder, type)
    {
        decoder->clockController.addClock(this);
    }
    ~ClockedRenderer()
    {
        decoder->clockController.addClock(this);
    }
    void setPaused(bool paused) override;
    void kill() override;
};

class VideoRenderer : public ClockedRenderer
{
    Q_OBJECT

    StreamDecoder *subtitleStreamDecoder = nullptr;
public:
    VideoRenderer(Decoder *decoder, QVideoSink *sink);

    void kill() override;

    void setSubtitleStream(StreamDecoder *stream) override;
private:

    void init() override;
    void loop() override;

    QVideoSink *sink;
};

class AudioRenderer : public ClockedRenderer
{
    Q_OBJECT
public:
    AudioRenderer(Decoder *decoder, QAudioOutput *output);
    ~AudioRenderer() = default;

    // Clock interface
    void syncTo(qint64 usecs) override;
    void adjustBy(qint64 usecs) override;
    void setPlaybackRate(float rate) override;

private slots:
    void updateAudio();

private:
    void updateOutput(const Codec *codec);
    void freeOutput();

    void init() override;
    void cleanup() override;
    void loop() override;
    void streamChanged() override;
    Type type() const override { return AudioClock; }

    int outputSamples(int inputSamples) {
        return qRound(inputSamples/playbackRate());
    }

    // Used for timing update calculations based on processed data
    qint64 audioBaseTime = 0;
    qint64 processedBase = 0;
    qint64 processedUSecs = 0;

    bool deviceChanged = false;
    QAudioOutput *output = nullptr;
    bool audioMuted = false;
    qint64 writtenUSecs = 0;
    qint64 latencyUSecs = 0;

    QAudioFormat format;
    QAudioSink *audioSink = nullptr;
    QIODevice *audioDevice = nullptr;
    SwrContext *resampler = nullptr;
    QByteArray bufferedData;
    qsizetype bufferWritten = 0;
};

}

QT_END_NAMESPACE

#endif

