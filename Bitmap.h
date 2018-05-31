//
// Windows BMP file definitions.
//
// Adapted from code written by Michael Sweet from Paul Bourke's
// web site: http://paulbourke.net/dataformats/bmp/
//

#ifndef _BITMAP_H_
#define _BITMAP_H_

#include <mbed.h>

// BITMAPFILEHEADER
// BITMAPINFOHEADER
// Optional Palette
// Raw Data

//
// Bitmap file data structures
//
// must align to 2-byte boundaries so it doesn't alter the memory image when 
// bytes are read from the file system into this footprint.
#pragma push
#pragma pack(2)

/// Bitmap file header
typedef struct                    /**** BMP file header structure ****/
    {
    uint16_t    bfType;           ///< Magic number for file 
    uint32_t    bfSize;           ///< Size of file 
    uint16_t    bfReserved1;      ///< reserved placeholder for the structure footprint
    uint16_t    bfReserved2;      ///< reserved placeholder for the structure footprint
    uint32_t    bfOffBits;        ///< Offset to bitmap data 
    } BITMAPFILEHEADER;

/// Bitmap information header
typedef struct                    /**** BMP file info structure ****/
    {
    uint32_t    biSize;           ///< Size of info header 
    uint32_t    biWidth;          ///< Width of image 
    uint32_t    biHeight;         ///< Height of image 
    uint16_t    biPlanes;         ///< Number of color planes 
    uint16_t    biBitCount;       ///< Number of bits per pixel 
    uint32_t    biCompression;    ///< Type of compression to use 
    uint32_t    biSizeImage;      ///< Size of image data 
    int32_t     biXPelsPerMeter;  ///< X pixels per meter 
    int32_t     biYPelsPerMeter;  ///< Y pixels per meter 
    uint32_t    biClrUsed;        ///< Number of colors used 
    uint32_t    biClrImportant;   ///< Number of important colors 
    } BITMAPINFOHEADER;
#pragma pop

#define BF_TYPE 0x4D42            /* "MB" */

/*
 * Constants for the biCompression field...
 */

#  define BI_RGB       0             /* No compression - straight BGR data */
#  define BI_RLE8      1             /* 8-bit run-length compression */
#  define BI_RLE4      2             /* 4-bit run-length compression */
#  define BI_BITFIELDS 3             /* RGB bitmap with RGB masks */

/// An RGB Quad data type used by the bitmap driver.
typedef struct                       /**** Colormap entry structure ****/
    {
    uint8_t  rgbBlue;          ///< Blue value 
    uint8_t  rgbGreen;         ///< Green value 
    uint8_t  rgbRed;           ///< Red value 
    uint8_t  rgbReserved;      ///< Reserved 
    } RGBQUAD;

//typedef struct                       /**** Bitmap information structure ****/
//    {
//    BITMAPINFOHEADER bmiHeader;      /* Image header */
//    RGBQUAD          bmiColors[256]; /* Image colormap */
//    } BITMAPINFO;


#pragma push
#pragma pack(2)

/// Icon file type file header.
typedef struct                  /**** ICO file header structure ****/
    {
    uint16_t    Reserved_zero;  ///< Always zero
    uint16_t    icType;         ///< 1 for .ico, 2 for .cur
    uint16_t    icImageCount;   ///< number of images in the file    
    } ICOFILEHEADER;

/// Icon file type directory entry structure
typedef struct                  /**** ICO file Directory Entry structure (1 or more) ****/
    {
    uint8_t     biWidth;        ///< Width of image 
    uint8_t     biHeight;       ///< Height of image 
    uint8_t     biClrUsed;      ///< Number of colors used 
    uint8_t     Reserved_zero;  ///< reserved placeholder for the structure footprint
    uint16_t    biPlanes;       ///< Number of color planes (ICO should be 0 or 1, CUR horz hotspot 
    uint16_t    biBitCount;     ///< Number of bits per pixel (ICO bits per pixel, CUR vert hotspot 
    uint32_t    biSizeImage;    ///< Size of image data 
    uint32_t    bfOffBits;      ///< Offset into file for the bitmap data 
    } ICODIRENTRY;
#pragma pop

#define IC_TYPE 0x0001            /* 1 = ICO (icon), 2 = CUR (cursor) */

#endif // _BITMAP_H_
