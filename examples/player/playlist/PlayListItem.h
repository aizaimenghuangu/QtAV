/******************************************************************************
    PlayListItem.h: description
    Copyright (C) 2013 Wang Bin <wbsecg1@gmail.com>

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

    Alternatively, this file may be used under the terms of the GNU
    General Public License version 3.0 as published by the Free Software
    Foundation and appearing in the file LICENSE.GPL included in the
    packaging of this file.  Please review the following information to
    ensure the GNU General Public License version 3.0 requirements will be
    met: http://www.gnu.org/copyleft/gpl.html.
******************************************************************************/


#ifndef PLAYLISTITEM_H
#define PLAYLISTITEM_H

#include <QtCore/QString>
#include <QtCore/QVariant>

class PlayListItem
{
public:
    PlayListItem();
    void setTitle(const QString& title);
    QString title() const;
    void setUrl(const QString& url);
    QString url() const;
    void setStars(int s);
    int stars() const;
    void setLastTime(qint64 ms);
    qint64 lastTime() const;
    QString lastTimeString() const;
    void setDuration(qint64 ms);
    qint64 duration() const;
    QString durationString() const;
    //icon
private:
    QString mTitle;
    QString mUrl;
    int mStars;
    qint64 mLastTime, mDuration;
    QString mLastTimeS, mDurationS;
};

Q_DECLARE_METATYPE(PlayListItem);

QDataStream& operator>> (QDataStream& s, PlayListItem& p);
QDataStream& operator<< (QDataStream& s, const PlayListItem& p);

#endif // PLAYLISTITEM_H
