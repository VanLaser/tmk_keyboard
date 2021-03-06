/*
Copyright 2011 Jun Wako <wakojun@gmail.com>

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <stdint.h>
#include <stdbool.h>

#include "action.h"
#include "print.h"
#include "util.h"
#include "debug.h"
#include "ps2.h"
#include "host.h"
#include "led.h"
#include "matrix.h"

#ifdef EXTRA_BUTTONS_ENABLE

#include <avr/io.h>
#include "timer.h"

#ifndef DEBOUNCE
	#define DEBOUNCE 5
#endif

static uint8_t buttons = 0;
static uint8_t buttons_debouncing = 0;
static bool debouncing = false;
static uint16_t debouncing_time = 0;

#endif

#ifdef THUMBSTICK_ENABLE

#include "mousekey.h"

#define STICK_MAX 1023
#define STICK_MIN 0
#define STICK_CENTER 512
#define STICK_SLOP 64

static int8_t map_value(int16_t v)
{
	v -= STICK_CENTER;
	int sign = 1;
	if (v < 0)
	{
		sign = -1;
		v = -v;
	}

	if (v < STICK_SLOP)
		return 0;

	return (sign * mk_max_speed * v) / 320;
}

#endif

static void matrix_make(uint8_t code);
static void matrix_break(uint8_t code);

/*
 * Matrix Array usage:
 * 'Scan Code Set 2' is assigned into 256(32x8)cell matrix.
 * Hmm, it is very sparse and not efficient :(
 *
 * Notes:
 * Both 'Hanguel/English'(F1) and 'Hanja'(F2) collide with 'Delete'(E0 71) and 'Down'(E0 72).
 * These two Korean keys need exceptional handling and are not supported for now. Sorry.
 *
 *    8bit wide
 *   +---------+
 *  0|         |
 *  :|   XX    | 00-7F for normal codes(without E0-prefix)
 *  f|_________|
 * 10|         |
 *  :|  E0 YY  | 80-FF for E0-prefixed codes
 * 1f|         |     (<YY>|0x80) is used as matrix position.
 *   +---------+
 *
 * Exceptions:
 * 0x83:    F7(0x83) This is a normal code but beyond  0x7F.
 * 0xFC:    PrintScreen
 * 0xFE:    Pause
 */
static uint8_t matrix[MATRIX_ROWS];
#define ROW(code)      (code>>3)
#define COL(code)      (code&0x07)

// matrix positions for exceptional keys
#define F7             (0x83)
#define PRINT_SCREEN   (0xFC)
#define PAUSE          (0xFE)

static bool is_modified = false;

#ifdef EXTRA_BUTTONS_ENABLE
static void init_buttons()
{
	//input with pull-up resistor
	BUTTON_ONE_DDR  &= ~(1<<BUTTON_ONE_DATA_BIT);
	BUTTON_ONE_PORT |=  (1<<BUTTON_ONE_DATA_BIT);
	BUTTON_TWO_DDR  &= ~(1<<BUTTON_TWO_DATA_BIT);
	BUTTON_TWO_PORT |=  (1<<BUTTON_TWO_DATA_BIT);
}

static uint8_t read_buttons()
{
	return (BUTTON_ONE_PIN & (1 << BUTTON_ONE_DATA_BIT) ? 0 : (1 << 0)) |
	       (BUTTON_TWO_PIN & (1 << BUTTON_TWO_DATA_BIT) ? 0 : (1 << 1));
}

#endif

#ifdef THUMBSTICK_ENABLE

#include "LUFA/Drivers/Peripheral/ADC.h"

static void thumbstick_read(uint8_t chanmask, int8_t *value, int8_t *last_value,
		bool *dirty)
{
	*value = map_value(ADC_GetChannelReading(ADC_REFERENCE_AVCC | chanmask));

	if (*value != *last_value) {
		*last_value = *value;
		*dirty = true;
	}
}

void process_thumbstick(void)
{
	// Cache the prior read to avoid over-reporting mouse movement
	static int8_t last_x = 0;
	static int8_t last_y = 0;
	int8_t x;
	int8_t y;

	bool dirty = false;
	thumbstick_read(ADC_CHANNEL7, &x, &last_x, &dirty);
	thumbstick_read(ADC_CHANNEL6, &y, &last_y, &dirty);

	if (dirty || x || y) {
		mousekey_set_xyvh(x, -y, 0, 0);
		mousekey_send();
		dprintf("x = %d, y = %d\n", x, y);
	}
}

#endif

void matrix_init(void)
{
    debug_enable = true;
    ps2_host_init();

    // initialize matrix state: all keys off
    for (uint8_t i=0; i < MATRIX_ROWS; i++) matrix[i] = 0x00;

#ifdef EXTRA_BUTTONS_ENABLE
    init_buttons();
#endif

#ifdef THUMBSTICK_ENABLE
    ADC_Init(ADC_SINGLE_CONVERSION | ADC_PRESCALE_32);
    ADC_SetupChannel(6); // A1 -> PF6
    ADC_SetupChannel(7); // A0 -> PF7
#endif

    return;
}



/*
 * PS/2 Scan Code Set 2: Exceptional Handling
 *
 * There are several keys to be handled exceptionally.
 * The scan code for these keys are varied or prefix/postfix'd
 * depending on modifier key state.
 *
 * Keyboard Scan Code Specification:
 *     http://www.microsoft.com/whdc/archive/scancode.mspx
 *     http://download.microsoft.com/download/1/6/1/161ba512-40e2-4cc9-843a-923143f3456c/scancode.doc
 *
 *
 * 1) Insert, Delete, Home, End, PageUp, PageDown, Up, Down, Right, Left
 *     a) when Num Lock is off
 *     modifiers | make                      | break
 *     ----------+---------------------------+----------------------
 *     Ohter     |                    <make> | <break>
 *     LShift    | E0 F0 12           <make> | <break>  E0 12
 *     RShift    | E0 F0 59           <make> | <break>  E0 59
 *     L+RShift  | E0 F0 12  E0 F0 59 <make> | <break>  E0 59 E0 12
 *
 *     b) when Num Lock is on
 *     modifiers | make                      | break
 *     ----------+---------------------------+----------------------
 *     Other     | E0 12              <make> | <break>  E0 F0 12
 *     Shift'd   |                    <make> | <break>
 *
 *     Handling: These prefix/postfix codes are ignored.
 *
 *
 * 2) Keypad /
 *     modifiers | make                      | break
 *     ----------+---------------------------+----------------------
 *     Ohter     |                    <make> | <break>
 *     LShift    | E0 F0 12           <make> | <break>  E0 12
 *     RShift    | E0 F0 59           <make> | <break>  E0 59
 *     L+RShift  | E0 F0 12  E0 F0 59 <make> | <break>  E0 59 E0 12
 *
 *     Handling: These prefix/postfix codes are ignored.
 *
 *
 * 3) PrintScreen
 *     modifiers | make         | break
 *     ----------+--------------+-----------------------------------
 *     Other     | E0 12  E0 7C | E0 F0 7C  E0 F0 12
 *     Shift'd   |        E0 7C | E0 F0 7C
 *     Control'd |        E0 7C | E0 F0 7C
 *     Alt'd     |           84 | F0 84
 *
 *     Handling: These prefix/postfix codes are ignored, and both scan codes
 *               'E0 7C' and 84 are seen as PrintScreen.
 *
 * 4) Pause
 *     modifiers | make(no break code)
 *     ----------+--------------------------------------------------
 *     Other     | E1 14 77 E1 F0 14 F0 77
 *     Control'd | E0 7E E0 F0 7E
 *
 *     Handling: Both code sequences are treated as a whole.
 *               And we need a ad hoc 'pseudo break code' hack to get the key off
 *               because it has no break code.
 *
 */
uint8_t matrix_scan(void)
{

    // scan code reading states
    static enum {
        INIT,
        F0,
        E0,
        E0_F0,
        // Pause
        E1,
        E1_14,
        E1_14_77,
        E1_14_77_E1,
        E1_14_77_E1_F0,
        E1_14_77_E1_F0_14,
        E1_14_77_E1_F0_14_F0,
        // Control'd Pause
        E0_7E,
        E0_7E_E0,
        E0_7E_E0_F0,
    } state = INIT;


    is_modified = false;

    // 'pseudo break code' hack
    if (matrix_is_on(ROW(PAUSE), COL(PAUSE))) {
        matrix_break(PAUSE);
    }

    uint8_t code = ps2_host_recv();
    if (code) xprintf("%i\r\n", code);
    if (!ps2_error) {
        switch (state) {
            case INIT:
                switch (code) {
                    case 0xE0:
                        state = E0;
                        break;
                    case 0xF0:
                        state = F0;
                        break;
                    case 0xE1:
                        state = E1;
                        break;
                    case 0x83:  // F7
                        matrix_make(F7);
                        state = INIT;
                        break;
                    case 0x84:  // Alt'd PrintScreen
                        matrix_make(PRINT_SCREEN);
                        state = INIT;
                        break;
                    case 0x00:  // Overrun [3]p.25
                        matrix_clear();
                        clear_keyboard();
                        print("Overrun\n");
                        state = INIT;
                        break;
                    case 0xAA:  // Self-test passed
                    case 0xFC:  // Self-test failed
                        printf("BAT %s\n", (code == 0xAA) ? "OK" : "NG");
                        led_set(host_keyboard_leds());
                        state = INIT;
                        break;
                    default:    // normal key make
                        if (code < 0x80) {
                            matrix_make(code);
                        } else {
                            matrix_clear();
                            clear_keyboard();
                            xprintf("unexpected scan code at INIT: %02X\n", code);
                        }
                        state = INIT;
                }
                break;
            case E0:    // E0-Prefixed
                switch (code) {
                    case 0x12:  // to be ignored
                    case 0x59:  // to be ignored
                        state = INIT;
                        break;
                    case 0x7E:  // Control'd Pause
                        state = E0_7E;
                        break;
                    case 0xF0:
                        state = E0_F0;
                        break;
                    default:
                        if (code < 0x80) {
                            matrix_make(code|0x80);
                        } else {
                            matrix_clear();
                            clear_keyboard();
                            xprintf("unexpected scan code at E0: %02X\n", code);
                        }
                        state = INIT;
                }
                break;
            case F0:    // Break code
                switch (code) {
                    case 0x83:  // F7
                        matrix_break(F7);
                        state = INIT;
                        break;
                    case 0x84:  // Alt'd PrintScreen
                        matrix_break(PRINT_SCREEN);
                        state = INIT;
                        break;
                    case 0xF0:
                        matrix_clear();
                        clear_keyboard();
                        xprintf("unexpected scan code at F0: F0(clear and cont.)\n");
                        break;
                    default:
                    if (code < 0x80) {
                        matrix_break(code);
                    } else {
                        matrix_clear();
                        clear_keyboard();
                        xprintf("unexpected scan code at F0: %02X\n", code);
                    }
                    state = INIT;
                }
                break;
            case E0_F0: // Break code of E0-prefixed
                switch (code) {
                    case 0x12:  // to be ignored
                    case 0x59:  // to be ignored
                        state = INIT;
                        break;
                    default:
                        if (code < 0x80) {
                            matrix_break(code|0x80);
                        } else {
                            matrix_clear();
                            clear_keyboard();
                            xprintf("unexpected scan code at E0_F0: %02X\n", code);
                        }
                        state = INIT;
                }
                break;
            // following are states of Pause
            case E1:
                switch (code) {
                    case 0x14:
                        state = E1_14;
                        break;
                    default:
                        state = INIT;
                }
                break;
            case E1_14:
                switch (code) {
                    case 0x77:
                        state = E1_14_77;
                        break;
                    default:
                        state = INIT;
                }
                break;
            case E1_14_77:
                switch (code) {
                    case 0xE1:
                        state = E1_14_77_E1;
                        break;
                    default:
                        state = INIT;
                }
                break;
            case E1_14_77_E1:
                switch (code) {
                    case 0xF0:
                        state = E1_14_77_E1_F0;
                        break;
                    default:
                        state = INIT;
                }
                break;
            case E1_14_77_E1_F0:
                switch (code) {
                    case 0x14:
                        state = E1_14_77_E1_F0_14;
                        break;
                    default:
                        state = INIT;
                }
                break;
            case E1_14_77_E1_F0_14:
                switch (code) {
                    case 0xF0:
                        state = E1_14_77_E1_F0_14_F0;
                        break;
                    default:
                        state = INIT;
                }
                break;
            case E1_14_77_E1_F0_14_F0:
                switch (code) {
                    case 0x77:
                        matrix_make(PAUSE);
                        state = INIT;
                        break;
                    default:
                        state = INIT;
                }
                break;
            // Following are states of Control'd Pause
            case E0_7E:
                if (code == 0xE0)
                    state = E0_7E_E0;
                else
                    state = INIT;
                break;
            case E0_7E_E0:
                if (code == 0xF0)
                    state = E0_7E_E0_F0;
                else
                    state = INIT;
                break;
            case E0_7E_E0_F0:
                if (code == 0x7E)
                    matrix_make(PAUSE);
                state = INIT;
                break;
            default:
                state = INIT;
        }
    }

    // TODO: request RESEND when error occurs?
/*
    if (PS2_IS_FAILED(ps2_error)) {
        uint8_t ret = ps2_host_send(PS2_RESEND);
        xprintf("Resend: %02X\n", ret);
    }
*/
#ifdef EXTRA_BUTTONS_ENABLE

    uint8_t btns = read_buttons();
    
    if (buttons_debouncing != btns)
    {
	    buttons_debouncing = btns;
	    debouncing = true;
	    debouncing_time = timer_read();
	    print(".");
    }

    if (debouncing && (timer_elapsed(debouncing_time) > DEBOUNCE))
    {
	    buttons = buttons_debouncing;
	    debouncing = false;

	    if (buttons & (1<<0))
		    matrix_make(0x08);
	    else
		    matrix_break(0x08);

	    if (buttons & (1<<1))
		    matrix_make(0x10);
	    else
		    matrix_break(0x10);

	    print("xxx\n");
    }

#endif

#ifdef THUMBSTICK_ENABLE
    process_thumbstick();
#endif

    return 1;
}

inline
bool matrix_is_on(uint8_t row, uint8_t col)
{
    return (matrix[row] & (1<<col));
}

inline
uint8_t matrix_get_row(uint8_t row)
{
    return matrix[row];
}

uint8_t matrix_key_count(void)
{
    uint8_t count = 0;
    for (uint8_t i = 0; i < MATRIX_ROWS; i++) {
        count += bitpop(matrix[i]);
    }
    return count;
}


inline
static void matrix_make(uint8_t code)
{
    if (!matrix_is_on(ROW(code), COL(code))) {
        matrix[ROW(code)] |= 1<<COL(code);
        is_modified = true;
    }
}

inline
static void matrix_break(uint8_t code)
{
    if (matrix_is_on(ROW(code), COL(code))) {
        matrix[ROW(code)] &= ~(1<<COL(code));
        is_modified = true;
    }
}

void matrix_clear(void)
{
    for (uint8_t i=0; i < MATRIX_ROWS; i++) matrix[i] = 0x00;
}
