/// RA8875 Display Controller Library.
///
/// This is being created for a Raio RA8875-based display from buydisplay.com,
/// which is 480 x 272 using a 4-wire SPI interface. Support is provided for
/// both a keypad and a resistive touch-screen.
///
/// This driver has been fully tested with an 800 x 480 variant (also using
/// 4-wire SPI).
///
/// 20161106: Updated the initialization to set the various registers based on
///           the BuyDisplay.com example code. This altered several registers
///           for the 800x480 display driver.
///
#include "RA8875.h"

//#include "Utility.h"            // private memory manager
#ifndef UTILITY_H
#define swMalloc malloc         // use the standard
#define swFree free
#endif

//#define DEBUG "RAIO"
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

// Defaults. Users can override this with the init() method.
#define RA8875_DISPLAY_WIDTH  480
#define RA8875_DISPLAY_HEIGHT 272
#define RA8875_COLORDEPTH_BPP 16    /* Not an API */

#ifdef PERF_METRICS
#define PERFORMANCE_RESET performance.reset()
#define REGISTERPERFORMANCE(a) RegisterPerformance(a)
#define COUNTIDLETIME(a) CountIdleTime(a)
static const char *metricsName[] = {
    "Cls", "Pixel", "Pixel Stream", "Boolean Stream",
    "Read Pixel", "Read Pixel Stream",
    "Line",
    "Rectangle", "Rounded Rectangle",
    "Triangle", "Circle", "Ellipse"
};
uint16_t commandsUsed[256];  // track which commands are used with simple counter of number of hits.
#else
#define PERFORMANCE_RESET
#define REGISTERPERFORMANCE(a)
#define COUNTIDLETIME(a)
#endif

// When it is going to poll a register for completion, how many
// uSec should it wait between each polling activity.
#define POLLWAITuSec 10

// Private RawKeyMap for the Keyboard interface
static const uint8_t DefaultKeyMap[22] = {
    0,
    1,  2,  3,  4,  5,  6,  7,  8,  9, 10,
    11, 12, 13, 14, 15, 16, 17, 18, 19, 20,
    255
};

static const char * ErrMessages[] = {
    "noerror",                ///< no errors, command completed successfully
    "bad parameter",          ///< one or more parameters are invalid
    "file not found",         ///< specified file could not be found
    "not bmp format",         ///< file is not a .bmp file
    "not ico format",         ///< file is not a .ico file
    "not supported format",   ///< file format is not yet supported
    "image too big",          ///< image is too large for the screen
    "not enough ram",         ///< could not allocate ram for scanline
    "touch cal. timeout",     ///< calibration could not complete in time
    "external abort",         ///< during an idle callback, the user code initiated an abort
};

RA8875::RA8875(PinName mosi, PinName miso, PinName sclk, PinName csel, PinName reset,
    const char *name)
    : GraphicsDisplay(name)
    , spi(mosi, miso, sclk, csel)
    , cs(csel)
    , res(reset)
{
    useTouchPanel = TP_NONE;
    m_irq = NULL;
    m_i2c = NULL;
    c_callback = NULL;
    obj_callback = NULL;
    method_callback = NULL;
    idle_callback = NULL;
}


RA8875::RA8875(PinName mosi, PinName miso, PinName sclk, PinName csel, PinName reset,
    PinName sda, PinName scl, PinName irq, const char * name)
    : GraphicsDisplay(name)
    , spi(mosi, miso, sclk, csel)
    , cs(csel)
    , res(reset)
{
    useTouchPanel = TP_CAP;
    m_irq = new InterruptIn(irq);
    m_i2c = new I2C(sda, scl);
    c_callback = NULL;
    obj_callback = NULL;
    method_callback = NULL;
    idle_callback = NULL;

    // Cap touch panel config
    m_addr = (FT5206_I2C_ADDRESS << 1);
    m_i2c->frequency(FT5206_I2C_FREQUENCY);

    // Interrupt
    m_irq->mode(PullUp);
    m_irq->enable_irq();
    m_irq->fall(callback(this, &RA8875::TouchPanelISR));
    TouchPanelInit();
}


//RA8875::~RA8875()
//{
//}

RetCode_t RA8875::init(int width, int height, int color_bpp, uint8_t poweron, bool keypadon, bool touchscreenon)
{
    font = NULL;                                // no external font, use internal.
    pKeyMap = DefaultKeyMap;                    // set default key map
    _select(false);                             // deselect the display
    frequency(RA8875_DEFAULT_SPI_FREQ);         // data rate
    Reset();
    // Set PLL based on display size from buy-display.com sample code
    if (width == 800) {
        WriteCommand(0x88, 0x0C);               // PLLC1 - Phase Lock Loop registers
    } else {
        WriteCommand(0x88, 0x0B);               // PLLC1 - Phase Lock Loop registers
    }
    wait_ms(1);
    WriteCommand(0x89, 0x02);
    wait_ms(1);

    // System Config Register (SYSR)
    screenbpp = color_bpp;
    if (color_bpp == 16) {
        WriteCommand(0x10, 0x0C);               // 16-bpp (65K colors) color depth, 8-bit interface
    } else { // color_bpp == 8
        WriteCommand(0x10, 0x00);               // 8-bpp (256 colors)
    }

    // Set Pixel Clock Setting Register (PCSR) based on display size from buy-display.com sample code
    if (width == 800) {
        WriteCommand(0x04, 0x81);               // PDAT on PCLK falling edge, PCLK = 4 x System Clock
        wait_ms(1);

        // Horizontal Settings
        screenwidth = width;
        WriteCommand(0x14, width/8 - 1);            //HDWR//Horizontal Display Width Setting Bit[6:0]
        WriteCommand(0x15, 0x00);                   //HNDFCR//Horizontal Non-Display Period fine tune Bit[3:0]
        WriteCommand(0x16, 0x03);                   //HNDR//Horizontal Non-Display Period Bit[4:0]
        WriteCommand(0x17, 0x03);                   //HSTR//HSYNC Start Position[4:0]
        WriteCommand(0x18, 0x0B);                   //HPWR//HSYNC Polarity ,The period width of HSYNC.

        // Vertical Settings
        screenheight = height;
        WriteCommand(0x19, (height-1)&0xFF);        //VDHR0 //Vertical Display Height Bit [7:0]
        WriteCommand(0x1a, (height-1)>>8);          //VDHR1 //Vertical Display Height Bit [8]
        WriteCommand(0x1b, 0x20);                   //VNDR0 //Vertical Non-Display Period Bit [7:0]
        WriteCommand(0x1c, 0x00);                   //VNDR1 //Vertical Non-Display Period Bit [8]
        WriteCommand(0x1d, 0x16);                   //VSTR0 //VSYNC Start Position[7:0]
        WriteCommand(0x1e, 0x00);                   //VSTR1 //VSYNC Start Position[8]
        WriteCommand(0x1f, 0x01);                   //VPWR  //VSYNC Polarity ,VSYNC Pulse Width[6:0]
    } else {
        WriteCommand(0x04, 0x82);               // PDAT on PCLK falling edge, PCLK = 4 x System Clock
        wait_ms(1);

        // Horizontal Settings
        screenwidth = width;
        WriteCommand(0x14, width/8 - 1);            //HDWR//Horizontal Display Width Setting Bit[6:0]
        WriteCommand(0x15, 0x02);                   //HNDFCR//Horizontal Non-Display Period fine tune Bit[3:0]
        WriteCommand(0x16, 0x03);                   //HNDR//Horizontal Non-Display Period Bit[4:0]
        WriteCommand(0x17, 0x01);                   //HSTR//HSYNC Start Position[4:0]
        WriteCommand(0x18, 0x03);                   //HPWR//HSYNC Polarity ,The period width of HSYNC.

        // Vertical Settings
        screenheight = height;
        WriteCommand(0x19, (height-1)&0xFF);        //VDHR0 //Vertical Display Height Bit [7:0]
        WriteCommand(0x1a, (height-1)>>8);          //VDHR1 //Vertical Display Height Bit [8]
        WriteCommand(0x1b, 0x0F);                   //VNDR0 //Vertical Non-Display Period Bit [7:0]
        WriteCommand(0x1c, 0x00);                   //VNDR1 //Vertical Non-Display Period Bit [8]
        WriteCommand(0x1d, 0x0e);                   //VSTR0 //VSYNC Start Position[7:0]
        WriteCommand(0x1e, 0x06);                   //VSTR1 //VSYNC Start Position[8]
        WriteCommand(0x1f, 0x01);                   //VPWR  //VSYNC Polarity ,VSYNC Pulse Width[6:0]
    }

    portraitmode = false;

    if (width >= 800 && height >= 480 && color_bpp > 8) {
        WriteCommand(0x20, 0x00);               // DPCR - 1-layer mode when the resolution is too high
    } else {
        WriteCommand(0x20, 0x80);               // DPCR - 2-layer mode
    }

    // Set display image to Blue on Black as default
    window(0,0, width, height);             // Initialize to full screen
    SetTextCursorControl();
    foreground(Blue);
    background(Black);
    cls(3);

    Power(poweron);
    Backlight_u8(poweron);
    if (keypadon)
        KeypadInit();
    if (touchscreenon) {
        if (useTouchPanel == TP_NONE)
            useTouchPanel = TP_RES;
        TouchPanelInit();
    }
#ifdef PERF_METRICS
    performance.start();
    ClearPerformance();
#endif
    return noerror;
}


RetCode_t RA8875::Reset(void)
{
    RetCode_t ret;

    #if 0
    if (res != (PinName)NC) {
        res = 0;                            // Active low - assert reset
        wait_ms(2);                         // must be > 1024 clock periods. (@25 MHz, this is 40.96 usec)
        res = 1;                            // de-assert reset
    }
    #endif
    ret = WriteCommand(0x01, 0x01);         // Apply Display Off, Reset
    wait_ms(2);                             // no idea if I need to wait, or how long
    if (ret == noerror) {
        ret = WriteCommand(0x01, 0x00);     // Display off, Remove reset
        wait_ms(2);                         // no idea if I need to wait, or how long
    }
    return ret;
}


const char * RA8875::GetErrorMessage(RetCode_t code)
{
    if (code >= LastErrCode)
        code = bad_parameter;
    return ErrMessages[code];
}


uint16_t RA8875::GetDrawingLayer(void)
{
    return (ReadCommand(0x41) & 0x01);
}


RetCode_t RA8875::SelectDrawingLayer(uint16_t layer, uint16_t * prevLayer)
{
    unsigned char mwcr1 = ReadCommand(0x41); // retain all but the currently selected layer

    if (prevLayer)
        *prevLayer = mwcr1 & 1;

    mwcr1 &= ~0x01; // remove the current layer
    if (screenwidth >= 800 && screenheight >= 480 && screenbpp > 8) {
        layer = 0;
    } else if (layer > 1) {
        layer = 0;
    }
    return WriteCommand(0x41, mwcr1 | layer);
}


RA8875::LayerMode_T RA8875::GetLayerMode(void)
{
    return (LayerMode_T)(ReadCommand(0x52) & 0x7);
}


RetCode_t RA8875::SetLayerMode(LayerMode_T mode)
{
    unsigned char ltpr0 = ReadCommand(0x52) & ~0x7; // retain all but the display layer mode

    if (mode <= (LayerMode_T)6) {
        WriteCommand(0x52, ltpr0 | (mode & 0x7));
        return noerror;
    } else {
        return bad_parameter;
    }
}


RetCode_t RA8875::SetLayerTransparency(uint8_t layer1, uint8_t layer2)
{
    if (layer1 > 8)
        layer1 = 8;
    if (layer2 > 8)
        layer2 = 8;
    WriteCommand(0x53, ((layer2 & 0xF) << 4) | (layer1 & 0xF));
    return noerror;
}


RetCode_t RA8875::SetBackgroundTransparencyColor(color_t color)
{
    return _writeColorTrio(0x67, color);
}


color_t RA8875::GetBackgroundTransparencyColor(void)
{
    RGBQUAD q;

    q.rgbRed = ReadCommand(0x67);
    q.rgbGreen = ReadCommand(0x68);
    q.rgbBlue = ReadCommand(0x69);
    return RGBQuadToRGB16(&q, 0);
}


RetCode_t RA8875::KeypadInit(bool scanEnable, bool longDetect, uint8_t sampleTime, uint8_t scanFrequency,
                             uint8_t longTimeAdjustment, bool interruptEnable, bool wakeupEnable)
{
    uint8_t value = 0;

    if (sampleTime > 3 || scanFrequency > 7 || longTimeAdjustment  > 3)
        return bad_parameter;
    value |= (scanEnable) ? 0x80 : 0x00;
    value |= (longDetect) ? 0x40 : 0x00;
    value |= (sampleTime & 0x03) << 4;
    value |= (scanFrequency & 0x07);
    WriteCommand(0xC0, value);   // KSCR1 - Enable Key Scan (and ignore possibility of an error)

    value = 0;
    value |= (wakeupEnable) ? 0x80 : 0x00;
    value |= (longTimeAdjustment & 0x03) << 2;
    WriteCommand(0xC1, value);  // KSCR2 - (and ignore possibility of an error)

    value = ReadCommand(0xF0);          // (and ignore possibility of an error)
    value &= ~0x10;
    value |= (interruptEnable) ? 0x10 : 0x00;
    return WriteCommand(0xF0, value);   // INT
}


RetCode_t RA8875::SetKeyMap(const uint8_t * CodeList)
{
    pKeyMap = CodeList;
    return noerror;
}


bool RA8875::readable(void)
{
    return (ReadCommand(0xF1) & 0x10);  // check KS status - true if kbhit
}


uint8_t RA8875::getc(void)
{
    //#define GETC_DEV      // for development
#ifdef GETC_DEV
    uint8_t keyCode1, keyCode2;
#endif
    uint8_t keyCode3;
    static uint8_t count = 0;
    uint8_t col, row;
    uint8_t key;

    while (!readable()) {
        wait_us(POLLWAITuSec);
        // COUNTIDLETIME(POLLWAITuSec);     // As it is voluntary to call the getc and pend. Don't tally it.
        if (idle_callback) {
            if (external_abort == (*idle_callback)(getc_wait)) {
                return 0;
            }
        }
    }
    // read the key press number
    uint8_t keyNumReg = ReadCommand(0xC1) & 0x03;
    count++;
    switch (keyNumReg) {
        case 0x01:      // one key
            keyCode3 = ReadCommand(0xC2);
#ifdef GETC_DEV
            keyCode2 = 0;
            keyCode1 = 0;
#endif
            break;
        case 0x02:      // two keys
            keyCode3 = ReadCommand(0xC3);
#ifdef GETC_DEV
            keyCode2 = ReadCommand(0xC2);
            keyCode1 = 0;
#endif
            break;
        case 0x03:      // three keys
            keyCode3 = ReadCommand(0xC4);
#ifdef GETC_DEV
            keyCode2 = ReadCommand(0xC3);
            keyCode1 = ReadCommand(0xC2);
#endif
            break;
        default:         // no keys (key released)
            keyCode3 = 0xFF;
#ifdef GETC_DEV
            keyCode2 = 0;
            keyCode1 = 0;
#endif
            break;
    }
    if (keyCode3 == 0xFF)
        key = pKeyMap[0];                    // Key value 0
    else {
        row = (keyCode3 >> 4) & 0x03;
        col = (keyCode3 &  7);
        key = row * 5 + col + 1;    // Keys value 1 - 20
        if (key > 21) {
            key = 21;
        }
        key = pKeyMap[key];
        key |= (keyCode3 & 0x80);   // combine the key held flag
    }
#if GETC_DEV // for Development only
    SetTextCursor(0, 20);
    printf("   Reg: %02x\r\n", keyNumReg);
    printf("  key1: %02x\r\n", keyCode1);
    printf("  key2: %02x\r\n", keyCode2);
    printf("  key3: %02x\r\n", keyCode3);
    printf(" count: %02X\r\n", count);
    printf("   key: %02X\r\n", key);
#endif
    WriteCommand(0xF1, 0x10);       // Clear KS status
    return key;
}


#ifdef PERF_METRICS
void RA8875::ClearPerformance()
{
    int i;

    for (i=0; i<METRICCOUNT; i++)
        metrics[i] = 0;
    idletime_usec = 0;
    for (i=0; i<256; i++)
        commandsUsed[i] = 0;
}


void RA8875::RegisterPerformance(method_e method)
{
    unsigned long elapsed = performance.read_us();

    if (method < METRICCOUNT && elapsed > metrics[method])
        metrics[method] = elapsed;
}


void RA8875::CountIdleTime(uint32_t t)
{
    idletime_usec += t;
}


void RA8875::ReportPerformance(Serial & pc)
{
    int i;

    pc.printf("\r\nPerformance Metrics\r\n");
    for (i=0; i<METRICCOUNT; i++) {
        pc.printf("%10d uS %s\r\n", metrics[i], metricsName[i]);
    }
    pc.printf("%10d uS Idle time polling display for ready.\r\n", idletime_usec);
    for (i=0; i<256; i++) {
        if (commandsUsed[i])
            pc.printf("Command %02X used %5d times.\r\n", i, commandsUsed[i]);
    }
}
#endif


bool RA8875::Intersect(rect_t rect, point_t p)
{
    if (p.x >= min(rect.p1.x, rect.p2.x) && p.x <= max(rect.p1.x, rect.p2.x)
    && p.y >= min(rect.p1.y, rect.p2.y) && p.y <= max(rect.p1.y, rect.p2.y))
        return true;
    else
        return false;
}


bool RA8875::Intersect(rect_t rect1, rect_t rect2)
{
#if 1
    // If one rectangle is on left side of other
    if (max(rect1.p1.x,rect1.p2.x) < min(rect2.p1.x,rect2.p2.x)
        || min(rect1.p1.x, rect1.p2.x) > max(rect2.p1.x, rect2.p2.x))
        return false;
    // If one rectangle is above other
    if (max(rect1.p1.y, rect1.p2.y) < min(rect2.p1.y, rect2.p2.y)
        || min(rect1.p1.y, rect1.p2.y) > max(rect2.p1.y, rect2.p2.y))
        return false;
    return true;            // all that's left is they overlap
#else
    point_t bl, tr;
    bl.x = rect2.p1.x;
    bl.y = rect2.p2.y;
    tr.x = rect2.p2.x;
    tr.y = rect2.p1.y;
    if (Intersect(rect1, rect2.p1) || Intersect(rect1, rect2.p2)
        || Intersect(rect1, bl) || Intersect(rect1, tr))
        return true;
    else
        return false;
#endif
}


bool RA8875::Intersect(rect_t * pRect1, const rect_t * pRect2)
{
    if (Intersect(*pRect1, *pRect2)) {
        rect_t iSect;

        iSect.p1.x = max(min(pRect1->p1.x,pRect1->p2.x),min(pRect2->p1.x,pRect2->p2.x));
        iSect.p1.y = max(min(pRect1->p1.y,pRect1->p2.y),min(pRect2->p1.y,pRect2->p2.y));
        iSect.p2.x = min(max(pRect1->p1.x,pRect1->p2.x),max(pRect2->p1.x,pRect2->p2.x));
        iSect.p2.y = min(max(pRect1->p1.y,pRect1->p2.y),max(pRect2->p1.y,pRect2->p2.y));
        *pRect1 = iSect;
        return true;
    } else {
        return false;
    }
}


RetCode_t RA8875::WriteCommandW(uint8_t command, uint16_t data)
{
    WriteCommand(command, data & 0xFF);
    WriteCommand(command+1, data >> 8);
    return noerror;
}


RetCode_t RA8875::WriteCommand(unsigned char command, unsigned int data)
{
#ifdef PERF_METRICS
    if (commandsUsed[command] < 65535)
        commandsUsed[command]++;
#endif
    _select(true);
    _spiwrite(0x80);            // RS:1 (Cmd/Status), RW:0 (Write)
    _spiwrite(command);
    if (data <= 0xFF) {   // only if in the valid range
        _spiwrite(0x00);
        _spiwrite(data);
    }
    _select(false);
    return noerror;
}


RetCode_t RA8875::WriteDataW(uint16_t data)
{
    _select(true);
    _spiwrite(0x00);            // RS:0 (Data), RW:0 (Write)
    _spiwrite(data & 0xFF);
    _spiwrite(data >> 8);
    _select(false);
    return noerror;
}


RetCode_t RA8875::WriteData(unsigned char data)
{
    _select(true);
    _spiwrite(0x00);            // RS:0 (Data), RW:0 (Write)
    _spiwrite(data);
    _select(false);
    return noerror;
}


unsigned char RA8875::ReadCommand(unsigned char command)
{
    WriteCommand(command);
    return ReadData();
}

uint16_t RA8875::ReadCommandW(unsigned char command)
{
    WriteCommand(command);
    return ReadDataW();
}

unsigned char RA8875::ReadData(void)
{
    unsigned char data;

    _select(true);
    _spiwrite(0x40);            // RS:0 (Data), RW:1 (Read)
    data = _spiread();
    _select(false);
    return data;
}


uint16_t RA8875::ReadDataW(void)
{
    uint16_t data;

    _select(true);
    _spiwrite(0x40);            // RS:0 (Data), RW:1 (Read)
    data  = _spiread();
    data |= (_spiread() << 8);
    _select(false);
    return data;
}


unsigned char RA8875::ReadStatus(void)
{
    unsigned char data;

    _select(true);
    _spiwrite(0xC0);            // RS:1 (Cmd/Status), RW:1 (Read) (Read STSR)
    data = _spiread();
    _select(false);
    return data;
}


/// @todo add a timeout and return false, but how long
/// to wait since some operations can be very long.
bool RA8875::_WaitWhileBusy(uint8_t mask)
{
    int i = 20000/POLLWAITuSec; // 20 msec max

    while (i-- && ReadStatus() & mask) {
        wait_us(POLLWAITuSec);
        COUNTIDLETIME(POLLWAITuSec);
        if (idle_callback) {
            if (external_abort == (*idle_callback)(status_wait)) {
                return false;
            }
        }
    }
    if (i)
        return true;
    else
        return false;
}


/// @todo add a timeout and return false, but how long
/// to wait since some operations can be very long.
bool RA8875::_WaitWhileReg(uint8_t reg, uint8_t mask)
{
    int i = 20000/POLLWAITuSec; // 20 msec max

    while (i-- && ReadCommand(reg) & mask) {
        wait_us(POLLWAITuSec);
        COUNTIDLETIME(POLLWAITuSec);
        if (idle_callback) {
            if (external_abort == (*idle_callback)(command_wait)) {
                return false;
            }
        }
    }
    if (i)
        return true;
    else
        return false;
}

// RRRR RGGG GGGB BBBB
// 4321 0543 2104 3210
//           RRRG GGBB
//           2102 1010
uint8_t RA8875::_cvt16to8(color_t c16)
{
    return ((c16 >> 8) & 0xE0)
        | ((c16 >> 6) & 0x1C)
        | ((c16 >> 3) & 0x03);
}

//           RRRG GGBB
//           2102 1010
// RRRR RGGG GGGB BBBB
// 2101 0543 2104 3210
color_t RA8875::_cvt8to16(uint8_t c8)
{
    color_t c16;
    color_t temp = (color_t)c8;

    c16 = ((temp & 0xE0) << 8)
        | ((temp & 0xC0) << 5)
        | ((temp & 0x1C) << 6)
        | ((temp & 0x1C) << 3)
        | ((temp & 0x03) << 3)
        | ((temp & 0x03) << 1)
        | ((temp & 0x03) >> 1);
    c16 = (c16 << 8) | (c16 >> 8);
    return c16;
}

RetCode_t RA8875::_writeColorTrio(uint8_t regAddr, color_t color)
{
    RetCode_t rt = noerror;

    if (screenbpp == 16) {
        WriteCommand(regAddr+0, (color>>11));                  // BGCR0
        WriteCommand(regAddr+1, (unsigned char)(color>>5));    // BGCR1
        rt = WriteCommand(regAddr+2, (unsigned char)(color));       // BGCR2
    } else {
        uint8_t r, g, b;

        // RRRR RGGG GGGB BBBB      RGB
        // RRR   GGG    B B
        r = (uint8_t)((color) >> 13);
        g = (uint8_t)((color) >> 8);
        b = (uint8_t)((color) >> 3);
        WriteCommand(regAddr+0, r);  // BGCR0
        WriteCommand(regAddr+1, g);  // BGCR1
        rt = WriteCommand(regAddr+2, b);  // BGCR2
    }
    return rt;
}

color_t RA8875::_readColorTrio(uint8_t regAddr)
{
    color_t color;
    uint8_t r, g, b;

    r = ReadCommand(regAddr+0);
    g = ReadCommand(regAddr+1);
    b = ReadCommand(regAddr+2);
    if (screenbpp == 16) {
        // 000R RRRR 00GG GGGG 000B BBBB
        // RRRR RGGG GGGB BBBB
        color  = (r & 0x1F) << 11;
        color |= (g & 0x3F) << 5;
        color |= (b & 0x1F);
    } else {
        // RRRG GGBB
        // RRRR RGGG GGGB BBBB
        color  = (r & 0x07) << 13;
        color |= (g & 0x07) << 8;
        color |= (b & 0x03) << 3;
    }
    return color;
}


dim_t RA8875::fontwidth(void)
{
    if (font == NULL)
        return (((ReadCommand(0x22) >> 2) & 0x3) + 1) * 8;
    else
        return extFontWidth;
}


dim_t RA8875::fontheight(void)
{
    if (font == NULL)
        return (((ReadCommand(0x22) >> 0) & 0x3) + 1) * 16;
    else
        return extFontHeight;
}


RetCode_t RA8875::locate(textloc_t column, textloc_t row)
{
    return SetTextCursor(column * fontwidth(), row * fontheight());
}


int RA8875::columns(void)
{
    return screenwidth / fontwidth();
}


int RA8875::rows(void)
{
    return screenheight / fontheight();
}


dim_t RA8875::width(void)
{
    if (portraitmode)
        return screenheight;
    else
        return screenwidth;
}


dim_t RA8875::height(void)
{
    if (portraitmode)
        return screenwidth;
    else
        return screenheight;
}


dim_t RA8875::color_bpp(void)
{
    return screenbpp;
}

RetCode_t RA8875::SetTextCursor(point_t p)
{
    return SetTextCursor(p.x, p.y);
}

RetCode_t RA8875::SetTextCursor(loc_t x, loc_t y)
{
    INFO("SetTextCursor(%d, %d)", x, y);
    cursor_x = x;     // set these values for non-internal fonts
    cursor_y = y;
    WriteCommandW(0x2A, x);
    WriteCommandW(0x2C, y);
    return noerror;
}

point_t RA8875::GetTextCursor(void)
{
    point_t p;

    p.x = GetTextCursor_X();
    p.y = GetTextCursor_Y();
    return p;
}

loc_t RA8875::GetTextCursor_Y(void)
{
    loc_t y;

    if (font == NULL)
        y = ReadCommand(0x2C) | (ReadCommand(0x2D) << 8);
    else
        y = cursor_y;
    INFO("GetTextCursor_Y = %d", y);
    return y;
}


loc_t RA8875::GetTextCursor_X(void)
{
    loc_t x;

    if (font == NULL)
        x = ReadCommand(0x2A) | (ReadCommand(0x2B) << 8);
    else
        x = cursor_x;
    INFO("GetTextCursor_X = %d", x);
    return x;
}


RetCode_t RA8875::SetTextCursorControl(cursor_t cursor, bool blink)
{
    unsigned char mwcr0 = ReadCommand(0x40) & 0x0F; // retain direction, auto-increase
    unsigned char mwcr1 = ReadCommand(0x41) & 0x01; // retain currently selected layer
    unsigned char horz = 0;
    unsigned char vert = 0;

    mwcr0 |= 0x80;                  // text mode
    if (cursor != NOCURSOR)
        mwcr0 |= 0x40;              // visible
    if (blink)
        mwcr0 |= 0x20;              // blink
    WriteCommand(0x40, mwcr0);      // configure the cursor
    WriteCommand(0x41, mwcr1);      // close the graphics cursor
    WriteCommand(0x44, 0x1f);       // The cursor flashing cycle
    switch (cursor) {
        case IBEAM:
            horz = 0x01;
            vert = 0x1F;
            break;
        case UNDER:
            horz = 0x07;
            vert = 0x01;
            break;
        case BLOCK:
            horz = 0x07;
            vert = 0x1F;
            break;
        case NOCURSOR:
        default:
            break;
    }
    WriteCommand(0x4e, horz);       // The cursor size horz
    WriteCommand(0x4f, vert);       // The cursor size vert
    return noerror;
}


RetCode_t RA8875::SetTextFont(RA8875::font_t font)
{
    if (/*font >= RA8875::ISO8859_1 && */ font <= RA8875::ISO8859_4) {
        WriteCommand(0x21, (unsigned int)(font));
        return noerror;
    } else {
        return bad_parameter;
    }
}


RetCode_t RA8875::SetOrientation(RA8875::orientation_t angle)
{
    uint8_t fncr1Val = ReadCommand(0x22);
    uint8_t dpcrVal = ReadCommand(0x20);

    fncr1Val &= ~0x10;      // remove the old direction bit
    dpcrVal &= ~0x0C;       // remove the old scan direction bits
    switch (angle) {
        case RA8875::normal:
            //fncr1Val |= 0x10;
            //dpcrVal |= 0x00;
            portraitmode = false;
            break;
        case RA8875::rotate_90:
            fncr1Val |= 0x10;
            dpcrVal |= 0x08;
            portraitmode = true;
            break;
        case RA8875::rotate_180:
            //fncr1Val |= 0x00;
            dpcrVal |= 0x0C;
            portraitmode = false;
            break;
        case RA8875::rotate_270:
            fncr1Val |= 0x10;
            dpcrVal |= 0x04;
            portraitmode = true;
            break;
        default:
            return bad_parameter;
    }
    INFO("Orientation: %d, %d", angle, portraitmode);
    WriteCommand(0x22, fncr1Val);
    return WriteCommand(0x20, dpcrVal);
}


RetCode_t RA8875::SetTextFontControl(fill_t fillit,
                                     RA8875::HorizontalScale hScale,
                                     RA8875::VerticalScale vScale,
                                     RA8875::alignment_t alignment)
{
    if (hScale >= 1 && hScale <= 4 &&
            vScale >= 1 && vScale <= 4) {
        uint8_t fncr1Val = ReadCommand(0x22);

        fncr1Val &= ~0x10;      // do not disturb the rotate flag
        if (alignment == align_full)
            fncr1Val |= 0x80;
        if (fillit == NOFILL)
            fncr1Val |= 0x40;
        fncr1Val |= ((hScale - 1) << 2);
        fncr1Val |= ((vScale - 1) << 0);
        return WriteCommand(0x22, fncr1Val);
    } else {
        return bad_parameter;
    }
}


RetCode_t RA8875::SetTextFontSize(RA8875::HorizontalScale hScale, RA8875::VerticalScale vScale)
{
    unsigned char reg = ReadCommand(0x22);

    if (vScale == -1)
        vScale = hScale;
    if (hScale >= 1 && hScale <= 4 && vScale >= 1 && vScale <= 4) {
        reg &= 0xF0;    // keep the high nibble as is.
        reg |= ((hScale - 1) << 2);
        reg |= ((vScale - 1) << 0);
        WriteCommand(0x22, reg);
        return noerror;
    } else {
        return bad_parameter;
    }
}

RetCode_t RA8875::GetTextFontSize(RA8875::HorizontalScale * hScale, RA8875::VerticalScale * vScale)
{
    unsigned char reg = ReadCommand(0x22);

    if (hScale)
        *hScale = 1 + (reg >> 2) & 0x03;
    if (vScale)
        *vScale = 1 + reg & 0x03;
    return noerror;
}

int RA8875::_putc(int c)
{
    if (font == NULL) {
        return _internal_putc(c);
    } else {
        return _external_putc(c);
    }
}



// Questions to ponder -
// - if we choose to wrap to the next line, because the character won't fit on the current line,
//      should it erase the space to the width of the screen (in case there is leftover junk there)?
// - it currently wraps from the bottom of the screen back to the top. I have pondered what
//      it might take to scroll the screen - but haven't thought hard enough about it.
//
int RA8875::_external_putc(int c)
{
    if (c) {
        if (c == '\r') {
            cursor_x = windowrect.p1.x;
        } else if (c == '\n') {
            cursor_y += extFontHeight;
        } else {
            dim_t charWidth, charHeight;
            const uint8_t * charRecord;

            charRecord = getCharMetrics(c, &charWidth, &charHeight);
            //int advance = charwidth(c);
            INFO("(%d,%d) - (%d,%d):(%d,%d), charWidth: %d '%c", cursor_x, cursor_y,
                windowrect.p1.x, windowrect.p1.y, windowrect.p2.x, windowrect.p2.y,
                charWidth, c);
            if (charRecord) {
                //cursor_x += advance;
                if (cursor_x + charWidth >= windowrect.p2.x) {
                    cursor_x = windowrect.p1.x;
                    cursor_y += charHeight;
                }
                if (cursor_y + charHeight >= windowrect.p2.y) {
                    cursor_y = windowrect.p1.y;               // @todo Should it scroll?
                }
                (void)character(cursor_x, cursor_y, c);
                cursor_x += charWidth;
            }
        }
    }
    return c;
}


int RA8875::_internal_putc(int c)
{
    if (c) {
        unsigned char mwcr0;

        mwcr0 = ReadCommand(0x40);
        if ((mwcr0 & 0x80) == 0x00) {
            WriteCommand(0x40, 0x80 | mwcr0);    // Put in Text mode if not already
        }
        if (c == '\r') {
            loc_t x;
            x = ReadCommand(0x30) | (ReadCommand(0x31) << 8);   // Left edge of active window
            WriteCommandW(0x2A, x);
        } else if (c == '\n') {
            loc_t y;
            y = ReadCommand(0x2C) | (ReadCommand(0x2D) << 8);   // current y location
            y += fontheight();
            if (y >= height())               // @TODO after bottom of active window, then scroll window?
                y = 0;
            WriteCommandW(0x2C, y);
        } else {
            WriteCommand(0x02);                 // RA8875 Internal Fonts
            _select(true);
            WriteData(c);
            _WaitWhileBusy(0x80);
            _select(false);
        }
    }
    return c;
}


RetCode_t RA8875::_StartGraphicsStream(void)
{
    WriteCommand(0x40,0x00);    // Graphics write mode
    WriteCommand(0x02);         // Prepare for streaming data
    return noerror;
}


RetCode_t RA8875::_EndGraphicsStream(void)
{
    return noerror;
}


RetCode_t RA8875::_putp(color_t pixel)
{
    WriteDataW((pixel>>8) | (pixel<<8));
    return noerror;
}


void RA8875::puts(loc_t x, loc_t y, const char * string)
{
    SetTextCursor(x,y);
    puts(string);
}


void RA8875::puts(const char * string)
{
    if (font == NULL) {
        WriteCommand(0x40,0x80);    // Put in Text mode if internal font
    }
    if (*string != '\0') {
        while (*string) {           // @TODO calling individual _putc is slower... optimizations?
            _putc(*string++);
        }
    }
}


RetCode_t RA8875::SetGraphicsCursor(loc_t x, loc_t y)
{
    WriteCommandW(0x46, x);
    WriteCommandW(0x48, y);
    return noerror;
}

RetCode_t RA8875::SetGraphicsCursor(point_t p)
{
    return SetGraphicsCursor(p.x, p.y);
}

point_t RA8875::GetGraphicsCursor(void)
{
    point_t p;

    p.x = ReadCommandW(0x46);
    p.y = ReadCommandW(0x48);
    return p;
}

RetCode_t RA8875::SetGraphicsCursorRead(loc_t x, loc_t y)
{
    WriteCommandW(0x4A, x);
    WriteCommandW(0x4C, y);
    return noerror;
}

RetCode_t RA8875::window(rect_t r)
{
    return window(r.p1.x, r.p1.y, r.p2.x + 1 - r.p1.x, r.p2.y + 1 - r.p1.y);
}

RetCode_t RA8875::window(loc_t x, loc_t y, dim_t width, dim_t height)
{
    INFO("window(%d,%d,%d,%d)", x, y, width, height);
    if (width == (dim_t)-1)
        width = screenwidth - x;
    if (height == (dim_t)-1)
        height = screenheight - y;
    windowrect.p1.x = x;
    windowrect.p1.y = y;
    windowrect.p2.x = x + width - 1;
    windowrect.p2.y = y + height - 1;
    GraphicsDisplay::window(x,y, width,height);
    WriteCommandW(0x30, x);
    WriteCommandW(0x32, y);
    WriteCommandW(0x34, (x+width-1));
    WriteCommandW(0x36, (y+height-1));
    //SetTextCursor(x,y);
    //SetGraphicsCursor(x,y);
    return noerror;
}


RetCode_t RA8875::cls(uint16_t layers)
{
    RetCode_t ret;

    PERFORMANCE_RESET;
    if (layers == 0) {
        ret = clsw(FULLWINDOW);
    } else if (layers > 3) {
        ret = bad_parameter;
    } else {
        uint16_t prevLayer = GetDrawingLayer();
        if (layers & 1) {
            SelectDrawingLayer(0);
            clsw(FULLWINDOW);
        }
        if (layers & 2) {
            SelectDrawingLayer(1);
            clsw(FULLWINDOW);
        }
        SelectDrawingLayer(prevLayer);
    }
    ret = SetTextCursor(0,0);
    ret = locate(0,0);
    REGISTERPERFORMANCE(PRF_CLS);
    return ret;
}


RetCode_t RA8875::clsw(RA8875::Region_t region)
{
    PERFORMANCE_RESET;
    WriteCommand(0x8E, (region == ACTIVEWINDOW) ? 0xC0 : 0x80);
    if (!_WaitWhileReg(0x8E, 0x80)) {
        REGISTERPERFORMANCE(PRF_CLS);
        return external_abort;
    }
    REGISTERPERFORMANCE(PRF_CLS);
    return noerror;
}


RetCode_t RA8875::pixel(point_t p, color_t color)
{
    return pixel(p.x, p.y, color);
}

RetCode_t RA8875::pixel(point_t p)
{
    return pixel(p.x, p.y);
}

RetCode_t RA8875::pixel(loc_t x, loc_t y, color_t color)
{
    RetCode_t ret;

    PERFORMANCE_RESET;
    ret = pixelStream(&color, 1, x,y);
    REGISTERPERFORMANCE(PRF_DRAWPIXEL);
    return ret;
}


RetCode_t RA8875::pixel(loc_t x, loc_t y)
{
    RetCode_t ret;

    PERFORMANCE_RESET;
    color_t color = GetForeColor();
    ret = pixelStream(&color, 1, x, y);
    REGISTERPERFORMANCE(PRF_DRAWPIXEL);
    return ret;
}


RetCode_t RA8875::pixelStream(color_t * p, uint32_t count, loc_t x, loc_t y)
{
    PERFORMANCE_RESET;
    SetGraphicsCursor(x, y);
    _StartGraphicsStream();
    _select(true);
    _spiwrite(0x00);         // Cmd: write data
    while (count--) {
        if (screenbpp == 16) {
            _spiwrite(*p >> 8);
            _spiwrite(*p & 0xFF);
        } else {
            _spiwrite(_cvt16to8(*p));
        }
        p++;
    }
    _select(false);
    _EndGraphicsStream();
    REGISTERPERFORMANCE(PRF_PIXELSTREAM);
    return(noerror);
}

RetCode_t RA8875::booleanStream(loc_t x, loc_t y, dim_t w, dim_t h, const uint8_t * boolStream)
{
    PERFORMANCE_RESET;
    rect_t restore = windowrect;

    window(x, y, w, h);
    SetGraphicsCursor(x, y);
    _StartGraphicsStream();
    _select(true);
    _spiwrite(0x00);         // Cmd: write data
    while (h--) {
        uint8_t pixels = w;
        uint8_t bitmask = 0x01;

        while (pixels) {
            uint8_t byte = *boolStream;
            //INFO("byte, mask: %02X, %02X", byte, bitmask);
            color_t c = (byte & bitmask) ? _foreground : _background;
                if (screenbpp == 16) {
                    _spiwrite(c >> 8);
                    _spiwrite(c & 0xFF);
                } else {
                    _spiwrite(_cvt16to8(c));
                }
            bitmask <<= 1;
            if (pixels > 1 && bitmask == 0) {
                bitmask = 0x01;
                boolStream++;
            }
            pixels--;
        }
        boolStream++;
    }
    _select(false);
    _EndGraphicsStream();
    window(restore);
    REGISTERPERFORMANCE(PRF_BOOLSTREAM);
    return(noerror);
}

color_t RA8875::getPixel(loc_t x, loc_t y)
{
    color_t pixel;

    PERFORMANCE_RESET;
    WriteCommand(0x40,0x00);    // Graphics write mode
    SetGraphicsCursorRead(x, y);
    WriteCommand(0x02);
    _select(true);
    _spiwrite(0x40);         // Cmd: read data
    _spiwrite(0x00);         // dummy read
    if (screenbpp == 16) {
        pixel  = _spiread();
        pixel |= (_spiread() << 8);
    } else {
        pixel = _cvt8to16(_spiread());
    }
    _select(false);
    REGISTERPERFORMANCE(PRF_READPIXEL);
    return pixel;
}


RetCode_t RA8875::getPixelStream(color_t * p, uint32_t count, loc_t x, loc_t y)
{
    color_t pixel;
    RetCode_t ret = noerror;

    PERFORMANCE_RESET;
    ret = WriteCommand(0x40,0x00);    // Graphics write mode
    ret = SetGraphicsCursorRead(x, y);
    ret = WriteCommand(0x02);
    _select(true);
    _spiwrite(0x40);         // Cmd: read data
    _spiwrite(0x00);         // dummy read
    if (screenbpp == 16)
        _spiwrite(0x00);     // dummy read is only necessary when in 16-bit mode
    while (count--) {
        if (screenbpp == 16) {
            pixel  = _spiread();
            pixel |= (_spiread() << 8);
        } else {
            pixel = _cvt8to16(_spiread());
        }
        *p++ = pixel;
    }
    _select(false);
    REGISTERPERFORMANCE(PRF_READPIXELSTREAM);
    return ret;
}


RetCode_t RA8875::line(point_t p1, point_t p2)
{
    return line(p1.x, p1.y, p2.x, p2.y);
}


RetCode_t RA8875::line(point_t p1, point_t p2, color_t color)
{
    return line(p1.x, p1.y, p2.x, p2.y, color);
}


RetCode_t RA8875::line(loc_t x1, loc_t y1, loc_t x2, loc_t y2, color_t color)
{
    foreground(color);
    return line(x1,y1,x2,y2);
}


RetCode_t RA8875::line(loc_t x1, loc_t y1, loc_t x2, loc_t y2)
{
    PERFORMANCE_RESET;
    if (x1 == x2 && y1 == y2) {
        pixel(x1, y1);
    } else {
        WriteCommandW(0x91, x1);
        WriteCommandW(0x93, y1);
        WriteCommandW(0x95, x2);
        WriteCommandW(0x97, y2);
        unsigned char drawCmd = 0x00;       // Line
        WriteCommand(0x90, drawCmd);
        WriteCommand(0x90, 0x80 + drawCmd); // Start drawing.
        if (!_WaitWhileReg(0x90, 0x80)) {
            REGISTERPERFORMANCE(PRF_DRAWLINE);
            return external_abort;
        }
    }
    REGISTERPERFORMANCE(PRF_DRAWLINE);
    return noerror;
}


RetCode_t RA8875::ThickLine(point_t p1, point_t p2, dim_t thickness, color_t color)
{
    if (thickness == 1) {
        line(p1,p2, color);
    } else {
        int dx = abs(p2.x-p1.x), sx = p1.x<p2.x ? 1 : -1;
        int dy = abs(p2.y-p1.y), sy = p1.y<p2.y ? 1 : -1;
        int err = (dx>dy ? dx : -dy)/2, e2;

        for (;;) {
            fillcircle(p1.x, p1.y, thickness/2, color);
            if (p1.x==p2.x && p1.y==p2.y)
                break;
            e2 = err;
            if (e2 >-dx)
                { err -= dy; p1.x += sx; }
            if (e2 < dy)
                { err += dx; p1.y += sy; }
        }
    }
    return noerror;
}


//
// Rectangle functions all mostly helpers to the basic rectangle function
//

RetCode_t RA8875::fillrect(rect_t r, color_t color, fill_t fillit)
{
    return rect(r.p1.x, r.p1.y, r.p2.x, r.p2.y, color, fillit);
}

RetCode_t RA8875::fillrect(loc_t x1, loc_t y1, loc_t x2, loc_t y2,
                           color_t color, fill_t fillit)
{
    return rect(x1,y1,x2,y2,color,fillit);
}

RetCode_t RA8875::rect(rect_t r, color_t color, fill_t fillit)
{
    return rect(r.p1.x, r.p1.y, r.p2.x, r.p2.y, color, fillit);
}

RetCode_t RA8875::rect(loc_t x1, loc_t y1, loc_t x2, loc_t y2,
                       color_t color, fill_t fillit)
{
    foreground(color);
    return rect(x1,y1,x2,y2,fillit);
}

RetCode_t RA8875::rect(loc_t x1, loc_t y1, loc_t x2, loc_t y2,
                       fill_t fillit)
{
    RetCode_t ret = noerror;
    PERFORMANCE_RESET;
    // check for bad_parameter
    if (x1 < 0 || x1 >= screenwidth || x2 < 0 || x2 >= screenwidth
    || y1 < 0 || y1 >= screenheight || y2 < 0 || y2 >= screenheight) {
        ret = bad_parameter;
    } else {
        if (x1 == x2 && y1 == y2) {
            pixel(x1, y1);
        } else if (x1 == x2) {
            line(x1, y1, x2, y2);
        } else if (y1 == y2) {
            line(x1, y1, x2, y2);
        } else {
            WriteCommandW(0x91, x1);
            WriteCommandW(0x93, y1);
            WriteCommandW(0x95, x2);
            WriteCommandW(0x97, y2);
            unsigned char drawCmd = 0x10;   // Rectangle
            if (fillit == FILL)
                drawCmd |= 0x20;
            WriteCommand(0x90, drawCmd);
            ret = WriteCommand(0x90, 0x80 + drawCmd); // Start drawing.
            if (!_WaitWhileReg(0x90, 0x80)) {
                REGISTERPERFORMANCE(PRF_DRAWRECTANGLE);
                return external_abort;
            }
        }
    }
    REGISTERPERFORMANCE(PRF_DRAWRECTANGLE);
    return ret;
}


//
// rounded rectangle functions are mostly helpers to the base round rect
//

RetCode_t RA8875::fillroundrect(rect_t r, dim_t radius1, dim_t radius2, color_t color, fill_t fillit)
{
    return roundrect(r.p1.x, r.p1.y, r.p2.x, r.p2.y, radius1, radius2, color, fillit);
}

RetCode_t RA8875::fillroundrect(loc_t x1, loc_t y1, loc_t x2, loc_t y2,
                                dim_t radius1, dim_t radius2, color_t color, fill_t fillit)
{
    foreground(color);
    return roundrect(x1,y1,x2,y2,radius1,radius2,fillit);
}

RetCode_t RA8875::roundrect(rect_t r, dim_t radius1, dim_t radius2, color_t color, fill_t fillit)
{
    return roundrect(r.p1.x, r.p1.y, r.p2.x, r.p2.y, radius1, radius2, color, fillit);
}

RetCode_t RA8875::roundrect(loc_t x1, loc_t y1, loc_t x2, loc_t y2,
                            dim_t radius1, dim_t radius2, color_t color, fill_t fillit)
{
    foreground(color);
    return roundrect(x1,y1,x2,y2,radius1,radius2,fillit);
}


RetCode_t RA8875::roundrect(loc_t x1, loc_t y1, loc_t x2, loc_t y2,
                            dim_t radius1, dim_t radius2, fill_t fillit)
{
    RetCode_t ret = noerror;

    PERFORMANCE_RESET;
    if (x1 < 0 || x1 >= screenwidth || x2 < 0 || x2 >= screenwidth
    || y1 < 0 || y1 >= screenheight || y2 < 0 || y2 >= screenheight) {
        ret = bad_parameter;
    } else if (x1 > x2 || y1 > y2 || (radius1 > (x2-x1)/2) || (radius2 > (y2-y1)/2) ) {
        ret = bad_parameter;
    } else if (x1 == x2 && y1 == y2) {
        pixel(x1, y1);
    } else if (x1 == x2) {
        line(x1, y1, x2, y2);
    } else if (y1 == y2) {
        line(x1, y1, x2, y2);
    } else {
        WriteCommandW(0x91, x1);
        WriteCommandW(0x93, y1);
        WriteCommandW(0x95, x2);
        WriteCommandW(0x97, y2);
        WriteCommandW(0xA1, radius1);
        WriteCommandW(0xA3, radius2);
        // Should not need this...
        WriteCommandW(0xA5, 0);
        WriteCommandW(0xA7, 0);
        unsigned char drawCmd = 0x20;       // Rounded Rectangle
        if (fillit == FILL)
            drawCmd |= 0x40;
        WriteCommand(0xA0, drawCmd);
        WriteCommand(0xA0, 0x80 + drawCmd); // Start drawing.
        if (!_WaitWhileReg(0xA0, 0x80)) {
            REGISTERPERFORMANCE(PRF_DRAWROUNDEDRECTANGLE);
            return external_abort;
        }
    }
    REGISTERPERFORMANCE(PRF_DRAWROUNDEDRECTANGLE);
    return ret;
}


//
// triangle functions
//

RetCode_t RA8875::triangle(loc_t x1, loc_t y1, loc_t x2, loc_t y2,
                           loc_t x3, loc_t y3, color_t color, fill_t fillit)
{
    RetCode_t ret;

    if (x1 < 0 || x1 >= screenwidth || x2 < 0 || x2 >= screenwidth || x3 < 0 || x3 >= screenwidth
    || y1 < 0 || y1 >= screenheight || y2 < 0 || y2 >= screenheight || y3 < 0 || y3 >= screenheight)
        ret = bad_parameter;
    foreground(color);
    ret = triangle(x1,y1,x2,y2,x3,y3,fillit);
    return ret;
}


RetCode_t RA8875::filltriangle(loc_t x1, loc_t y1, loc_t x2, loc_t y2,
                               loc_t x3, loc_t y3, color_t color, fill_t fillit)
{
    RetCode_t ret;

    foreground(color);
    ret = triangle(x1,y1,x2,y2,x3,y3,fillit);
    return ret;
}


RetCode_t RA8875::triangle(loc_t x1, loc_t y1 ,loc_t x2, loc_t y2,
                           loc_t x3, loc_t y3, fill_t fillit)
{
    RetCode_t ret = noerror;

    PERFORMANCE_RESET;
    if (x1 == x2 && y1 == y2 && x1 == x3 && y1 == y3) {
        pixel(x1, y1);
    } else {
        WriteCommandW(0x91, x1);
        WriteCommandW(0x93, y1);
        WriteCommandW(0x95, x2);
        WriteCommandW(0x97, y2);
        WriteCommandW(0xA9, x3);
        WriteCommandW(0xAB, y3);
        unsigned char drawCmd = 0x01;       // Triangle
        if (fillit == FILL)
            drawCmd |= 0x20;
        WriteCommand(0x90, drawCmd);
        WriteCommand(0x90, 0x80 + drawCmd); // Start drawing.
        if (!_WaitWhileReg(0x90, 0x80)) {
            REGISTERPERFORMANCE(PRF_DRAWTRIANGLE);
            return external_abort;
        }
    }
    REGISTERPERFORMANCE(PRF_DRAWTRIANGLE);
    return ret;
}


RetCode_t RA8875::circle(point_t p, dim_t radius,
                         color_t color, fill_t fillit)
{
    foreground(color);
    return circle(p.x,p.y,radius,fillit);
}


RetCode_t RA8875::fillcircle(point_t p, dim_t radius,
                             color_t color, fill_t fillit)
{
    foreground(color);
    return circle(p.x,p.y,radius,fillit);
}


RetCode_t RA8875::circle(point_t p, dim_t radius, fill_t fillit)
{
    return circle(p.x,p.y,radius,fillit);
}


RetCode_t RA8875::circle(loc_t x, loc_t y, dim_t radius,
                         color_t color, fill_t fillit)
{
    foreground(color);
    return circle(x,y,radius,fillit);
}


RetCode_t RA8875::fillcircle(loc_t x, loc_t y, dim_t radius,
                             color_t color, fill_t fillit)
{
    foreground(color);
    return circle(x,y,radius,fillit);
}


RetCode_t RA8875::circle(loc_t x, loc_t y, dim_t radius, fill_t fillit)
{
    RetCode_t ret = noerror;

    PERFORMANCE_RESET;
    if (radius <= 0 || (x - radius) < 0 || (x + radius) > screenwidth
    || (y - radius) < 0 || (y + radius) > screenheight) {
        ret = bad_parameter;
    } else if (radius == 1) {
        pixel(x,y);
    } else {
        WriteCommandW(0x99, x);
        WriteCommandW(0x9B, y);
        WriteCommand(0x9d, radius & 0xFF);
        unsigned char drawCmd = 0x00;       // Circle
        if (fillit == FILL)
            drawCmd |= 0x20;
        WriteCommand(0x90, drawCmd);
        WriteCommand(0x90, 0x40 + drawCmd); // Start drawing.
        if (!_WaitWhileReg(0x90, 0x40)) {
            REGISTERPERFORMANCE(PRF_DRAWCIRCLE);
            return external_abort;
        }
    }
    REGISTERPERFORMANCE(PRF_DRAWCIRCLE);
    return ret;
}


RetCode_t RA8875::ellipse(loc_t x, loc_t y, dim_t radius1, dim_t radius2, color_t color, fill_t fillit)
{
    foreground(color);
    return ellipse(x,y,radius1,radius2,fillit);
}


RetCode_t RA8875::fillellipse(loc_t x, loc_t y, dim_t radius1, dim_t radius2, color_t color, fill_t fillit)
{
    foreground(color);
    return ellipse(x,y,radius1,radius2,fillit);
}


RetCode_t RA8875::ellipse(loc_t x, loc_t y, dim_t radius1, dim_t radius2, fill_t fillit)
{
    RetCode_t ret = noerror;

    PERFORMANCE_RESET;
    if (radius1 <= 0 || radius2 <= 0 || (x - radius1) < 0 || (x + radius1) > screenwidth
    || (y - radius2) < 0 || (y + radius2) > screenheight) {
        ret = bad_parameter;
    } else if (radius1 == 1 && radius2 == 1) {
        pixel(x, y);
    } else {
        WriteCommandW(0xA5, x);
        WriteCommandW(0xA7, y);
        WriteCommandW(0xA1, radius1);
        WriteCommandW(0xA3, radius2);
        unsigned char drawCmd = 0x00;   // Ellipse
        if (fillit == FILL)
            drawCmd |= 0x40;
        WriteCommand(0xA0, drawCmd);
        WriteCommand(0xA0, 0x80 + drawCmd); // Start drawing.
        if (!_WaitWhileReg(0xA0, 0x80)) {
            REGISTERPERFORMANCE(PRF_DRAWELLIPSE);
            return external_abort;
        }
    }
    REGISTERPERFORMANCE(PRF_DRAWELLIPSE);
    return ret;
}


RetCode_t RA8875::frequency(unsigned long Hz, unsigned long Hz2)
{
    spiwritefreq = Hz;
    if (Hz2 != 0)
        spireadfreq = Hz2;
    else
        spireadfreq = Hz/2;
    _setWriteSpeed(true);
    //       __   ___
    // Clock   ___A     Rising edge latched
    //       ___ ____
    // Data  ___X____
    spi.format(8, 3);           // 8 bits and clock to data phase 0
    return noerror;
}

void RA8875::_setWriteSpeed(bool writeSpeed)
{
    if (writeSpeed) {
        spi.frequency(spiwritefreq);
        spiWriteSpeed = true;
    } else {
        spi.frequency(spireadfreq);
        spiWriteSpeed = false;
    }
}



RetCode_t RA8875::BlockMove(uint8_t dstLayer, uint8_t dstDataSelect, point_t dstPoint,
    uint8_t srcLayer, uint8_t srcDataSelect, point_t srcPoint,
    dim_t bte_width, dim_t bte_height,
    uint8_t bte_op_code, uint8_t bte_rop_code)
{
    uint8_t cmd;

    PERFORMANCE_RESET;
    ///@todo range check and error return rather than to secretly fix
    srcPoint.x &= 0x3FF;    // prevent high bits from doing unexpected things
    srcPoint.y &= 0x1FF;
    dstPoint.x &= 0x3FF;
    dstPoint.y &= 0x1FF;
    WriteCommandW(0x54, srcPoint.x);
    WriteCommandW(0x56, ((dim_t)(srcLayer & 1) << 15) | srcPoint.y);
    WriteCommandW(0x58, dstPoint.x);
    WriteCommandW(0x5A, ((dim_t)(dstLayer & 1) << 15) | dstPoint.y);
    WriteCommandW(0x5C, bte_width);
    WriteCommandW(0x5E, bte_height);
    WriteCommand(0x51,  ((bte_rop_code & 0x0F) << 4) | (bte_op_code & 0x0F));
    cmd = ((srcDataSelect & 1) << 6) | ((dstDataSelect & 1) << 5);
    WriteCommand(0x50, 0x80 | cmd);     // enable the BTE
    if (!_WaitWhileBusy(0x40)) {
        REGISTERPERFORMANCE(PRF_BLOCKMOVE);
        return external_abort;
    }
    REGISTERPERFORMANCE(PRF_BLOCKMOVE);
    return noerror;
}


RetCode_t RA8875::Power(bool on)
{
    WriteCommand(0x01, (on) ? 0x80 : 0x00);
    return noerror;
}


RetCode_t RA8875::Backlight_u8(uint8_t brightness)
{
    static bool is_enabled = false;

    if (brightness == 0) {
        WriteCommand(0x8a); // Disable the PWM
        WriteData(0x00);
        is_enabled = false;
    } else if (!is_enabled) {
        WriteCommand(0x8a); // Enable the PWM
        WriteData(0x80);
        WriteCommand(0x8a); // Not sure why this is needed, but following the pattern
        WriteData(0x81);    // open PWM (SYS_CLK / 2 as best I can tell)
        is_enabled = true;
    }
    WriteCommand(0x8b, brightness);  // Brightness parameter 0xff-0x00
    return noerror;
}

uint8_t RA8875::GetBacklight_u8(void)
{
    return ReadCommand(0x8b);
}

RetCode_t RA8875::Backlight(float brightness)
{
    unsigned char b;

    if (brightness >= 1.0)
        b = 255;
    else if (brightness <= 0.0)
        b = 0;
    else
        b = (unsigned char)(brightness * 255);
    return Backlight_u8(b);
}

float RA8875::GetBacklight(void)
{
    return (float)(GetBacklight_u8())/255;
}

RetCode_t RA8875::SelectUserFont(const uint8_t * _font)
{
    INFO("Cursor(%d,%d)  %p", cursor_x, cursor_y, _font);
    INFO("Text C(%d,%d)", GetTextCursor_X(), GetTextCursor_Y());
    if (_font) {
        HexDump("Font Memory", _font, 16);
        extFontHeight = _font[6];
        uint32_t totalWidth = 0;
        uint16_t firstChar = _font[3] * 256 + _font[2];
        uint16_t lastChar  = _font[5] * 256 + _font[4];
        uint16_t i;

        for (i=firstChar; i<=lastChar; i++) {
            // 8 bytes of preamble to the first level lookup table
            uint16_t offsetToCharLookup = 8 + 4 * (i - firstChar);    // 4-bytes: width(pixels), 16-bit offset from table start, 0
            totalWidth += _font[offsetToCharLookup];
        }
        extFontWidth = totalWidth / (lastChar - firstChar);
        INFO("Font Metrics: Avg W: %2d, H: %2d, First:%d, Last:%d", extFontWidth, extFontHeight, firstChar, lastChar);
    }
    SetTextCursor(GetTextCursor_X(), GetTextCursor_Y());  // soft-font cursor -> hw cursor
    font = _font;
    return GraphicsDisplay::SelectUserFont(_font);
}

RetCode_t RA8875::background(color_t color)
{
    GraphicsDisplay::background(color);
    return _writeColorTrio(0x60, color);
}


RetCode_t RA8875::background(unsigned char r, unsigned char g, unsigned char b)
{
    background(RGB(r,g,b));
    return noerror;
}


RetCode_t RA8875::foreground(color_t color)
{
    GraphicsDisplay::foreground(color);
    return _writeColorTrio(0x63, color);
}


RetCode_t RA8875::foreground(unsigned char r, unsigned char g, unsigned char b)
{
    foreground(RGB(r,g,b));
    return noerror;
}


color_t RA8875::GetForeColor(void)
{
    return _readColorTrio(0x63);
}


color_t RA8875::DOSColor(int i)
{
    const color_t colors[16] = {
        Black,    Blue,       Green,       Cyan,
        Red,      Magenta,    Brown,       Gray,
        Charcoal, BrightBlue, BrightGreen, BrightCyan,
        Orange,   Pink,       Yellow,      White
    };
    if (i >= 0 && i < 16)
        return colors[i];
    else
        return 0;
}


const char * RA8875::DOSColorNames(int i)
{
    const char * names[16] = {
        "Black",    "Blue",       "Green",       "Cyan",
        "Red",      "Magenta",    "Brown",       "Gray",
        "Charcoal", "BrightBlue", "BrightGreen", "BrightCyan",
        "Orange",   "Pink",       "Yellow",      "White"
    };
    if (i >= 0 && i < 16)
        return names[i];
    else
        return NULL;
}


///////////////////////////////////////////////////////////////
// Private functions

unsigned char RA8875::_spiwrite(unsigned char data)
{
    unsigned char retval;

    if (!spiWriteSpeed)
        _setWriteSpeed(true);
    retval = spi.write(data);
    return retval;
}


unsigned char RA8875::_spiread(void)
{
    unsigned char retval;
    unsigned char data = 0;

    if (spiWriteSpeed)
        _setWriteSpeed(false);
    retval = spi.read(data);
    return retval;
}


RetCode_t RA8875::_select(bool chipsel)
{
    // cs = (chipsel == true) ? 0 : 1;
    spi.udma_cs((chipsel == true) ? 0 : 1);
    return noerror;
}


RetCode_t RA8875::PrintScreen(uint16_t layer, loc_t x, loc_t y, dim_t w, dim_t h, const char *Name_BMP)
{
    (void)layer;

    // AttachPrintHandler(this, RA8875::_printCallback);
    // return PrintScreen(x,y,w,h);
    return PrintScreen(x, y, w, h, Name_BMP);
}

RetCode_t RA8875::_printCallback(RA8875::filecmd_t cmd, uint8_t * buffer, uint16_t size)
{
    HexDump("CB", buffer, size);
    switch(cmd) {
        case RA8875::OPEN:
            //pc.printf("About to write %lu bytes\r\n", *(uint32_t *)buffer);
            _printFH = fopen("file.bmp", "w+b");
            if (_printFH == 0)
                return file_not_found;
            break;
        case RA8875::WRITE:
            //pc.printf("  Write %4u bytes\r\n", size);
            fwrite(buffer, 1, size, _printFH);
            break;
        case RA8875::CLOSE:
            //pc.printf("  close\r\n");
            fclose(_printFH);
            _printFH = 0;
            break;
        default:
            //pc.printf("Unexpected callback %d\r\n", cmd);
            return file_not_found;
            //break;
    }
    return noerror;
}

RetCode_t RA8875::PrintScreen(loc_t x, loc_t y, dim_t w, dim_t h)
{
    BITMAPFILEHEADER BMP_Header;
    BITMAPINFOHEADER BMP_Info;
    uint8_t * lineBuffer = NULL;
    color_t * pixelBuffer = NULL;
    color_t * pixelBuffer2 = NULL;

    INFO("(%d,%d) - (%d,%d)", x,y,w,h);
    if (x >= 0 && x < screenwidth
            && y >= 0 && y < screenheight
            && w > 0 && x + w <= screenwidth
            && h > 0 && y + h <= screenheight) {

        BMP_Header.bfType = BF_TYPE;
        BMP_Header.bfSize = (w * h * sizeof(RGBQUAD)) + sizeof(BMP_Header) + sizeof(BMP_Header);
        BMP_Header.bfReserved1 = 0;
        BMP_Header.bfReserved2 = 0;
        BMP_Header.bfOffBits = sizeof(BMP_Header) + sizeof(BMP_Header);

        BMP_Info.biSize = sizeof(BMP_Info);
        BMP_Info.biWidth = w;
        BMP_Info.biHeight = h;
        BMP_Info.biPlanes = 1;
        BMP_Info.biBitCount = 24;
        BMP_Info.biCompression = BI_RGB;
        BMP_Info.biSizeImage = 0;
        BMP_Info.biXPelsPerMeter = 0;
        BMP_Info.biYPelsPerMeter = 0;
        BMP_Info.biClrUsed = 0;
        BMP_Info.biClrImportant = 0;

        // Allocate the memory we need to proceed
        int lineBufSize = ((24 * w + 7)/8);
        lineBuffer = (uint8_t *)swMalloc(lineBufSize);
        if (lineBuffer == NULL) {
            ERR("Not enough RAM for PrintScreen lineBuffer");
            return(not_enough_ram);
        }

        #define DOUBLEBUF /* one larger buffer instead of two */

        #ifdef DOUBLEBUF
        // In the "#else", pixelBuffer2 malloc returns a value,
        // but is actually causing a failure later.
        // This test helps determine if it is truly out of memory,
        // or if malloc is broken.
        pixelBuffer = (color_t *)swMalloc(2 * w * sizeof(color_t));
        pixelBuffer2 = pixelBuffer + (w * sizeof(color_t));
        #else
        pixelBuffer = (color_t *)swMalloc(w * sizeof(color_t));
        pixelBuffer2 = (color_t *)swMalloc(w * sizeof(color_t));
        #endif
        if (pixelBuffer == NULL || pixelBuffer2 == NULL) {
            ERR("Not enough RAM for pixelBuffer");
            #ifndef DOUBLEBUF
            if (pixelBuffer2)
                swFree(pixelBuffer2);
            #endif
            if (pixelBuffer)
                swFree(pixelBuffer);
            swFree(lineBuffer);
            return(not_enough_ram);
        }

        // Get the file primed...
        privateCallback(OPEN, (uint8_t *)&BMP_Header.bfSize, 4);

        // Be optimistic - don't check for errors.
        HexDump("BMP_Header", (uint8_t *)&BMP_Header, sizeof(BMP_Header));
        //fwrite(&BMP_Header, sizeof(char), sizeof(BMP_Header), Image);
        privateCallback(WRITE, (uint8_t *)&BMP_Header, sizeof(BMP_Header));

        HexDump("BMP_Info", (uint8_t *)&BMP_Info, sizeof(BMP_Info));
        //fwrite(&BMP_Info, sizeof(char), sizeof(BMP_Info), Image);
        privateCallback(WRITE, (uint8_t *)&BMP_Info, sizeof(BMP_Info));

        //color_t transparency = GetBackgroundTransparencyColor();
        LayerMode_T ltpr0 = GetLayerMode();

        uint16_t prevLayer = GetDrawingLayer();
        // If only one of the layers is visible, select that layer
        switch(ltpr0) {
            case ShowLayer0:
                SelectDrawingLayer(0);
                break;
            case ShowLayer1:
                SelectDrawingLayer(1);
                break;
            default:
                break;
        }

        // Read the display from the last line toward the top
        // so we can write the file in one pass.
        for (int j = h - 1; j >= 0; j--) {
            if (ltpr0 >= 2)             // Need to combine the layers...
                SelectDrawingLayer(0);  // so read layer 0 first
            // Read one line of pixels to a local buffer
            if (getPixelStream(pixelBuffer, w, x,y+j) != noerror) {
                ERR("getPixelStream error, and no recovery handler...");
            }
            if (ltpr0 >= 2) {           // Need to combine the layers...
                SelectDrawingLayer(1);  // so read layer 1 next
                if (getPixelStream(pixelBuffer2, w, x,y+j) != noerror) {
                    ERR("getPixelStream error, and no recovery handler...");
                }
            }
            INFO("1st Color: %04X", pixelBuffer[0]);
            HexDump("Raster", (uint8_t *)pixelBuffer, w);
            // Convert the local buffer to RGBQUAD format
            int lb = 0;
            for (int i=0; i<w; i++) {
                RGBQUAD q0 = RGB16ToRGBQuad(pixelBuffer[x+i]);      // Scale to 24-bits
                RGBQUAD q1 = RGB16ToRGBQuad(pixelBuffer2[x+i]);     // Scale to 24-bits
                switch (ltpr0) {
                    case 0:
                    case 1:
                    case 2: // lighten-overlay  (@TODO Not supported yet)
                    case 6: // Floating Windows     (@TODO not sure how to support)
                    default: // Reserved...
                        lineBuffer[lb++] = q0.rgbBlue;
                        lineBuffer[lb++] = q0.rgbGreen;
                        lineBuffer[lb++] = q0.rgbRed;
                        break;
                    case 3: // transparent mode (@TODO Read the background color register for transparent)
                    case 4: // boolean or
                        lineBuffer[lb++] = q0.rgbBlue | q1.rgbBlue;
                        lineBuffer[lb++] = q0.rgbGreen | q1.rgbGreen;
                        lineBuffer[lb++] = q0.rgbRed | q1.rgbRed;
                        break;
                    case 5: // boolean AND
                        lineBuffer[lb++] = q0.rgbBlue & q1.rgbBlue;
                        lineBuffer[lb++] = q0.rgbGreen & q1.rgbGreen;
                        lineBuffer[lb++] = q0.rgbRed & q1.rgbRed;
                        break;
                }
            }
            if (j == h - 1) {
                HexDump("Line", lineBuffer, lineBufSize);
            }
            // Write to disk
            //fwrite(lineBuffer, sizeof(char), lb, Image);
            privateCallback(WRITE, (uint8_t *)lineBuffer, lb);
        }
        SelectDrawingLayer(prevLayer);
        //fclose(Image);
        privateCallback(CLOSE, NULL, 0);

        #ifndef DOUBLEBUF
        if (pixelBuffer2)
            swFree(pixelBuffer2);
        #endif
        if (pixelBuffer)
            swFree(pixelBuffer);
        swFree(lineBuffer);
        INFO("Image closed");
        return noerror;
    } else {
        return bad_parameter;
    }
}

RetCode_t RA8875::PrintScreen(loc_t x, loc_t y, dim_t w, dim_t h, const char *Name_BMP)
{
    BITMAPFILEHEADER BMP_Header;
    BITMAPINFOHEADER BMP_Info;
    uint8_t * lineBuffer = NULL;
    color_t * pixelBuffer = NULL;
    color_t * pixelBuffer2 = NULL;

    INFO("(%d,%d) - (%d,%d) %s", x,y,w,h,Name_BMP);
    if (x >= 0 && x < screenwidth
            && y >= 0 && y < screenheight
            && w > 0 && x + w <= screenwidth
            && h > 0 && y + h <= screenheight) {

        BMP_Header.bfType = BF_TYPE;
        BMP_Header.bfSize = (w * h * sizeof(RGBQUAD)) + sizeof(BMP_Header) + sizeof(BMP_Header);
        BMP_Header.bfReserved1 = 0;
        BMP_Header.bfReserved2 = 0;
        BMP_Header.bfOffBits = sizeof(BMP_Header) + sizeof(BMP_Header);

        BMP_Info.biSize = sizeof(BMP_Info);
        BMP_Info.biWidth = w;
        BMP_Info.biHeight = h;
        BMP_Info.biPlanes = 1;
        BMP_Info.biBitCount = 24;
        BMP_Info.biCompression = BI_RGB;
        BMP_Info.biSizeImage = 0;
        BMP_Info.biXPelsPerMeter = 0;
        BMP_Info.biYPelsPerMeter = 0;
        BMP_Info.biClrUsed = 0;
        BMP_Info.biClrImportant = 0;

        // Allocate the memory we need to proceed
        int lineBufSize = ((24 * w + 7)/8);
        lineBuffer = (uint8_t *)swMalloc(lineBufSize);
        if (lineBuffer == NULL) {
            ERR("Not enough RAM for PrintScreen lineBuffer");
            return(not_enough_ram);
        }

        #define DOUBLEBUF /* one larger buffer instead of two */

        #ifdef DOUBLEBUF
        // In the "#else", pixelBuffer2 malloc returns a value,
        // but is actually causing a failure later.
        // This test helps determine if it is truly out of memory,
        // or if malloc is broken.
        pixelBuffer = (color_t *)swMalloc(2 * w * sizeof(color_t));
        pixelBuffer2 = pixelBuffer + (w * sizeof(color_t));
        #else
        pixelBuffer = (color_t *)swMalloc(w * sizeof(color_t));
        pixelBuffer2 = (color_t *)swMalloc(w * sizeof(color_t));
        #endif
        if (pixelBuffer == NULL || pixelBuffer2 == NULL) {
            ERR("Not enough RAM for pixelBuffer");
            #ifndef DOUBLEBUF
            if (pixelBuffer2)
                swFree(pixelBuffer2);
            #endif
            if (pixelBuffer)
                swFree(pixelBuffer);
            swFree(lineBuffer);
            return(not_enough_ram);
        }

        FILE *Image = fopen(Name_BMP, "wb");
        if (!Image) {
            ERR("Can't open file for write");
            #ifndef DOUBLEBUF
            if (pixelBuffer2)
                swFree(pixelBuffer2);
            #endif
            if (pixelBuffer)
                swFree(pixelBuffer);
            swFree(lineBuffer);
            return(file_not_found);
        }

        // Be optimistic - don't check for errors.
        HexDump("BMP_Header", (uint8_t *)&BMP_Header, sizeof(BMP_Header));
        fwrite(&BMP_Header, sizeof(char), sizeof(BMP_Header), Image);

        HexDump("BMP_Info", (uint8_t *)&BMP_Info, sizeof(BMP_Info));
        fwrite(&BMP_Info, sizeof(char), sizeof(BMP_Info), Image);

        //color_t transparency = GetBackgroundTransparencyColor();
        LayerMode_T ltpr0 = GetLayerMode();

        uint16_t prevLayer = GetDrawingLayer();
        // If only one of the layers is visible, select that layer
        switch(ltpr0) {
            case ShowLayer0:
                SelectDrawingLayer(0);
                break;
            case ShowLayer1:
                SelectDrawingLayer(1);
                break;
            default:
                break;
        }

        // Read the display from the last line toward the top
        // so we can write the file in one pass.
        for (int j = h - 1; j >= 0; j--) {
            if (ltpr0 >= 2)             // Need to combine the layers...
                SelectDrawingLayer(0);  // so read layer 0 first
            // Read one line of pixels to a local buffer
            if (getPixelStream(pixelBuffer, w, x,y+j) != noerror) {
                ERR("getPixelStream error, and no recovery handler...");
            }
            if (ltpr0 >= 2) {           // Need to combine the layers...
                SelectDrawingLayer(1);  // so read layer 1 next
                if (getPixelStream(pixelBuffer2, w, x,y+j) != noerror) {
                    ERR("getPixelStream error, and no recovery handler...");
                }
            }
            INFO("1st Color: %04X", pixelBuffer[0]);
            HexDump("Raster", (uint8_t *)pixelBuffer, w);
            // Convert the local buffer to RGBQUAD format
            int lb = 0;
            for (int i=0; i<w; i++) {
                RGBQUAD q0 = RGB16ToRGBQuad(pixelBuffer[x+i]);      // Scale to 24-bits
                RGBQUAD q1 = RGB16ToRGBQuad(pixelBuffer2[x+i]);     // Scale to 24-bits
                switch (ltpr0) {
                    case 0:
                    case 1:
                    case 2: // lighten-overlay  (@TODO Not supported yet)
                    case 6: // Floating Windows     (@TODO not sure how to support)
                    default: // Reserved...
                        lineBuffer[lb++] = q0.rgbBlue;
                        lineBuffer[lb++] = q0.rgbGreen;
                        lineBuffer[lb++] = q0.rgbRed;
                        break;
                    case 3: // transparent mode (@TODO Read the background color register for transparent)
                    case 4: // boolean or
                        lineBuffer[lb++] = q0.rgbBlue | q1.rgbBlue;
                        lineBuffer[lb++] = q0.rgbGreen | q1.rgbGreen;
                        lineBuffer[lb++] = q0.rgbRed | q1.rgbRed;
                        break;
                    case 5: // boolean AND
                        lineBuffer[lb++] = q0.rgbBlue & q1.rgbBlue;
                        lineBuffer[lb++] = q0.rgbGreen & q1.rgbGreen;
                        lineBuffer[lb++] = q0.rgbRed & q1.rgbRed;
                        break;
                }
            }
            if (j == h - 1) {
                HexDump("Line", lineBuffer, lineBufSize);
            }
            // Write to disk
            fwrite(lineBuffer, sizeof(char), lb, Image);
        }
        SelectDrawingLayer(prevLayer);
        fclose(Image);
        #ifndef DOUBLEBUF
        if (pixelBuffer2)
            swFree(pixelBuffer2);
        #endif
        if (pixelBuffer)
            swFree(pixelBuffer);
        swFree(lineBuffer);
        INFO("Image closed");
        return noerror;
    } else {
        return bad_parameter;
    }
}


// ##########################################################################
// ##########################################################################
// ##########################################################################

#ifdef TESTENABLE

#include "BPG_Arial08x08.h"
#include "BPG_Arial20x20.h"

//      ______________  ______________  ______________  _______________
//     /_____   _____/ /  ___________/ /  ___________/ /_____   ______/
//          /  /      /  /            /  /                  /  /
//         /  /      /  /___         /  /__________        /  /
//        /  /      /  ____/        /__________   /       /  /
//       /  /      /  /                       /  /       /  /
//      /  /      /  /__________  ___________/  /       /  /
//     /__/      /_____________/ /_____________/       /__/
//
//    Everything from here down is test code.
//
bool SuppressSlowStuff = false;

void TextWrapTest(RA8875 & display, Serial & pc)
{
    if (!SuppressSlowStuff)
        pc.printf("Text Wrap Test\r\n");
    display.background(Black);
    display.foreground(Blue);
    display.cls();
    display.Backlight_u8(255);
    display.puts("Text Wrap Test.\r\n");
    for (int i=1; i<60; i++) {
        display.printf("L%2d\n", i % 17);
        if (!SuppressSlowStuff)
            wait_ms(100);
    }
    if (!SuppressSlowStuff)
        wait_ms(3000);
}


void ShowKey(RA8875 & display, int key)
{
    loc_t col, row;
    dim_t r1 = 25;
    color_t color = (key & 0x80) ? Red : Green;

    key &= 0x7F;        // remove the long-press flag
    row = (key - 1) / 5;
    col = (key - 1) % 5;
    if (col > 5) col = 5;
    if (row > 4) row = 4;
    display.circle(450 - + (2 * r1) * col, 200 - (2 * r1) * row, r1-2, color, FILL);
}

void HideKey(RA8875 & display, int key)
{
    loc_t col, row;
    dim_t r1 = 25;

    row = (key - 1) / 5;
    col = (key - 1) % 5;
    if (col > 5) col = 5;
    if (row > 4) row = 4;
    display.background(Black);
    display.circle(450 - (2 * r1) * col, 200 - (2 * r1) * row, r1-2, Black, FILL);
    display.circle(450 - (2 * r1) * col, 200 - (2 * r1) * row, r1-2, Blue);
}

void KeyPadTest(RA8875 & display, Serial & pc)
{
    const uint8_t myMap[22] = {
        0,
        'a', 'b', 'c', 'd', 'e',
        'f', 'g', 'h', 'i', 'j',
        'k', 'l', 'm', 'n', 'o',
        'p', 'q', 'r', 's', 't',
        'x'
    };

    display.background(Black);
    display.foreground(Blue);
    display.cls();
    display.Backlight_u8(255);
    display.puts("KeyPad Test. Touch the keypad...");
    pc.printf("\r\n"
              "Raw KeyPad Test. Keypad returns the key-number.\r\n"
              "Press [most] any PC keyboard key to advance to next test.\r\n");
    RetCode_t ret = display.KeypadInit(true, true, 3, 7, 3);
    if (ret != noerror)
        pc.printf("returncode from KeypadInit is %d\r\n", ret);
    int lastKey = 0;
    while (!pc.readable()) {
        if (display.readable()) {
            int key = display.getc();
            if (key) {
                if (((key & 0x7F) != lastKey) && (lastKey != 0))
                    HideKey(display, lastKey);
                ShowKey(display, key);
                lastKey = key & 0x7F;
            } else {
                // erase the last one
                if (lastKey)
                    HideKey(display, lastKey);
            }
        }
    }
    (void)pc.getc();
    pc.printf("\r\n"
              "Map KeyPad Test. Keypad returns the remapped key 'a' - 't'.\r\n"
              "Press [most] any PC keyboard key to advance to exit test.\r\n");
    display.SetKeyMap(myMap);
    while (!pc.readable()) {
        if (display.readable()) {
            int key = display.getc();
            bool longPress = key & 0x80;
            display.SetTextCursor(0, 120);
            display.printf("Long Press: %d\r\n", longPress);
            display.printf("  Remapped: %c %02X\r\n", (key) ? key & 0x7F : ' ', key);
        }
    }
    (void)pc.getc();
    display.SetKeyMap();
    pc.printf("\r\n");
}

void TextCursorTest(RA8875 & display, Serial & pc)
{
    const char * iCursor  = "The I-Beam cursor should be visible for this text.\r\n";
    const char * uCursor  = "The Underscore cursor should be visible for this text.\r\n";
    const char * bCursor  = "The Block cursor should be visible for this text.\r\n";
    const char * bbCursor = "The Blinking Block cursor should be visible for this text.\r\n";
    const char * p;
    int delay = 60;

    if (!SuppressSlowStuff)
        pc.printf("Text Cursor Test\r\n");
    else
        delay = 0;
    display.background(Black);
    display.foreground(Blue);
    display.cls();
    display.Backlight_u8(255);
    display.puts("Text Cursor Test.");

    // visible, non-blinking
    display.SetTextCursor(0,20);
    display.SetTextCursorControl(RA8875::IBEAM, false);
    p = iCursor;
    while (*p) {
        display._putc(*p++);
        wait_ms(delay);
    }

    display.SetTextCursorControl(RA8875::UNDER, false);
    p = uCursor;
    while (*p) {
        display._putc(*p++);
        wait_ms(delay);
    }

    display.SetTextCursorControl(RA8875::BLOCK, false);
    p = bCursor;
    while (*p) {
        display._putc(*p++);
        wait_ms(delay);
    }

    display.SetTextCursorControl(RA8875::BLOCK, true);
    p = bbCursor;
    while (*p) {
        display._putc(*p++);
        wait_ms(delay);
    }
    wait_ms(delay * 20);
    display.SetTextCursorControl(RA8875::NOCURSOR, false);
}


void BacklightTest(RA8875 & display, Serial & pc, float ramptime)
{
    char buf[60];
    unsigned int w = (ramptime * 1000)/ 256;
    int delay = 200;

    if (!SuppressSlowStuff)
        pc.printf("Backlight Test - ramp over %f sec.\r\n", ramptime);
    else {
        delay = 0;
        w = 0;
    }
    display.Backlight_u8(0);
    display.background(White);
    display.foreground(Blue);
    display.cls();
    display.puts("RA8875 Backlight Test - Ramp up.");
    wait_ms(delay);
    for (int i=0; i <= 255; i++) {
        snprintf(buf, sizeof(buf), "%3d, %4d", i, w);
        display.puts(100,100,buf);
        display.Backlight_u8(i);
        wait_ms(w);
    }
}


void BacklightTest2(RA8875 & display, Serial & pc)
{
    int delay = 20;

    if (!SuppressSlowStuff)
        pc.printf("Backlight Test 2\r\n");
    else
        delay = 0;

    // Dim it out at the end of the tests.
    display.foreground(Blue);
    display.puts(0,0, "Ramp Backlight down.");
    // Ramp it off
    for (int i=255; i != 0; i--) {
        display.Backlight_u8(i);
        wait_ms(delay);
    }
    display.Backlight_u8(0);
}


void ExternalFontTest(RA8875 & display, Serial & pc)
{
    if (!SuppressSlowStuff)
        pc.printf("External Font Test\r\n");
    display.background(Black);
    display.foreground(Blue);
    display.cls();
    display.puts("External Font Test.");
    display.Backlight(1);

    display.SelectUserFont(BPG_Arial08x08);
    display.puts(0,30, "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz\r\n");

    display.SelectUserFont(BPG_Arial20x20);
    display.puts("0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz\r\n");

    display.SelectUserFont();

    display.puts("Normal font again.");
    //display.window(0,0, display.width(), display.height());
}


void DOSColorTest(RA8875 & display, Serial & pc)
{
    if (!SuppressSlowStuff)
        pc.printf("DOS Color Test\r\n");
    display.background(Black);
    display.foreground(Blue);
    display.cls();
    display.puts("DOS Colors - Fore");
    display.puts(280,0, "Back");
    display.background(Gray);
    for (int i=0; i<16; i++) {
        display.foreground(display.DOSColor(i));
        display.puts(160, i*16, display.DOSColorNames(i));
        display.background(Black);
    }
    display.foreground(White);
    for (int i=0; i<16; i++) {
        display.background(display.DOSColor(i));
        display.puts(360, i*16, display.DOSColorNames(i));
        display.foreground(White);
    }
}


void WebColorTest(RA8875 & display, Serial & pc)
{
    if (!SuppressSlowStuff)
        pc.printf("Web Color Test\r\n");
    display.background(Black);
    display.foreground(Blue);
    display.window(0,0, display.width(), display.height());
    display.cls();
    display.SetTextFontSize(1,1);
    display.puts(200,0, "Web Color Test");
    display.SetTextCursor(0,0);
    display.puts("  ");
    for (int i=0; i<16; i++)
        display.printf("%X", i&0xF);
    display.puts("\r\n0 ");
    for (int i=0; i<sizeof(WebColors)/sizeof(WebColors[0]); i++) {
        display.background(WebColors[i]);
        display.puts(" ");
        if (i % 16 == 15 && i < 255) {
            display.printf("\r\n%X ", ((i+1)/16));
        }
    }
    display.SetTextFontSize(1,1);
}


void PixelTest(RA8875 & display, Serial & pc)
{
    int i, c, x, y;

    if (!SuppressSlowStuff)
        pc.printf("Pixel Test\r\n");
    display.background(Black);
    display.foreground(Blue);
    display.cls();
    display.puts("Pixel Test");
    for (i=0; i<1000; i++) {
        x = rand() % 480;
        y = 16 + rand() % (272-16);
        c = rand() % 16;
        //pc.printf("  (%d,%d) - %d\r\n", x,y,r1);
        display.pixel(x,y, display.DOSColor(c));
    }
}


void LineTest(RA8875 & display, Serial & pc)
{
    int i, x, y, x2, y2;

    if (!SuppressSlowStuff)
        pc.printf("Line Test\r\n");
    display.background(Black);
    display.foreground(Blue);
    display.cls();
    display.puts("Line Test");
    for (i=0; i<16; i++) {
        // Lines
        x = rand() % 480;
        y = rand() % 272;
        x2 = rand() % 480;
        y2 = rand() % 272;
        display.line(x,y, x2,y2, display.DOSColor(i));
    }
    display.foreground(BrightRed);
    display.foreground(BrightGreen);
    display.foreground(BrightBlue);
    display.line(55,50, 79,74, BrightRed);
    display.line(57,50, 81,74, BrightGreen);
    display.line(59,50, 83,74, BrightBlue);
    // horz
    display.line(30,40, 32,40, BrightRed);
    display.line(30,42, 32,42, BrightGreen);
    display.line(30,44, 32,44, BrightBlue);
    // vert
    display.line(20,40, 20,42, BrightRed);
    display.line(22,40, 22,42, BrightGreen);
    display.line(24,40, 24,42, BrightBlue);
    // compare point to line-point
    display.pixel(20,50, BrightRed);
    display.pixel(22,50, BrightGreen);
    display.pixel(24,50, BrightBlue);
    display.line(20,52, 20,52, BrightRed);
    display.line(22,52, 22,52, BrightGreen);
    display.line(24,52, 24,52, BrightBlue);

    // point
    display.line(50,50, 50,50, Red);
    display.line(52,52, 52,52, Green);
    display.line(54,54, 54,54, Blue);
    display.line(60,60, 60,60, BrightRed);
    display.line(62,62, 62,62, BrightGreen);
    display.line(64,64, 64,64, BrightBlue);
    display.line(70,70, 70,70, DarkRed);
    display.line(72,72, 72,72, DarkGreen);
    display.line(74,74, 74,74, DarkBlue);
}


void RectangleTest(RA8875 & display, Serial & pc)
{
    int i, x1,y1, x2,y2;

    if (!SuppressSlowStuff)
        pc.printf("Rectangle Test\r\n");
    display.background(Black);
    display.foreground(Blue);
    display.cls();
    display.puts("Rectangle Test");
    for (i=0; i<16; i++) {
        x1 = rand() % 240;
        y1 = 50 + rand() % 200;
        x2 = rand() % 240;
        y2 = 50 + rand() % 200;
        display.rect(x1,y1, x2,y2, display.DOSColor(i));

        x1 = 240 + rand() % 240;
        y1 = 50 + rand() % 200;
        x2 = 240 + rand() % 240;
        y2 = 50 + rand() % 200;
        display.rect(x1,y1, x2,y2, FILL);
    }
}


void LayerTest(RA8875 & display, Serial & pc)
{
    loc_t i, x1,y1, x2,y2, r1,r2;

    if (!SuppressSlowStuff)
        pc.printf("Layer Test\r\n");

    display.SelectDrawingLayer(0);
    display.background(Black);
    display.foreground(Blue);
    display.cls();
    display.puts("Layer 0");
    for (i=0; i<16; i++) {
        x1 = rand() % 240;
        y1 = 50 + rand() % 200;
        x2 = x1 + rand() % 100;
        y2 = y1 + rand() % 100;
        r1 = rand() % (x2 - x1)/2;
        r2 = rand() % (y2 - y1)/2;
        display.roundrect(x1,y1, x2,y2, r1,r2, display.DOSColor(i));
        if (!SuppressSlowStuff)
            wait_ms(20);
    }
    if (!SuppressSlowStuff)
        wait_ms(1000);

    display.SelectDrawingLayer(1);
    display.background(Black);
    display.foreground(Yellow);
    display.cls();
    display.puts(240,0, "Layer 1");
    for (i=0; i<16; i++) {
        x1 = 300 + rand() % 100;
        y1 = 70 + rand() % 200;
        r1 = rand() % min(y1 - 20, 100);
        display.circle(x1,y1,r1, display.DOSColor(i));
        if (!SuppressSlowStuff)
            wait_ms(20);
    }
    display.SetLayerMode(RA8875::ShowLayer1);        // Show it after the build-up
    if (!SuppressSlowStuff)
        wait_ms(2000);

    display.SelectDrawingLayer(0);
    display.SetLayerMode(RA8875::ShowLayer0);        // Show Layer 0 again
    if (!SuppressSlowStuff)
        wait_ms(1000);
    display.SetLayerMode(RA8875::TransparentMode);        // Transparent mode
    if (!SuppressSlowStuff)
        wait_ms(1000);
    for (i=0; i<=8; i++) {
        display.SetLayerTransparency(i, 8-i);
        if (!SuppressSlowStuff)
            wait_ms(200);
    }

    // Restore before we exit
    display.SetLayerTransparency(0, 0);
    display.SetLayerMode(RA8875::ShowLayer0);        // Restore to layer 0
}


void RoundRectTest(RA8875 & display, Serial & pc)
{
    loc_t i, x1,y1, x2,y2, r1,r2;

    if (!SuppressSlowStuff)
        pc.printf("Round Rectangle Test\r\n");
    display.background(Black);
    display.foreground(Blue);
    display.cls();
    display.puts("Rounded Rectangle Test");

    for (i=0; i<16; i++) {
        x1 = rand() % 240;
        y1 = 50 + rand() % 200;
        x2 = x1 + rand() % 100;
        y2 = y1 + rand() % 100;
        r1 = rand() % (x2 - x1)/2;
        r2 = rand() % (y2 - y1)/2;
        display.roundrect(x1,y1, x2,y2, 5,8, display.DOSColor(i));

        x1 = 240 + rand() % 240;
        y1 = 50 + rand() % 200;
        x2 = x1 + rand() % 100;
        y2 = y1 + rand() % 100;
        r1 = rand() % (x2 - x1)/2;
        r2 = rand() % (y2 - y1)/2;
        display.roundrect(x1,y1, x2,y2, r1,r2, FILL);
    }
}


void TriangleTest(RA8875 & display, Serial & pc)
{
    int i, x1, y1, x2, y2, x3, y3;

    if (!SuppressSlowStuff)
        pc.printf("Triangle Test\r\n");
    display.background(Black);
    display.foreground(Blue);
    display.cls();
    display.puts(0,0, "Triangle Test");

    x1 = 150;
    y1 = 2;
    x2 = 190;
    y2 = 7;
    x3 = 170;
    y3 = 16;
    display.triangle(x1,y1, x2,y2, x3,y3);

    x1 = 200;
    y1 = 2;
    x2 = 240;
    y2 = 7;
    x3 = 220;
    y3 = 16;
    display.filltriangle(x1,y1, x2,y2, x3,y3, BrightRed);

    x1 = 300;
    y1 = 2;
    x2 = 340;
    y2 = 7;
    x3 = 320;
    y3 = 16;
    display.triangle(x1,y1, x2,y2, x3,y3, NOFILL);

    x1 = 400;
    y1 = 2;
    x2 = 440;
    y2 = 7;
    x3 = 420;
    y3 = 16;
    display.triangle(x1,y1, x2,y2, x3,y3, Blue);

    for (i=0; i<16; i++) {
        x1 = rand() % 240;
        y1 = 50 + rand() % 200;
        x2 = rand() % 240;
        y2 = 50 + rand() % 200;
        x3 = rand() % 240;
        y3 = 50 + rand() % 200;
        display.triangle(x1,y1, x2,y2, x3,y3, display.DOSColor(i));
        x1 = 240 + rand() % 240;
        y1 = 50 + rand() % 200;
        x2 = 240 + rand() % 240;
        y2 = 50 + rand() % 200;
        x3 = 240 + rand() % 240;
        y3 = 50 + rand() % 200;
        display.triangle(x1,y1, x2,y2, x3,y3, FILL);
    }
}


void CircleTest(RA8875 & display, Serial & pc)
{
    int i, x, y, r1;

    if (!SuppressSlowStuff)
        pc.printf("Circle Test\r\n");
    display.background(Black);
    display.foreground(Blue);
    display.cls();
    display.puts("Circle Test");
    for (i=0; i<16; i++) {
        x = 100 + rand() % 100;
        y = 70 + rand() % 200;
        r1 = rand() % min(y - 20, 100);
        //pc.printf("  (%d,%d) - %d\r\n", x,y,r1);
        display.circle(x,y,r1, display.DOSColor(i));

        x = 300 + rand() % 100;
        y = 70 + rand() % 200;
        r1 = rand() % min(y - 20, 100);
        //pc.printf("  (%d,%d) - %d FILL\r\n", x,y,r1);
        display.circle(x,y,r1, display.DOSColor(i), FILL);
    }
}


void EllipseTest(RA8875 & display, Serial & pc)
{
    int i,x,y,r1,r2;

    if (!SuppressSlowStuff)
        pc.printf("Ellipse Test\r\n");
    display.background(Black);
    display.foreground(Blue);
    display.cls();
    display.puts("Ellipse Test");
    for (i=0; i<16; i++) {
        x = 100 + rand() % 100;
        y = 70 + rand() % 200;
        r1 = rand() % min(y - 20, 100);
        r2 = rand() % min(y - 20, 100);
        display.ellipse(x,y,r1,r2, display.DOSColor(i));

        x = 300 + rand() % 100;
        y = 70 + rand() % 200;
        r1 = rand() % min(y - 20, 100);
        r2 = rand() % min(y - 20, 100);
        display.ellipse(x,y,r1,r2, FILL);
    }
}


void TestGraphicsBitmap(RA8875 & display, Serial & pc)
{
    LocalFileSystem local("local");
    if (!SuppressSlowStuff)
        pc.printf("Bitmap File Load\r\n");
    display.background(Black);
    display.foreground(Blue);
    display.cls();
    display.puts("Graphics Test, loading /local/TestPat.bmp");
    wait(3);

    int r = display.RenderBitmapFile(0,0, "/local/TestPat.bmp");
    if (!SuppressSlowStuff)
        pc.printf("  returned %d\r\n", r);
}


void TouchPanelTest(RA8875 & display, Serial & pc)
{
    Timer t;
    int x, y;
    tpMatrix_t calmatrix;

    display.background(Black);
    display.foreground(Blue);
    display.cls();
    display.puts("Touch Panel Test\r\n");
    pc.printf("Touch Panel Test\r\n");
    display.TouchPanelInit();
    pc.printf("  TP: c - calibrate, r - restore, t - test\r\n");
    int c = pc.getc();
    if (c == 'c') {
        point_t pTest[3] =
        { { 50, 50 }, {450, 150}, {225,250} };
        point_t pSample[3];
        for (int i=0; i<3; i++) {
            display.foreground(Blue);
            display.printf(" (%3d,%3d) => ", pTest[i].x, pTest[i].y);
            display.line(pTest[i].x-10, pTest[i].y, pTest[i].x+10, pTest[i].y, White);
            display.line(pTest[i].x, pTest[i].y-10, pTest[i].x, pTest[i].y+10, White);
            while (!display.TouchPanelA2DFiltered(&x, &y))
                wait_ms(20);
            pSample[i].x = x;
            pSample[i].y = y;
            display.line(pTest[i].x-10, pTest[i].y, pTest[i].x+10, pTest[i].y, Black);
            display.line(pTest[i].x, pTest[i].y-10, pTest[i].x, pTest[i].y+10, Black);
            display.foreground(Blue);
            display.printf(" (%4d,%4d)\r\n", x,y);
            while (display.TouchPanelA2DFiltered(&x, &y))
                wait_ms(20);
            wait(2);
        }
        display.TouchPanelComputeCalibration(pTest, pSample, &calmatrix);
        display.printf(" Writing calibration to tpcal.cfg\r\n");
        FILE * fh = fopen("/local/tpcal.cfg", "wb");
        if (fh) {
            fwrite(&calmatrix, sizeof(calmatrix), 1, fh);
            fclose(fh);
        }
        display.printf(" Calibration is complete.");
    } else if (c == 'r') {
        display.printf(" Reading calibration from tpcal.cfg\r\n");
        FILE * fh = fopen("/local/tpcal.cfg", "rb");
        if (fh) {
            fread(&calmatrix, sizeof(calmatrix), 1, fh);
            fclose(fh);
        }
        display.printf(" Calibration is complete.");
        display.TouchPanelSetMatrix(&calmatrix);
    }
    t.start();
    do {
        point_t point = {0, 0};
        if (display.TouchPanelReadable(&point)) {
            display.pixel(point.x, point.y, Red);
        }
    } while (t.read_ms() < 30000);
    pc.printf(">");
}


void SpeedTest(RA8875 & display, Serial & pc)
{
    Timer t;
    SuppressSlowStuff = true;
    pc.printf("\r\nSpeedTest disables delays, runs tests, reports overall time.\r\n");
    t.start();
    // do stuff fast
    TextCursorTest(display, pc);
    TextWrapTest(display, pc);
    BacklightTest(display, pc, 0);
    BacklightTest2(display, pc);
    ExternalFontTest(display, pc);
    DOSColorTest(display, pc);
    WebColorTest(display, pc);
    PixelTest(display, pc);
    LineTest(display, pc);
    RectangleTest(display, pc);
    RoundRectTest(display, pc);
    TriangleTest(display, pc);
    CircleTest(display, pc);
    EllipseTest(display, pc);
    LayerTest(display, pc);
    //TestGraphicsBitmap(display, pc);
    pc.printf("SpeedTest completed in %d msec\r\n", t.read_ms());
#ifdef PERF_METRICS
    display.ReportPerformance(pc);
#endif
    SuppressSlowStuff = false;
}


void PrintScreen(RA8875 & display, Serial & pc)
{
    if (!SuppressSlowStuff)
        pc.printf("PrintScreen\r\n");
    display.PrintScreen( 0,0, 480,272, "/local/Capture.bmp");
}


void RunTestSet(RA8875 & lcd, Serial & pc)
{
    int q = 0;
    int automode = 0;
    const unsigned char modelist[] = "BDWtGLlFROTPCEbw";   // auto-test in this order.

    while(1) {
        pc.printf("\r\n"
                  "B - Backlight up      b - backlight dim\r\n"
                  "D - DOS Colors        W - Web Colors\r\n"
                  "t - text cursor       G - Graphics Bitmap\r\n"
                  "L - Lines             F - external Font\r\n"
                  "R - Rectangles        O - rOund rectangles\r\n"
                  "T - Triangles         P - Pixels  \r\n"
                  "C - Circles           E - Ellipses\r\n"
                  "A - Auto Test mode    S - Speed Test\r\n"
                  "K - Keypad Test       s - touch screen test\r\n"
                  "p - print screen      r - reset  \r\n"
                  "l - layer test        w - wrapping text \r\n"
#ifdef PERF_METRICS
                  "0 - clear performance 1 - report performance\r\n"
#endif
                  "> ");
        if (automode == -1 || pc.readable()) {
            automode = -1;
            q = pc.getc();
            while (pc.readable())
                pc.getc();
        } else if (automode >= 0) {
            q = modelist[automode];
        }
        switch(q) {
#ifdef PERF_METRICS
            case '0':
                lcd.ClearPerformance();
                break;
            case '1':
                lcd.ReportPerformance(pc);
                break;
#endif
            case 'A':
                automode = 0;
                break;
            case 'B':
                BacklightTest(lcd, pc, 2);
                break;
            case 'b':
                BacklightTest2(lcd, pc);
                break;
            case 'D':
                DOSColorTest(lcd, pc);
                break;
            case 'K':
                KeyPadTest(lcd, pc);
                break;
            case 'W':
                WebColorTest(lcd, pc);
                break;
            case 't':
                TextCursorTest(lcd, pc);
                break;
            case 'w':
                TextWrapTest(lcd, pc);
                break;
            case 'F':
                ExternalFontTest(lcd, pc);
                break;
            case 'L':
                LineTest(lcd, pc);
                break;
            case 'l':
                LayerTest(lcd, pc);
                break;
            case 'R':
                RectangleTest(lcd, pc);
                break;
            case 'O':
                RoundRectTest(lcd, pc);
                break;
            case 'p':
                PrintScreen(lcd, pc);
                break;
            case 'S':
                SpeedTest(lcd, pc);
                break;
            case 's':
                TouchPanelTest(lcd, pc);
                break;
            case 'T':
                TriangleTest(lcd, pc);
                break;
            case 'P':
                PixelTest(lcd, pc);
                break;
            case 'G':
                TestGraphicsBitmap(lcd, pc);
                break;
            case 'C':
                CircleTest(lcd, pc);
                break;
            case 'E':
                EllipseTest(lcd, pc);
                break;
            case 'r':
                pc.printf("Resetting ...\r\n");
                wait_ms(20);
                mbed_reset();
                break;
            case ' ':
                break;
            default:
                printf("huh?\n");
                break;
        }
        if (automode >= 0) {
            automode++;
            if (automode >= sizeof(modelist))
                automode = 0;
            wait_ms(2000);
        }
        wait_ms(200);
    }
}

#endif // TESTENABLE
