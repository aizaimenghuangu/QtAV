/******************************************************************************
    VideoRendererTypes: type id and manually id register function
    Copyright (C) 2013 Wang Bin <wbsecg1@gmail.com>

*   This file is part of QtAV

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
******************************************************************************/

#ifndef QTAV_VIDEODECODERTYPES_H
#define QTAV_VIDEODECODERTYPES_H

#include <QtAV/VideoDecoder.h>

namespace QtAV {

extern Q_EXPORT VideoDecoderId VideoDecoderId_FFmpeg;
extern Q_EXPORT VideoDecoderId VideoDecoderId_FFmpeg_DXVA;
extern Q_EXPORT VideoDecoderId VideoDecoderId_FFmpeg_VAAPI;
extern Q_EXPORT VideoDecoderId VideoDecoderId_FFmpeg_VDPAU;
extern Q_EXPORT VideoDecoderId VideoDecoderId_FFmpeg_VDA;
extern Q_EXPORT VideoDecoderId VideoDecoderId_CUDA;


Q_EXPORT void VideoDecoder_RegisterAll();

} //namespace QtAV
#endif // QTAV_VIDEODECODERTYPES_H