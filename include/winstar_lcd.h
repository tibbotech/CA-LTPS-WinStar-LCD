#ifndef __TIBBO_LTPS_WINSTAR_LCD_INCLUDED__
#define __TIBBO_LTPS_WINSTAR_LCD_INCLUDED__


/*! \file winstar_lcd.h
 *  \brief Management class for WinStar and compatible LCD displays
 *  \author Alexey Skoufin
 *  \copyright (c) 2017 Tibbo Technology, Inc.
 *
 * This code assumes that display is connected trough tibbit #41 (8-bit port
 * extender). Low 7 bits of tibbit must be connected to the display inputs as
 * shown below. Eight (msb) bit remains unused.
 * \verbatim
 *  Bit#        LCD pin
 *  -----       -------
 *  0           4 (A0)
 *  1           5 (R/W)
 *  2           6 (E)
 *  3,4,5,6     11,12,13,14 (DB4 to DB7)
 * \endverbatim
 *  +5VDC and GND may be taken from pins 0 and 1 of Tibbit #41 or from Tibbit #00-3
 */


#include <stdint.h>
#ifdef __arm__
#include "ltps/capi/Ci2c_smbus.h"
#else
#include "stubs.h"
#endif


#define I2C_MAX_BURST_LEN 32


class WinStarLCD {
protected:
    enum mcp_reg { // MCP64008 registers
        DIR = 0x00,
        IOCON = 0x05,
        PULLUP = 0x06,
        GPIO = 0x09
    };
    enum ctl_bit { // MCP64008 control bits
        M_COMMAND = 0x00,
        M_DATA = 0x01,
        M_WRITE = 0x00,
        M_READ = 0x02,
        CLOCK_BIT = 0x04
    };
protected: // Members
    uint8_t _buf[I2C_MAX_BURST_LEN];
    uint8_t _bufp;
    uint8_t _mode;
    uint8_t _dev_addr;
    Ci2c_smbus _i2c;
protected: // Methods
    inline void i2c_out(uint8_t);
    inline void rawdata(uint8_t);
    void _do_init();
public:
    WinStarLCD();
    ~WinStarLCD();
    int init(int);
    int init(const char *);
    void flush();
    void command(uint8_t);
    void data(uint8_t);
    void clear();
    void home();
    void echo(const char *);
    void setAddr(uint8_t);
    void showCursor(bool);
};


#endif

