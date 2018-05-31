/// This file contains the RA8875 Touch panel methods.
///
/// It combines both resistive and capacitive touch methods, and tries
/// to make them nearly transparent alternates for each other.
///
#include "RA8875.h"

#define NOTOUCH_TIMEOUT_uS 100000
#define TOUCH_TICKER_uS      1000


// Translate from FT5206 Event Flag to Touch Code to API-match the
// alternate resistive touch screen driver common in the RA8875
// displays.
static const TouchCode_t EventFlagToTouchCode[4] = {
    touch,      // 00b Put Down
    release,    // 01b Put Up
    held,       // 10b Contact
    no_touch    // 11b Reserved
};


RetCode_t RA8875::TouchPanelInit(void)
{
    panelTouched = false;
    if (useTouchPanel == TP_CAP) {
        // Set to normal mode
        writeRegister8(FT5206_DEVICE_MODE, 0);
    } else {
        //TPCR0: Set enable bit, default sample time, wakeup, and ADC clock
        WriteCommand(TPCR0, TP_ENABLE | TP_ADC_SAMPLE_DEFAULT_CLKS | TP_ADC_CLKDIV_DEFAULT);
        // TPCR1: Set auto/manual, Ref voltage, debounce, manual mode params
        WriteCommand(TPCR1, TP_MODE_DEFAULT | TP_DEBOUNCE_DEFAULT);
        WriteCommand(INTC1, ReadCommand(INTC1) | RA8875_INT_TP);        // reg INTC1: Enable Touch Panel Interrupts (D2 = 1)
        WriteCommand(INTC2, RA8875_INT_TP);                            // reg INTC2: Clear any TP interrupt flag
        touchSample = 0;
        touchState = no_cal;
        touchTicker.attach_us(callback(this, &RA8875::_TouchTicker), TOUCH_TICKER_uS);
        touchTimer.start();
        touchTimer.reset();
    }
    return noerror;
}


RetCode_t RA8875::TouchPanelInit(uint8_t bTpEnable, uint8_t bTpAutoManual, uint8_t bTpDebounce, uint8_t bTpManualMode, uint8_t bTpAdcClkDiv, uint8_t bTpAdcSampleTime)
{
    if (useTouchPanel == TP_CAP) {
        TouchPanelInit();
    } else {
        // Parameter bounds check
        if( \
                !(bTpEnable == TP_ENABLE || bTpEnable == TP_ENABLE) || \
                !(bTpAutoManual == TP_MODE_AUTO || bTpAutoManual == TP_MODE_MANUAL) || \
                !(bTpDebounce == TP_DEBOUNCE_OFF || bTpDebounce == TP_DEBOUNCE_ON) || \
                !(bTpManualMode <= TP_MANUAL_LATCH_Y) || \
                !(bTpAdcClkDiv <= TP_ADC_CLKDIV_128) || \
                !(bTpAdcSampleTime <= TP_ADC_SAMPLE_65536_CLKS) \
          ) return bad_parameter;
        // Construct the config byte for TPCR0 and write them
        WriteCommand(TPCR0, bTpEnable | bTpAdcClkDiv | bTpAdcSampleTime);    // Note: Wakeup is never enabled
        // Construct the config byte for TPCR1 and write them
        WriteCommand(TPCR1, bTpManualMode | bTpDebounce | bTpManualMode);    // Note: Always uses internal Vref.
        // Set up the interrupt flag and enable bits
        WriteCommand(INTC1, ReadCommand(INTC1) | RA8875_INT_TP);        // reg INTC1: Enable Touch Panel Interrupts (D2 = 1)
        WriteCommand(INTC2, RA8875_INT_TP);                            // reg INTC2: Clear any TP interrupt flag
        touchSample = 0;
        touchState = no_cal;
        if (bTpEnable == TP_ENABLE) {
            touchTicker.attach_us(callback(this, &RA8875::_TouchTicker), TOUCH_TICKER_uS);
            touchTimer.start();
            touchTimer.reset();
        } else {
            touchTicker.detach();
            touchTimer.stop();
        }
    }
    return noerror;
}


int RA8875::TouchChannels(void)
{
    if (useTouchPanel == TP_CAP) {
        return 5;   // based on the FT5206 hardware
    } else if (useTouchPanel == TP_RES) {
        return 1;   // based on the RA8875 resistive touch driver
    } else {
        return 0;   // it isn't enabled, so there are none.
    }
}


// +----------------------------------------------------+
// |                                                    |
// |  1                                                 |
// |                                                    |
// |                                                    |
// |                                               2    |
// |                                                    |
// |                                                    |
// |                         3                          |
// |                                                    |
// +----------------------------------------------------+

RetCode_t RA8875::TouchPanelCalibrate(tpMatrix_t * matrix)
{
    return TouchPanelCalibrate(NULL, matrix);
}

RetCode_t RA8875::TouchPanelCalibrate(const char * msg, tpMatrix_t * matrix, int maxwait_s)
{
    point_t pTest[3];
    point_t pSample[3];
    int x,y;
    Timer timeout;  // timeout guards for not-installed, stuck, user not present...

    timeout.start();
    while (TouchPanelA2DFiltered(&x, &y) && timeout.read() < maxwait_s) {
        wait_ms(20);
        if (idle_callback) {
            if (external_abort == (*idle_callback)(touchcal_wait)) {
                return external_abort;
            }
        }
    }
    cls();
    if (msg)
        puts(msg);
    SetTextCursor(0,height()/2);
    pTest[0].x = 50;
    pTest[0].y = 50;
    pTest[1].x = width() - 50;
    pTest[1].y = height()/2;
    pTest[2].x = width()/2;
    pTest[2].y = height() - 50;

    for (int i=0; i<3; i++) {
        foreground(Blue);
        printf(" (%3d,%3d) => ", pTest[i].x, pTest[i].y);
        line(pTest[i].x-10, pTest[i].y, pTest[i].x+10, pTest[i].y, White);
        line(pTest[i].x, pTest[i].y-10, pTest[i].x, pTest[i].y+10, White);
        while (!TouchPanelA2DFiltered(&x, &y) && timeout.read() < maxwait_s) {
            wait_ms(20);
            if (idle_callback) {
                if (external_abort == (*idle_callback)(touchcal_wait)) {
                    return external_abort;
                }
            }
        }
        pSample[i].x = x;
        pSample[i].y = y;
        line(pTest[i].x-10, pTest[i].y, pTest[i].x+10, pTest[i].y, Black);
        line(pTest[i].x, pTest[i].y-10, pTest[i].x, pTest[i].y+10, Black);
        foreground(Blue);
        printf(" (%4d,%4d)\r\n", x,y);
        while (TouchPanelA2DFiltered(&x, &y) && timeout.read() < maxwait_s) {
            wait_ms(20);
            if (idle_callback) {
                if (external_abort == (*idle_callback)(touchcal_wait)) {
                    return external_abort;
                }
            }
        }
        for (int t=0; t<100; t++) {
            wait_ms(20);
            if (idle_callback) {
                if (external_abort == (*idle_callback)(touchcal_wait)) {
                    return external_abort;
                }
            }
        }
    }
    if (timeout.read() >= maxwait_s)
        return touch_cal_timeout;
    else
        return TouchPanelComputeCalibration(pTest, pSample, matrix);
}


/**********************************************************************
 *
 *     Function: TouchPanelReadable()
 *
 *  Description: Given a valid set of calibration factors and a point
 *                value reported by the touch screen, this function
 *                calculates and returns the true (or closest to true)
 *                display point below the spot where the touch screen
 *                was touched.
 *
 *
 *
 *  Argument(s): displayPtr (output) - Pointer to the calculated
 *                                      (true) display point.
 *               screenPtr (input) - Pointer to the reported touch
 *                                    screen point.
 *               matrixPtr (input) - Pointer to calibration factors
 *                                    matrix previously calculated
 *                                    from a call to
 *                                    setCalibrationMatrix()
 *
 *
 *  The function simply solves for Xd and Yd by implementing the
 *   computations required by the translation matrix.
 *
 *                                              /-     -\
 *              /-    -\     /-            -\   |       |
 *              |      |     |              |   |   Xs  |
 *              |  Xd  |     | A    B    C  |   |       |
 *              |      |  =  |              | * |   Ys  |
 *              |  Yd  |     | D    E    F  |   |       |
 *              |      |     |              |   |   1   |
 *              \-    -/     \-            -/   |       |
 *                                              \-     -/
 *
 *  It must be kept brief to avoid consuming CPU cycles.
 *
 *       Return: OK - the display point was correctly calculated
 *                     and its value is in the output argument.
 *               NOT_OK - an error was detected and the function
 *                         failed to return a valid point.
 *
 *                 NOTE!    NOTE!    NOTE!
 *
 *  setCalibrationMatrix() and getDisplayPoint() will do fine
 *  for you as they are, provided that your digitizer
 *  resolution does not exceed 10 bits (1024 values).  Higher
 *  resolutions may cause the integer operations to overflow
 *  and return incorrect values.  If you wish to use these
 *  functions with digitizer resolutions of 12 bits (4096
 *  values) you will either have to a) use 64-bit signed
 *  integer variables and math, or b) judiciously modify the
 *  operations to scale results by a factor of 2 or even 4.
 *
 */
TouchCode_t RA8875::TouchPanelReadable(point_t * TouchPoint)
{
    TouchCode_t ts = no_touch;

    if (useTouchPanel == TP_RES) {
        int a2dX = 0;
        int a2dY = 0;
        
        touchInfo[0].touchID = 0;
        ts = TouchPanelA2DFiltered(&a2dX, &a2dY);
        if (ts != no_touch) {
            panelTouched = true;
            numberOfTouchPoints = 1;

            if (tpMatrix.Divider != 0) {
                /* Operation order is important since we are doing integer */
                /*  math. Make sure you add all terms together before      */
                /*  dividing, so that the remainder is not rounded off     */
                /*  prematurely.                                           */
                touchInfo[0].coordinates.x = ( (tpMatrix.An * a2dX) +
                                  (tpMatrix.Bn * a2dY) + tpMatrix.Cn
                                ) / tpMatrix.Divider ;
                touchInfo[0].coordinates.y = ( (tpMatrix.Dn * a2dX) +
                                  (tpMatrix.En * a2dY) + tpMatrix.Fn
                                ) / tpMatrix.Divider ;
            } else {
                ts = no_cal;
            }
        } else {
            numberOfTouchPoints = 0;
        }
        touchInfo[0].touchCode = ts;
    } else /* (useTouchPanel == TP_CAP) */ {
        ;
    }
    if (panelTouched == true) {
        panelTouched = false;
        if (TouchPoint) {
            *TouchPoint = touchInfo[0].coordinates;
            ts = touchInfo[0].touchCode;
        } else {
            ts = touch;
        }
    }
    return ts;
}


TouchCode_t RA8875::TouchPanelGet(point_t * TouchPoint)
{
    TouchCode_t t = no_touch;

    while (true) {
        t = TouchPanelReadable(TouchPoint);
        if (t != no_touch)
            break;
        if (idle_callback) {
            if (external_abort == (*idle_callback)(touch_wait)) {
                return no_touch;
            }
        }
    }
    return t;
}

// Below here are primarily "helper" functions. While many are accessible
// to the user code, they usually don't need to be called.

RetCode_t RA8875::TouchPanelSetMatrix(tpMatrix_t * matrixPtr)
{
    if (matrixPtr == NULL || matrixPtr->Divider == 0)
        return bad_parameter;
    memcpy(&tpMatrix, matrixPtr, sizeof(tpMatrix_t));
    touchState = no_touch;
    return noerror;
}

static void InsertionSort(int * buf, int bufsize)
{
    int i, j;
    int temp;

    for(i = 1; i < bufsize; i++) {
        temp = buf[i];
        j = i;
        while( j && (buf[j-1] > temp) ) {
            buf[j] = buf[j-1];
            j = j-1;
        }
        buf[j] = temp;
    } // End of sort
}


void RA8875::_TouchTicker(void)
{
    if (touchTimer.read_us() > NOTOUCH_TIMEOUT_uS) {
        touchSample = 0;
        if (touchState == held)
            touchState = release;
        else
            touchState = no_touch;
        touchTimer.reset();
    }
}

TouchCode_t RA8875::TouchPanelA2DRaw(int *x, int *y)
{
    if( (ReadCommand(INTC2) & RA8875_INT_TP) ) {        // Test for TP Interrupt pending in register INTC2
        touchTimer.reset();
        *y = ReadCommand(TPYH) << 2 | ( (ReadCommand(TPXYL) & 0xC) >> 2 );   // D[9:2] from reg TPYH, D[1:0] from reg TPXYL[3:2]
        *x = ReadCommand(TPXH) << 2 | ( (ReadCommand(TPXYL) & 0x3)      );   // D[9:2] from reg TPXH, D[1:0] from reg TPXYL[1:0]
        WriteCommand(INTC2, RA8875_INT_TP);            // reg INTC2: Clear that TP interrupt flag
        touchState = touch;
    } else {
        touchState = no_touch;
    }
    return touchState;
}

TouchCode_t RA8875::TouchPanelA2DFiltered(int *x, int *y)
{
    static int xbuf[TPBUFSIZE], ybuf[TPBUFSIZE];
    static int lastX, lastY;
    int i, j;
    TouchCode_t ret = touchState;

    if( (ReadCommand(INTC2) & RA8875_INT_TP) ) {        // Test for TP Interrupt pending in register INTC2
        touchTimer.reset();
        // Get the next data samples
        ybuf[touchSample] =  ReadCommand(TPYH) << 2 | ( (ReadCommand(TPXYL) & 0xC) >> 2 );   // D[9:2] from reg TPYH, D[1:0] from reg TPXYL[3:2]
        xbuf[touchSample] =  ReadCommand(TPXH) << 2 | ( (ReadCommand(TPXYL) & 0x3)      );   // D[9:2] from reg TPXH, D[1:0] from reg TPXYL[1:0]
        // Check for a complete set
        if(++touchSample == TPBUFSIZE) {
            // Buffers are full, so process them using Finn's method described in Analog Dialogue No. 44, Feb 2010
            // This requires sorting the samples in order of size, then discarding the top 25% and
            //   bottom 25% as noise spikes. Finally, the middle 50% of the values are averaged to
            //   reduce Gaussian noise.
#if 1
            InsertionSort(ybuf, TPBUFSIZE);
            InsertionSort(xbuf, TPBUFSIZE);
#else
            // Sort the Y buffer using an Insertion Sort
            for(i = 1; i <= TPBUFSIZE; i++) {
                temp = ybuf[i];
                j = i;
                while( j && (ybuf[j-1] > temp) ) {
                    ybuf[j] = ybuf[j-1];
                    j = j-1;
                }
                ybuf[j] = temp;
            } // End of Y sort
            // Sort the X buffer the same way
            for(i = 1; i <= TPBUFSIZE; i++) {
                temp = xbuf[i];
                j = i;
                while( j && (xbuf[j-1] > temp) ) {
                    xbuf[j] = xbuf[j-1];
                    j = j-1;
                }
                xbuf[j] = temp;
            } // End of X sort
#endif
            // Average the middle half of the  Y values and report them
            j = 0;
            for(i = (TPBUFSIZE/4) - 1; i < TPBUFSIZE - TPBUFSIZE/4; i++ ) {
                j += ybuf[i];
            }
            *y = lastY = j * (float)2/TPBUFSIZE;    // This is the average
            // Average the middle half of the  X values and report them
            j = 0;
            for(i = (TPBUFSIZE/4) - 1; i < TPBUFSIZE - TPBUFSIZE/4; i++ ) {
                j += xbuf[i];
            }
            *x = lastX = j * (float)2/TPBUFSIZE;    // This is the average
            // Tidy up and return
            if (touchState == touch || touchState == held)
                touchState = held;
            else
                touchState = touch;
            ret = touchState;
            touchSample = 0;             // Ready to start on the next set of data samples
        } else {
            // Buffer not yet full, so do not return any results yet
            if (touchState == touch || touchState == held) {
                *x = lastX;
                *y = lastY;
                ret = touchState = held;
            }
        }
        WriteCommand(INTC2, RA8875_INT_TP);            // reg INTC2: Clear that TP interrupt flag
    } // End of initial if -- data has been read and processed
    else {
        if (touchState == touch || touchState == held) {
            *x = lastX;
            *y = lastY;
            ret = touchState = held;
        } else if (touchState == release) {
            *x = lastX;
            *y = lastY;
            ret = release;
            touchState = no_touch;
        }
    }
    return ret;
}

/*   The following section is derived from Carlos E. Vidales.
 *
 *   Copyright (c) 2001, Carlos E. Vidales. All rights reserved.
 *
 *   This sample program was written and put in the public domain
 *    by Carlos E. Vidales.  The program is provided "as is"
 *    without warranty of any kind, either expressed or implied.
 *   If you choose to use the program within your own products
 *    you do so at your own risk, and assume the responsibility
 *    for servicing, repairing or correcting the program should
 *    it prove defective in any manner.
 *   You may copy and distribute the program's source code in any
 *    medium, provided that you also include in each copy an
 *    appropriate copyright notice and disclaimer of warranty.
 *   You may also modify this program and distribute copies of
 *    it provided that you include prominent notices stating
 *    that you changed the file(s) and the date of any change,
 *    and that you do not charge any royalties or licenses for
 *    its use.
 *
 *   This file contains functions that implement calculations
 *    necessary to obtain calibration factors for a touch screen
 *    that suffers from multiple distortion effects: namely,
 *    translation, scaling and rotation.
 *
 *   The following set of equations represent a valid display
 *    point given a corresponding set of touch screen points:
 *
 *                                              /-     -\
 *              /-    -\     /-            -\   |       |
 *              |      |     |              |   |   Xs  |
 *              |  Xd  |     | A    B    C  |   |       |
 *              |      |  =  |              | * |   Ys  |
 *              |  Yd  |     | D    E    F  |   |       |
 *              |      |     |              |   |   1   |
 *              \-    -/     \-            -/   |       |
 *                                              \-     -/
 *    where:
 *           (Xd,Yd) represents the desired display point
 *                    coordinates,
 *           (Xs,Ys) represents the available touch screen
 *                    coordinates, and the matrix
 *           /-   -\
 *           |A,B,C|
 *           |D,E,F| represents the factors used to translate
 *           \-   -/  the available touch screen point values
 *                    into the corresponding display
 *                    coordinates.
 *    Note that for practical considerations, the utilities
 *     within this file do not use the matrix coefficients as
 *     defined above, but instead use the following
 *     equivalents, since floating point math is not used:
 *            A = An/Divider
 *            B = Bn/Divider
 *            C = Cn/Divider
 *            D = Dn/Divider
 *            E = En/Divider
 *            F = Fn/Divider
 *    The functions provided within this file are:
 *          setCalibrationMatrix() - calculates the set of factors
 *                                    in the above equation, given
 *                                    three sets of test points.
 *               getDisplayPoint() - returns the actual display
 *                                    coordinates, given a set of
 *                                    touch screen coordinates.
 * translateRawScreenCoordinates() - helper function to transform
 *                                    raw screen points into values
 *                                    scaled to the desired display
 *                                    resolution.
 */

/**********************************************************************
 *
 *     Function: setCalibrationMatrix()
 *
 *  Description: Calling this function with valid input data
 *                in the display and screen input arguments
 *                causes the calibration factors between the
 *                screen and display points to be calculated,
 *                and the output argument - matrixPtr - to be
 *                populated.
 *
 *               This function needs to be called only when new
 *                calibration factors are desired.
 *
 *
 *  Argument(s): displayPtr (input) - Pointer to an array of three
 *                                     sample, reference points.
 *               screenPtr (input) - Pointer to the array of touch
 *                                    screen points corresponding
 *                                    to the reference display points.
 *               matrixPtr (output) - Pointer to the calibration
 *                                     matrix computed for the set
 *                                     of points being provided.
 *
 *
 *  From the article text, recall that the matrix coefficients are
 *   resolved to be the following:
 *
 *
 *      Divider =  (Xs0 - Xs2)*(Ys1 - Ys2) - (Xs1 - Xs2)*(Ys0 - Ys2)
 *
 *
 *
 *                 (Xd0 - Xd2)*(Ys1 - Ys2) - (Xd1 - Xd2)*(Ys0 - Ys2)
 *            A = ---------------------------------------------------
 *                                   Divider
 *
 *
 *                 (Xs0 - Xs2)*(Xd1 - Xd2) - (Xd0 - Xd2)*(Xs1 - Xs2)
 *            B = ---------------------------------------------------
 *                                   Divider
 *
 *
 *                 Ys0*(Xs2*Xd1 - Xs1*Xd2) +
 *                             Ys1*(Xs0*Xd2 - Xs2*Xd0) +
 *                                           Ys2*(Xs1*Xd0 - Xs0*Xd1)
 *            C = ---------------------------------------------------
 *                                   Divider
 *
 *
 *                 (Yd0 - Yd2)*(Ys1 - Ys2) - (Yd1 - Yd2)*(Ys0 - Ys2)
 *            D = ---------------------------------------------------
 *                                   Divider
 *
 *
 *                 (Xs0 - Xs2)*(Yd1 - Yd2) - (Yd0 - Yd2)*(Xs1 - Xs2)
 *            E = ---------------------------------------------------
 *                                   Divider
 *
 *
 *                 Ys0*(Xs2*Yd1 - Xs1*Yd2) +
 *                             Ys1*(Xs0*Yd2 - Xs2*Yd0) +
 *                                           Ys2*(Xs1*Yd0 - Xs0*Yd1)
 *            F = ---------------------------------------------------
 *                                   Divider
 *
 *
 *       Return: OK - the calibration matrix was correctly
 *                     calculated and its value is in the
 *                     output argument.
 *               NOT_OK - an error was detected and the
 *                         function failed to return a valid
 *                         set of matrix values.
 *                        The only time this sample code returns
 *                        NOT_OK is when Divider == 0
 *
 *
 *
 *                 NOTE!    NOTE!    NOTE!
 *
 *  setCalibrationMatrix() and getDisplayPoint() will do fine
 *  for you as they are, provided that your digitizer
 *  resolution does not exceed 10 bits (1024 values).  Higher
 *  resolutions may cause the integer operations to overflow
 *  and return incorrect values.  If you wish to use these
 *  functions with digitizer resolutions of 12 bits (4096
 *  values) you will either have to a) use 64-bit signed
 *  integer variables and math, or b) judiciously modify the
 *  operations to scale results by a factor of 2 or even 4.
 *
 */
RetCode_t RA8875::TouchPanelComputeCalibration(point_t * displayPtr, point_t * screenPtr, tpMatrix_t * matrixPtr)
{
    RetCode_t retValue = noerror;

    tpMatrix.Divider = ((screenPtr[0].x - screenPtr[2].x) * (screenPtr[1].y - screenPtr[2].y)) -
                       ((screenPtr[1].x - screenPtr[2].x) * (screenPtr[0].y - screenPtr[2].y)) ;

    if( tpMatrix.Divider == 0 )  {
        retValue = bad_parameter;
    }  else   {
        tpMatrix.An = ((displayPtr[0].x - displayPtr[2].x) * (screenPtr[1].y - screenPtr[2].y)) -
                      ((displayPtr[1].x - displayPtr[2].x) * (screenPtr[0].y - screenPtr[2].y)) ;

        tpMatrix.Bn = ((screenPtr[0].x - screenPtr[2].x) * (displayPtr[1].x - displayPtr[2].x)) -
                      ((displayPtr[0].x - displayPtr[2].x) * (screenPtr[1].x - screenPtr[2].x)) ;

        tpMatrix.Cn = (screenPtr[2].x * displayPtr[1].x - screenPtr[1].x * displayPtr[2].x) * screenPtr[0].y +
                      (screenPtr[0].x * displayPtr[2].x - screenPtr[2].x * displayPtr[0].x) * screenPtr[1].y +
                      (screenPtr[1].x * displayPtr[0].x - screenPtr[0].x * displayPtr[1].x) * screenPtr[2].y ;

        tpMatrix.Dn = ((displayPtr[0].y - displayPtr[2].y) * (screenPtr[1].y - screenPtr[2].y)) -
                      ((displayPtr[1].y - displayPtr[2].y) * (screenPtr[0].y - screenPtr[2].y)) ;

        tpMatrix.En = ((screenPtr[0].x - screenPtr[2].x) * (displayPtr[1].y - displayPtr[2].y)) -
                      ((displayPtr[0].y - displayPtr[2].y) * (screenPtr[1].x - screenPtr[2].x)) ;

        tpMatrix.Fn = (screenPtr[2].x * displayPtr[1].y - screenPtr[1].x * displayPtr[2].y) * screenPtr[0].y +
                      (screenPtr[0].x * displayPtr[2].y - screenPtr[2].x * displayPtr[0].y) * screenPtr[1].y +
                      (screenPtr[1].x * displayPtr[0].y - screenPtr[0].x * displayPtr[1].y) * screenPtr[2].y ;
        touchState = no_touch;
        if (matrixPtr)
            memcpy(matrixPtr, &tpMatrix, sizeof(tpMatrix_t));
    }
    return( retValue ) ;
}

////////////////// Capacitive Touch Panel

uint8_t RA8875::readRegister8(uint8_t reg) {
    char val;
    
    m_i2c->write(m_addr, (const char *)&reg, 1);
    m_i2c->read(m_addr, &val, 1);
    return (uint8_t)val;
}

void RA8875::writeRegister8(uint8_t reg, uint8_t val) {
    char data[2];
    
    data[0] = (char)reg;
    data[1] = (char)val;
    m_i2c->write((int)FT5206_I2C_ADDRESS, data, 2);
}


// Interrupt for touch detection
void RA8875::TouchPanelISR(void)
{
    getTouchPositions();
    panelTouched = true;
}

uint8_t RA8875::getTouchPositions(void) {
    uint8_t valXH;
    uint8_t valYH;

    numberOfTouchPoints = readRegister8(FT5206_TD_STATUS) & 0xF;
    gesture = readRegister8(FT5206_GEST_ID);
    
    // If the switch statement was based only on numberOfTouchPoints, it would not
    // be able to generate notification for 'release' events (as it is no longer touched).
    // Therefore, forcing a 5, and it intentially falls through each lower case.
    switch (5) {    // numberOfTouchPoints
        case 5:
            valXH  = readRegister8(FT5206_TOUCH5_XH);
            valYH  = readRegister8(FT5206_TOUCH5_YH);
            touchInfo[4].touchCode = EventFlagToTouchCode[valXH >> 6];
            touchInfo[4].touchID   = (valYH >> 4);
            touchInfo[4].coordinates.x = (valXH & 0x0f)*256 + readRegister8(FT5206_TOUCH5_XL);
            touchInfo[4].coordinates.y = (valYH & 0x0f)*256 + readRegister8(FT5206_TOUCH5_YL);
        case 4:
            valXH  = readRegister8(FT5206_TOUCH4_XH);
            valYH  = readRegister8(FT5206_TOUCH4_YH);
            touchInfo[3].touchCode = EventFlagToTouchCode[valXH >> 6];
            touchInfo[3].touchID   = (valYH >> 4);
            touchInfo[3].coordinates.x = (readRegister8(FT5206_TOUCH4_XH) & 0x0f)*256 + readRegister8(FT5206_TOUCH4_XL);
            touchInfo[3].coordinates.y = (valYH & 0x0f)*256 + readRegister8(FT5206_TOUCH4_YL);
        case 3:
            valXH  = readRegister8(FT5206_TOUCH3_XH);
            valYH  = readRegister8(FT5206_TOUCH3_YH);
            touchInfo[2].touchCode = EventFlagToTouchCode[valXH >> 6];
            touchInfo[2].touchID   = (valYH >> 4);
            touchInfo[2].coordinates.x = (readRegister8(FT5206_TOUCH3_XH) & 0x0f)*256 + readRegister8(FT5206_TOUCH3_XL);
            touchInfo[2].coordinates.y = (valYH & 0x0f)*256 + readRegister8(FT5206_TOUCH3_YL);
        case 2:
            valXH  = readRegister8(FT5206_TOUCH2_XH);
            valYH  = readRegister8(FT5206_TOUCH2_YH);
            touchInfo[1].touchCode = EventFlagToTouchCode[valXH >> 6];
            touchInfo[1].touchID   = (valYH >> 4);
            touchInfo[1].coordinates.x  = (readRegister8(FT5206_TOUCH2_XH) & 0x0f)*256 + readRegister8(FT5206_TOUCH2_XL);
            touchInfo[1].coordinates.y  = (valYH & 0x0f)*256 + readRegister8(FT5206_TOUCH2_YL);
        case 1:
            valXH  = readRegister8(FT5206_TOUCH1_XH);
            valYH  = readRegister8(FT5206_TOUCH1_YH);
            touchInfo[0].touchCode = EventFlagToTouchCode[valXH >> 6];
            touchInfo[0].touchID   = (valYH >> 4);
            touchInfo[0].coordinates.x = (readRegister8(FT5206_TOUCH1_XH) & 0x0f)*256 + readRegister8(FT5206_TOUCH1_XL);
            touchInfo[0].coordinates.y = (valYH & 0x0f)*256 + readRegister8(FT5206_TOUCH1_YL);
            break;
        default:
            break;
    }
    return numberOfTouchPoints;
}

// #### end of touch panel code additions
