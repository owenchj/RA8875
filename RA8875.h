///
/// @mainpage RA8875 Display Controller Driver library
///
/// The RA8875 Display controller is a powerful interface for low cost displays. It
/// can support displays up to 800 x 600 pixels x 16-bit color. Another common
/// implementation is 480 x 272 x 16 with two layers. The two layers can be
/// exchanged, or blended in various ways (transparency, OR, AND, and more).
/// It includes graphics acceleration capabilities for drawing primitives,
/// such as line, rectangle, circles, and more.
///
/// It is not a display for super-fast animations, video, picture frames and so forth,
/// at least when using the SPI ports. Performance has not been evaluated with one
/// of the parallel port options.
///
/// The controller additionally supports backlight control (via PWM), keypad scanning
/// (for a 4 x 5 matrix) and resistive touch-panel support. Recently support for a
/// capacitive touch screen was integrated, in a manner that makes the resistive and
/// capactive interfaces nearly identical.
///
/// @section Display_Config Display Configuration
///
/// This section details basics for bringing the display online. At a minimum,
/// the display is instantiated. After that any of the available commands
/// may be issued.
///
/// During the instantiation, the display is powered on, cleared, and the backlight
/// is energized. Additionally, the keypad and touchscreen features are activated.
/// It is important to keep in mind that the keypad had the default mapping, and
/// the touchscreen does not have the calibration matrix configured, so additional
/// steps may be necessary.
///
/// @code
/// RA8875 lcd(p5, p6, p7, p12, NC, "tft");
/// lcd.init();
/// lcd.foreground(Blue);
/// lcd.line(0,0, 479,271);
/// ...
/// @endcode
///
/// @section Touch_Panel Touch Panel
///
/// The supported touch panel interface is for a resistive panel, and is natively
/// supported by the RA8875 controller. There are a few steps to enable this interface.
///
/// @subsection Touch_Panel_Enable Touch Panel Enable
///
/// See @ref TouchPanelInit has two forms - fully automatic, and controlled. See the APIs for
/// details.
///
/// @subsection Touch_Panel_Calibration
///
/// The touch panel is not initially calibrated on startup. The application should
/// provide a means to activate the calibration process, and that should not require
/// the touchscreen as it may not yet be usable. Alternately, a calibration matrix
/// can be loaded from non-volatile and installed.
///
/// @section Keypad Keypad
///
/// The keypad has a default keypad mapping, but there is an API that permits
/// installing a custom keymap.
///
/// @todo Add APIs for the 2nd PWM channel, which might be quite useful as a simple
///     beeper.
/// @todo Figure out how to "init()" in the constructor. I ran into some issues if
///     the display was instantiated before main(), and the code would not run,
///     thus the exposure and activation of the init() function. If the constructor
///     was within main(), then it seemed to work as expected.
///
#ifndef RA8875_H
#define RA8875_H
#include <mbed.h>

#include "RA8875_Regs.h"
#include "GraphicsDisplay.h"

#define RA8875_DEFAULT_SPI_FREQ 5000000

// Define this to enable code that monitors the performance of various
// graphics commands.
//#define PERF_METRICS

// What better place for some test code than in here and the companion
// .cpp file. See also the bottom of this file.
//#define TESTENABLE

/// DOS colors - slightly color enhanced
#define Black       (color_t)(RGB(0,0,0))
#define Blue        (color_t)(RGB(0,0,187))
#define Green       (color_t)(RGB(0,187,0))
#define Cyan        (color_t)(RGB(0,187,187))
#define Red         (color_t)(RGB(187,0,0))
#define Magenta     (color_t)(RGB(187,0,187))
#define Brown       (color_t)(RGB(63,63,0))
#define Gray        (color_t)(RGB(187,187,187))
#define Charcoal    (color_t)(RGB(85,85,85))
#define BrightBlue  (color_t)(RGB(0,0,255))
#define BrightGreen (color_t)(RGB(0,255,0))
#define BrightCyan  (color_t)(RGB(0,255,255))
#define BrightRed   (color_t)(RGB(255,0,0))
#define Orange      (color_t)(RGB(255,85,85))
#define Pink        (color_t)(RGB(255,85,255))
#define Yellow      (color_t)(RGB(187,187,0))
#define White       (color_t)(RGB(255,255,255))

#define DarkBlue    (color_t)(RGB(0,0,63))
#define DarkGreen   (color_t)(RGB(0,63,0))
#define DarkCyan    (color_t)(RGB(0,63,63))
#define DarkRed     (color_t)(RGB(63,0,0))
#define DarkMagenta (color_t)(RGB(63,0,63))
#define DarkBrown   (color_t)(RGB(63,63,0))
#define DarkGray    (color_t)(RGB(63,63,63))

#define min(a,b) ((a<b)?a:b)
#define max(a,b) ((a>b)?a:b)


/// FT5206 definitions follow
#define FT5206_I2C_FREQUENCY                400000

#define FT5206_I2C_ADDRESS                  0x38
#define FT5206_NUMBER_OF_REGISTERS          31   // there are more registers, but this
                                                 // is enough to get all 5 touch coordinates.

#define FT5206_NUMBER_OF_TOTAL_REGISTERS    0xFE

#define FT5206_DEVICE_MODE                  0x00 // Normal, test, etc.
#define FT5206_GEST_ID                      0x01 // Gesture detected
#define FT5206_TD_STATUS                    0x02 // How many points detected (3:0). 1-5 is valid.

#define FT5206_TOUCH1_XH                    0x03 // Event Flag, Touch X Position
#define FT5206_TOUCH1_XL                    0x04
#define FT5206_TOUCH1_YH                    0x05 // Touch ID, Touch Y Position
#define FT5206_TOUCH1_YL                    0x06

#define FT5206_TOUCH2_XH                    0x09 // Event Flag, Touch X Position
#define FT5206_TOUCH2_XL                    0x0a
#define FT5206_TOUCH2_YH                    0x0b // Touch ID, Touch Y Position
#define FT5206_TOUCH2_YL                    0x0c

#define FT5206_TOUCH3_XH                    0x0f // Event Flag, Touch X Position
#define FT5206_TOUCH3_XL                    0x10
#define FT5206_TOUCH3_YH                    0x11 // Touch ID, Touch Y Position
#define FT5206_TOUCH3_YL                    0x12

#define FT5206_TOUCH4_XH                    0x15 // Event Flag, Touch X Position
#define FT5206_TOUCH4_XL                    0x16
#define FT5206_TOUCH4_YH                    0x17 // Touch ID, Touch Y Position
#define FT5206_TOUCH4_YL                    0x18

#define FT5206_TOUCH5_XH                    0x1b // Event Flag, Touch X Position
#define FT5206_TOUCH5_XL                    0x1c
#define FT5206_TOUCH5_YH                    0x1d // Touch ID, Touch Y Position
#define FT5206_TOUCH5_YL                    0x1e

// For typical usage, the registers listed below are not used.
#define FT5206_ID_G_THGROUP                 0x80 // Valid touching detect threshold
#define FT5206_ID_G_THPEAK                  0x81 // Valid touching peak detect threshold
#define FT5206_ID_G_THCAL                   0x82 // The threshold when calculating the focus of touching
#define FT5206_ID_G_THWATER                 0x83 // The threshold when there is surface water
#define FT5206_ID_G_THTEMP                  0x84 // The threshold of temperature compensation
#define FT5206_ID_G_CTRL                    0x86 // Power control mode
#define FT5206_ID_G_TIME_ENTER_MONITOR      0x87 // The timer of entering monitor status
#define FT5206_ID_G_PERIODACTIVE            0x88 // Period Active
#define FT5206_ID_G_PERIODMONITOR           0x89 // The timer of entering idle while in monitor status
#define FT5206_ID_G_AUTO_CLB_MODE           0xA0 // Auto calibration mode

#define FT5206_TOUCH_LIB_VERSION_H          0xA1 // Firmware Library Version H byte
#define FT5206_TOUCH_LIB_VERSION_L          0xA2 // Firmware Library Version L byte
#define FT5206_ID_G_CIPHER                  0xA3 // Chip vendor ID
#define FT5206_G_MODE                       0xA4 // The interrupt status to host
#define FT5206_ID_G_PMODE                   0xA5 // Power Consume Mode
#define FT5206_FIRMID                       0xA6 // Firmware ID
#define FT5206_ID_G_STATE                   0xA7 // Running State
#define FT5206_ID_G_FT5201ID                0xA8 // CTPM Vendor ID
#define FT5206_ID_G_ERR                     0xA9 // Error Code
#define FT5206_ID_G_CLB                     0xAA // Configure TP module during calibration in Test Mode
#define FT5206_ID_G_B_AREA_TH               0xAE // The threshold of big area
#define FT5206_LOG_MSG_CNT                  0xFE // The log MSG count
#define FT5206_LOG_CUR_CHA                  0xFF // Current character of log message, will point to the next
                                                 // character when one character is read.
#define FT5206_GEST_ID_MOVE_UP              0x10
#define FT5206_GEST_ID_MOVE_LEFT            0x14
#define FT5206_GEST_ID_MOVE_DOWN            0x18
#define FT5206_GEST_ID_MOVE_RIGHT           0x1c
#define FT5206_GEST_ID_ZOOM_IN              0x48
#define FT5206_GEST_ID_ZOOM_OUT             0x49
#define FT5206_GEST_ID_NO_GESTURE           0x00

#define FT5206_EVENT_FLAG_PUT_DOWN          0x00
#define FT5206_EVENT_FLAG_PUT_UP            0x01
#define FT5206_EVENT_FLAG_CONTACT           0x02
#define FT5206_EVENT_FLAG_RESERVED          0x03

#define FT5206_ID_G_POLLING_MODE            0x00
#define FT5206_ID_G_TRIGGER_MODE            0x01

#define FT5206_ID_G_PMODE_ACTIVE            0x00
#define FT5206_ID_G_PMODE_MONITOR           0x01
#define FT5206_ID_G_PMODE_HIBERNATE         0x03

#define FT5206_ID_G_STATE_CONFIGURE         0x00
#define FT5206_ID_G_STATE_WORK              0x01
#define FT5206_ID_G_STATE_CALIBRATION       0x02
#define FT5206_ID_G_STATE_FACTORY           0x03
#define FT5206_ID_G_STATE_AUTO_CALIBRATION  0x04
/// end of FT5206 definitions


//namespace SW_graphics
//{

class FPointerDummy;    // used by the callback methods.

/// This is a graphics library for the Raio RA8875 Display Controller chip
/// attached to a 4-wire SPI interface.
///
/// It offers both primitive and high level APIs.
///
/// Central to this API is a coordinate system, where the origin (0,0) is in
/// the top-left corner of the display, and the width (x) extends positive to the
/// right and the height (y) extends positive toward the bottom.
///
/// @note As there are both graphics and text commands, one must take care to use
/// the proper coordinate system for each. Some of the text APIs are in units
/// of column and row, which is measured in character positions (and dependent
/// on the font size), where other text APIs permit pixel level positioning.
///
/// @code
/// #include "RA8875.h"
/// RA8875 lcd(p5, p6, p7, p12, NC, "tft");
///
/// int main()
/// {
///     lcd.init();
///     lcd.printf("printing 3 x 2 = %d", 3*2);
///     lcd.circle(       400,25,  25,               BrightRed);
///     lcd.fillcircle(   400,25,  15,               RGB(128,255,128));
///     lcd.ellipse(      440,75,  35,20,            BrightBlue);
///     lcd.fillellipse(  440,75,  25,10,            Blue);
///     lcd.triangle(     440,100, 475,110, 450,125, Magenta);
///     lcd.filltriangle( 445,105, 467,111, 452,120, Cyan);
///     lcd.rect(         400,130, 475,155,          Brown);
///     lcd.fillrect(     405,135, 470,150,          Pink);
///     lcd.roundrect(    410,160, 475,190, 10,8,    Yellow);
///     lcd.fillroundrect(415,165, 470,185,  5,3,    Orange);
///     lcd.line(         430,200, 460,230,          RGB(0,255,0));
///     for (int i=0; i<=30; i+=5)
///         lcd.pixel(435+i,200+i, White);
/// }
/// @endcode
///
/// @todo Add Scroll support for text.
/// @todo Add Hardware reset signal - but testing to date indicates it is not needed.
/// @todo Add high level objects - x-y graph, meter, others... but these will
///     probably be best served in another class, since they may not
///     be needed for many uses.
///
class RA8875 : public GraphicsDisplay
{
public:
    /// cursor type to be shown as the text cursor.
    typedef enum
    {
        NOCURSOR,   ///< cursor is hidden
        IBEAM,      ///< | cursor
        UNDER,      ///< _ cursor
        BLOCK       ///< Block cursor
    } cursor_t;

    /// font type selection.
    typedef enum
    {
        ISO8859_1,      ///< ISO8859-1 font
        ISO8859_2,      ///< ISO8859-2 font
        ISO8859_3,      ///< ISO8859-3 font
        ISO8859_4       ///< ISO8859-4 font
    } font_t;

    /// display orientation
    typedef enum
    {
        normal,         ///< normal (landscape) orientation
        rotate_0 = normal,  ///< alternate to 'normal'
        rotate_90,      ///< rotated clockwise 90 degree
        rotate_180,     ///< rotated (clockwise) 180 degree
        rotate_270,     ///< rotated clockwise 270 degree
    } orientation_t;

    /// alignment
    typedef enum
    {
        align_none,     ///< align - none
        align_full      ///< align - full
    } alignment_t;

    /// Font Horizontal Scale factor - 1, 2, 3 4
    typedef int HorizontalScale;

    /// Font Vertical Scale factor - 1, 2, 3, 4
    typedef int VerticalScale;

    /// Clear screen region
    typedef enum
    {
        FULLWINDOW,     ///< Full screen
        ACTIVEWINDOW    ///< active window/region
    } Region_t;

    /// Set the Layer Display Mode. @ref SetLayerMode
    typedef enum
    {
        ShowLayer0,         ///< Only layer 0 is visible, layer 1 is hidden (default)
        ShowLayer1,         ///< Only layer 1 is visible, layer 0 is hidden
        LightenOverlay,     ///< Lighten-overlay mode
        TransparentMode,    ///< Transparent mode
        BooleanOR,          ///< Boolean OR mode
        BooleanAND,         ///< Boolean AND mode
        FloatingWindow      ///< Floating Window mode
    } LayerMode_T;

    /// Touch Panel modes
    typedef enum
    {
        TP_Auto,               ///< Auto touch detection mode
        TP_Manual,             ///< Manual touch detection mode
    } tpmode_t;

    /// printscreen callback commands
    typedef enum
    {
        OPEN,       ///< command to open the file. cast uint32_t * to the buffer to get the total size to be written.
        WRITE,      ///< command to write some data, buffer points to the data and the size is in bytes.
        CLOSE,      ///< command to close the file
    } filecmd_t;

    /// print screen callback
    ///
    /// The special form of the print screen will pass one blob at a time
    /// to the callback. There are basic command declaring that the stream
    /// can be opened, a block written, and the stream closed. There is
    /// also a command to communicate the total size being delivered.
    ///
    /// @code
    /// lcd.PrintScreen(x,y,w,h,callback);
    /// ...
    /// void callback(filecmd_t cmd, uint8_t * buffer, uint16_t size) {
    ///     switch(cmd) {
    ///         case OPEN:
    ///             pc.printf("About to write %u bytes\r\n", *(uint32_t *)buffer);
    ///             fh = fopen("file.bmp", "w+b");
    ///             break;
    ///         case WRITE:
    ///             fwrite(buffer, size, fh);
    ///             break;
    ///         case CLOSE:
    ///             fclose(fh);
    ///             break;
    ///         default:
    ///             pc.printf("Unexpected callback %d\r\n", cmd);
    ///             break;
    ///     }
    /// }
    /// @endcode
    ///
    /// @param cmd is the command to execute. See @ref filecmd_t.
    /// @param buffer is a pointer to the buffer being passed.
    /// @param size is the number of bytes in the buffer.
    /// @returns the noerror signal.
    ///
    typedef RetCode_t (* PrintCallback_T)(filecmd_t cmd, uint8_t * buffer, uint16_t size);

    /// Idle reason provided in the Idle Callback
    typedef enum {
        unknown,            ///< reason has not been assigned (this should not happen)
        status_wait,        ///< driver is polling the status register while busy
        command_wait,       ///< driver is polling the command register while busy
        getc_wait,          ///< user has called the getc function
        touch_wait,         ///< user has called the touch function
        touchcal_wait       ///< driver is performing a touch calibration
    } IdleReason_T;

    /// Idle Callback
    ///
    /// This defines the interface for an idle callback. That is, when the
    /// driver is held up, pending some event, it can call a previously registered
    /// idle function. This could be most useful for servicing a watchdog.
    ///
    /// The user code, which is notified via this API, can force the idle
    /// to abort, by returning the external_abort value back to the driver.
    /// It is important to note that the abort could leave the driver in
    /// an undesireable state, so this should be used with care.
    ///
    /// @note Should it be called the BusyCallback? It is true, that it will
    ///     call this function when the RA8875 is busy, but this is also
    ///     when the CPU is largely idle.
    ///
    /// @code
    /// RetCode_t myIdle_handler(RA8875::IdleReason_T reason)
    /// {
    ///     idleFlasher = !idleFlasher;
    ///     if (it_has_been_too_long())
    ///         return external_abort;
    ///     else
    ///         return noerror;
    /// }
    /// @endcode
    ///
    /// @param reason informs the callback why it is idle.
    /// @returns noerror to allow the driver continue waiting.
    /// @returns external_abort if the pending action should be aborted.
    ///
    typedef RetCode_t (* IdleCallback_T)(IdleReason_T reason);

    /// Basic constructor for a display based on the RAiO RA8875
    /// display controller, which can be used with no touchscreen,
    /// or the RA8875 managed resistive touchscreen.
    ///
    /// This constructor differs from the alternate by supportting
    /// either No Touch Screen, or the RA8875 built-in resistive
    /// touch screen. If the application requires the use of the
    /// capacitive touchscreen, the alternate constructor should
    /// be used.
    ///
    /// This configures the registers and calls the @ref init method.
    ///
    /// @code
    /// #include "RA8875.h"
    /// RA8875 lcd(p5, p6, p7, p12, NC, "tft");
    ///
    /// int main()
    /// {
    ///     lcd.init();
    ///     lcd.printf("printing 3 x 2 = %d", 3*2);
    ///     lcd.circle(400,25, 25, BrightRed);
    /// }
    /// @endcode
    ///
    /// @param[in] mosi is the SPI master out slave in pin on the mbed.
    /// @param[in] miso is the SPI master in slave out pin on the mbed.
    /// @param[in] sclk is the SPI shift clock pin on the mbed.
    /// @param[in] csel is the DigitalOut pin on the mbed to use as the
    ///         active low chip select for the display controller.
    /// @param[in] reset is the DigitalOut pin on the mbed to use as the
    ///         active low reset input on the display controller -
    ///         but this is not currently used.
    /// @param[in] name is a text name for this object, which will permit
    ///         capturing stdout to puts() and printf() directly to it.
    ///
    RA8875(PinName mosi, PinName miso, PinName sclk, PinName csel, PinName reset,
        const char * name = "lcd");


    /// Constructor for a display based on the RAiO RA8875
    /// display controller (use for TouchScreen: Capacitive only)
    ///
    /// This constructor differs from the alternate by including support
    /// for the Capactive Touch screen.
    ///
    /// @code
    /// #include "RA8875.h"
    /// RA8875 lcd(p5, p6, p7, p12, NC, p9,p10,p13, "tft");
    ///
    /// int main()
    /// {
    ///     lcd.init();
    ///     lcd.printf("printing 3 x 2 = %d", 3*2);
    ///     lcd.circle(400,25, 25, BrightRed);
    ///     TouchCode_t tp = lcd.TouchPanelReadable();
    ///     if (tp == touch)
    ///         ...
    /// }
    /// @endcode
    ///
    /// @param[in] mosi is the SPI master out slave in pin on the mbed.
    /// @param[in] miso is the SPI master in slave out pin on the mbed.
    /// @param[in] sclk is the SPI shift clock pin on the mbed.
    /// @param[in] csel is the DigitalOut pin on the mbed to use as the
    ///         active low chip select for the display controller.
    /// @param[in] reset is the DigitalOut pin on the mbed to use as the
    ///         active low reset input on the display controller -
    ///         but this is not currently used.
    /// @param[in] sda is the I2C Serial Data pin you are wiring to the FT5206.
    /// @param[in] scl is the I2C Serial Clock pin you are wiring to the FT5206.
    /// @param[in] irq is the Interrupt Request pin you are wiring to the FT5206.
    /// @param[in] name is a text name for this object, which will permit
    ///         capturing stdout to puts() and printf() directly to it.
    ///
    RA8875(PinName mosi, PinName miso, PinName sclk, PinName csel, PinName reset,
        PinName sda, PinName scl, PinName irq, const char * name = "lcd");


    // Destructor doesn't have much to do as this would typically be created
    // at startup, and not at runtime.
    //~RA8875();

    /// Initialize the driver.
    ///
    /// The RA8875 can control typical displays from the 480x272 to 800x480, and it supports 8 or 16-bit color.
    /// It also supports 2 graphics layers, but it cannot support 2 layers at the maximum color depth and
    /// screen size. When configured under 480x400, it will support both 16-bit color depth and 2 drawing layers.
    /// Above 480x400 it support either 16-bit color, or 2 layers, but not both.
    ///
    /// Typical of the displays that are readily purchased, you will find 480x272 and 800x480 resolutions.
    ///
    /// @param[in] width in pixels to configure the display for. This parameter is optional
    ///             and the default is 480.
    /// @param[in] height in pixels to configure the display for. This parameter is optional
    ///             and the default is 272.
    /// @param[in] color_bpp can be either 8 or 16, but must be consistent
    ///             with the width and height parameters. This parameter is optional
    ///             and the default is 16.
    /// @param[in] poweron defines if the display should be initialized into the power-on or off state.
    ///            If power is non-zero(on), the backlight is set to this value. This parameter is optional
    ///             and the default is 255 (on and full brightness). See @ref Power.
    /// @param[in] keypadon defines if the keypad support should be enabled. This parameter is optional
    ///             and the default is true (enabled). See @ref KeypadInit.
    /// @param[in] touchscreeenon defines if the touchscreen support should be enabled.
    ///             This parameter is optional and the default is true (enabled). See @ref TouchPanelInit.
    ///             - If the constructor was called with support for the capacitive driver, this
    ///             parameter causes the driver to initialize.
    ///             - If the constructor was called without support for the capacitive driver, this
    ///             parameter is used to enable and initialize the resistive touchscreen driver.
    /// @returns success/failure code. See @ref RetCode_t.
    ///
    RetCode_t init(int width = 480, int height = 272, int color_bpp = 16,
        uint8_t poweron = 255, bool keypadon = true, bool touchscreeenon = true);


    /// Get a pointer to the error code.
    ///
    /// This method returns a pointer to a text string that matches the
    /// code. See @ref RetCode_t.
    ///
    /// @param[in] code is the return value from RetCode_t to look up.
    /// @returns a pointer to the text message representing code. If code
    ///     is not a valid value, then it returns the text for bad_parameter;
    ///
    const char * GetErrorMessage(RetCode_t code);


    /// Select the drawing layer for subsequent commands.
    ///
    /// If the screen configuration is 480 x 272, or if it is 800 x 480
    /// and 8-bit color, the the display supports two layers, which can
    /// be independently drawn on and shown. Additionally, complex
    /// operations involving both layers are permitted.
    ///
    /// @attention If the current display configuration does not support
    ///     multiple layers, then layer 0 will be selected.
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
    virtual RetCode_t SelectDrawingLayer(uint16_t layer, uint16_t * prevLayer = NULL);


    /// Get the currently active drawing layer.
    ///
    /// This returns a value, 0 or 1, based on the screen configuration
    /// and the currently active drawing layer.
    ///
    /// @code
    ///     uint16_t prevLayer = lcd.GetDrawingLayer();
    ///     lcd.SelectDrawingLayer(x);
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
    virtual uint16_t GetDrawingLayer(void);


    /// Set the Layer presentation mode.
    ///
    /// This sets the presentation mode for layers, and permits showing
    /// a single layer, or applying a mode where the two layers
    /// are combined using one of the hardware methods.
    ///
    /// Refer to the RA8875 data sheet for full details.
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
    /// @param[in] mode sets the mode in the Layer Transparency Register.
    /// @returns success/failure code. See @ref RetCode_t.
    ///
    RetCode_t SetLayerMode(LayerMode_T mode);


    /// Get the Layer presentation mode.
    ///
    /// This gets the current layer mode. See @ref LayerMode_T.
    ///
    /// @returns layer mode.
    ///
    LayerMode_T GetLayerMode(void);


    /// Set the layer transparency for each layer.
    ///
    /// Set the transparency, where the range of values is
    /// from zero (fully visible) to eight (fully transparent).
    /// The input value is automatically limited to this range.
    ///
    /// @code
    ///     // draw something on each layer, then step-fade across
    ///     display.SetLayerMode(RA8875::TransparentMode);
    ///     for (i=0; i<=8; i++) {
    ///         display.SetLayerTransparency(i, 8-i);
    ///         wait_ms(200);
    ///     }
    /// @endcode
    ///
    /// @param[in] layer1 sets the layer 1 transparency.
    /// @param[in] layer2 sets the layer 2 transparency.
    /// @returns success/failure code. See @ref RetCode_t.
    ///
    RetCode_t SetLayerTransparency(uint8_t layer1, uint8_t layer2);


    /// Set the background color register used for transparency.
    ///
    /// This command sets the background color registers that are used
    /// in the transparent color operations involving the layers.
    ///
    /// @param[in] color is optional and expressed in 16-bit format. If not
    ///     supplied, a default of Black is used.
    /// @returns success/failure code. See @ref RetCode_t.
    ///
    RetCode_t SetBackgroundTransparencyColor(color_t color = RGB(0,0,0));


    /// Get the background color value used for transparency.
    ///
    /// This command reads the background color registers that define
    /// the transparency color for operations involving layers.
    ///
    /// @returns the color.
    ///
    color_t GetBackgroundTransparencyColor(void);


    /// Initialize theTouch Panel controller with default values
    ///
    /// This activates the simplified touch panel init, which may work for
    /// most uses. The alternate API is available if fine-grained control
    /// of the numerous settings of the resistive panel is needed.
    ///
    /// @returns success/failure code. See @ref RetCode_t.
    ///
    RetCode_t TouchPanelInit(void);


    /// Initialize the Touch Panel controller with detailed settings.
    ///
    /// This is the detailed touch panel init, which provides the ability
    /// to set nearly every option.
    ///
    /// @note If the capacitive touch panel was constructed, this behaves
    ///     the same as the simplified version.
    ///
    /// @param[in]  bTpEnable           Touch Panel enable/disable control:
    ///                                 - TP_ENABLE: enable the touch panel
    ///                                 - TP_DISABLE: disable the touch panel
    /// @param[in]  bTpAutoManual       Touch Panel operating mode:
    ///                                 - TP_MODE_AUTO: automatic capture
    ///                                 - TP_MODE_MANUAL: manual capture
    /// @param[in]  bTpDebounce         Debounce circuit enable for touch panel interrupt:
    ///                                 - TP_DEBOUNCE_OFF: disable the debounce circuit
    ///                                 - TP_DEBOUNCE_ON: enable the debounce circuit
    /// @param[in]  bTpManualMode       When Manual Mode is selected, this sets the mode:
    ///                                 - TP_MANUAL_IDLE: touch panel is idle
    ///                                 - TP_MANUAL_WAIT: wait for touch panel event
    ///                                 - TP_MANUAL_LATCH_X: latch X data
    ///                                 - TP_MANUAL_LATCH_Y: latch Y data
    /// @param[in]  bTpAdcClkDiv        Sets the ADC clock as a fraction of the System CLK:
    ///                                 - TP_ADC_CLKDIV_1: Use CLK
    ///                                 - TP_ADC_CLKDIV_2: Use CLK/2
    ///                                 - TP_ADC_CLKDIV_4: Use CLK/4
    ///                                 - TP_ADC_CLKDIV_8: Use CLK/8
    ///                                 - TP_ADC_CLKDIV_16: Use CLK/16
    ///                                 - TP_ADC_CLKDIV_32: Use CLK/32
    ///                                 - TP_ADC_CLKDIV_64: Use CLK/64
    ///                                 - TP_ADC_CLKDIV_128: Use CLK/128
    /// @param[in]  bTpAdcSampleTime    Touch Panel sample time delay before ADC data is ready:
    ///                                 - TP_ADC_SAMPLE_512_CLKS: Wait 512 system clocks
    ///                                 - TP_ADC_SAMPLE_1024_CLKS: Wait 1024 system clocks
    ///                                 - TP_ADC_SAMPLE_2048_CLKS: Wait 2048 system clocks
    ///                                 - TP_ADC_SAMPLE_4096_CLKS: Wait 4096 system clocks
    ///                                 - TP_ADC_SAMPLE_8192_CLKS: Wait 8192 system clocks
    ///                                 - TP_ADC_SAMPLE_16384_CLKS: Wait 16384 system clocks
    ///                                 - TP_ADC_SAMPLE_32768_CLKS: Wait 32768 system clocks
    ///                                 - TP_ADC_SAMPLE_65536_CLKS: Wait 65536 system clocks
    /// @returns success/failure code. See @ref RetCode_t.
    ///
    RetCode_t TouchPanelInit(uint8_t bTpEnable, uint8_t bTpAutoManual, uint8_t bTpDebounce,
        uint8_t bTpManualMode, uint8_t bTpAdcClkDiv, uint8_t bTpAdcSampleTime);


    /// Get the screen calibrated point of touch.
    ///
    /// This method determines if there is a touch and if so it will provide
    /// the screen-relative touch coordinates. This method can be used in
    /// a manner similar to Serial.readable(), to determine if there was a
    /// touch and indicate that - but not care about the coordinates. Alternately,
    /// if a valid pointer to a point_t is provided, then if a touch is detected
    /// the point_t will be populated with data.
    ///
    /// @code
    ///     Timer t;
    ///     t.start();
    ///     do {
    ///        point_t point = {0, 0};
    ///        if (display.TouchPanelReadable(&point)) {
    ///            display.pixel(point, Red);
    ///        }
    ///    } while (t.read_ms() < 30000);
    /// @endcode
    ///
    /// @param[out] TouchPoint is a pointer to a point_t, which is set as the touch point, if a touch is registered.
    /// @returns a value indicating the state of the touch,
    ///         - no_cal:   no calibration matrix is available, touch coordinates are not returned.
    ///         - no_touch: no touch is detected, touch coordinates are not returned.
    ///         - touch:    touch is detected, touch coordinates are returned.
    ///         - held:     held after touch, touch coordinates are returned.
    ///         - release:  indicates a release, touch coordinates are returned.
    ///
    TouchCode_t TouchPanelReadable(point_t * TouchPoint = NULL);


    /// Get the reported touch gesture, if any.
    ///
    /// If it could detect a gesture, it will return a value based on
    /// the interpreted gesture.
    ///
    /// Valid gesture values are:
    /// @li 0x00 No gesture
    /// @li 0x48 Zoom in
    /// @li 0x49 Zoom out
    ///
    /// The following gestures are defined in the FT5206 specification, but
    /// do not appear to work.
    /// @li 0x10 Move up
    /// @li 0x14 Move left
    /// @li 0x18 Move down
    /// @li 0x1C Move right
    ///
    /// @returns gesture information.
    ///
    uint8_t TouchGesture(void) { return gesture; }


    /// Get the count of registered touches.
    ///
    /// @returns count of touch points to communicate; 0 to 5.
    ///
    int TouchCount(void) { return numberOfTouchPoints; }


    /// Get the count of possible touch channels.
    ///
    /// @returns count of touch channels supported by the hardware.
    ///
    int TouchChannels(void);


    /// Get the Touch ID value for a specified touch channel.
    ///
    /// Touch ID is a tracking number based on the order of the touch
    /// detections. The first touch is ID 0, the next is ID 1, and
    /// so on. If the first touch is lifted (no touch), the touch count
    /// decrements, and the remaining touch is communicated on
    /// touch channel zero, even as the Touch ID remains as originally
    /// reported (1 in this example). In this way, it is easy to track
    /// a specific touch.
    ///
    /// It is possible to query the data for a channel that is not
    /// presently reported as touched.
    ///
    /// @param[in] channel is the touch channel, from 0 to 4, or 0 to getTouchCount()-1
    ///     It defaults to 0, in case the user is not interested in multi-touch.
    /// @returns the touch ID, or 15 if you get the ID for an untouched channel.
    /// @returns 0 if an invalid channel is queried.
    ///
    uint8_t TouchID(uint8_t channel = 0) { return (channel < 5) ? touchInfo[channel].touchID : touchInfo[0].touchID; }


    /// Get the Touch Code for a touch channel.
    ///
    /// It is possible to query the data for a channel that is not
    /// presently reported as touched.
    ///
    /// @param[in] channel is the touch channel, from 0 to 4, or 0 to getTouchCount()-1
    ///     It defaults to 0, in case the user is not interested in multi-touch.
    /// @returns the touch code (@ref TouchCode_t).
    /// @returns channel 0 information if an invalid channel is queried.
    ///
    TouchCode_t TouchCode(uint8_t channel = 0) { return (channel < 5) ? touchInfo[channel].touchCode : touchInfo[0].touchCode; }


    /// Get the coordinates for a touch channel.
    ///
    /// This returns the (X,Y) coordinates for a touch channel.
    ///
    ///
    /// It is possible to query the data for a channel that is not
    /// presently reported as touched.
    ///
    /// @param[in] channel is an optional touch channel, from 0 to 4, or 0 to getTouchCount()-1.
    ///     It defaults to 0, in case the user is not interested in multi-touch.
    /// @returns the coordinates as a point_t structure.
    /// @returns channel 0 information if an invalid channel is queried.
    ///
    point_t TouchCoordinates(uint8_t channel = 0)  { return (channel < 5) ? touchInfo[channel].coordinates : touchInfo[0].coordinates; }


    /// Poll the TouchPanel and on a touch event return the a to d filtered x, y coordinates.
    ///
    /// This method reads the touch controller, which has a 10-bit range for each the
    /// x and the y axis.
    ///
    /// @note The returned values are not in display (pixel) units but are in analog to
    ///     digital converter units.
    ///
    /// @note This API is usually not needed and is likely to be deprecated.
    ///     See @ref TouchPanelComputeCalibration.
    ///     See @ref TouchPanelReadable.
    ///
    /// @param[out] x is the x scale a/d value.
    /// @param[out] y is the y scale a/d value.
    /// @returns a value indicating the state of the touch,
    ///         - no_cal:   no calibration matrix is available, touch coordinates are not returned.
    ///         - no_touch: no touch is detected, touch coordinates are not returned.
    ///         - touch:    touch is detected, touch coordinates are returned.
    ///         - held:     held after touch, touch coordinates are returned.
    ///         - release:  indicates a release, touch coordinates are returned.
    ///
    TouchCode_t TouchPanelA2DFiltered(int *x, int *y);


    /// Poll the TouchPanel and on a touch event return the a to d raw x, y coordinates.
    ///
    /// This method reads the touch controller, which has a 10-bit range for each the
    /// x and the y axis. A number of samples of the raw data are taken, filtered,
    /// and the results are returned.
    ///
    /// @note The returned values are not in display (pixel) units but are in analog to
    ///     digital converter units.
    ///
    /// @note This API is usually not needed and is likely to be deprecated.
    ///     See @ref TouchPanelComputeCalibration.
    ///     See @ref TouchPanelReadable.
    ///
    /// @param[out] x is the x scale a/d value.
    /// @param[out] y is the y scale a/d value.
    /// @returns a value indicating the state of the touch,
    ///         - no_cal:   no calibration matrix is available, touch coordinates are not returned.
    ///         - no_touch: no touch is detected, touch coordinates are not returned.
    ///         - touch:    touch is detected, touch coordinates are returned.
    ///         - held:     held after touch, touch coordinates are returned.
    ///         - release:  indicates a release, touch coordinates are returned.
    ///
    TouchCode_t TouchPanelA2DRaw(int *x, int *y);


    /// Wait for a touch panel touch and return it.
    ///
    /// This method is similar to Serial.getc() in that it will wait for a touch
    /// and then return. In order to extract the coordinates of the touch, a
    /// valid pointer to a point_t must be provided.
    ///
    /// @note There is no timeout on this function, so its use is not recommended.
    ///
    /// @code
    ///     Timer t;
    ///     t.start();
    ///     do {
    ///        point_t point = {0, 0};
    ///        display.TouchPanelGet(&point);   // hangs here until touch
    ///        display.pixel(point, Red);
    ///    } while (t.read_ms() < 30000);
    /// @endcode
    ///
    /// @param[out] TouchPoint is the touch point, if a touch is registered.
    /// @returns a value indicating the state of the touch,
    ///         - no_cal:   no calibration matrix is available, touch coordinates are not returned.
    ///         - no_touch: no touch is detected, touch coordinates are not returned.
    ///         - touch:    touch is detected, touch coordinates are returned.
    ///         - held:     held after touch, touch coordinates are returned.
    ///         - release:  indicates a release, touch coordinates are returned.
    ///
    TouchCode_t TouchPanelGet(point_t * TouchPoint);


    /// Calibrate the touch panel.
    ///
    /// This method accepts two lists - one list is target points in ,
    /// display coordinates and the other is a lit of raw touch coordinate
    /// values. It generates a calibration matrix for later use. This
    /// matrix is also accessible to the calling API, which may store
    /// the matrix in persistent memory and then install the calibration
    /// matrix on the next power cycle. By doing so, it can avoid the
    /// need to calibrate on every power cycle.
    ///
    /// @note The methods "TouchPanelComputeCalibration", "TouchPanelReadable", and
    ///     indirectly the "TouchPanelSetMatrix" methods are all derived
    ///     from a program by Carlos E. Vidales. See the copyright note
    ///     for further details. See also the article
    ///     http://www.embedded.com/design/system-integration/4023968/How-To-Calibrate-Touch-Screens
    ///
    /// @copyright Copyright (c) 2001, Carlos E. Vidales. All rights reserved.
    ///     This sample program was written and put in the public domain
    ///      by Carlos E. Vidales.  The program is provided "as is"
    ///      without warranty of any kind, either expressed or implied.
    ///     If you choose to use the program within your own products
    ///      you do so at your own risk, and assume the responsibility
    ///      for servicing, repairing or correcting the program should
    ///      it prove defective in any manner.
    ///     You may copy and distribute the program's source code in any
    ///      medium, provided that you also include in each copy an
    ///      appropriate copyright notice and disclaimer of warranty.
    ///     You may also modify this program and distribute copies of
    ///      it provided that you include prominent notices stating
    ///      that you changed the file(s) and the date of any change,
    ///      and that you do not charge any royalties or licenses for
    ///      its use.
    ///
    /// @param[in] display is a pointer to a set of 3 points, which
    ///             are in display units of measure. These are the targets
    ///             the calibration was aiming for.
    /// @param[in] screen is a pointer to a set of 3 points, which
    ///             are in touchscreen units of measure. These are the
    ///             registered touches.
    /// @param[out] matrix is an optional parameter to hold the calibration matrix
    ///             as a result of the calibration. This can be saved in
    ///             non-volatile memory to recover the calibration after a power fail.
    /// @returns success/failure code. See @ref RetCode_t.
    ///
    RetCode_t TouchPanelComputeCalibration(point_t display[3], point_t screen[3], tpMatrix_t * matrix);


    /// Perform the touch panel calibration process.
    ///
    /// This method provides the easy "shortcut" to calibrating the touch panel.
    /// The process will automatically generate the calibration points, present
    /// the targets on-screen, detect the touches, compute the calibration
    /// matrix, and optionally provide the calibration matrix to the calling code
    /// for persistence in non-volatile memory.
    ///
    /// @param[out] matrix is an optional parameter to hold the calibration matrix
    ///             as a result of the calibration. This can be saved in
    ///             non-volatile memory to recover the calibration after a power fail.
    /// @returns success/failure code. See @ref RetCode_t.
    ///
    RetCode_t TouchPanelCalibrate(tpMatrix_t * matrix = NULL);


    /// Perform the touch panel calibration process.
    ///
    /// This method provides the easy "shortcut" to calibrating the touch panel.
    /// The process will automatically generate the calibration points, present
    /// the targets on-screen, detect the touches, compute the calibration
    /// matrix, and optionally provide the calibration matrix to the calling code
    /// for persistence in non-volatile memory.
    ///
    /// @param[in] msg is a text message to present on the screen during the
    ///             calibration process.
    /// @param[out] matrix is an optional parameter to hold the calibration matrix
    ///             as a result of the calibration. This can be saved in
    ///             non-volatile memory to recover the calibration after a power fail.
    /// @param[in] maxwait_s is the maximum number of seconds to wait for a touch
    ///             calibration. If no touch panel installed, it then reports
    ///             touch_cal_timeout.
    /// @returns success/failure code. See @ref RetCode_t.
    ///
    RetCode_t TouchPanelCalibrate(const char * msg, tpMatrix_t * matrix = NULL, int maxwait_s = 15);


    /// Set the calibration matrix for the touch panel.
    ///
    /// This method is used to set the calibration matrix for the touch panel. After
    /// performing the calibration (See @ref TouchPanelComputeCalibration), the matrix can be stored.
    /// On a subsequence power cycle, the matrix may be restored from non-volatile and
    /// passed in to this method. It will then be held to perform the corrections when
    /// reading the touch panel point.
    ///
    /// @code
    /// FILE * fh = fopen("/local/tpmatrix.cfg", "r");
    /// if (fh) {
    ///     tpMatrix_t matrix;
    ///     if (fread(fh, &matrix, sizeof(tpMatrix_t))) {
    ///         lcd.TouchPanelSetMatrix(&matrix);
    ///     }
    ///     fclose(fh);
    /// }
    /// @endcode
    ///
    /// @param[in] matrix is a pointer to the touch panel calibration matrix.
    /// @returns success/failure code. See @ref RetCode_t.
    ///
    RetCode_t TouchPanelSetMatrix(tpMatrix_t * matrix);


#if 0
    /// Append interrupt handler for specific RA8875 interrupt source
    ///
    /// @param[in]    bISRType        Interrupt Source, should be:
    ///                                - RA8875_INT_KEYSCAN: KEYCAN interrupt
    ///                                - RA8875_INT_DMA: DMA interrupt
    ///                                - RA8875_INT_TP: Touch panel interrupt
    ///                                - RA8875_INT_BTE: BTE process complete interrupt
    ///                                - RA8875_INT_BTEMCU_FONTWR: Multi-purpose interrupt (see spec sheet)
    /// @param[in]    fptr is a callback function to handle the interrupt event.
    /// @returns       none
    ///
    void AppendISR(uint8_t bISRType, void(*fptr)(void));

    /// Unappend interrupt handler for specific RA8875 interrupt source
    ///
    /// @param[in]    bISRType        Interrupt Source, should be:
    ///                                - RA8875_INT_KEYSCAN: KEYCAN interrupt
    ///                                - RA8875_INT_DMA: DMA interrupt
    ///                                - RA8875_INT_TP: Touch panel interrupt
    ///                                - RA8875_INT_BTE: BTE process complete interrupt
    ///                                - RA8875_INT_BTEMCU_FONTWR: Multi-purpose interrupt (see spec sheet)
    /// @return       none
    ///
    void UnAppendISR(uint8_t bISRType);
#endif


    /// Initialize the keypad interface on the RA8875 controller.
    ///
    /// Enables the keypad subsystem. It will scan the 4 x 5 matrix
    /// and make available key presses.
    ///
    /// @note See section 5-13 of RAIO RA8875 data sheet for more details.
    /// @note When using the display from buy-display.com, be sure that
    ///     the option for the keypad is configured on the hardware.
    ///
    /// All parameters are optional.
    /// @param[in] scanEnable when true, enables the key scan function (default: true).
    /// @param[in] longDetect when true, additionally enables the long key held detection (default: false).
    /// @param[in] sampleTime setting (range: 0 - 3, default: 0).
    /// @param[in] scanFrequency setting (range: 0 - 7, default: 0).
    /// @param[in] longTimeAdjustment (range: 0 - 3, default: 0).
    /// @param[in] interruptEnable when true, enables interrupts from keypress (default: false).
    /// @param[in] wakeupEnable when true, activates the wakeup function (default: false).
    ///
    /// @returns success/failure code. See @ref RetCode_t.
    ///
    RetCode_t  KeypadInit(bool scanEnable = true, bool longDetect = false,
        uint8_t sampleTime = 0, uint8_t scanFrequency = 0,
        uint8_t longTimeAdjustment = 0,
        bool interruptEnable = false, bool wakeupEnable = false);


    /// Create Key Code definitions for the key matrix.
    ///
    /// This API provides a table of 22 key-code assignments for the matrix of keys.
    /// This can be used to translate the keys 1 - 20 into some other value, as
    /// well as to communicate the "no key" (zero) and "error state" (21).
    ///
    /// In this way, a keypad could easily emulate a piece of a keyboard, transforming
    /// 0 - 20 into the values 0, '0', '1', '2', '3', '4', '5', '6', '7', '8',
    /// '9', '+', '-', '*' , '/', '=', '(bs)', '(cr)', and so on...
    ///
    /// @code
    /// //        Return Value by Row, Column   Example reassignment
    /// //    Column    0    1    2    3    4
    /// //          +-------------------------+  +-------------------------+
    /// // Row   0  |   1    2    3    4    5 |  | '7'  '8'  '9'  ',' '<-' |
    /// //       1  |   6    7    8    9   10 |  | '4'  '5'  '6'  '/'  '-' |
    /// //       2  |  11   12   13   14   15 |  | '1'  '2'  '3'  '*'  '+' |
    /// //       3  |  16   17   18   19   20 |  | '0'  '.'  '('  ')' '\n' |
    /// //          +-------------------------+  +-------------------------+
    /// //     Return value  0 = No Key pressed
    /// //     Return value 21 = Error
    /// const uint8_t CodeList[22] =
    ///     {0, '7', '8', '9', ',', '\h',
    ///         '4', '5', '6', '/', '-',
    ///         '1', '2', '3', '*', '+',
    ///         '0', '.', '(', ')', '\n',
    ///         '\x1b'};
    ///     lcd.SetKeyMap(CodeList);
    /// @endcode
    ///
    /// @param[in] CodeList is a pointer to an always available byte-array
    ///             where the first 22 bytes are used as the transformation
    ///             from raw code to your reassigned value.
    ///            If CodeList is NULL, the original raw value key map is
    ///             restored.
    /// @returns noerror.
    ///
    RetCode_t SetKeyMap(const uint8_t * CodeList = NULL);


    /// Determine if a key has been hit
    ///
    /// @returns true if a key has been hit
    ///
    bool readable();


    /// Blocking read of the keypad.
    ///
    /// @note: This is a blocking read, so it is important to first call _kbhit()
    ///         to avoid hanging your processes.
    ///
    /// A keypad connected to the RA8875 is connected in a matrix of 4 rows and 5 columns.
    /// When pressed, this method will return a code in the range of 1 through 20, reserving
    /// the value 0 to indicate that no key is pressed.
    ///
    /// Additionally, if configured to detect a "long press", bit 7 will be set to indicate
    /// this. In this situation, first a "normal press" would be detected and signaled and
    /// soon after that a "long press" of the same key would be detected and communicated.
    ///
    /// @return 8-bit where bit 7 indicates a long press. The remaining bits indicate the
    ///     keypress using 0 = no key pressed, 1 - 20 = the key pressed.
    ///
    uint8_t getc();


    /// Determine if a point is within a rectangle.
    ///
    /// @param[in] rect is a rectangular region to use.
    /// @param[in] p is a point to analyze to see if it is within the rect.
    /// @returns true if p is within rect.
    ///
    bool Intersect(rect_t rect, point_t p);

    /// Determine if a rectangle intersects another rectangle.
    ///
    /// @param[in] rect1 is a rectangular region.
    /// @param[in] rect2 is a second rectangular region.
    /// @returns true if any part of rect2 intersects rect1.
    ///
    bool Intersect(rect_t rect1, rect_t rect2);

    /// Determine if a rectangle intersects another rectangle and provides
    /// the area of intersection.
    ///
    /// @code
    ///     +---------------------+
    ///     | rect1               |
    ///     |                     |
    ///     |          +------------------+
    ///     |          | rect3    |       |
    ///     |          |          |       |
    ///     +---------------------+       |
    ///                | rect2            |
    ///                +------------------+
    /// @endcode
    ///
    /// @note that the first parameter is a pointer to a rect and the
    ///
    /// @param[inout] pRect1 is a pointer to a rectangular region, and returns
    ///             the area of intersection.
    /// @param[in] pRect2 is a pointer to a second rectangular region.
    /// @returns true if pRect1 and pRect2 intersect and pRect1 is written with
    ///             the rectangle describing the intersection.
    ///
    bool Intersect(rect_t * rect1, const rect_t * rect2);


    /// Write a command to the display with a word of data.
    ///
    /// This is a high level command, and may invoke several primitives.
    ///
    /// @param[in] command is the command to write.
    /// @param[in] data is data to be written to the command register.
    /// @returns success/failure code. See @ref RetCode_t.
    ///
    RetCode_t WriteCommandW(uint8_t command, uint16_t data);


    /// Write a command to the display
    ///
    /// This is a high level command, and may invoke several primitives.
    ///
    /// @param[in] command is the command to write.
    /// @param[in] data is optional data to be written to the command register
    ///     and only occurs if the data is in the range [0 - 0xFF].
    /// @returns success/failure code. See @ref RetCode_t.
    ///
    virtual RetCode_t WriteCommand(unsigned char command, unsigned int data = 0xFFFF);


    /// Write a data word to the display
    ///
    /// This is a high level command, and may invoke several primitives.
    ///
    /// @param[in] data is the data to write.
    /// @returns success/failure code. See @ref RetCode_t.
    ///
    RetCode_t WriteDataW(uint16_t data);


    /// Write a data byte to the display
    ///
    /// This is a high level command, and may invoke several primitives.
    ///
    /// @param[in] data is the data to write.
    /// @returns success/failure code. See @ref RetCode_t.
    ///
    virtual RetCode_t WriteData(unsigned char data);


    /// Read a command register
    ///
    /// @param[in] command is the command register to read.
    /// @returns the value read from the register.
    ///
    unsigned char ReadCommand(unsigned char command);


    /// Read a word from a command register
    ///
    /// @param[in] command is the command register to read.
    /// @returns the value read from the register.
    ///
    uint16_t ReadCommandW(unsigned char command);


    /// Read a data byte from the display
    ///
    /// This is a high level command, and may invoke several primitives.
    ///
    /// @returns data that was read.
    ///
    unsigned char ReadData(void);


    /// Read a word from the display
    ///
    /// This is a high level command, and may invoke several primitives.
    ///
    /// @returns data that was read.
    ///
    uint16_t ReadDataW(void);


    /// Read the display status
    ///
    /// This is a high level command, and may invoke several primitives.
    ///
    /// @returns data that was read.
    ///
    unsigned char ReadStatus(void);


    /// get the width in pixels of the currently active font
    ///
    /// @returns font width in pixels.
    ///
    dim_t fontwidth(void);


    /// get the height in pixels of the currently active font
    ///
    /// @returns font height in pixels.
    ///
    dim_t fontheight(void);


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


    /// get the screen width in pixels
    ///
    /// @returns screen width in pixels.
    ///
    virtual dim_t width(void);


    /// get the screen height in pixels
    ///
    /// @returns screen height in pixels.
    ///
    virtual dim_t height(void);


    /// get the color depth in bits per pixel.
    ///
    /// @returns 8 or 16 only.
    ///
    virtual dim_t color_bpp(void);

    /// Set cursor position based on the current font size.
    ///
    /// @param[in] column is the horizontal position in character positions
    /// @param[in] row is the vertical position in character positions
    /// @returns success/failure code. See @ref RetCode_t.
    ///
    virtual RetCode_t locate(textloc_t column, textloc_t row);


    /// Prepare the controller to write text to the screen by positioning
    /// the cursor.
    ///
    /// @code
    ///     lcd.SetTextCursor(100, 25);
    ///     lcd.puts("Hello");
    /// @endcode
    ///
    /// @param[in] x is the horizontal position in pixels (from the left edge)
    /// @param[in] y is the vertical position in pixels (from the top edge)
    /// @returns success/failure code. See @ref RetCode_t.
    ///
    RetCode_t SetTextCursor(loc_t x, loc_t y);


    /// Prepare the controller to write text to the screen by positioning
    /// the cursor.
    ///
    /// @code
    ///     point_t point = {100, 25};
    ///     lcd.SetTextCursor(point);
    ///     lcd.puts("Hello");
    /// @endcode
    ///
    /// @param[in] p is the x:y point in pixels from the top-left.
    /// @returns success/failure code. See @ref RetCode_t.
    ///
    RetCode_t SetTextCursor(point_t p);


    /// Get the current cursor position in pixels.
    ///
    /// @code
    ///     point_t point = GetTextCursor();
    ///     if (point.x > 100 && point.y > 150)
    ///         //...
    /// @endcode
    ///
    /// @returns cursor position.
    ///
    point_t GetTextCursor(void);


    /// Get the current cursor horizontal position in pixels.
    ///
    /// @returns cursor position horizontal offset.
    ///
    loc_t GetTextCursor_X(void);


    /// Get the current cursor vertical position in pixels.
    ///
    /// @returns cursor position vertical offset.
    ///
    loc_t GetTextCursor_Y(void);


    /// Configure additional Cursor Control settings.
    ///
    /// This API lets you modify other cursor control settings;
    /// Cursor visible/hidden, Cursor blink/normal,
    /// Cursor I-Beam/underscore/box.
    ///
    /// @param[in] cursor can be set to NOCURSOR (default), IBEAM,
    ///         UNDER, or BLOCK.
    /// @param[in] blink can be set to true or false (default false)
    /// @returns success/failure code. See @ref RetCode_t
    ///
    RetCode_t SetTextCursorControl(cursor_t cursor = NOCURSOR, bool blink = false);


    /// Select the built-in ISO 8859-X font to use next.
    ///
    /// Supported fonts: ISO 8859-1, -2, -3, -4
    ///
    /// @note This only modifies the choice of font from the RA8875 internal
    ///     fonts.
    ///
    /// @param[in] font selects the font for the subsequent text rendering.
    ///
    /// @note if either hScale or vScale is outside of its permitted range,
    ///     the command is not executed.
    /// @returns success/failure code. See @ref RetCode_t.
    ///
    RetCode_t SetTextFont(font_t font = ISO8859_1);


    /// Sets the display orientation.
    ///
    /// @note This command does not let you "merge" text onto an existing
    ///       image, since it reuses the memory for the new orientation.
    ///       Therefore, it is recommended that you issue a cls() prior
    ///       to sending text to the screen, or you end with a blended
    ///       image that is probably not as intended.
    ///
    /// @note This command only operates on the RA8875 internal fonts.
    ///
    /// @code
    ///     lcd.cls();
    ///     lcd.SetOrientation(RA8875::normal);
    ///     lcd.puts(30,30, "Normal Landscape");
    ///     wait_ms(2500);
    ///
    ///     lcd.cls();
    ///     lcd.SetOrientation(RA8875::rotate_90);
    ///     lcd.puts(30,30, "Rotated 90 Text\r\n");
    ///     wait_ms(2500);
    ///
    ///     lcd.cls();
    ///     lcd.SetOrientation(RA8875::rotate_180);
    ///     lcd.puts(30,30, "Rotated 180 Text\r\n");
    ///     wait_ms(2500);
    ///
    ///     lcd.cls();
    ///     lcd.SetOrientation(RA8875::rotate_270);
    ///     lcd.puts(30,30, "Rotated 270 Text\r\n");
    ///     wait_ms(2500);
    /// @endcode
    ///
    /// @param[in] angle defaults to normal, but can be rotated
    ///         - normal | rotate_0
    ///         - rotate_90 (clockwise)
    ///         - rotate_180
    ///         - rotate_270 (clockwise)
    /// @returns success/failure code. See @ref RetCode_t.
    ///
    RetCode_t SetOrientation(orientation_t angle = normal);


    /// Control the font behavior.
    ///
    /// This command lets you make several modifications to any text that
    /// will be written to the screen.
    ///
    /// @note This command only operates on the RA8875 internal fonts.
    ///
    /// Options can be combined:
    /// Default:
    /// @li Full alignment disabled,
    /// @li Font with Background color,
    /// @li Font in normal orientiation, or rotated 90, 180, or 270 clockwise,
    /// @li Horizontal scale x 1, 2, 3, or 4
    /// @li Vertical scale x 1, 2, 3, or 4
    ///
    /// @note alignment is a special mode for the fonts, when mixing half and
    ///     full fonts on one presentation. 'align_full' starts each full
    ///     character on an even alignment. See section 7-4-7 of the RA8875
    ///     specification.
    ///
    /// @param[in] fillit defaults to FILL, but can be NOFILL
    /// @param[in] hScale defaults to 1, but can be 1, 2, 3, or 4,
    ///     and scales the font size by this amount.
    /// @param[in] vScale defaults to 1, but can be 1, 2, 3, or 4,
    ///     and scales the font size by this amount.
    /// @param[in] alignment defaults to align_none, but can be
    ///     align_full.
    ///
    /// @note if either hScale or vScale is outside of its permitted range,
    ///     the command is not executed.
    /// @returns success/failure code. See @ref RetCode_t.
    ///
    RetCode_t SetTextFontControl(fill_t fillit = FILL,
        HorizontalScale hScale = 1,
        VerticalScale vScale = 1,
        alignment_t alignment = align_none);


    /// Control the font size of the RA8875 internal fonts.
    ///
    /// This command lets you set the font enlargement for both horizontal
    /// and vertical, independent of the rotation, background, and
    /// alignment. See @ref SetTextFontControl.
    ///
    /// @note This command only operates on the RA8875 internal fonts.
    ///
    /// @param[in] hScale defaults to 1, but can be 1, 2, 3, or 4,
    ///     and scales the font size by this amount.
    /// @param[in] vScale is an optional parameter that defaults to the hScale value,
    ///     but can be 1, 2, 3, or 4, and scales the font size by this amount.
    ///
    /// @code
    ///     lcd.SetTextFontSize(2);     // Set the font to 2x normal size
    ///     lcd.puts("Two times");
    ///     lcd.SetTextFontSize(2,3);   // Set the font to 2x Width and 3x Height
    ///     lcd.puts("2*2 3*h");
    ///     lcd.SetTextFontSize();      // Restore to normal size in both dimensions
    ///     lcd.puts("normal");
    /// @endcode
    ///
    /// @note if either hScale or vScale is outside of its permitted range,
    ///     the command is not executed.
    /// @returns success/failure code. See @ref RetCode_t.
    ///
    RetCode_t SetTextFontSize(HorizontalScale hScale = 1, VerticalScale vScale = -1);


    /// Get the text font size of the RA8875 internal fonts.
    ///
    /// This command lets you retrieve the current settings for the font
    /// horizontal and vertical scale factors. The return value is
    /// one of the scale factors 1, 2, 3, or 4.
    ///
    /// @param[out] hScale is a pointer to memory where the horizontal scale factor
    ///     will be written. If the pointer is null, that item will be ignored.
    /// @param[out] vScale is a pointer to memory where the vertical scale factor
    ///     will be written. If the pointer is null, that item will be ignored.
    /// @returns success/failure code. See @ref RetCode_t.
    ///
    RetCode_t GetTextFontSize(HorizontalScale * hScale, VerticalScale * vScale);

    /// put a character on the screen.
    ///
    /// @param[in] c is the character.
    /// @returns the character, or EOF if there is an error.
    ///
    virtual int _putc(int c);


    /// Write string of text to the display
    ///
    /// @code
    ///     lcd.puts("Test STring");
    /// @endcode
    ///
    /// @param[in] string is the null terminated string to send to the display.
    ///
    void puts(const char * string);


    /// Write string of text to the display at the specified location.
    ///
    /// @code
    ///     lcd.puts(10,25, "Test STring");
    /// @endcode
    ///
    /// @param[in] x is the horizontal position in pixels (from the left edge)
    /// @param[in] y is the vertical position in pixels (from the top edge)
    /// @param[in] string is the null terminated string to send to the display.
    ///
    void puts(loc_t x, loc_t y, const char * string);


    /// Prepare the controller to write binary data to the screen by positioning
    /// the memory cursor.
    ///
    /// @param[in] x is the horizontal position in pixels (from the left edge)
    /// @param[in] y is the vertical position in pixels (from the top edge)
    /// @returns success/failure code. See @ref RetCode_t.
    ///
    virtual RetCode_t SetGraphicsCursor(loc_t x, loc_t y);

    /// Prepare the controller to write binary data to the screen by positioning
    /// the memory cursor.
    ///
    /// @param[in] p is the point representing the cursor position to set
    /// @returns success/failure code. See @ref RetCode_t.
    ///
    virtual RetCode_t SetGraphicsCursor(point_t p);

    /// Read the current graphics cursor position as a point.
    ///
    /// @returns the graphics cursor as a point.
    ///
    virtual point_t GetGraphicsCursor(void);


    /// Prepare the controller to read binary data from the screen by positioning
    /// the memory read cursor.
    ///
    /// @param[in] x is the horizontal position in pixels (from the left edge)
    /// @param[in] y is the vertical position in pixels (from the top edge)
    /// @returns success/failure code. See @ref RetCode_t.
    ///
    virtual RetCode_t SetGraphicsCursorRead(loc_t x, loc_t y);


    /// Set the window, constraining where items are written to the screen.
    ///
    /// After setting the window, text and graphics are constrained to this
    /// window. Text will wrap from the right edge back to the left and down
    /// one row and from the bottom to the top. Graphics drawing will be clipped
    /// at the edge of the window.
    ///
    /// @note If the initial text write is outside the window, it will be shown
    /// where the cursor position it. Once the write hits the right edge of
    /// the defined window, it will then wrap back to the left edge. Once it
    /// hits the bottom, it wraps to the top of the window. For this reason,
    /// it is common to set the text cursor to the window.
    ///
    /// @code
    ///     rect_t r = {10,10, 90,90};
    ///     lcd.window(r);
    ///     lcd.SetTextCursor(r.p1.x, r.p1.y);
    ///     lcd.puts("012345678901234567890123456789012345678901234567890");
    ///     lcd.window(); restore to full screen
    /// @endcode
    ///
    /// @param[in] r is the rect_t used to set the window.
    /// @returns success/failure code. See @ref RetCode_t.
    ///
    virtual RetCode_t window(rect_t r);


    /// Set the window, constraining where items are written to the screen.
    ///
    /// After setting the window, text and graphics are constrained to this
    /// window. Text will wrap from the right edge back to the left and down
    /// one row and from the bottom to the top. Graphics drawing will be clipped
    /// at the edge of the window.
    ///
    /// @note if no parameters are provided, it restores the window to full screen.
    ///
    /// @note If the initial text write is outside the window, it will be shown
    /// where the cursor position it. Once the write hits the right edge of
    /// the defined window, it will then wrap back to the left edge. Once it
    /// hits the bottom, it wraps to the top of the window. For this reason,
    /// it is common to set the text cursor to the window.
    ///
    /// @code
    ///     lcd.window(10,10, 80,80);
    ///     lcd.SetTextCursor(10,10);
    ///     lcd.puts("012345678901234567890123456789012345678901234567890");
    ///     lcd.window(); restore to full screen
    /// @endcode
    ///
    /// @param[in] x is the left edge in pixels.
    /// @param[in] y is the top edge in pixels.
    /// @param[in] width is the window width in pixels.
    /// @param[in] height is the window height in pixels.
    /// @returns success/failure code. See @ref RetCode_t.
    ///
    virtual RetCode_t window(loc_t x = 0, loc_t y = 0, dim_t width = (dim_t)-1, dim_t height = (dim_t)-1);


    /// Clear either the specified layer, or the active layer.
    ///
    /// The behavior is to clear the whole screen for the specified
    /// layer. When not specified, the active drawing layer is cleared.
    /// This command can also be used to specifically clear either,
    /// or both layers. See @ref clsw().
    ///
    /// @code
    ///     lcd.cls();
    /// @endcode
    ///
    /// @param[in] layers is optional. If not provided, the active layer
    ///     is cleared. If bit 0 is set, layer 0 is cleared, if bit
    ///     1 is set, layer 1 is cleared. If both are set, both layers
    ///     are cleared. Any other value does not cause an action.
    ///
    /// @returns success/failure code. See @ref RetCode_t.
    ///
    virtual RetCode_t cls(uint16_t layers = 0);


    /// Clear the screen, or clear only the active window.
    ///
    /// The default behavior is to clear the whole screen. With the optional
    /// parameter, the action can be restricted to the active window, which
    /// can be set with the See @ref window method.
    ///
    /// @code
    ///     lcd.window(20,20, 40,10);
    ///     lcd.clsw();
    /// @endcode
    ///
    /// @param[in] region is an optional parameter that defaults to FULLWINDOW
    ///         or may be set to ACTIVEWINDOW.
    /// @returns success/failure code. See @ref RetCode_t.
    ///
    RetCode_t clsw(RA8875::Region_t region = FULLWINDOW);


    /// Set the background color.
    ///
    /// @param[in] color is expressed in 16-bit format.
    /// @returns success/failure code. See @ref RetCode_t.
    ///
    virtual RetCode_t background(color_t color);


    /// Set the background color.
    ///
    /// @param[in] r is the red element of the color.
    /// @param[in] g is the green element of the color.
    /// @param[in] b is the blue element of the color.
    /// @returns success/failure code. See @ref RetCode_t.
    ///
    virtual RetCode_t background(unsigned char r, unsigned char g, unsigned char b);


    /// Set the foreground color.
    ///
    /// @param[in] color is expressed in 16-bit format.
    /// @returns success/failure code. See @ref RetCode_t.
    ///
    virtual RetCode_t foreground(color_t color);


    /// Set the foreground color.
    ///
    /// @param[in] r is the red element of the color.
    /// @param[in] g is the green element of the color.
    /// @param[in] b is the blue element of the color.
    /// @returns success/failure code. See @ref RetCode_t.
    ///
    virtual RetCode_t foreground(unsigned char r, unsigned char g, unsigned char b);


    /// Get the current foreground color value.
    ///
    /// @returns the current foreground color.
    ///
    color_t GetForeColor(void);


    /// Draw a pixel in the specified color.
    ///
    /// @note Unlike many other operations, this does not
    ///         set the forecolor!
    ///
    /// @param[in] p is the point_t defining the location.
    /// @returns success/failure code. See @ref RetCode_t.
    ///
    virtual RetCode_t pixel(point_t p, color_t color);


    /// Draw a pixel in the current foreground color.
    ///
    /// @param[in] p is the point_t defining the location.
    /// @returns success/failure code. See @ref RetCode_t.
    ///
    virtual RetCode_t pixel(point_t p);


    /// Draw a pixel in the specified color.
    ///
    /// @note Unlike many other operations, this does not
    ///         set the forecolor!
    ///
    /// @param[in] x is the horizontal offset to this pixel.
    /// @param[in] y is the vertical offset to this pixel.
    /// @param[in] color defines the color for the pixel.
    /// @returns success/failure code. See @ref RetCode_t.
    ///
    virtual RetCode_t pixel(loc_t x, loc_t y, color_t color);


    /// Draw a pixel in the current foreground color.
    ///
    /// @param[in] x is the horizontal offset to this pixel.
    /// @param[in] y is the veritical offset to this pixel.
    /// @returns success/failure code. See @ref RetCode_t.
    ///
    virtual RetCode_t pixel(loc_t x, loc_t y);


    /// Get a pixel from the display.
    ///
    /// @param[in] x is the horizontal offset to this pixel.
    /// @param[in] y is the vertical offset to this pixel.
    /// @returns the pixel. see @color_t
    ///
    virtual color_t getPixel(loc_t x, loc_t y);


    /// Write an RGB565 stream of pixels to the display.
    ///
    /// @param[in] p is a pointer to a color_t array to write.
    /// @param[in] count is the number of pixels to write.
    /// @param[in] x is the horizontal position on the display.
    /// @param[in] y is the vertical position on the display.
    /// @returns success/failure code. See @ref RetCode_t.
    ///
    virtual RetCode_t pixelStream(color_t * p, uint32_t count, loc_t x, loc_t y);


    /// Get a stream of pixels from the display.
    ///
    /// @param[in] p is a pointer to a color_t array to accept the stream.
    /// @param[in] count is the number of pixels to read.
    /// @param[in] x is the horizontal offset to this pixel.
    /// @param[in] y is the vertical offset to this pixel.
    /// @returns success/failure code. See @ref RetCode_t.
    ///
    virtual RetCode_t getPixelStream(color_t * p, uint32_t count, loc_t x, loc_t y);


    /// Write a boolean stream to the display.
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
    virtual RetCode_t booleanStream(loc_t x, loc_t y, dim_t w, dim_t h, const uint8_t * boolStream);


    /// Draw a line in the specified color
    ///
    /// @note As a side effect, this changes the current
    ///     foreground color for subsequent operations.
    ///
    /// @param[in] p1 is the point to start the line.
    /// @param[in] p2 is the point to end the line.
    /// @param[in] color defines the foreground color.
    /// @returns success/failure code. See @ref RetCode_t.
    ///
    RetCode_t line(point_t p1, point_t p2, color_t color);


    /// Draw a line
    ///
    /// Draws a line using the foreground color setting.
    ///
    /// @param[in] p1 is the point to start the line.
    /// @param[in] p2 is the point to end the line.
    /// @returns success/failure code. See @ref RetCode_t.
    ///
    RetCode_t line(point_t p1, point_t p2);


    /// Draw a line in the specified color
    ///
    /// @note As a side effect, this changes the current
    ///     foreground color for subsequent operations.
    ///
    /// @param[in] x1 is the horizontal start of the line.
    /// @param[in] y1 is the vertical start of the line.
    /// @param[in] x2 is the horizontal end of the line.
    /// @param[in] y2 is the vertical end of the line.
    /// @param[in] color defines the foreground color.
    /// @returns success/failure code. See @ref RetCode_t.
    ///
    RetCode_t line(loc_t x1, loc_t y1, loc_t x2, loc_t y2, color_t color);


    /// Draw a line
    ///
    /// Draws a line using the foreground color setting.
    ///
    /// @param[in] x1 is the horizontal start of the line.
    /// @param[in] y1 is the vertical start of the line.
    /// @param[in] x2 is the horizontal end of the line.
    /// @param[in] y2 is the vertical end of the line.
    /// @returns success/failure code. See @ref RetCode_t.
    ///
    RetCode_t line(loc_t x1, loc_t y1, loc_t x2, loc_t y2);


    /// Draw a thick line
    ///
    /// Draw a line of a specified thickness and color.
    ///
    /// In order to draw a thick line, this draws filled circles using
    /// bresenham's algorithm to move the center point of the circle.
    /// As a result, this is much slower than drawing a 1-pixel line which
    /// uses the hardware line drawing algorithm.
    ///
    /// Drawing multiple parallel lines to create a thick line is faster,
    /// however the line drawing was not guaranteed to fill every pixel
    /// on the diagonals.
    ///
    /// @param[in] p1 is the point to start the line.
    /// @param[in] p2 is the point to end the line.
    /// @param[in] thickness is the line thickness.
    /// @param[in] color defines the foreground color.
    /// @returns success/failure code. See @ref RetCode_t.
    ///
    RetCode_t ThickLine(point_t p1, point_t p2, dim_t thickness, color_t color);


    /// Draw a rectangle in the specified color
    ///
    /// @note As a side effect, this changes the current
    ///     foreground color for subsequent operations.
    ///
    /// @param[in] rect defines the rectangle.
    /// @param[in] color defines the foreground color.
    /// @param[in] fillit is optional to FILL the rectangle. default is NOFILL.
    /// @returns success/failure code. See @ref RetCode_t.
    ///
    RetCode_t rect(rect_t rect, color_t color, fill_t fillit = NOFILL);


    /// Draw a filled rectangle in the specified color
    ///
    /// @note As a side effect, this changes the current
    ///     foreground color for subsequent operations.
    ///
    /// @param[in] rect defines the rectangle.
    /// @param[in] color defines the foreground color.
    /// @param[in] fillit is optional to FILL the rectangle. default is NOFILL.
    /// @returns success/failure code. See @ref RetCode_t.
    ///
    RetCode_t fillrect(rect_t rect, color_t color, fill_t fillit = FILL);


    /// Draw a rectangle in the specified color
    ///
    /// @note As a side effect, this changes the current
    ///     foreground color for subsequent operations.
    ///
    /// @param[in] x1 is the horizontal start of the line.
    /// @param[in] y1 is the vertical start of the line.
    /// @param[in] x2 is the horizontal end of the line.
    /// @param[in] y2 is the vertical end of the line.
    /// @param[in] color defines the foreground color.
    /// @param[in] fillit is optional to FILL the rectangle. default is FILL.
    /// @returns success/failure code. See @ref RetCode_t.
    ///
    RetCode_t rect(loc_t x1, loc_t y1, loc_t x2, loc_t y2,
        color_t color, fill_t fillit = NOFILL);


    /// Draw a filled rectangle in the specified color
    ///
    /// @note As a side effect, this changes the current
    ///     foreground color for subsequent operations.
    ///
    /// @param[in] x1 is the horizontal start of the line.
    /// @param[in] y1 is the vertical start of the line.
    /// @param[in] x2 is the horizontal end of the line.
    /// @param[in] y2 is the vertical end of the line.
    /// @param[in] color defines the foreground color.
    /// @param[in] fillit is optional to NOFILL the rectangle. default is FILL.
    /// @returns success/failure code. See @ref RetCode_t.
    ///
    virtual RetCode_t fillrect(loc_t x1, loc_t y1, loc_t x2, loc_t y2,
        color_t color, fill_t fillit = FILL);


    /// Draw a rectangle
    ///
    /// Draws a rectangle using the foreground color setting.
    ///
    /// @param[in] x1 is the horizontal start of the line.
    /// @param[in] y1 is the vertical start of the line.
    /// @param[in] x2 is the horizontal end of the line.
    /// @param[in] y2 is the vertical end of the line.
    /// @param[in] fillit is optional to FILL the rectangle. default is NOFILL.
    /// @returns success/failure code. See @ref RetCode_t.
    ///
    RetCode_t rect(loc_t x1, loc_t y1, loc_t x2, loc_t y2,
        fill_t fillit = NOFILL);


    /// Draw a filled rectangle with rounded corners using the specified color.
    ///
    /// This draws a rounded rectangle. A numbers of checks are made on the values,
    /// and it could reduce this to drawing a line (if either x1 == x2, or y1 == y2),
    /// or a single point (x1 == x2 && y1 == y2). If the radius parameters are
    /// > 1/2 the length of that side (width or height), an error value is returned.
    ///
    /// @note As a side effect, this changes the current
    ///     foreground color for subsequent operations.
    ///
    /// @param[in] x1 is the horizontal start of the line and must be <= x2.
    /// @param[in] y1 is the vertical start of the line and must be <= y2.
    /// @param[in] x2 is the horizontal end of the line and must be >= x1.
    /// @param[in] y2 is the vertical end of the line and must be >= y1.
    /// @param[in] radius1 defines the horizontal radius of the curved corner. Take care
    ///         that this value < 1/2 the width of the rectangle, or bad_parameter
    ///         is returned.
    /// @param[in] radius2 defines the vertical radius of the curved corner. Take care
    ///         that this value < 1/2 the height of the rectangle, or bad_parameter
    ///         is returned.
    /// @param[in] color defines the foreground color.
    /// @param[in] fillit is optional to FILL the rectangle. default is NOFILL.
    /// @returns success/failure code. See @ref RetCode_t.
    ///
    RetCode_t fillroundrect(loc_t x1, loc_t y1, loc_t x2, loc_t y2,
        dim_t radius1, dim_t radius2, color_t color, fill_t fillit = FILL);


    /// Draw a filled rectangle with rounded corners using the specified color.
    ///
    /// This draws a rounded rectangle. A numbers of checks are made on the values,
    /// and it could reduce this to drawing a line (if either x1 == x2, or y1 == y2),
    /// or a single point (x1 == x2 && y1 == y2). If the radius parameters are
    /// > 1/2 the length of that side (width or height), an error value is returned.
    ///
    /// @note As a side effect, this changes the current
    ///     foreground color for subsequent operations.
    ///
    /// @param[in] r is the rectangle to draw.
    /// @param[in] radius1 defines the horizontal radius of the curved corner. Take care
    ///         that this value < 1/2 the width of the rectangle, or bad_parameter
    ///         is returned.
    /// @param[in] radius2 defines the vertical radius of the curved corner. Take care
    ///         that this value < 1/2 the height of the rectangle, or bad_parameter
    ///         is returned.
    /// @param[in] color defines the foreground color.
    /// @param[in] fillit is optional to FILL the rectangle. default is NOFILL.
    /// @returns success/failure code. See @ref RetCode_t.
    ///
    RetCode_t fillroundrect(rect_t r,
        dim_t radius1, dim_t radius2, color_t color, fill_t fillit = FILL);


    /// Draw a rectangle with rounded corners using the specified color.
    ///
    /// This draws a rounded rectangle. A numbers of checks are made on the values,
    /// and it could reduce this to drawing a line (if either x1 == x2, or y1 == y2),
    /// or a single point (x1 == x2 && y1 == y2). If the radius parameters are
    /// > 1/2 the length of that side (width or height), an error value is returned.
    ///
    /// @note As a side effect, this changes the current
    ///     foreground color for subsequent operations.
    ///
    /// @param[in] r is the rectangle to draw.
    /// @param[in] radius1 defines the horizontal radius of the curved corner. Take care
    ///         that this value < 1/2 the width of the rectangle, or bad_parameter
    ///         is returned.
    /// @param[in] radius2 defines the vertical radius of the curved corner. Take care
    ///         that this value < 1/2 the height of the rectangle, or bad_parameter
    ///         is returned.
    /// @param[in] color defines the foreground color.
    /// @param[in] fillit is optional to FILL the rectangle. default is NOFILL.
    /// @returns success/failure code. See @ref RetCode_t.
    ///
    RetCode_t roundrect(rect_t r,
        dim_t radius1, dim_t radius2, color_t color, fill_t fillit = NOFILL);


    /// Draw a rectangle with rounded corners using the specified color.
    ///
    /// This draws a rounded rectangle. A numbers of checks are made on the values,
    /// and it could reduce this to drawing a line (if either x1 == x2, or y1 == y2),
    /// or a single point (x1 == x2 && y1 == y2). If the radius parameters are
    /// > 1/2 the length of that side (width or height), an error value is returned.
    ///
    /// @note As a side effect, this changes the current
    ///     foreground color for subsequent operations.
    ///
    /// @param[in] x1 is the horizontal start of the line and must be <= x2.
    /// @param[in] y1 is the vertical start of the line and must be <= y2.
    /// @param[in] x2 is the horizontal end of the line and must be >= x1.
    /// @param[in] y2 is the vertical end of the line and must be >= y1.
    /// @param[in] radius1 defines the horizontal radius of the curved corner. Take care
    ///         that this value < 1/2 the width of the rectangle, or bad_parameter
    ///         is returned.
    /// @param[in] radius2 defines the vertical radius of the curved corner. Take care
    ///         that this value < 1/2 the height of the rectangle, or bad_parameter
    ///         is returned.
    /// @param[in] color defines the foreground color.
    /// @param[in] fillit is optional to FILL the rectangle. default is NOFILL.
    /// @returns success/failure code. See @ref RetCode_t.
    ///
    RetCode_t roundrect(loc_t x1, loc_t y1, loc_t x2, loc_t y2,
        dim_t radius1, dim_t radius2, color_t color, fill_t fillit = NOFILL);


    /// Draw a rectangle with rounded corners.
    ///
    /// This draws a rounded rectangle. A numbers of checks are made on the values,
    /// and it could reduce this to drawing a line (if either x1 == x2, or y1 == y2),
    /// or a single point (x1 == x2 && y1 == y2). If the radius parameters are
    /// > 1/2 the length of that side (width or height), an error value is returned.
    ///
    /// @param[in] x1 is the horizontal start of the line and must be <= x2.
    /// @param[in] y1 is the vertical start of the line and must be <= y2.
    /// @param[in] x2 is the horizontal end of the line and must be >= x1.
    /// @param[in] y2 is the vertical end of the line and must be >= y1.
    /// @param[in] radius1 defines the horizontal radius of the curved corner. Take care
    ///         that this value < 1/2 the width of the rectangle, or bad_parameter
    ///         is returned.
    /// @param[in] radius2 defines the vertical radius of the curved corner. Take care
    ///         that this value < 1/2 the height of the rectangle, or bad_parameter
    ///         is returned.
    /// @param[in] fillit is optional to FILL the rectangle. default is NOFILL.
    /// @returns success/failure code. See @ref RetCode_t.
    ///
    RetCode_t roundrect(loc_t x1, loc_t y1, loc_t x2, loc_t y2,
        dim_t radius1, dim_t radius2, fill_t fillit = NOFILL);


    /// Draw a triangle in the specified color.
    ///
    /// @note As a side effect, this changes the current
    ///     foreground color for subsequent operations.
    ///
    /// @param[in] x1 is the horizontal for point 1.
    /// @param[in] y1 is the vertical for point 1.
    /// @param[in] x2 is the horizontal for point 2.
    /// @param[in] y2 is the vertical for point 2.
    /// @param[in] x3 is the horizontal for point 3.
    /// @param[in] y3 is the vertical for point 3.
    /// @param[in] color defines the foreground color.
    /// @param[in] fillit is optional to FILL the rectangle. default is NOFILL.
    /// @returns success/failure code. See @ref RetCode_t.
    ///
    RetCode_t triangle(loc_t x1, loc_t y1, loc_t x2, loc_t y2,
        loc_t x3, loc_t y3, color_t color, fill_t fillit = NOFILL);


    /// Draw a filled triangle in the specified color.
    ///
    /// @note As a side effect, this changes the current
    ///     foreground color for subsequent operations.
    ///
    /// @param[in] x1 is the horizontal for point 1.
    /// @param[in] y1 is the vertical for point 1.
    /// @param[in] x2 is the horizontal for point 2.
    /// @param[in] y2 is the vertical for point 2.
    /// @param[in] x3 is the horizontal for point 3.
    /// @param[in] y3 is the vertical for point 3.
    /// @param[in] color defines the foreground color.
    /// @param[in] fillit is optional to FILL the rectangle. default is NOFILL.
    /// @returns success/failure code. See @ref RetCode_t.
    ///
    RetCode_t filltriangle(loc_t x1, loc_t y1, loc_t x2, loc_t y2,
        loc_t x3, loc_t y3, color_t color, fill_t fillit = FILL);


    /// Draw a triangle
    ///
    /// Draws a triangle using the foreground color setting.
    ///
    /// @param[in] x1 is the horizontal for point 1.
    /// @param[in] y1 is the vertical for point 1.
    /// @param[in] x2 is the horizontal for point 2.
    /// @param[in] y2 is the vertical for point 2.
    /// @param[in] x3 is the horizontal for point 3.
    /// @param[in] y3 is the vertical for point 3.
    /// @param[in] fillit is optional to FILL the rectangle. default is NOFILL.
    /// @returns success/failure code. See @ref RetCode_t.
    ///
    RetCode_t triangle(loc_t x1, loc_t y1, loc_t x2, loc_t y2,
        loc_t x3, loc_t y3, fill_t fillit = NOFILL);


    /// Draw a circle using the specified color.
    ///
    /// @note As a side effect, this changes the current
    ///     foreground color for subsequent operations.
    ///
    /// @param[in] p defines the center of the circle.
    /// @param[in] radius defines the size of the circle.
    /// @param[in] color defines the foreground color.
    /// @param[in] fillit is optional to FILL the circle. default is NOFILL.
    /// @returns success/failure code. See @ref RetCode_t.
    ///
    RetCode_t circle(point_t p, dim_t radius, color_t color, fill_t fillit = NOFILL);


    /// Draw a filled circle using the specified color.
    ///
    /// @note As a side effect, this changes the current
    ///     foreground color for subsequent operations.
    ///
    /// @param[in] p defines the center of the circle.
    /// @param[in] radius defines the size of the circle.
    /// @param[in] color defines the foreground color.
    /// @param[in] fillit is optional to FILL the circle. default is FILL.
    /// @returns success/failure code. See @ref RetCode_t.
    ///
    RetCode_t fillcircle(point_t p, dim_t radius, color_t color, fill_t fillit = FILL);


    /// Draw a circle.
    ///
    /// Draws a circle using the foreground color setting.
    ///
    /// @param[in] p defines the center of the circle.
    /// @param[in] radius defines the size of the circle.
    /// @param[in] fillit is optional to FILL the circle. default is NOFILL.
    /// @returns success/failure code. See @ref RetCode_t.
    ///
    RetCode_t circle(point_t p, dim_t radius, fill_t fillit = NOFILL);


    /// Draw a circle using the specified color.
    ///
    /// @note As a side effect, this changes the current
    ///     foreground color for subsequent operations.
    ///
    /// @param[in] x is the horizontal center of the circle.
    /// @param[in] y is the vertical center of the circle.
    /// @param[in] radius defines the size of the circle.
    /// @param[in] color defines the foreground color.
    /// @param[in] fillit is optional to FILL the circle. default is NOFILL.
    /// @returns success/failure code. See @ref RetCode_t.
    ///
    RetCode_t circle(loc_t x, loc_t y, dim_t radius, color_t color, fill_t fillit = NOFILL);


    /// Draw a filled circle using the specified color.
    ///
    /// @note As a side effect, this changes the current
    ///     foreground color for subsequent operations.
    ///
    /// @param[in] x is the horizontal center of the circle.
    /// @param[in] y is the vertical center of the circle.
    /// @param[in] radius defines the size of the circle.
    /// @param[in] color defines the foreground color.
    /// @param[in] fillit is optional to FILL the circle. default is FILL.
    /// @returns success/failure code. See @ref RetCode_t.
    ///
    RetCode_t fillcircle(loc_t x, loc_t y, dim_t radius, color_t color, fill_t fillit = FILL);


    /// Draw a circle.
    ///
    /// Draws a circle using the foreground color setting.
    ///
    /// @param[in] x is the horizontal center of the circle.
    /// @param[in] y is the vertical center of the circle.
    /// @param[in] radius defines the size of the circle.
    /// @param[in] fillit is optional to FILL the circle. default is NOFILL.
    /// @returns success/failure code. See @ref RetCode_t.
    ///
    RetCode_t circle(loc_t x, loc_t y, dim_t radius, fill_t fillit = NOFILL);

    /// Draw an Ellipse using the specified color
    ///
    /// @note As a side effect, this changes the current
    ///     foreground color for subsequent operations.
    ///
    /// @param[in] x is the horizontal center of the ellipse.
    /// @param[in] y is the vertical center of the ellipse.
    /// @param[in] radius1 defines the horizontal radius of the ellipse.
    /// @param[in] radius2 defines the vertical radius of the ellipse.
    /// @param[in] color defines the foreground color.
    /// @param[in] fillit defines whether the circle is filled or not.
    /// @returns success/failure code. See @ref RetCode_t.
    ///
    RetCode_t ellipse(loc_t x, loc_t y, dim_t radius1, dim_t radius2,
        color_t color, fill_t fillit = NOFILL);


    /// Draw a filled Ellipse using the specified color
    ///
    /// @note As a side effect, this changes the current
    ///     foreground color for subsequent operations.
    ///
    /// @param[in] x is the horizontal center of the ellipse.
    /// @param[in] y is the vertical center of the ellipse.
    /// @param[in] radius1 defines the horizontal radius of the ellipse.
    /// @param[in] radius2 defines the vertical radius of the ellipse.
    /// @param[in] color defines the foreground color.
    /// @param[in] fillit defines whether the circle is filled or not.
    /// @returns success/failure code. See @ref RetCode_t.
    ///
    RetCode_t fillellipse(loc_t x, loc_t y, dim_t radius1, dim_t radius2,
        color_t color, fill_t fillit = FILL);


    /// Draw an Ellipse
    ///
    /// Draws it using the foreground color setting.
    ///
    /// @param[in] x is the horizontal center of the ellipse.
    /// @param[in] y is the vertical center of the ellipse.
    /// @param[in] radius1 defines the horizontal radius of the ellipse.
    /// @param[in] radius2 defines the vertical radius of the ellipse.
    /// @param[in] fillit defines whether the circle is filled or not.
    /// @returns success/failure code. See @ref RetCode_t.
    ///
    RetCode_t ellipse(loc_t x, loc_t y, dim_t radius1, dim_t radius2, fill_t fillit = NOFILL);



    /// Block Move
    ///
    /// The Block Move API activates the RA8875 Block Transfer Engine. Due to the complex
    /// set of possible operations, the user should read the related sections of the
    /// RA8875 user manual.
    ///
    /// Some operations may require that other registers are configured, such as the
    /// foreground and background color registers, and others. Those must be set
    /// outside of this API.
    ///
    /// @code
    /// int main()
    /// {
    ///     point_t src;
    ///     point_t dst;
    ///     TouchCode_t touch;
    ///     const dim_t RECT_W = 100;
    ///     const dim_t RECT_H = 100;
    ///
    ///     pc.baud(460800);    //I like a snappy terminal, so crank it up!
    ///     pc.printf("\r\nRA8875 BTE Move Test - Build " __DATE__ " " __TIME__ "\r\n");
    ///     lcd.init(LCD_W,LCD_H,LCD_C, BL_NORM);
    ///     lcd.TouchPanelInit();
    ///     #ifndef CAP_TOUCH
    ///     InitTS();   // Calibration for resistive touch panel
    ///     #endif
    ///
    ///     RetCode_t r = lcd.RenderImageFile(0,0,"/local/fullscrn.jpg");
    ///     if (r) pc.printf("  Error: %d; %s\r\n", r, lcd.GetErrorMessage(r));
    ///     while (1) {
    ///         touch = lcd.TouchPanelReadable();
    ///         if (touch) {
    ///             point_t xy = lcd.TouchCoordinates();
    ///             TouchCode_t t = lcd.TouchCode();
    ///
    ///             if (t == touch) {
    ///                 src = ComputeTopLeftOfRect(xy, RECT_W/2, RECT_H/2, LCD_W, LCD_H);
    ///             } else if (t == release) {
    ///                 dst = ComputeTopLeftOfRect(xy, RECT_W/2, RECT_H/2, LCD_W, LCD_H);
    ///                 r = lcd.BlockMove(0,0,dst, 0,0,src, RECT_W,RECT_H, 0x2, 0xC);
    ///             }
    ///         }
    ///     }
    /// }
    /// @endcode
    ///
    /// @param[in] dstLayer layer [5B.7]. layer value is 0 or 1 representing layer 1 and 2.
    /// @param[in] dstDataSelect [50.5] defines the destination data type 0: block, 1: linear.
    /// @param[in] dstPoint [58-5B] is a point_t defining the destination coordinate.
    /// @param[in] srcLayer layer [57.7]. layer value is 0 or 1 representing layer 1 and 2.
    /// @param[in] srcDataSelect [50.6] defines the source data type 0: block, 1: linear.
    /// @param[in] srcPoint [54-57] is a point_t defining the source coordinate.
    /// @param[in] bte_width [5C-5D]. operation width.
    /// @param[in] bte_height [5E-5F]. operation height.
    /// @param[in] bte_op_code [51.3-0] defines the raster operation function
    ///             (write/read/move/...)
    /// @param[in] bte_rop_code [51.7-4] defines what type of BTE operation to perform
    ///             (what is placed at the destination)
    /// @returns success/failure code. See @ref RetCode_t.
    ///
    RetCode_t BlockMove(uint8_t dstLayer, uint8_t dstDataSelect, point_t dstPoint,
        uint8_t srcLayer, uint8_t srcDataSelect, point_t srcPoint,
        dim_t bte_width, dim_t bte_height,
        uint8_t bte_op_code, uint8_t bte_rop_code);


    /// Control display power
    ///
    /// @param[in] on when set to true will turn on the display, when false it is turned off.
    /// @returns success/failure code. See @ref RetCode_t.
    ///
    RetCode_t Power(bool on);


    /// Reset the display controller via the Software Reset interface.
    ///
    /// @returns success/failure code. See @ref RetCode_t.
    ///
    RetCode_t Reset(void);


    /// Set backlight brightness.
    ///
    /// When the built-in PWM is used to control the backlight, this
    /// API can be used to set the brightness.
    ///
    /// @param[in] brightness ranges from 0 (off) to 255 (full on)
    /// @returns success/failure code. See @ref RetCode_t.
    ///
    RetCode_t Backlight_u8(uint8_t brightness);


    /// Get backlight brightness.
    ///
    /// @returns backlight setting from 0 (off) to 255 (full on).
    ///
    uint8_t GetBacklight_u8(void);

    /// Set backlight brightness.
    ///
    /// When the built-in PWM is used to control the backlight, this
    /// API can be used to set the brightness.
    ///
    /// @param[in] brightness ranges from 0.0 (off) to 1.0 (full on)
    /// @returns success/failure code. See @ref RetCode_t.
    ///
    RetCode_t Backlight(float brightness);


    /// Get backlight brightness.
    ///
    /// @returns backlight setting from 0 (off) to 1.0 (full on).
    ///
    float GetBacklight(void);


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

    /// Get the currently selected user font.
    ///
    /// @returns a pointer to the font, or null, if no user font is selected.
    ///
    virtual const uint8_t * GetUserFont(void) { return font; }

    /// Get the RGB value for a DOS color.
    ///
    /// @code
    ///     color_t color = DOSColor(12);
    /// @endcode
    ///
    /// @param[in] i is the color, in the range 0 to 15;
    /// @returns the RGB color of the selected index, or 0
    ///     if the index is out of bounds.
    ///
    color_t DOSColor(int i);


    /// Get the color name (string) for a DOS color.
    ///
    /// @code
    ///     printf("color is %s\n", DOSColorNames(12));
    /// @endcode
    ///
    /// @param[in] i is the color, in the range 0 to 15;
    /// @returns a pointer to a string with the color name,
    ///     or NULL if the index is out of bounds.
    ///
    const char * DOSColorNames(int i);


    /// Advanced method indicating the start of a graphics stream.
    ///
    /// This is called prior to a stream of pixel data being sent.
    /// This may cause register configuration changes in the derived
    /// class in order to prepare the hardware to accept the streaming
    /// data.
    ///
    /// Following this command, a series of See @ref _putp() commands can
    /// be used to send individual pixels to the screen.
    ///
    /// To conclude the graphics stream, See @ref _EndGraphicsStream should
    /// be callled.
    ///
    /// @returns error code.
    ///
    virtual RetCode_t _StartGraphicsStream(void);


    /// Advanced method to put a single color pixel to the screen.
    ///
    /// This method may be called as many times as necessary after
    /// See @ref _StartGraphicsStream() is called, and it should be followed
    /// by _EndGraphicsStream.
    ///
    /// @code
    ///     _putp(DOSColor(12));
    /// @endcode
    ///
    /// @param[in] pixel is a color value to be put on the screen.
    /// @returns error code.
    ///
    virtual RetCode_t _putp(color_t pixel);


    /// Advanced method indicating the end of a graphics stream.
    ///
    /// This is called to conclude a stream of pixel data that was sent.
    /// This may cause register configuration changes in the derived
    /// class in order to stop the hardware from accept the streaming
    /// data.
    ///
    /// @returns error code.
    ///
    virtual RetCode_t _EndGraphicsStream(void);


    /// Set the SPI port frequency (in Hz).
    ///
    /// This uses the mbed SPI driver, and is therefore dependent on
    /// its capabilities. The RA8875 can accept writes via SPI faster
    /// than a read can be performed. The frequency set by this API
    /// is for the SPI writes. It will automatically reduce the SPI
    /// clock rate when a read is performed, and restore it for the
    /// next write. Alternately, the 2nd parameters permits setting
    /// the read speed rather than letting it compute it automatically.
    ///
    /// @note The primary effect of this is to recover more CPU cycles
    ///     for your application code. Keep in mind that when more than
    ///     one command is sent to the display controller, that it
    ///     will wait for the controller to finish the prior command.
    ///     In this case, the performance is limited by the RA8875.
    ///
    /// @param[in] Hz is the frequency in Hz, tested range includes the
    ///     range from 1,000,000 (1MHz) to 10,000,000 (10 MHz). Values
    ///     outside this range will be accepted, but operation may
    ///     be unreliable. This depends partially on your hardware design
    ///     and the wires connecting the display module.
    ///     The default value is 5,000,000, which should work for most
    ///     applications as a starting point.
    /// @param[in] Hz2 is an optional parameter and will set the read
    ///     speed independently of the write speed.
    /// @returns success/failure code. See @ref RetCode_t.
    ///
    RetCode_t frequency(unsigned long Hz = RA8875_DEFAULT_SPI_FREQ, unsigned long Hz2 = 0);


    /// This method captures the specified area as a 24-bit bitmap file.
    ///
    /// Even though this is a 16-bit display, the stored image is in
    /// 24-bit format.
    ///
    /// This method will interrogate the current display setting and
    /// create a bitmap based on those settings. For instance, if
    /// only layer 1 is visible, then the bitmap is only layer 1. However,
    /// if there is some other operation in effect (transparent mode).
    ///
    /// @param[in] x is the left edge of the region to capture
    /// @param[in] y is the top edge of the region to capture
    /// @param[in] w is the width of the region to capture
    /// @param[in] h is the height of the region to capture.
    /// @param[out] Name_BMP is the filename to write the image to.
    /// @return success or error code.
    ///
    RetCode_t PrintScreen(loc_t x, loc_t y, dim_t w, dim_t h, const char *Name_BMP);


    /// This method captures the specified area as a 24-bit bitmap file
    /// and delivers it to the previously attached callback.
    ///
    /// Even though this is a 16-bit display, the stored image is in
    /// 24-bit format.
    ///
    /// This method will interrogate the current display setting and
    /// create a bitmap based on those settings. For instance, if
    /// only layer 1 is visible, then the bitmap is only layer 1. However,
    /// if there is some other operation in effect (transparent mode), it
    /// will return the blended image.
    ///
    /// @param[in] x is the left edge of the region to capture
    /// @param[in] y is the top edge of the region to capture
    /// @param[in] w is the width of the region to capture
    /// @param[in] h is the height of the region to capture.
    /// @return success or error code.
    ///
    RetCode_t PrintScreen(loc_t x, loc_t y, dim_t w, dim_t h);


    /// PrintScreen callback registration.
    ///
    /// This method attaches a simple c-compatible callback of type PrintCallback_T.
    /// Then, the PrintScreen(x,y,w,h) method is called. Each chunk of data in the
    /// BMP file to be created is passed to this callback.
    ///
    /// @param callback is the optional callback function. Without a callback function
    ///     it will unregister the handler.
    ///
    void AttachPrintHandler(PrintCallback_T callback = NULL) { c_callback = callback; }


    /// PrintScreen callback registration.
    ///
    /// This method attaches a c++ class method as a callback of type PrintCallback_T.
    /// Then, the PrintScreen(x,y,w,h) method is called. Each chunk of data in the
    /// BMP file to be created is passed to this callback.
    ///
    /// @param object is the class hosting the callback function.
    /// @param method is the callback method in the object to activate.
    ///
    template <class T>
    void AttachPrintHandler(T *object, RetCode_t (T::*method)(void)) {
        obj_callback    = (FPointerDummy *)object;
        method_callback = (uint32_t (FPointerDummy::*)(uint32_t, uint8_t *, uint16_t))method;
    }


    /// This method captures the specified area as a 24-bit bitmap file,
    /// including the option of layer selection.
    ///
    /// @note This method is deprecated as the alternate PrintScreen API
    ///         automatically examines the display layer configuration.
    ///         Therefore, calls to this API will ignore the layer parameter
    ///         and automatically execute the other method.
    ///
    /// Even though this is a 16-bit display, the stored image is in
    /// 24-bit format.
    ///
    /// @param[in] layer is 0 or 1 to select the layer to extract.
    /// @param[in] x is the left edge of the region to capture
    /// @param[in] y is the top edge of the region to capture
    /// @param[in] w is the width of the region to capture
    /// @param[in] h is the height of the region to capture.
    /// @param[out] Name_BMP is the filename to write the image to.
    /// @return success or error code.
    ///
    RetCode_t PrintScreen(uint16_t layer, loc_t x, loc_t y, dim_t w, dim_t h, const char *Name_BMP);


    /// idle callback registration.
    ///
    /// This method attaches a simple c-compatible callback of type IdleCallback_T.
    /// Then, at any time when the display driver is waiting, it will call the
    /// registered function. This is probably most useful if you want to service
    /// a watchdog, when you may have called an API that will "hang" waiting
    /// on the user.
    ///
    /// @param callback is the idle callback function. Without a callback function
    ///     it will unregister the handler.
    ///
    void AttachIdleHandler(IdleCallback_T callback = NULL) { idle_callback = callback; }


#ifdef PERF_METRICS
    /// Clear the performance metrics to zero.
    void ClearPerformance();

    /// Count idle time.
    ///
    /// @param[in] t is the amount of idle time to accumulate.
    ///
    void CountIdleTime(uint32_t t);

    /// Report the performance metrics for drawing functions using
    /// the available serial channel.
    ///
    /// @param[in,out] pc is the serial channel to write to.
    ///
    void ReportPerformance(Serial & pc);
#endif


private:
    /// Touch panel parameters - common to both resistive and capacitive

    /// Data type to indicate which TP, if any, is in use.
    typedef enum {
        TP_NONE,            ///< no touch panel in use
        TP_RES,             ///< resistive touch panel using RA8875
        TP_CAP,             ///< capacitive touch panel using FT5206
    } WhichTP_T;

    /// boolean flag set true when using Capacitive touch panel, and false
    /// for resistive.
    WhichTP_T useTouchPanel;    ///< Indicates which TP is selected for use.

    /// Touch State used by TouchPanelReadable. See @ref TouchCode_t.
    TouchCode_t touchState;

    ////////////////// Start of Capacitive Touch Panel parameters

    uint8_t getTouchPositions(void);
    void TouchPanelISR(void);
    uint16_t numberOfTouchPoints;
    uint8_t gesture;            ///< Holds the reported gesture information.

    /// Touch Information data structure
    typedef struct {
        uint8_t touchID;        ///< Contains the touch ID, which is the "order" of touch, from 0 to n-1
        TouchCode_t touchCode;  ///< Contains the touch code; no_touch, touch, held, release
        point_t coordinates;    ///< Contains the X,Y coordinate of the touch
    } touchInfo_T;

    touchInfo_T touchInfo[5];   /// Contains the actual touch information in an array from 0 to n-1

    InterruptIn * m_irq;
    I2C * m_i2c;
    int m_addr;
    uint8_t data[2];

    bool panelTouched;
    void writeRegister8(uint8_t reg, uint8_t val);
    uint8_t readRegister8(uint8_t reg);


    ////////////////// Start of Resistive Touch Panel parameters

    /// Resistive Touch Panel register name definitions
    #define TPCR0   0x70
    #define TPCR1   0x71
    #define TPXH    0x72
    #define TPYH    0x73
    #define TPXYL   0x74
    #define INTC1   0xF0
    #define INTC2   0xF1

    /// Specify the default settings for the Touch Panel, where different from the chip defaults
    #define TP_MODE_DEFAULT             TP_MODE_AUTO
    #define TP_DEBOUNCE_DEFAULT         TP_DEBOUNCE_ON
    #define TP_ADC_CLKDIV_DEFAULT       TP_ADC_CLKDIV_8

    #define TP_ADC_SAMPLE_DEFAULT_CLKS  TP_ADC_SAMPLE_8192_CLKS

    /// Other Touch Panel params
    #define TPBUFSIZE   16       // Depth of the averaging buffers for x and y data

    // Needs both a ticker and a timer. (could have created a timer from the ticker, but this is easier).
    // on a touch, the timer is reset.
    // the ticker monitors the timer to see if it has been a long time since
    // a touch, and if so, it then clears the sample counter so it doesn't get partial old
    // and partial new.

    /// Touch Panel ticker
    Ticker touchTicker;

    /// Touch Panel timer
    Timer touchTimer;

    /// keeps track of which sample we're collecting to filter out the noise.
    int touchSample;

    /// Private function for touch ticker callback.
    void _TouchTicker(void);

    /// Touch Panel calibration matrix.
    tpMatrix_t tpMatrix;

    ////////////////// End of Touch Panel parameters


    /// Internal function to put a character using the built-in (internal) font engine
    ///
    /// @param[in] c is the character to put to the screen.
    /// @returns the character put.
    ///
    int _internal_putc(int c);

    /// Internal function to put a character using the external font engine
    ///
    /// @param[in] c is the character to put to the screen.
    /// @returns the character put.
    ///
    int _external_putc(int c);

    /// Internal function to get the actual width of a character when using the external font engine
    ///
    /// @param[in] c is the character to get the width.
    /// @returns the width in pixels of the character. zero if not found.
    ///
    int _external_getCharWidth(int c);

    /// Write color to an RGB register set
    ///
    /// This API takes a color value, and writes it into the specified
    /// color registers, which are a trio of 3 registers. The actual
    /// trio write is performed based on whether the display is configured
    /// for 8 or 16 bits per pixel.
    ///
    /// @param[in] regAddr is the register address starting the trio
    /// @param[in] color is the color to write
    /// @returns success/failure code. See @ref RetCode_t.
    ///
    RetCode_t _writeColorTrio(uint8_t regAddr, color_t color);

    /// Read color from an RGB register set
    ///
    /// This API reads a color value from a trio of registers. The actual
    /// trio write is performed based on whether the display is configured
    /// for 8 or 16 bits per pixel.
    ///
    /// @param[in] regAddr is the register address starting the trio
    /// @returns color_t value
    ///
    color_t _readColorTrio(uint8_t regAddr);


    /// Convert a 16-bit color value to an 8-bit value
    ///
    /// @param[in] c16 is the 16-bit color value to convert.
    /// @returns 8-bit color value.
    ///
    uint8_t _cvt16to8(color_t c16);

    /// Convert an 8-bit color value to a 16-bit value
    ///
    /// @param[in] c8 is the 8-bit color value to convert.
    /// @returns 16-bit color value.
    ///
    color_t _cvt8to16(uint8_t c8);

    /// Select the peripheral to use it.
    ///
    /// @param[in] chipsel when true will select the peripheral, and when false
    ///     will deselect the chip. This is the logical selection, and
    ///     the pin selection is the invert of this.
    /// @returns success/failure code. See @ref RetCode_t.
    ///
    RetCode_t _select(bool chipsel);

    /// Wait while the status register indicates the controller is busy.
    ///
    /// @param[in] mask is the mask of bits to monitor.
    /// @returns true if a normal exit.
    /// @returns false if a timeout exit.
    ///
    bool _WaitWhileBusy(uint8_t mask);

    /// Wait while the the register anded with the mask is true.
    ///
    /// @param[in] reg is the register to monitor
    /// @param[in] mask is the bit mask to monitor
    /// @returns true if it was a normal exit
    /// @returns false if it was a timeout that caused the exit.
    ///
    bool _WaitWhileReg(uint8_t reg, uint8_t mask);

    /// set the spi port to either the write or the read speed.
    ///
    /// This is a private API used to toggle between the write
    /// and the read speed for the SPI port to the RA8875, since
    /// it can accept writes faster than reads.
    ///
    /// @param[in] writeSpeed when true selects the write frequency,
    ///     and when false it selects the read frequency.
    ///
    void _setWriteSpeed(bool writeSpeed);

    /// The most primitive - to write a data value to the SPI interface.
    ///
    /// @param[in] data is the value to write.
    /// @returns a value read from the port, since SPI is often shift
    ///     in while shifting out.
    ///
    unsigned char _spiwrite(unsigned char data);

    /// The most primitive - to read a data value to the SPI interface.
    ///
    /// This is really just a specialcase of the write command, where
    /// the value zero is written in order to read.
    ///
    /// @returns a value read from the port, since SPI is often shift
    ///     in while shifting out.
    ///
    unsigned char _spiread();

    const uint8_t * pKeyMap;

    SPI spi;                        ///< spi port
    bool spiWriteSpeed;             ///< indicates if the current mode is write or read
    unsigned long spiwritefreq;     ///< saved write freq
    unsigned long spireadfreq;      ///< saved read freq
    DigitalOut cs;                  ///< chip select pin, assumed active low
    DigitalOut res;                 ///< reset pin, assumed active low

    // display metrics to avoid lengthy spi read queries
    uint8_t screenbpp;              ///< configured bits per pixel
    dim_t screenwidth;              ///< configured screen width
    dim_t screenheight;             ///< configured screen height
    rect_t windowrect;              ///< window commands are held here for speed of access
    bool portraitmode;              ///< set true when in portrait mode (w,h are reversed)

    const unsigned char * font;     ///< reference to an external font somewhere in memory
    uint8_t extFontHeight;          ///< computed from the font table when the user sets the font
    uint8_t extFontWidth;           ///< computed from the font table when the user sets the font

    loc_t cursor_x, cursor_y;       ///< used for external fonts only

    #ifdef PERF_METRICS
    typedef enum
    {
        PRF_CLS,
        PRF_DRAWPIXEL,
        PRF_PIXELSTREAM,
        PRF_BOOLSTREAM,
        PRF_READPIXEL,
        PRF_READPIXELSTREAM,
        PRF_DRAWLINE,
        PRF_DRAWRECTANGLE,
        PRF_DRAWROUNDEDRECTANGLE,
        PRF_DRAWTRIANGLE,
        PRF_DRAWCIRCLE,
        PRF_DRAWELLIPSE,
        PRF_BLOCKMOVE,
        METRICCOUNT
    } method_e;
    unsigned long metrics[METRICCOUNT];
    unsigned long idletime_usec;
    void RegisterPerformance(method_e method);
    Timer performance;
    #endif

    RetCode_t _printCallback(RA8875::filecmd_t cmd, uint8_t * buffer, uint16_t size);

    FILE * _printFH;             ///< PrintScreen file handle

    RetCode_t privateCallback(filecmd_t cmd, uint8_t * buffer, uint16_t size) {
        if (c_callback != NULL) {
            return (*c_callback)(cmd, buffer, size);
        }
        else {
            if (obj_callback != NULL && method_callback != NULL) {
                return (obj_callback->*method_callback)(cmd, buffer, size);
            }
        }
        return noerror;
    }

    RetCode_t (* c_callback)(filecmd_t cmd, uint8_t * buffer, uint16_t size);
    FPointerDummy  *obj_callback;
    RetCode_t (FPointerDummy::*method_callback)(filecmd_t cmd, uint8_t * buffer, uint16_t size);
    RetCode_t (* idle_callback)(IdleReason_T reason);
};


//}     // namespace

//using namespace SW_graphics;


#ifdef TESTENABLE
//      ______________  ______________  ______________  _______________
//     /_____   _____/ /  ___________/ /  ___________/ /_____   ______/
//          /  /      /  /            /  /                  /  /
//         /  /      /  /___         /  /__________        /  /
//        /  /      /  ____/        /__________   /       /  /
//       /  /      /  /                       /  /       /  /
//      /  /      /  /__________  ___________/  /       /  /
//     /__/      /_____________/ /_____________/       /__/

#include "WebColors.h"
#include <algorithm>

extern "C" void mbed_reset();

/// This activates a small set of tests for the graphics library.
///
/// Call this API and pass it the reference to the display class.
/// It will then run a series of tests. It accepts interaction via
/// stdin to switch from automatic test mode to manual, run a specific
/// test, or to exit the test mode.
///
/// @param[in] lcd is a reference to the display class.
/// @param[in] pc is a reference to a serial interface, typically the USB to PC.
///
void RunTestSet(RA8875 & lcd, Serial & pc);


// To enable the test code, uncomment this section, or copy the
// necessary pieces to your "main()".
//
// #include "mbed.h"
// #include "RA8875.h"
// RA8875 lcd(p5, p6, p7, p12, NC, "tft");    // MOSI, MISO, SCK, /ChipSelect, /reset, name
// Serial pc(USBTX, USBRX);
// extern "C" void mbed_reset();
// int main()
// {
//     pc.baud(460800);    // I like a snappy terminal, so crank it up!
//     pc.printf("\r\nRA8875 Test - Build " __DATE__ " " __TIME__ "\r\n");
//
//     pc.printf("Turning on display\r\n");
//     lcd.init();
//     lcd.Reset();
//     lcd.Power(true);  // display power is on, but the backlight is independent
//     lcd.Backlight(0.5);
//     RunTestSet(lcd, pc);
// }

#endif // TESTENABLE

#endif
