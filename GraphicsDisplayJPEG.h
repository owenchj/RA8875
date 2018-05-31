/// Wrapping ChaN's TJpgDec into this graphics engine
///

#ifndef GraphicsDisplayJPEG_H
#define GraphicsDisplayJPEG_H

/*----------------------------------------------------------------------------/
/ TJpgDec - Tiny JPEG Decompressor include file               (C)ChaN, 2012
/----------------------------------------------------------------------------*/
#ifndef _TJPGDEC
#define _TJPGDEC

/*---------------------------------------------------------------------------*/
/* System Configurations */

#define JD_SZBUF        512 /* Size of stream input buffer */
#define JD_FORMAT       1   /* Output pixel format 0:RGB888 (3 BYTE/pix), 1:RGB565 (1 WORD/pix) */
#define JD_USE_SCALE    1   /* Use descaling feature for output */
#define JD_TBLCLIP      1   /* Use table for saturation (might be a bit faster but increases 1K bytes of code size) */

/*---------------------------------------------------------------------------*/

#include "DisplayDefs.h"

/// Error code results for the jpeg engine
typedef enum {
    JDR_OK = noerror,                   ///< 0: Succeeded 
    JDR_INTR = external_abort,          ///< 1: Interrupted by output function 
    JDR_INP = bad_parameter,            ///< 2: Device error or wrong termination of input stream 
    JDR_MEM1 = not_enough_ram,          ///< 3: Insufficient memory pool for the image 
    JDR_MEM2 = not_enough_ram,          ///< 4: Insufficient stream input buffer 
    JDR_PAR = bad_parameter,            ///< 5: Parameter error 
    JDR_FMT1 = not_supported_format,    ///< 6: Data format error (may be damaged data) 
    JDR_FMT2 = not_supported_format,    ///< 7: Right format but not supported 
    JDR_FMT3 = not_supported_format     ///< 8: Not supported JPEG standard 
} JRESULT;



/// Rectangular structure definition for the jpeg engine
typedef struct {
    loc_t left;         ///< left coord
    loc_t right;        ///< right coord
    loc_t top;          ///< top coord
    loc_t bottom;       ///< bottom coord
} JRECT;


/// Decompressor object structure for the jpeg engine
typedef struct JDEC JDEC;

/// Internal structure for the jpeg engine
struct JDEC {
    uint16_t dctr;              ///< Number of bytes available in the input buffer 
    uint8_t * dptr;             ///< Current data read ptr 
    uint8_t * inbuf;            ///< Bit stream input buffer 
    uint8_t dmsk;               ///< Current bit in the current read byte 
    uint8_t scale;              ///< Output scaling ratio 
    uint8_t msx;                ///< MCU size in unit of block (width, ...) 
    uint8_t msy;                ///< MCU size in unit of block (..., height) 
    uint8_t qtid[3];            ///< Quantization table ID of each component 
    int16_t dcv[3];             ///< Previous DC element of each component 
    uint16_t nrst;              ///< Restart inverval 
    uint16_t width;             ///< Size of the input image (pixel width, ...) 
    uint16_t height;            ///< Size of the input image (..., pixel height) 
    uint8_t * huffbits[2][2];   ///< Huffman bit distribution tables [id][dcac] 
    uint16_t * huffcode[2][2];  ///< Huffman code word tables [id][dcac] 
    uint8_t * huffdata[2][2];   ///< Huffman decoded data tables [id][dcac] 
    int32_t * qttbl[4];         ///< Dequaitizer tables [id] 
    void * workbuf;             ///< Working buffer for IDCT and RGB output 
    uint8_t * mcubuf;           ///< Working buffer for the MCU 
    void * pool;                ///< Pointer to available memory pool 
    uint16_t sz_pool;           ///< Size of momory pool (bytes available) 
    uint16_t (*infunc)(JDEC * jd, uint8_t * buffer, uint16_t bufsize);  ///< Pointer to jpeg stream input function 
    void * device;              ///< Pointer to I/O device identifiler for the session 
};


#endif /* _TJPGDEC */

#endif // GraphicsDisplayJPEG_H
