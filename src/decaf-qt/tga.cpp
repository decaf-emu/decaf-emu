/* This file is part of the KDE project
   Copyright (C) 2003 Dominik Seichter <domseichter@web.de>
   Copyright (C) 2004 Ignacio Castaño <castano@ludicon.com>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the Lesser GNU General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.
*/

/* this code supports:
 * reading:
 *     uncompressed and run length encoded indexed, grey and color tga files.
 *     image types 1, 2, 3, 9, 10 and 11.
 *     only RGB color maps with no more than 256 colors.
 *     pixel formats 8, 15, 24 and 32.
 * writing:
 *     uncompressed true color tga files
 */

#include "tga.h"

#include <assert.h>

#include <QtGui/QImage>
#include <QtCore/QDataStream>


typedef quint32 uint;
typedef quint16 ushort;
typedef quint8 uchar;

namespace { // Private.

    // Header format of saved files.
    uchar targaMagic[12] = { 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

    enum TGAType {
        TGA_TYPE_INDEXED        = 1,
        TGA_TYPE_RGB            = 2,
        TGA_TYPE_GREY           = 3,
        TGA_TYPE_RLE_INDEXED    = 9,
        TGA_TYPE_RLE_RGB        = 10,
        TGA_TYPE_RLE_GREY       = 11
    };

#define TGA_INTERLEAVE_MASK 0xc0
#define TGA_INTERLEAVE_NONE 0x00
#define TGA_INTERLEAVE_2WAY 0x40
#define TGA_INTERLEAVE_4WAY 0x80

#define TGA_ORIGIN_MASK     0x30
#define TGA_ORIGIN_LEFT     0x00
#define TGA_ORIGIN_RIGHT    0x10
#define TGA_ORIGIN_LOWER    0x00
#define TGA_ORIGIN_UPPER    0x20

    struct TgaHeader {
        uchar id_length;
        uchar colormap_type;
        uchar image_type;
        ushort colormap_index;
        ushort colormap_length;
        uchar colormap_size;
        ushort x_origin;
        ushort y_origin;
        ushort width;
        ushort height;
        uchar pixel_size;
        uchar flags;

        enum { SIZE = 18 }; // const static int SIZE = 18;
    };

    static QDataStream & operator>> ( QDataStream & s, TgaHeader & head )
    {
        s >> head.id_length;
        s >> head.colormap_type;
        s >> head.image_type;
        s >> head.colormap_index;
        s >> head.colormap_length;
        s >> head.colormap_size;
        s >> head.x_origin;
        s >> head.y_origin;
        s >> head.width;
        s >> head.height;
        s >> head.pixel_size;
        s >> head.flags;
        /*qDebug() << "id_length: " << head.id_length << " - colormap_type: " << head.colormap_type << " - image_type: " << head.image_type;
        qDebug() << "colormap_index: " << head.colormap_index << " - colormap_length: " << head.colormap_length << " - colormap_size: " << head.colormap_size;
        qDebug() << "x_origin: " << head.x_origin << " - y_origin: " << head.y_origin << " - width:" << head.width << " - height:" << head.height << " - pixelsize: " << head.pixel_size << " - flags: " << head.flags;*/
        return s;
    }

    static bool IsSupported( const TgaHeader & head )
    {
        if( head.image_type != TGA_TYPE_INDEXED &&
            head.image_type != TGA_TYPE_RGB &&
            head.image_type != TGA_TYPE_GREY &&
            head.image_type != TGA_TYPE_RLE_INDEXED &&
            head.image_type != TGA_TYPE_RLE_RGB &&
            head.image_type != TGA_TYPE_RLE_GREY )
        {
            return false;
        }
        if( head.image_type == TGA_TYPE_INDEXED ||
            head.image_type == TGA_TYPE_RLE_INDEXED )
        {
            if( head.colormap_length > 256 || head.colormap_size != 24 || head.colormap_type != 1 )
            {
                return false;
            }
        }
        if( head.image_type == TGA_TYPE_RGB ||
            head.image_type == TGA_TYPE_GREY ||
            head.image_type == TGA_TYPE_RLE_RGB ||
            head.image_type == TGA_TYPE_RLE_GREY )
        {
            if( head.colormap_type != 0 )
            {
                return false;
            }
        }
        if( head.width == 0 || head.height == 0 )
        {
            return false;
        }
        if( head.pixel_size != 8 && head.pixel_size != 16 &&
            head.pixel_size != 24 && head.pixel_size != 32 )
        {
            return false;
        }
        return true;
    }

    struct Color555 {
        ushort b : 5;
        ushort g : 5;
        ushort r : 5;
    };

    struct TgaHeaderInfo {
        bool rle;
        bool pal;
        bool rgb;
        bool grey;

        TgaHeaderInfo( const TgaHeader & tga ) : rle(false), pal(false), rgb(false), grey(false)
        {
            switch( tga.image_type ) {
                case TGA_TYPE_RLE_INDEXED:
                    rle = true;
                    // no break is intended!
                case TGA_TYPE_INDEXED:
                    pal = true;
                    break;

                case TGA_TYPE_RLE_RGB:
                    rle = true;
                    // no break is intended!
                case TGA_TYPE_RGB:
                    rgb = true;
                    break;

                case TGA_TYPE_RLE_GREY:
                    rle = true;
                    // no break is intended!
                case TGA_TYPE_GREY:
                    grey = true;
                    break;

                default:
                    // Error, unknown image type.
                    break;
            }
        }
    };



    static bool LoadTGA( QDataStream & s, const TgaHeader & tga, QImage &img )
    {
        // Create image.
        img = QImage( tga.width, tga.height, QImage::Format_RGB32 );

        TgaHeaderInfo info(tga);

                // Bits 0-3 are the numbers of alpha bits (can be zero!)
                const int numAlphaBits = tga.flags & 0xf;
                // However alpha exists only in the 32 bit format.
        if( ( tga.pixel_size == 32 ) && ( tga.flags & 0xf ) ) {
            img = QImage( tga.width, tga.height, QImage::Format_ARGB32 );
        }

        uint pixel_size = (tga.pixel_size/8);
        uint size = tga.width * tga.height * pixel_size;

        if (size < 1)
        {
            //kDebug(399) << "This TGA file is broken with size " << size;
            return false;
        }

        // Read palette.
        char palette[768];
        if( info.pal ) {
            // @todo Support palettes in other formats!
            s.readRawData( palette, 3 * tga.colormap_length );
        }

        // Allocate image.
        uchar * const image = new uchar[size];

        if( info.rle ) {
            // Decode image.
            char * dst = (char *)image;
            int num = size;

            while (num > 0) {
                // Get packet header.
                uchar c;
                s >> c;

                uint count = (c & 0x7f) + 1;
                num -= count * pixel_size;

                if (c & 0x80) {
                    // RLE pixels.
                                        assert(pixel_size <= 8);
                    char pixel[8];
                    s.readRawData( pixel, pixel_size );
                    do {
                        memcpy(dst, pixel, pixel_size);
                        dst += pixel_size;
                    } while (--count);
                }
                else {
                    // Raw pixels.
                    count *= pixel_size;
                    s.readRawData( dst, count );
                    dst += count;
                }
            }
        }
        else {
            // Read raw image.
            s.readRawData( (char *)image, size );
        }

        // Convert image to internal format.
        int y_start, y_step, y_end;
        if( tga.flags & TGA_ORIGIN_UPPER ) {
            y_start = 0;
            y_step = 1;
            y_end = tga.height;
        }
        else {
            y_start = tga.height - 1;
            y_step = -1;
            y_end = -1;
        }

        uchar * src = image;

        for( int y = y_start; y != y_end; y += y_step ) {
            QRgb * scanline = (QRgb *) img.scanLine( y );

            if( info.pal ) {
                // Paletted.
                for( int x = 0; x < tga.width; x++ ) {
                    uchar idx = *src++;
                    scanline[x] = qRgb( palette[3*idx+2], palette[3*idx+1], palette[3*idx+0] );
                }
            }
            else if( info.grey ) {
                // Greyscale.
                for( int x = 0; x < tga.width; x++ ) {
                    scanline[x] = qRgb( *src, *src, *src );
                    src++;
                }
            }
            else {
                // True Color.
                if( tga.pixel_size == 16 ) {
                    for( int x = 0; x < tga.width; x++ ) {
                        Color555 c = *reinterpret_cast<Color555 *>(src);
                        scanline[x] = qRgb( (c.r << 3) | (c.r >> 2), (c.g << 3) | (c.g >> 2), (c.b << 3) | (c.b >> 2) );
                        src += 2;
                    }
                }
                else if( tga.pixel_size == 24 ) {
                    for( int x = 0; x < tga.width; x++ ) {
                        scanline[x] = qRgb( src[2], src[1], src[0] );
                        src += 3;
                    }
                }
                else if( tga.pixel_size == 32 ) {
                    for( int x = 0; x < tga.width; x++ ) {
                                                // ### TODO: verify with images having really some alpha data
                                                const uchar alpha = ( src[3] << ( 8 - numAlphaBits ) );
                        scanline[x] = qRgba( src[2], src[1], src[0], alpha );
                        src += 4;
                    }
                }
            }
        }

        // Free image.
        delete [] image;

        return true;
    }

} // namespace


TGAHandler::TGAHandler()
{
}

bool TGAHandler::canRead() const
{
    if (canRead(device())) {
        setFormat("tga");
        return true;
    }
    return false;
}

bool TGAHandler::read(QImage *outImage)
{
   //kDebug(399) << "Loading TGA file!";

   QDataStream s( device() );
   s.setByteOrder(QDataStream::LittleEndian);


   // Read image header.
   TgaHeader tga;
   s >> tga;
   s.device()->seek(TgaHeader::SIZE + tga.id_length);

   // Check image file format.
   if (s.atEnd()) {
      //kDebug(399) << "This TGA file is not valid.";
      return false;
   }

   // Check supported file types.
   if (!IsSupported(tga)) {
      //kDebug(399) << "This TGA file is not supported.";
      return false;
   }


   QImage img;
   bool result = LoadTGA(s, tga, img);

   if (result == false) {
      //kDebug(399) << "Error loading TGA file.";
      return false;
   }


   *outImage = img;
   return true;
}
bool TGAHandler::read(QDataStream &s, QImage *outImage)
{
    //kDebug(399) << "Loading TGA file!";

    //QDataStream s( device() );
    s.setByteOrder( QDataStream::LittleEndian );


    // Read image header.
    TgaHeader tga;
    s >> tga;
    s.device()->seek( TgaHeader::SIZE + tga.id_length );

    // Check image file format.
    if( s.atEnd() ) {
        //kDebug(399) << "This TGA file is not valid.";
        return false;
    }

    // Check supported file types.
    if( !IsSupported(tga) ) {
        //kDebug(399) << "This TGA file is not supported.";
        return false;
    }


    QImage img;
    bool result = LoadTGA(s, tga, img);

    if( result == false ) {
        //kDebug(399) << "Error loading TGA file.";
        return false;
    }


    *outImage = img;
    return true;
}

bool TGAHandler::write(const QImage &image)
{
    QDataStream s( device() );
    s.setByteOrder( QDataStream::LittleEndian );

    const QImage& img = image;
    const bool hasAlpha = (img.format() == QImage::Format_ARGB32);
    for( int i = 0; i < 12; i++ )
        s << targaMagic[i];

    // write header
    s << quint16( img.width() ); // width
    s << quint16( img.height() ); // height
    s << quint8( hasAlpha ? 32 : 24 ); // depth (24 bit RGB + 8 bit alpha)
    s << quint8( hasAlpha ? 0x24 : 0x20 ); // top left image (0x20) + 8 bit alpha (0x4)

    for( int y = 0; y < img.height(); y++ )
        for( int x = 0; x < img.width(); x++ ) {
            const QRgb color = img.pixel( x, y );
            s << quint8( qBlue( color ) );
            s << quint8( qGreen( color ) );
            s << quint8( qRed( color ) );
            if( hasAlpha )
                s << quint8( qAlpha( color ) );
        }

    return true;
}

QByteArray TGAHandler::name() const
{
    return "tga";
}

bool TGAHandler::canRead(QIODevice *device)
{
    if (!device) {
        qWarning("TGAHandler::canRead() called with no device");
        return false;
    }

    qint64 oldPos = device->pos();
    QByteArray head = device->read(TgaHeader::SIZE);
    int readBytes = head.size();

    if (device->isSequential()) {
        for (int pos = readBytes - 1; pos >= 0; --pos) {
            device->ungetChar(head[pos]);
        }
    } else {
        device->seek(oldPos);
    }

    if (readBytes < TgaHeader::SIZE) {
        return false;
    }

    QDataStream stream(head);
    stream.setByteOrder(QDataStream::LittleEndian);
    TgaHeader tga;
    stream >> tga;
    return IsSupported(tga);
}


class TGAPlugin : public QImageIOPlugin
{
public:
    QStringList keys() const;
    Capabilities capabilities(QIODevice *device, const QByteArray &format) const;
    QImageIOHandler *create(QIODevice *device, const QByteArray &format = QByteArray()) const;
};

QStringList TGAPlugin::keys() const
{
    return QStringList() << "tga" << "TGA";
}

QImageIOPlugin::Capabilities TGAPlugin::capabilities(QIODevice *device, const QByteArray &format) const
{
    if (format == "tga" || format == "TGA")
        return Capabilities(CanRead | CanWrite);
    if (!format.isEmpty())
        return 0;
    if (!device->isOpen())
        return 0;

    Capabilities cap;
    if (device->isReadable() && TGAHandler::canRead(device))
        cap |= CanRead;
    if (device->isWritable())
        cap |= CanWrite;
    return cap;
}

QImageIOHandler *TGAPlugin::create(QIODevice *device, const QByteArray &format) const
{
    QImageIOHandler *handler = new TGAHandler;
    handler->setDevice(device);
    handler->setFormat(format);
    return handler;
}

//Q_EXPORT_STATIC_PLUGIN(TGAPlugin)
//Q_EXPORT_PLUGIN2(tga, TGAPlugin)