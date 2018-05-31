/* mbed GraphicsDisplay Display Library Base Class
 * Copyright (c) 2007-2009 sford
 * Released under the MIT License: http://mbed.org/license/mit
 *
 * Derivative work by D.Smart 2014
 */

#include "GraphicsDisplay.h"
#include "Bitmap.h"
#include "string.h"

//#include "Utility.h"            // private memory manager
#ifndef UTILITY_H
#define swMalloc malloc         // use the standard
#define swFree free
#endif

//#define DEBUG "GD  "
// ...
// INFO("Stuff to show %d", var); // new-line is automatically appended
//
#if (defined(DEBUG) && !defined(TARGET_LPC11U24))
#define INFO(x, ...) std::printf("[INF %s %4d] "x"\r\n", DEBUG, __LINE__, ##__VA_ARGS__);
#define WARN(x, ...) std::printf("[WRN %s %4d] "x"\r\n", DEBUG, __LINE__, ##__VA_ARGS__);
#define ERR(x, ...)  std::printf("[ERR %s %4d] "x"\r\n", DEBUG, __LINE__, ##__VA_ARGS__);
static void HexDump(const char * title, const uint8_t * p, int count)
{
    int i;
    char buf[100] = "0000: ";
    
    if (*title)
        INFO("%s", title);
    for (i=0; i<count; ) {
        sprintf(buf + strlen(buf), "%02X ", *(p+i));
        if ((++i & 0x0F) == 0x00) {
            INFO("%s", buf);
            if (i < count)
                sprintf(buf, "%04X: ", i);
            else
                buf[0] = '\0';
        }
    }
    if (strlen(buf))
        INFO("%s", buf);
}
#else
#define INFO(x, ...)
#define WARN(x, ...)
#define ERR(x, ...)
#define HexDump(a, b, c)
#endif


char mytolower(char a) {
    if (a >= 'A' && a <= 'Z')
        return (a - 'A' + 'a');
    else
        return a;
}
/// mystrnicmp exists because not all compiler libraries have this function.
///
/// Some have strnicmp, others _strnicmp, and others have C++ methods, which
/// is outside the scope of this C-portable set of functions.
///
/// @param l is a pointer to the string on the left
/// @param r is a pointer to the string on the right
/// @param n is the number of characters to compare
/// @returns -1 if l < r
/// @returns 0 if l == r
/// @returns +1 if l > r
///
int mystrnicmp(const char *l, const char *r, size_t n) {
    int result = 0;

    if (n != 0) {
        do {
            result = mytolower(*l++) - mytolower(*r++);
        } while ((result == 0) && (*l != '\0') && (--n > 0));
    }
    if (result < -1)
        result = -1;
    else if (result > 1)
        result = 1;
    return result;
}


GraphicsDisplay::GraphicsDisplay(const char *name) 
    : TextDisplay(name)
{
    font = NULL;
}

//GraphicsDisplay::~GraphicsDisplay()
//{
//}

RetCode_t GraphicsDisplay::SelectUserFont(const unsigned char * _font)
{
    font = _font;     // trusting them, but it might be good to put some checks in here...
    return noerror;
}

int GraphicsDisplay::character(int x, int y, int c)
{
    return fontblit(x, y, c);
}

RetCode_t GraphicsDisplay::window(rect_t r)
{
    return window(r.p1.x, r.p1.y, r.p2.x + 1 - r.p1.x, r.p2.y + 1 - r.p1.y);
}

RetCode_t GraphicsDisplay::window(loc_t x, loc_t y, dim_t _width, dim_t _height)
{
    if (_width == (dim_t)-1)
        _width = width() - x;
    if (_height == (dim_t)-1)
        _height = height() - y;

    // Save the window metrics
    windowrect.p1.x = x;
    windowrect.p1.y = y;
    windowrect.p2.x = x + _width - 1;
    windowrect.p2.y = y + _height - 1;
    // current pixel location
    _x = x;
    _y = y;
    return noerror;
}

RetCode_t GraphicsDisplay::WindowMax(void)
{
    return window(0,0, width(),height());
}

RetCode_t GraphicsDisplay::_putp(color_t color)
{
    pixel(_x, _y, color);
    // update pixel location based on window settings
    _x++;
    if(_x > windowrect.p2.x) {
        _x = windowrect.p1.x;
        _y++;
        if(_y > windowrect.p2.y) {
            _y = windowrect.p1.y;
        }
    }
    return noerror;
}

RetCode_t GraphicsDisplay::fill(loc_t x, loc_t y, dim_t w, dim_t h, color_t color)
{
    return fillrect(x,y, x+w, y+h, color);
}

RetCode_t GraphicsDisplay::cls(uint16_t layers)
{
    int restore = GetDrawingLayer();
    if (layers & 1) {
        SelectDrawingLayer(0);
        fill(0, 0, width(), height(), _background);
    }
    if (layers & 2) {
        SelectDrawingLayer(1);
        fill(0, 0, width(), height(), _background);
    }
    SelectDrawingLayer(restore);
    return noerror;
}

RetCode_t GraphicsDisplay::blit(loc_t x, loc_t y, dim_t w, dim_t h, const int * color)
{
    rect_t restore = windowrect;
    window(x, y, w, h);
    _StartGraphicsStream();
    for (int i=0; i<w*h; i++) {
        _putp(color[i]);
    }
    _EndGraphicsStream();
    //return WindowMax();
    return window(restore);
}

// 8 byte "info" section
//0x00,                // unknown    ????????
//0x00,                // unknown    ????????
//0x20,0x00,           // First char 32
//0x7F,0x00,           // Last char  127
//0x25,                // Font Height 37
//0x00,                // Unknown      0  ????????
//
//0x01,0x88,0x01,0x00  // ' ' is  1 pixel  wide, data is at offset 0x0188
//0x0B,0xAD,0x01,0x00  // '!' is 11 pixels wide, data is at offset 0x01AD
//0x0D,0xF7,0x01,0x00  // '"' is 13 pixels wide, data is at offset 0x01F7
//...
//0x00,...             // ' ' data stream.
//0x00,0x06,0x00,0x07,0x80,0x07,0xC0,0x07,0xC0,0x07,0xC0 // '!'
//...


const uint8_t * GraphicsDisplay::getCharMetrics(const unsigned char c, dim_t * width, dim_t * height)
{
    uint16_t offsetToCharLookup;
    uint16_t firstChar = font[3] * 256 + font[2];
    uint16_t lastChar  = font[5] * 256 + font[4];
    dim_t charHeight = font[6];
    const unsigned char * charRecord;   // width, data, data, data, ...
    
    INFO("first:%d, last:%d, c:%d", firstChar, lastChar, c);
    if (c < firstChar || c > lastChar)
        return NULL;       // advance zero pixels since it was unprintable...
    
    // 8 bytes of preamble to the first level lookup table
    offsetToCharLookup = 8 + 4 * (c - firstChar);    // 4-bytes: width(pixels), 16-bit offset from table start, 0
    dim_t charWidth = font[offsetToCharLookup];
    charRecord = font + font[offsetToCharLookup + 2] * 256 + font[offsetToCharLookup + 1];
    //INFO("hgt:%d, wdt:%d", charHeight, charWidth);
    if (width)
        *width = charWidth;
    if (height)
        *height = charHeight;
    return charRecord;
}


int GraphicsDisplay::fontblit(loc_t x, loc_t y, const unsigned char c)
{
    const uint8_t * charRecord;   // width, data, data, data, ...
    dim_t charWidth, charHeight;
    charRecord = getCharMetrics(c, &charWidth, &charHeight);
    if (charRecord) {
        INFO("hgt:%d, wdt:%d", charHeight, charWidth);
        // clip to the edge of the screen
        //if (x + charWidth >= width())
        //    charWidth = width() - x;
        //if (y + charHeight >= height())
        //    charHeight = height() - y;
        booleanStream(x,y,charWidth, charHeight, charRecord);
        return charWidth;
    } else {
        return 0;
    }
}

// BMP Color Palette is BGRx
//      BBBB BBBB GGGG GGGG RRRR RRRR 0000 0000
// RGB16 is
//      RRRR RGGG GGGB BBBB
// swap to little endian
//      GGGB BBBB RRRR RGGG
color_t GraphicsDisplay::RGBQuadToRGB16(RGBQUAD * colorPalette, uint16_t i)
{
    color_t c;
 
    c  = ((colorPalette[i].rgbBlue  >> 3) <<  0);
    c |= ((colorPalette[i].rgbGreen >> 2) <<  5);
    c |= ((colorPalette[i].rgbRed   >> 3) << 11);
    return c;
}

// RGB16 little endian 
//      GGGB BBBB RRRR RGGG
// swap
//      RRRR RGGG GGGB BBBB
//                RRRR R
// extend to BMP Color Palette is BGRx
//      BBBB BBBB GGGG GGGG RRRR RRRR 0000 0000
RGBQUAD GraphicsDisplay::RGB16ToRGBQuad(color_t c)
{
    RGBQUAD q;
    
    memset(&q, 0, sizeof(q));
    c = (c << 8) | (c >> 8);    // swap
    q.rgbBlue  = ((c & 0x001F) << 3) | (c & 0x07);          /* Blue value */
    q.rgbGreen = ((c & 0x07E0) >> 3) | ((c >> 9) & 0x03);   /* Green value */
    q.rgbRed   = ((c & 0xF800) >> 8) | ((c >> 13) & 0x07);  /* Red value */
    q.rgbReserved = 0;
    return q;
}

RetCode_t GraphicsDisplay::_RenderBitmap(loc_t x, loc_t y, uint32_t fileOffset, FILE * Image)
{
    BITMAPINFOHEADER BMP_Info;
    RGBQUAD * colorPalette = NULL;
    int colorCount;
    uint8_t * lineBuffer = NULL;
    color_t * pixelBuffer = NULL;
    uint16_t BPP_t;
    dim_t PixelWidth, PixelHeight;
    unsigned int    i, offset;
    int padd,j;
    #ifdef DEBUG
    //uint32_t start_data;
    #endif

    // Now, Read the bitmap info header
    fread(&BMP_Info, 1, sizeof(BMP_Info), Image);
    HexDump("BMP_Info", (uint8_t *)&BMP_Info, sizeof(BMP_Info));
    BPP_t = BMP_Info.biBitCount;
    INFO("biBitCount %04X", BPP_t);
    if (BPP_t != 1 && BPP_t != 4 && BPP_t != 8 && BPP_t != 16 && BPP_t != 24) { // Support 4, 8, 16, 24-bits per pixel
        fclose(Image);
        return(not_supported_format);
    }
    if (BMP_Info.biCompression != 0) {  // Only the "no comporession" option is supported.
        fclose(Image);
        return(not_supported_format);
    }
    PixelHeight = BMP_Info.biHeight;
    PixelWidth = BMP_Info.biWidth;
    INFO("(%d,%d) (%d,%d) (%d,%d)", x,y, PixelWidth,PixelHeight, width(), height());
    if (PixelHeight > height() + y || PixelWidth > width() + x) {
        fclose(Image);
        return(image_too_big);
    }
    if (BMP_Info.biBitCount <= 8) {
        int paletteSize;
        // Read the color palette
        colorCount = 1 << BMP_Info.biBitCount;
        paletteSize = sizeof(RGBQUAD) * colorCount;
        colorPalette = (RGBQUAD *)swMalloc(paletteSize);
        if (colorPalette == NULL) {
            fclose(Image);
            return(not_enough_ram);
        }
        fread(colorPalette, 1, paletteSize, Image);
        HexDump("Color Palette", (uint8_t *)colorPalette, paletteSize);
    }

    int lineBufSize = ((BPP_t * PixelWidth + 7)/8);
    INFO("BPP_t %d, PixelWidth %d, lineBufSize %d", BPP_t, PixelWidth, lineBufSize);
    lineBuffer = (uint8_t *)swMalloc(lineBufSize);
    if (lineBuffer == NULL) {
        swFree(colorPalette);
        fclose(Image);
        return(not_enough_ram);
    }
    pixelBuffer = (color_t *)swMalloc(PixelWidth * sizeof(color_t));
    if (pixelBuffer == NULL) {
        swFree(lineBuffer);
        if (colorPalette)
            swFree(colorPalette);
        fclose(Image);
        return(not_enough_ram);
    }

    padd = (lineBufSize % 4);
    if (padd)
        padd = 4 - padd;

    // Define window for top to bottom and left to right so writing auto-wraps
    rect_t restore = windowrect;
    window(x,y, PixelWidth,PixelHeight);
//    SetGraphicsCursor(x, y);
//    _StartGraphicsStream();

    //start_data = BMP_Info.bfOffBits;
    //HexDump("Raw Data", (uint8_t *)&start_data, 32);
    INFO("(%d,%d) (%d,%d), [%d,%d]", x,y, PixelWidth,PixelHeight, lineBufSize, padd);
    for (j = PixelHeight - 1; j >= 0; j--) {                //Lines bottom up
        offset = fileOffset + j * (lineBufSize + padd);     // start of line
        fseek(Image, offset, SEEK_SET);
        fread(lineBuffer, 1, lineBufSize, Image);           // read a line - slow !
        //INFO("offset: %6X", offset);
        //HexDump("Line", lineBuffer, lineBufSize);
        for (i = 0; i < PixelWidth; i++) {                  // copy pixel data to TFT
            if (BPP_t == 1) {
                uint8_t dPix = lineBuffer[i/8];
                uint8_t bMask = 0x80 >> (i % 8);
                uint8_t bit = (bMask & dPix) ? 0 : 1;
                pixelBuffer[i] = RGBQuadToRGB16(colorPalette, bit);
            } else if (BPP_t == 4) {
                uint8_t dPix = lineBuffer[i/2];
                if ((i & 1) == 0)
                    dPix >>= 4;
                dPix &= 0x0F;
                pixelBuffer[i] = RGBQuadToRGB16(colorPalette, dPix);
            } else if (BPP_t == 8) {
                pixelBuffer[i] = RGBQuadToRGB16(colorPalette, lineBuffer[i]);
            } else if (BPP_t == 16) {
                pixelBuffer[i] = lineBuffer[i];
            } else if (BPP_t == 24) {
                color_t color;
                color = RGB(lineBuffer[i*3+2], lineBuffer[i*3+1], lineBuffer[i*3+0]);
                pixelBuffer[i] = color;
            }
        }
        pixelStream(pixelBuffer, PixelWidth, x, y++);
    }
//    _EndGraphicsStream();
    window(restore);
    swFree(pixelBuffer);      // don't leak memory
    swFree(lineBuffer);
    if (colorPalette)
        swFree(colorPalette);
    return (noerror);
}


RetCode_t GraphicsDisplay::RenderImageFile(loc_t x, loc_t y, const char *FileName)
{
    if (mystrnicmp(FileName + strlen(FileName) - 4, ".bmp", 4) == 0) {
        return RenderBitmapFile(x,y,FileName);
    } else if (mystrnicmp(FileName + strlen(FileName) - 4, ".jpg", 4) == 0) {
        return RenderJpegFile(x,y,FileName);
    } else if (mystrnicmp(FileName + strlen(FileName) - 4, ".ico", 4) == 0) {
        return RenderIconFile(x,y,FileName);
    } else {
        return not_supported_format;
    }
}

RetCode_t GraphicsDisplay::RenderJpegFile(loc_t x, loc_t y, const char *Name_JPG)
{
    #define JPEG_WORK_SPACE_SIZE 3100   // Worst case requirements for the decompression
    JDEC * jdec;
    uint16_t * work;
    RetCode_t r = noerror;  // start optimistic
    FILE * fh = fopen(Name_JPG, "rb");
    
    if (!fh)
        return(file_not_found);
    //INFO("RenderJpegFile(%d,%d,%s)", x,y, Name_JPG);
    work = (uint16_t *)swMalloc(JPEG_WORK_SPACE_SIZE);
    if (work) {
        jdec = (JDEC *)swMalloc(sizeof(JDEC));
        if (jdec) {
            memset(work, 0, JPEG_WORK_SPACE_SIZE/sizeof(uint16_t));
            memset(jdec, 0, sizeof(JDEC));
            r = (RetCode_t)jd_prepare(jdec, NULL, work, JPEG_WORK_SPACE_SIZE, fh);
            INFO("jd_prepare returned %d", r);
            
            if (r == noerror) {
                img_x = x;  // save the origin for the privOutput function
                img_y = y;
                r = (RetCode_t)jd_decomp(jdec, NULL, 0);
            } else {
                r = not_supported_format;   // error("jd_prepare error:%d", r);
            }
            swFree(jdec);
        } else {
            WARN("checkpoint");
            r = not_enough_ram;
        }
        swFree(work);
    } else {
        WARN("checkpoint");
        r = not_enough_ram;
    }
    fclose(fh);
    return r;   // error("jd_decomp error:%d", r);
}

RetCode_t GraphicsDisplay::RenderBitmapFile(loc_t x, loc_t y, const char *Name_BMP)
{
    BITMAPFILEHEADER BMP_Header;

    INFO("Opening {%s}", Name_BMP);
    FILE *Image = fopen(Name_BMP, "rb");
    if (!Image) {
        return(file_not_found);
    }

    fread(&BMP_Header, 1, sizeof(BMP_Header), Image);      // get the BMP Header
    INFO("bfType %04X", BMP_Header.bfType);
    HexDump("BMP_Header", (uint8_t *)&BMP_Header, sizeof(BMP_Header));
    if (BMP_Header.bfType != BF_TYPE) {
        fclose(Image);
        return(not_bmp_format);
    }
    INFO("bfOffits %04X", BMP_Header.bfOffBits);
    RetCode_t rt = _RenderBitmap(x, y, BMP_Header.bfOffBits, Image);
    if (rt != noerror) {
        return rt;
    } else {
        fclose(Image);
        return (noerror);
    }
}

RetCode_t GraphicsDisplay::RenderIconFile(loc_t x, loc_t y, const char *Name_ICO)
{
    ICOFILEHEADER ICO_Header;
    ICODIRENTRY ICO_DirEntry;

    INFO("Opening {%s}", Name_ICO);
    FILE *Image = fopen(Name_ICO, "rb");
    if (!Image) {
        return(file_not_found);
    }

    fread(&ICO_Header, 1, sizeof(ICO_Header), Image);      // get the BMP Header
    HexDump("ICO_Header", (uint8_t *)&ICO_Header, sizeof(ICO_Header));
    if (ICO_Header.Reserved_zero != 0
    || ICO_Header.icType != IC_TYPE
    || ICO_Header.icImageCount == 0) {
        fclose(Image);
        return(not_ico_format);
    }

    // Read ONLY the first of n possible directory entries.
    fread(&ICO_DirEntry, 1, sizeof(ICO_DirEntry), Image);
    HexDump("ICO_DirEntry", (uint8_t *)&ICO_DirEntry, sizeof(ICO_DirEntry));
    INFO("biBitCount %04X", ICO_DirEntry.biBitCount);
    if (ICO_DirEntry.biBitCount != 0) {     // Expecting this to be zero for ico
        fclose(Image);
        return(not_supported_format);
    }

    RetCode_t rt = _RenderBitmap(x, y, ICO_DirEntry.bfOffBits, Image);
    if (rt == noerror) {
        fclose(Image);
        return (noerror);
    } else {
        return rt;
    }
}

int GraphicsDisplay::columns()
{
    return width() / 8;
}

int GraphicsDisplay::rows()
{
    return height() / 8;
}
