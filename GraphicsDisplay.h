/* mbed GraphicsDisplay Display Library Base Class
 * Copyright (c) 2007-2009 sford
 * Released under the MIT License: http://mbed.org/license/mit
 *
 * A library for providing a common base class for Graphics displays
 * To port a new display, derive from this class and implement
 * the constructor (setup the display), pixel (put a pixel
 * at a location), width and height functions. Everything else
 * (locate, printf, putc, cls, window, putp, fill, blit, blitbit) 
 * will come for free. You can also provide a specialised implementation
 * of window and putp to speed up the results
 */

#ifndef MBED_GRAPHICSDISPLAY_H
#define MBED_GRAPHICSDISPLAY_H
#include "Bitmap.h"
#include "TextDisplay.h"
#include "GraphicsDisplayJPEG.h"

/// The GraphicsDisplay class 
/// 
/// This graphics display class supports both graphics and text operations.
/// Typically, a subclass is derived from this which has localizations to
/// adapt to a specific hardware platform (e.g. a display controller chip),
/// that overrides methods in here to either add more capability or perhaps 
/// to improve performance, by leveraging specific hardware capabilities.
///
class GraphicsDisplay : public TextDisplay 
{
public:
    /// The constructor
    GraphicsDisplay(const char* name);
    
    //~GraphicsDisplay();
    
    /// Draw a pixel in the specified color.
    ///
    /// @note this method must be supported in the derived class.
    ///
    /// @param[in] x is the horizontal offset to this pixel.
    /// @param[in] y is the vertical offset to this pixel.
    /// @param[in] color defines the color for the pixel.
    /// @returns success/failure code. @see RetCode_t.
    ///
    virtual RetCode_t pixel(loc_t x, loc_t y, color_t color) = 0;
    
    /// Write a stream of pixels to the display.
    ///
    /// @note this method must be supported in the derived class.
    ///
    /// @param[in] p is a pointer to a color_t array to write.
    /// @param[in] count is the number of pixels to write.
    /// @param[in] x is the horizontal position on the display.
    /// @param[in] y is the vertical position on the display.
    /// @returns success/failure code. @see RetCode_t.
    ///
    virtual RetCode_t pixelStream(color_t * p, uint32_t count, loc_t x, loc_t y) = 0;

    /// Get a pixel from the display.
    ///
    /// @note this method must be supported in the derived class.
    ///
    /// @param[in] x is the horizontal offset to this pixel.
    /// @param[in] y is the vertical offset to this pixel.
    /// @returns the pixel. @see color_t
    ///
    virtual color_t getPixel(loc_t x, loc_t y) = 0;

    /// Get a stream of pixels from the display.
    ///
    /// @note this method must be supported in the derived class.
    ///
    /// @param[out] p is a pointer to a color_t array to accept the stream.
    /// @param[in] count is the number of pixels to read.
    /// @param[in] x is the horizontal offset to this pixel.
    /// @param[in] y is the vertical offset to this pixel.
    /// @returns success/failure code. @see RetCode_t.
    ///
    virtual RetCode_t getPixelStream(color_t * p, uint32_t count, loc_t x, loc_t y) = 0;
    
    /// get the screen width in pixels
    ///
    /// @note this method must be supported in the derived class.
    ///
    /// @returns screen width in pixels.
    ///
    virtual uint16_t width() = 0;
    
    /// get the screen height in pixels
    ///
    /// @note this method must be supported in the derived class.
    ///
    /// @returns screen height in pixels.
    ///
    virtual uint16_t height() = 0;

    /// Prepare the controller to write binary data to the screen by positioning
    /// the memory cursor.
    ///
    /// @note this method must be supported in the derived class.
    ///
    /// @param[in] x is the horizontal position in pixels (from the left edge)
    /// @param[in] y is the vertical position in pixels (from the top edge)
    /// @returns success/failure code. @see RetCode_t.
    ///
    virtual RetCode_t SetGraphicsCursor(loc_t x, loc_t y) = 0;
    
    
    /// Prepare the controller to write binary data to the screen by positioning
    /// the memory cursor.
    ///
    /// @param[in] p is the point representing the cursor position to set
    /// @returns success/failure code. See @ref RetCode_t.
    ///
    virtual RetCode_t SetGraphicsCursor(point_t p) = 0;
    
    /// Read the current graphics cursor position as a point.
    ///
    /// @returns the graphics cursor as a point.
    ///
    virtual point_t GetGraphicsCursor(void) = 0;

    /// Prepare the controller to read binary data from the screen by positioning
    /// the memory read cursor.
    ///
    /// @param[in] x is the horizontal position in pixels (from the left edge)
    /// @param[in] y is the vertical position in pixels (from the top edge)
    /// @returns success/failure code. @see RetCode_t.
    ///
    virtual RetCode_t SetGraphicsCursorRead(loc_t x, loc_t y) = 0;
    
    /// Draw a filled rectangle in the specified color
    ///
    /// @note As a side effect, this changes the current
    ///     foreground color for subsequent operations.
    ///
    /// @note this method must be supported in the derived class.
    ///
    /// @param[in] x1 is the horizontal start of the line.
    /// @param[in] y1 is the vertical start of the line.
    /// @param[in] x2 is the horizontal end of the line.
    /// @param[in] y2 is the vertical end of the line.
    /// @param[in] color defines the foreground color.
    /// @param[in] fillit is optional to NOFILL the rectangle. default is FILL.
    /// @returns success/failure code. @see RetCode_t.
    ///
    virtual RetCode_t fillrect(loc_t x1, loc_t y1, loc_t x2, loc_t y2, 
        color_t color, fill_t fillit = FILL) = 0;

    /// Select the drawing layer for subsequent commands.
    ///
    /// If the screen configuration is 480 x 272, or if it is 800 x 480 
    /// and 8-bit color, the the display supports two layers, which can 
    /// be independently drawn on and shown. Additionally, complex
    /// operations involving both layers are permitted.
    ///
    /// @code
    ///     //lcd.SetLayerMode(OnlyLayer0); // default is layer 0
    ///     lcd.rect(400,130, 475,155,Brown);
    ///     lcd.SelectDrawingLayer(1);
    ///     lcd.circle(400,25, 25, BrightRed);
    ///     wait(1);
    ///     lcd.SetLayerMode(ShowLayer1);
    /// @endcode
    ///
    /// @attention The user manual refers to Layer 1 and Layer 2, however the
    ///     actual register values are value 0 and 1. This API as well as
    ///     others that reference the layers use the values 0 and 1 for
    ///     cleaner iteration in the code.
    ///
    /// @param[in] layer is 0 or 1 to select the layer for subsequent 
    ///     commands.
    /// @param[out] prevLayer is an optiona pointer to where the previous layer
    ///     will be written, making it a little easer to restore layers.
    ///     Writes 0 or 1 when the pointer is not NULL.
    /// @returns success/failure code. See @ref RetCode_t.
    ///
    virtual RetCode_t SelectDrawingLayer(uint16_t layer, uint16_t * prevLayer = NULL) = 0;
 
    
    /// Get the currently active drawing layer.
    ///
    /// This returns a value, 0 or 1, based on the screen configuration
    /// and the currently active drawing layer.
    ///
    /// @code
    ///     uint16_t prevLayer;
    ///     lcd.SelectDrawingLayer(x, &prevLayer);
    ///     lcd.circle(400,25, 25, BrightRed);
    ///     lcd.SelectDrawingLayer(prevLayer);
    /// @endcode
    ///
    /// @attention The user manual refers to Layer 1 and Layer 2, however the
    ///     actual register values are value 0 and 1. This API as well as
    ///     others that reference the layers use the values 0 and 1 for
    ///     cleaner iteration in the code.
    ///
    /// @returns the current drawing layer; 0 or 1.
    /// 
    virtual uint16_t GetDrawingLayer(void) = 0;

    /// a function to write the command and data to the RA8875 chip.
    ///
    /// @param command is the RA8875 instruction to perform
    /// @param data is the optional data to the instruction.
    /// @returns success/failure code. @see RetCode_t.
    ///
    virtual RetCode_t WriteCommand(unsigned char command, unsigned int data = 0xFFFF) = 0;
    
    
    /// a function to write the data to the RA8875 chip.
    ///
    /// This is typically used after a command has been initiated, and where
    /// there may be a data stream to follow.
    ///
    /// @param data is the optional data to the instruction.
    /// @returns success/failure code. @see RetCode_t.
    ///
    virtual RetCode_t WriteData(unsigned char data) = 0;

    /// Set the window, which controls where items are written to the screen.
    ///
    /// When something hits the window width, it wraps back to the left side
    /// and down a row. 
    ///
    /// @note If the initial write is outside the window, it will
    ///     be captured into the window when it crosses a boundary. It may
    ///     be appropriate to SetGraphicsCursor() to a point in the window.
    ///
    /// @param[in] r is the rect_t rect to define the window.
    /// @returns success/failure code. @see RetCode_t.
    ///
    virtual RetCode_t window(rect_t r);

    /// Set the window, which controls where items are written to the screen.
    ///
    /// When something hits the window width, it wraps back to the left side
    /// and down a row. 
    ///
    /// @note If the initial write is outside the window, it will
    ///     be captured into the window when it crosses a boundary. It may
    ///     be appropriate to SetGraphicsCursor() to a point in the window.
    ///
    /// @note if no parameters are provided, it restores the window to full screen.
    ///
    /// @param[in] x is the left edge in pixels.
    /// @param[in] y is the top edge in pixels.
    /// @param[in] w is the window width in pixels.
    /// @param[in] h is the window height in pixels.
    /// @returns success/failure code. @see RetCode_t.
    ///
    virtual RetCode_t window(loc_t x = 0, loc_t y = 0, dim_t w = (dim_t)-1, dim_t h = (dim_t)-1);
    
    /// method to set the window region to the full screen.
    ///
    /// This restores the 'window' to the full screen, so that 
    /// other operations (@see cls) would clear the whole screen.
    ///
    /// @returns success/failure code. @see RetCode_t.
    ///
    virtual RetCode_t WindowMax(void);
    
    /// Clear the screen.
    ///
    /// The behavior is to clear the whole screen.
    ///
    /// @param[in] layers is ignored, but supports maintaining the same 
    ///     API for the graphics layer.
    /// @returns success/failure code. @see RetCode_t.
    ///
    virtual RetCode_t cls(uint16_t layers = 0);
    
    /// method to put a single color pixel to the screen.
    ///
    /// This method may be called as many times as necessary after 
    /// @see _StartGraphicsStream() is called, and it should be followed 
    /// by _EndGraphicsStream.
    ///
    /// @param[in] pixel is a color value to be put on the screen.
    /// @returns success/failure code. @see RetCode_t.
    ///
    virtual RetCode_t _putp(color_t pixel);

    /// method to fill a region.
    ///
    /// This method fills a region with the specified color. It essentially
    /// is an alias for fillrect, however this uses width and height rather
    /// than a second x,y pair.
    ///
    /// @param[in] x is the left-edge of the region.
    /// @param[in] y is the top-edge of the region.
    /// @param[in] w specifies the width of the region.
    /// @param[in] h specifies the height of the region.
    /// @param[in] color is the color value to use to fill the region
    /// @returns success/failure code. @see RetCode_t.
    /// 
    virtual RetCode_t fill(loc_t x, loc_t y, dim_t w, dim_t h, color_t color);
    
    /// method to stream bitmap data to the display
    ///
    /// This method fills a region from a stream of color data.
    ///
    /// @param[in] x is the left-edge of the region.
    /// @param[in] y is the top-edge of the region.
    /// @param[in] w specifies the width of the region.
    /// @param[in] h specifies the height of the region.
    /// @param[in] color is a pointer to a color stream with w x h values.
    /// @returns success/failure code. @see RetCode_t.
    /// 
    virtual RetCode_t blit(loc_t x, loc_t y, dim_t w, dim_t h, const int * color);    
    
    /// This method returns the width in pixels of the chosen character
    /// from the previously selected external font.
    ///
    /// @param[in] c is the character of interest.
    /// @param[in, out] width is a pointer to where the width will be stored.
    ///     This parameter is NULL tested and will only be written if not null
    ///     which is convenient if you only want the height.
    /// @param[in, out] height is a pointer to where the height will be stored.
    ///     This parameter is NULL tested and will only be written if not null
    ///     which is convenient if you only want the width.
    /// @returns a pointer to the raw character data or NULL if not found.
    ///
    virtual const uint8_t * getCharMetrics(const unsigned char c, dim_t * width, dim_t * height);
    
    /// This method transfers one character from the external font data
    /// to the screen.
    ///
    /// The font being used has already been set with the SelectUserFont
    /// API.
    ///
    /// @note the font data is in a special format as generate by
    ///         the mikroe font creator.
    ///         See http://www.mikroe.com/glcd-font-creator/
    ///
    /// @param[in] x is the horizontal pixel coordinate
    /// @param[in] y is the vertical pixel coordinate
    /// @param[in] c is the character to render
    /// @returns how far the cursor should advance to the right in pixels.
    /// @returns zero if the character could not be rendered.
    ///
    virtual int fontblit(loc_t x, loc_t y, const unsigned char c);
    
    /// This method returns the color value from a palette.
    ///
    /// This method accepts a pointer to a Bitmap color palette, which
    /// is a table in memory composed of RGB Quad values (r, g, b, 0),
    /// and an index into that table. It then extracts the color information
    /// and downsamples it to a color_t value which it returns.
    ///
    /// @note This method probably has very little value outside of
    ///         the internal methods for reading BMP files.
    ///
    /// @param[in] colorPaletteArray is the handle to the color palette array to use.
    /// @param[in] index is the index into the color palette.
    /// @returns the color in color_t format.
    ///
    color_t RGBQuadToRGB16(RGBQUAD * colorPaletteArray, uint16_t index);
    
    /// This method converts a 16-bit color value into a 24-bit RGB Quad.
    ///
    /// @param[in] c is the 16-bit color. @see color_t.
    /// @returns an RGBQUAD value. @see RGBQUAD
    ///
    RGBQUAD RGB16ToRGBQuad(color_t c);

    /// This method attempts to render a specified graphics image file at
    /// the specified screen location.
    ///
    /// This supports several variants of the following file types:
    /// \li Bitmap file format,
    /// \li Icon file format.
    ///
    /// @note The specified image width and height, when adjusted for the 
    ///     x and y origin, must fit on the screen, or the image will not
    ///     be shown (it does not clip the image).
    ///
    /// @note The file extension is tested, and if it ends in a supported
    ///     format, the appropriate handler is called to render that image.
    ///
    /// @param[in] x is the horizontal pixel coordinate
    /// @param[in] y is the vertical pixel coordinate
    /// @param[in] FileName refers to the fully qualified path and file on 
    ///     a mounted file system.
    /// @returns success or error code.
    ///
    RetCode_t RenderImageFile(loc_t x, loc_t y, const char *FileName);

    /// This method reads a disk file that is in jpeg format and 
    /// puts it on the screen.
    ///
    /// @param[in] x is the horizontal pixel coordinate
    /// @param[in] y is the vertical pixel coordinate
    /// @param[in] Name_JPG is the filename on the mounted file system.
    /// @returns success or error code.
    ///
    RetCode_t RenderJpegFile(loc_t x, loc_t y, const char *Name_JPG);

    /// This method reads a disk file that is in bitmap format and 
    /// puts it on the screen.
    ///
    /// Supported formats:
    /// \li 1-bit color format  (2 colors)
    /// \li 4-bit color format  (16 colors)
    /// \li 8-bit color format  (256 colors)
    /// \li 16-bit color format (65k colors)
    /// \li 24-bit color format (16M colors)
    /// \li compression: no.
    ///
    /// @note This is a slow operation, typically due to the use of
    ///         the file system, and partially because bmp files
    ///         are stored from the bottom up, and the memory is written
    ///         from the top down; as a result, it constantly 'seeks'
    ///         on the file system for the next row of information.
    ///
    /// As a performance test, a sample picture was timed. A family picture
    /// was converted to Bitmap format; shrunk to 352 x 272 pixels and save
    /// in 8-bit color format. The resulting file size was 94.5 KByte.
    /// The SPI port interface was set to 20 MHz.
    /// The original bitmap rendering software was purely in software, 
    /// pushing 1 pixel at a time to the write function, which did use SPI
    /// hardware (not pin wiggling) to transfer commands and data to the 
    /// display. Then, the driver was improved to leverage the capability
    /// of the derived display driver. As a final check, instead of the
    /// [known slow] local file system, a randomly chosen USB stick was 
    /// used. The performance results are impressive (but depend on the
    /// listed factors). 
    ///
    /// \li 34 seconds, LocalFileSystem, Software Rendering
    /// \li 9 seconds, LocalFileSystem, Hardware Rending for RA8875
    /// \li 3 seconds, MSCFileSystem, Hardware Rendering for RA8875
    /// 
    /// @param[in] x is the horizontal pixel coordinate
    /// @param[in] y is the vertical pixel coordinate
    /// @param[in] Name_BMP is the filename on the mounted file system.
    /// @returns success or error code.
    ///
    RetCode_t RenderBitmapFile(loc_t x, loc_t y, const char *Name_BMP);
    
    
    /// This method reads a disk file that is in ico format and 
    /// puts it on the screen.
    ///
    /// Reading the disk is slow, but a typical icon file is small
    /// so it should be ok.
    ///
    /// @note An Icon file can have more than one icon in it. This
    ///     implementation only processes the first image in the file.
    ///
    /// @param[in] x is the horizontal pixel coordinate
    /// @param[in] y is the vertical pixel coordinate
    /// @param[in] Name_ICO is the filename on the mounted file system.
    /// @returns success or error code.
    ///
    RetCode_t RenderIconFile(loc_t x, loc_t y, const char *Name_ICO);

    
    /// prints one character at the specified coordinates.
    ///
    /// This will print the character at the specified pixel coordinates.
    ///
    /// @param[in] x is the horizontal offset in pixels.
    /// @param[in] y is the vertical offset in pixels.
    /// @param[in] value is the character to print.
    /// @returns number of pixels to index to the right if a character was printed, 0 otherwise.
    ///
    virtual int character(int x, int y, int value);
    
    /// get the number of colums based on the currently active font
    ///
    /// @returns number of columns.
    ///    
    virtual int columns(void);

    /// get the number of rows based on the currently active font
    ///
    /// @returns number of rows.
    ///    
    virtual int rows(void);
    
    /// Select a User Font for all subsequent text.
    ///
    /// @note Tool to create the fonts is accessible from its creator
    ///     available at http://www.mikroe.com. 
    ///     For version 1.2.0.0, choose the "Export for TFT and new GLCD"
    ///     format.
    ///
    /// @param[in] font is a pointer to a specially formed font resource.
    /// @returns error code.
    ///
    virtual RetCode_t SelectUserFont(const uint8_t * font = NULL);


protected:

    /// Pure virtual method indicating the start of a graphics stream.
    ///
    /// This is called prior to a stream of pixel data being sent.
    /// This may cause register configuration changes in the derived
    /// class in order to prepare the hardware to accept the streaming
    /// data.
    ///
    /// @note this method must be supported in the derived class.
    ///
    /// @returns error code.
    ///
    virtual RetCode_t _StartGraphicsStream(void) = 0;
    
    /// Pure virtual method indicating the end of a graphics stream.
    ///
    /// This is called to conclude a stream of pixel data that was sent.
    /// This may cause register configuration changes in the derived
    /// class in order to stop the hardware from accept the streaming
    /// data.
    ///
    /// @note this method must be supported in the derived class.
    ///
    /// @returns error code.
    ///
    virtual RetCode_t _EndGraphicsStream(void) = 0;

    /// Protected method to render an image given a file handle and 
    /// coordinates.
    ///
    /// @param[in] x is the horizontal pixel coordinate
    /// @param[in] y is the vertical pixel coordinate
    /// @param[in] fileOffset is the offset into the file where the image data starts
    /// @param[in] Image is the filename stream already opened for the data.
    /// @returns success or error code.
    ///
    RetCode_t _RenderBitmap(loc_t x, loc_t y, uint32_t fileOffset, FILE * Image);

private:

    loc_t img_x;    /// x position of a rendered jpg
    loc_t img_y;    /// y position of a rendered jpg

    /// Analyze the jpeg data in preparation for decompression.
    ///
    JRESULT jd_prepare(JDEC * jd, uint16_t(* infunc)(JDEC * jd, uint8_t * buffer, uint16_t bufsize), void * pool, uint16_t poolsize, void * filehandle);
    
    /// Decompress the jpeg and render it.
    ///
    JRESULT jd_decomp(JDEC * jd, uint16_t(* outfunct)(JDEC * jd, void * stream, JRECT * rect), uint8_t scale);

    /// helper function to read data from the file system
    ///
    uint16_t privInFunc(JDEC * jd, uint8_t * buff, uint16_t ndata);

    /// helper function to read data from the file system
    ///
    uint16_t getJpegData(JDEC * jd, uint8_t *buff, uint16_t ndata);

    /// helper function to write data to the display
    ///
    uint16_t privOutFunc(JDEC * jd, void * bitmap, JRECT * rect);
    
    JRESULT mcu_output (
        JDEC * jd,   /* Pointer to the decompressor object */
        uint16_t (* outfunc)(JDEC * jd, void * stream, JRECT * rect),  /* RGB output function */
        uint16_t x,     /* MCU position in the image (left of the MCU) */
        uint16_t y      /* MCU position in the image (top of the MCU) */
    );

    int16_t bitext (    /* >=0: extracted data, <0: error code */
        JDEC * jd,   /* Pointer to the decompressor object */
        uint16_t nbit   /* Number of bits to extract (1 to 11) */
    );

    int16_t huffext (           /* >=0: decoded data, <0: error code */
        JDEC * jd,           /* Pointer to the decompressor object */
        const uint8_t * hbits,  /* Pointer to the bit distribution table */
        const uint16_t * hcode,  /* Pointer to the code word table */
        const uint8_t * hdata   /* Pointer to the data table */
    );

    JRESULT restart (
        JDEC * jd,   /* Pointer to the decompressor object */
        uint16_t rstn   /* Expected restert sequense number */
    );

    JRESULT mcu_load (
        JDEC * jd        /* Pointer to the decompressor object */
    );


protected:
    /// Pure virtual method to write a boolean stream to the display.
    ///
    /// This takes a bit stream in memory and using the current color settings
    /// it will stream it to the display. Along the way, each bit is translated
    /// to either the foreground or background color value and then that pixel
    /// is pushed onward.
    ///
    /// This is similar, but different, to the @ref pixelStream API, which is 
    /// given a stream of color values.
    /// 
    /// @param[in] x is the horizontal position on the display.
    /// @param[in] y is the vertical position on the display.
    /// @param[in] w is the width of the rectangular region to fill.
    /// @param[in] h is the height of the rectangular region to fill.
    /// @param[in] boolStream is the inline memory image from which to extract
    ///         the bitstream.
    /// @returns success/failure code. See @ref RetCode_t.
    ///
    virtual RetCode_t booleanStream(loc_t x, loc_t y, dim_t w, dim_t h, const uint8_t * boolStream) = 0;
    

    const unsigned char * font;     ///< reference to an external font somewhere in memory
    
    // pixel location
    short _x;                       ///< keeps track of current X location
    short _y;                       ///< keeps track of current Y location
    
    rect_t windowrect;              ///< window commands are held here for speed of access 
};

#endif

