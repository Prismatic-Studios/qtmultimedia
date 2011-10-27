/****************************************************************************
**
** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QTMEDIANAMESPACE_H
#define QTMEDIANAMESPACE_H

#include <QtCore/qpair.h>
#include <QtCore/qmetatype.h>

#include <qtmultimediadefs.h>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Multimedia)

namespace QtMultimedia
{
    enum MetaData
    {
        // Common
        Title,
        SubTitle,
        Author,
        Comment,
        Description,
        Category,
        Genre,
        Year,
        Date,
        UserRating,
        Keywords,
        Language,
        Publisher,
        Copyright,
        ParentalRating,
        RatingOrganisation,

        // Media
        Size,
        MediaType,
        Duration,

        // Audio
        AudioBitRate,
        AudioCodec,
        AverageLevel,
        ChannelCount,
        PeakValue,
        SampleRate,

        // Music
        AlbumTitle,
        AlbumArtist,
        ContributingArtist,
        Composer,
        Conductor,
        Lyrics,
        Mood,
        TrackNumber,
        TrackCount,

        CoverArtUrlSmall,
        CoverArtUrlLarge,

        // Image/Video
        Resolution,
        PixelAspectRatio,

        // Video
        VideoFrameRate,
        VideoBitRate,
        VideoCodec,

        PosterUrl,

        // Movie
        ChapterNumber,
        Director,
        LeadPerformer,
        Writer,

        // Photos
        CameraManufacturer,
        CameraModel,
        Event,
        Subject,
        Orientation,
        ExposureTime,
        FNumber,
        ExposureProgram,
        ISOSpeedRatings,
        ExposureBiasValue,
        DateTimeOriginal,
        DateTimeDigitized,
        SubjectDistance,
        MeteringMode,
        LightSource,
        Flash,
        FocalLength,
        ExposureMode,
        WhiteBalance,
        DigitalZoomRatio,
        FocalLengthIn35mmFilm,
        SceneCaptureType,
        GainControl,
        Contrast,
        Saturation,
        Sharpness,
        DeviceSettingDescription,

        PosterImage,
        CoverArtImage,
        ThumbnailImage

    };

    enum SupportEstimate
    {
        NotSupported,
        MaybeSupported,
        ProbablySupported,
        PreferredService
    };

    enum EncodingQuality
    {
        VeryLowQuality,
        LowQuality,
        NormalQuality,
        HighQuality,
        VeryHighQuality
    };

    enum EncodingMode
    {
        ConstantQualityEncoding,
        ConstantBitRateEncoding,
        AverageBitRateEncoding,
        TwoPassEncoding
    };

    enum AvailabilityError
    {
        NoError,
        ServiceMissingError,
        BusyError,
        ResourceError
    };

}

QT_END_NAMESPACE

Q_DECLARE_METATYPE(QtMultimedia::AvailabilityError)
Q_DECLARE_METATYPE(QtMultimedia::SupportEstimate)
Q_DECLARE_METATYPE(QtMultimedia::MetaData)
Q_DECLARE_METATYPE(QtMultimedia::EncodingMode)
Q_DECLARE_METATYPE(QtMultimedia::EncodingQuality)

QT_END_HEADER


#endif
