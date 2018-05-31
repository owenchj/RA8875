/* mbed TextDisplay Display Library Base Class
 * Copyright (c) 2007-2009 sford
 * Released under the MIT License: http://mbed.org/license/mit
 */

#include "TextDisplay.h"

//#define DEBUG "Text"
// ...
// INFO("Stuff to show %d", var); // new-line is automatically appended
//
#if (defined(DEBUG) && !defined(TARGET_LPC11U24))
#define INFO(x, ...) std::printf("[INF %s %3d] "x"\r\n", DEBUG, __LINE__, ##__VA_ARGS__);
#define WARN(x, ...) std::printf("[WRN %s %3d] "x"\r\n", DEBUG, __LINE__, ##__VA_ARGS__);
#define ERR(x, ...)  std::printf("[ERR %s %3d] "x"\r\n", DEBUG, __LINE__, ##__VA_ARGS__);
#else
#define INFO(x, ...)
#define WARN(x, ...)
#define ERR(x, ...)
#endif

TextDisplay::TextDisplay(const char *name) : Stream(name)
{
    _row = 0;
    _column = 0;
    if (name == NULL) {
        _path = NULL;
    } else {
        int len = strlen(name) + 2;
        _path = new char[len];
        snprintf(_path, len, "/%s", name);
    }
}

//TextDisplay::~TextDisplay()
//{
//    delete [] _path;
//}

int TextDisplay::_putc(int value)
{
    INFO("_putc(%d)", value);
    if(value == '\n') {
        _column = 0;
        _row++;
        if(_row >= rows()) {
            _row = 0;
        }
    } else {
        character(_column, _row, value);
        _column++;
        if(_column >= columns()) {
            _column = 0;
            _row++;
            if(_row >= rows()) {
                _row = 0;
            }
        }
    }
    return value;
}

// crude cls implementation, should generally be overwritten in derived class
RetCode_t TextDisplay::cls(uint16_t layers)
{
    INFO("cls()");
    locate(0, 0);
    for(int i=0; i<columns()*rows(); i++) {
        putc(' ');
    }
    return noerror;
}

RetCode_t TextDisplay::locate(textloc_t column, textloc_t row)
{
    INFO("locate(%d,%d)", column, row);
    _column = column;
    _row = row;
    return noerror;
}

int TextDisplay::_getc()
{
    return -1;
}

RetCode_t TextDisplay::foreground(uint16_t color)
{
    //INFO("foreground(%4X)", color);
    _foreground = color;
    return noerror;
}

RetCode_t TextDisplay::background(uint16_t color)
{
    //INFO("background(%4X)", color);
    _background = color;
    return noerror;
}

bool TextDisplay::claim(FILE *stream)
{
    if ( _path == NULL) {
        fprintf(stderr, "claim requires a name to be given in the instantiator of the TextDisplay instance!\r\n");
        return false;
    }
    if (freopen(_path, "w", stream) == NULL) {
        return false;       // Failed, should not happen
    }
    // make sure we use line buffering
    setvbuf(stdout, NULL, _IOLBF, columns());
    return true;
}

