#ifndef DISPLAYDEFS_H
#define DISPLAYDEFS_H

#define RGB(r,g,b) ( ((r<<8)&0xF800) | ((g<<3)&0x07E0) | (b>>3) )


/// Return values from functions. Use this number, or use the 
/// lookup function to get a text string. @see GetErrorMessage.
///
typedef enum
{
    noerror,                ///< no errors, command completed successfully
    bad_parameter,          ///< one or more parameters are invalid
    file_not_found,         ///< specified file could not be found
    not_bmp_format,         ///< file is not a .bmp file
    not_ico_format,         ///< file is not a .ico file
    not_supported_format,   ///< file format is not yet supported (e.g. bits per pixel, compression)
    image_too_big,          ///< image is too large for the screen
    not_enough_ram,         ///< could not allocate ram for scanline
    touch_cal_timeout,      ///< timeout while trying to calibrate touchscreen, perhaps it is not installed.
    external_abort,         ///< an external process caused an abort
    LastErrCode,            // Private marker.
} RetCode_t;

/// return values from TouchPanelReadable, TouchPanelA2DRaw, TouchPanelA2DFiltered.
/// @see TouchPanelReadable.
typedef enum
{
    no_touch,               ///< no touch is detected
    touch,                  ///< touch is detected
    held,                   ///< held after touch
    release,                ///< release is detected
    no_cal,                 ///< no calibration matrix is available
} TouchCode_t;

/// Data type that manages locations, which is typically an x or y pixel location,
/// which can range from -N to +N (even if the screen is 0 to +n). @see textloc_t.
typedef int16_t loc_t;

/// Data type that manages text locations, which are row or column values in
/// units of character, not pixel. @see loc_t.
typedef uint16_t textloc_t;

/// type that manages dimensions of width or height, which range from 0 to N.
typedef uint16_t dim_t;

/// type that manages x,y pairs
typedef struct
{
    loc_t x;             ///< x value in the point
    loc_t y;             ///< y value in the point
} point_t;

/// Data type that manages rectangles, which are pairs of points. It is recommended
/// that p1 contains the top-left point and p2 contains the bottom-right point,
/// even though eventually this should not matter.
typedef struct
{
    point_t p1;         ///< p1 defines one point on the rectangle
    point_t p2;         ///< p2 defines the opposite point on the rectangle
} rect_t;

/// Data type that manages the calibration matrix for the resistive touch panel.
///
/// This object, when instantiated, may be passed back and forth, stored
/// and loaded, but the internals are generally of little interest.
typedef struct
{
    int32_t An;         ///< calibration factor, see source for details
    int32_t Bn;         ///< calibration factor, see source for details
    int32_t Cn;         ///< calibration factor, see source for details
    int32_t Dn;         ///< calibration factor, see source for details
    int32_t En;         ///< calibration factor, see source for details
    int32_t Fn;         ///< calibration factor, see source for details
    int32_t Divider;    ///< calibration factor, see source for details
} tpMatrix_t;

/// color type definition to let the compiler help keep us honest.
/// 
/// colors can be defined with the RGB(r,g,b) macro, and there
/// are a number of predefined colors:
/// - Black,    Blue,       Green,       Cyan,
/// - Red,      Magenta,    Brown,       Gray,
/// - Charcoal, BrightBlue, BrightGreen, BrightCyan,
/// - Orange,   Pink,       Yellow,      White
///
typedef uint16_t color_t;   

/// background fill info for drawing Text, Rectangles, RoundedRectanges, Circles, Ellipses and Triangles.
typedef enum
{
    NOFILL,     ///< do not fill the object with the background color
    FILL        ///< fill the object space with the background color
} fill_t;

#endif // DISPLAYDEFS_H
