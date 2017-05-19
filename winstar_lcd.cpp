#include <stdlib.h>
#include <string.h>
#include "winstar_lcd.h"


static uint8_t _cp1251_chr_map[256] =
{
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
    0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F,
    0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2A, 0x2B, 0x2C, 0x2D, 0x2E, 0x2F,
    0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F,
    0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4A, 0x4B, 0x4C, 0x4D, 0x4E, 0x4F,
    0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5A, 0x5B, 0x5C, 0x5D, 0x5E, 0x5F,
    0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6A, 0x6B, 0x6C, 0x6D, 0x6E, 0x6F,
    0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7A, 0x7B, 0x7C, 0x7D, 0x7E, 0x7F,
    0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8A, 0x8B, 0x8C, 0x8D, 0x8E, 0x8F,
    0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9A, 0x9B, 0x9C, 0x9D, 0x9E, 0x9F,
    0xA0, 0xA1, 0xA2, 0xA3, 0xA4, 0xA5, 0xA6, 0xA7, 0xA8, 0xA9, 0xAA, 0xAB, 0xAC, 0xAD, 0xAE, 0xAF,
    0xB0, 0xB1, 0xB2, 0xB3, 0xB4, 0xB5, 0xB6, 0xB7, 0xB8, 0xB9, 0xBA, 0xBB, 0xBC, 0xBD, 0xBE, 0xBF,
    0xC0, 0xC1, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7, 0xC8, 0xC9, 0xCA, 0xCB, 0xCC, 0xCD, 0xCE, 0xCF, // 'A'
    0xD0, 0xD1, 0xD2, 0xD3, 0xD4, 0xD5, 0xD6, 0xD7, 0xD8, 0xD9, 0xDA, 0xDB, 0xDC, 0xDD, 0xDE, 0xDF, // 'Р'
    0xE0, 0xE1, 0xE2, 0xE3, 0xE4, 0xE5, 0xE6, 0xE7, 0xE8, 0xE9, 0xEA, 0xEB, 0xEC, 0xED, 0xEE, 0xEF,
    0xF0, 0xF1, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF7, 0xF8, 0xF9, 0xFA, 0xFB, 0xFC, 0xFD, 0xFE, 0xFF,
};


/*! \brief Constructor.
 * Constructs LCD object. The object then must be initialized by \c init() method call
 */
WinStarLCD::WinStarLCD(): _bufp(0), _mode(M_COMMAND|M_WRITE), _dev_addr(0x20), _i2c()
{
}


WinStarLCD::~WinStarLCD()
{
}


/*! \brief Appends data to the I2C transaction buffer, flushing buffer if necessary
 * \param[in] v Data to add to the buffer
 */
inline void
WinStarLCD::i2c_out(uint8_t v)
{
    if(I2C_MAX_BURST_LEN == _bufp)
        flush();
    _buf[_bufp++] = v;
}


/*! \brief Ouputs 7 bits of data by setting and then clearing clock bit.
 *  \note Eight (most signinficant) bit is unused
 *  \param[in] v Data to clock out
 */
inline void
WinStarLCD::rawdata(uint8_t v)
{
    i2c_out(v | CLOCK_BIT);
    i2c_out(v);
}


/*! \brief Main initialization procedure.
 * Function perform Tibbit #41 initialization by setting port extender pins
 * as output pins, pulling up lines to +5VDC and disabling address counter
 * increment.
 * Then, it issues "magic" display initialization sequence, which puts it into
 * 4-bit mode, clears display, hides cursor and moves cursor into home position
 */
void
WinStarLCD::_do_init()
{
    /* Set all lines as output */
    _i2c.W1b(_dev_addr, WinStarLCD::DIR, 0x00);

    /* Enable pullup on all lines */
    _i2c.W1b(_dev_addr, WinStarLCD::PULLUP, 0xFF);

    /* Disable address increment on burst writes */
    _i2c.W1b(_dev_addr, WinStarLCD::IOCON, 0x20);

    /* Now issue "magic" display init sequence: put it in 4-bit mode */
    rawdata(0x18);
    rawdata(0x18);
    rawdata(0x18);
    rawdata(0x10);
    flush();

    /* Clear display and set operation options */
    command(0x0C);
    clear();
    home();

    flush();
}


/*! \brief Initializes object using I/O bus number
 * \param[in] busn Bus number. Can be obtained by calling \c CI2c::find_bus() call
 * \retval < 0 if and error was occured
 * \retval 0 if initialization was successful
 */
int
WinStarLCD::init(int busn)
{
    if(_i2c.set_bus(busn) < 0)
        return -1;

    _do_init();
    return 0;
}


/*! \brief Initializes object using symbolic I/O slot name
 * \param[in] busn Slot name, in form -S<num>, where <num> is integer in range 1..28 for LTPS3.
 *          For other types of LTPS please consult hardware manual
 * \retval < 0 if and error was occured
 * \retval 0 if initialization was successful
 */
int
WinStarLCD::init(const char *busn)
{
    if(_i2c.set_bus(busn) < 0)
        return -1;

    _do_init();
    return 0;
}


/*! \brief Flushes internal data buffer, by issuing atomic I2C transaction
 */
void
WinStarLCD::flush()
{
    if(0 != _bufp) {
        _i2c.Wbb(_dev_addr, GPIO, _buf, _bufp);
        _bufp = 0;
    }
}


/*! \brief Sends LCD command
 * \param[in] c Command byte (see WinStar LCD documentation for more info)
 */
void
WinStarLCD::command(uint8_t c)
{
    uint8_t lo, hi;

    lo = c & 0x0F;
    hi = (c >> 4) & 0x0F;

    if(_mode != M_COMMAND|M_WRITE)
        flush();

    _mode = M_COMMAND | M_WRITE; // put display in command mode

    rawdata(_mode | (hi << 3));
    rawdata(_mode | (lo << 3));
}


/*! \brief Sends data byte
 * \param[in] c Data byte
 */
void
WinStarLCD::data(uint8_t v)
{
    uint8_t lo, hi;

    lo = v & 0x0F;
    hi = (v >> 4) & 0x0F;

    if(_mode != M_DATA|M_WRITE)
        flush();

    _mode = M_DATA|M_WRITE; // put display in data mode

    rawdata(_mode | (hi << 3));
    rawdata(_mode | (lo << 3));
}


/*! \brief Clears LCD
 */
void
WinStarLCD::clear()
{
    command(0x01);
}


/*! \brief Moves LCD cursor (visible or not) into home position (upper left corner)
 */
void
WinStarLCD::home()
{
    command(0x02);
}


/*! \brief Prints given string on the screen
 * \param[in] str String to print
 */
void
WinStarLCD::echo(const char *str)
{
    int l;

    if(NULL != str)
        for(l=strlen(str); l != 0; --l)
            data(_cp1251_chr_map[*str++]);
}


void
WinStarLCD::setAddr(uint8_t a)
{
    command(0x80 | a);
}


void
WinStarLCD::showCursor(bool)
{
}
