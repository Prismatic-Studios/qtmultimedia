:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
::
:: Copyright (C) 2019 The Qt Company Ltd.
:: Contact: https://www.qt.io/licensing/
::
:: This file is part of the QtQuick module of the Qt Toolkit.
::
:: $QT_BEGIN_LICENSE:LGPL$
:: Commercial License Usage
:: Licensees holding valid commercial Qt licenses may use this file in
:: accordance with the commercial license agreement provided with the
:: Software or, alternatively, in accordance with the terms contained in
:: a written agreement between you and The Qt Company. For licensing terms
:: and conditions see https://www.qt.io/terms-conditions. For further
:: information use the contact form at https://www.qt.io/contact-us.
::
:: GNU Lesser General Public License Usage
:: Alternatively, this file may be used under the terms of the GNU Lesser
:: General Public License version 3 as published by the Free Software
:: Foundation and appearing in the file LICENSE.LGPL3 included in the
:: packaging of this file. Please review the following information to
:: ensure the GNU Lesser General Public License version 3 requirements
:: will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
::
:: GNU General Public License Usage
:: Alternatively, this file may be used under the terms of the GNU
:: General Public License version 2.0 or (at your option) the GNU General
:: Public license version 3 or any later version approved by the KDE Free
:: Qt Foundation. The licenses are as published by the Free Software
:: Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
:: included in the packaging of this file. Please review the following
:: information to ensure the GNU General Public License requirements will
:: be met: https://www.gnu.org/licenses/gpl-2.0.html and
:: https://www.gnu.org/licenses/gpl-3.0.html.
::
:: $QT_END_LICENSE$
::
:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

qsb -b --glsl "150,120,100 es" --hlsl 50 --msl 12 -o rgba.vert.qsb rgba.vert
qsb --glsl "150,120,100 es" --hlsl 50 --msl 12 -o rgba.frag.qsb rgba.frag

qsb -b --glsl "150,120,100 es" --hlsl 50 --msl 12 -o yuv.vert.qsb yuv.vert
qsb --glsl "150,120,100 es" --hlsl 50 --msl 12 -o yuv_yv.frag.qsb yuv_yv.frag

qsb --glsl "150,120,100 es" --hlsl 50 --msl 12 -o nv12.frag.qsb nv12.frag
qsb --glsl "150,120,100 es" --hlsl 50 --msl 12 -o nv21.frag.qsb nv21.frag
qsb --glsl "150,120,100 es" --hlsl 50 --msl 12 -o uyvy.frag.qsb uyvy.frag
qsb --glsl "150,120,100 es" --hlsl 50 --msl 12 -o yuyv.frag.qsb yuyv.frag
qsb --glsl "150,120,100 es" --hlsl 50 --msl 12 -o ayuv.frag.qsb ayuv.frag
qsb --glsl "150,120,100 es" --hlsl 50 --msl 12 -o p010be.frag.qsb p010be.frag
qsb --glsl "150,120,100 es" --hlsl 50 --msl 12 -o p010be.frag.qsb p010be.frag
