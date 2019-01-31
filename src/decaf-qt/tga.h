/* This file is part of the KDE project
   Copyright (C) 2003 Dominik Seichter <domseichter@web.de>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the Lesser GNU General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.
*/

#ifndef KIMG_TGA_H
#define KIMG_TGA_H

#include <QtGui/QImageIOPlugin>

class TGAHandler : public QImageIOHandler
{
public:
    TGAHandler();

    bool canRead() const;
    static bool read(QDataStream &s, QImage *image);
	bool read(QImage *image);
    bool write(const QImage &image);

    QByteArray name() const;

    static bool canRead(QIODevice *device);
};

#endif
