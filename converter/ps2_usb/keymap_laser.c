/*
 * 
 * Use extra buttons hooked to the Atmega as F13 and F14 keys, mapped to mousekey buttons.
 *
 */
#include "keymap_common.h"

/* start with KEYMAP_FULL since it defines F13, F14 etc. */

const uint8_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {
    /* 0: default
     * ,---.   ,---------------. ,---------------. ,---------------. ,-----------.     ,-----------.
     * |Esc|   |F1 |F2 |F3 |F4 | |F5 |F6 |F7 |F8 | |F9 |F10|F11|F12| |PrS|ScL|Pau|     |Pwr|Slp|Wak|
     * `---'   `---------------' `---------------' `---------------' `-----------'     `-----------'
     * ,-----------------------------------------------------------. ,-----------. ,---------------.
     * |  `|  1|  2|  3|  4|  5|  6|  7|  8|  9|  0|  -|  =|Backspa| |Ins|Hom|PgU| |NmL|  /|  *|  -|
     * |-----------------------------------------------------------| |-----------| |---------------|
     * |Tab  |  Q|  W|  E|  R|  T|  Y|  U|  I|  O|  P|  [|  ]|    \| |Del|End|PgD| |  7|  8|  9|   |
     * |-----------------------------------------------------------| `-----------' |-----------|  +|
     * |CapsLo|  A|  S|  D|  F|  G|  H|  J|  K|  L|  ;|  '|Return  |               |  4|  5|  6|   |
     * |-----------------------------------------------------------|     ,---.     |---------------|
     * |Shift   |  Z|  X|  C|  V|  B|  N|  M|  ,|  ,|  /|Shift     |     |Up |     |  1|  2|  3|   |
     * |-----------------------------------------------------------| ,-----------. |-----------|Ent|
     * |Ctrl |Gui |Alt |         Space         |Alt |Gui |Menu|Ctrl| |Lef|Dow|Rig| |      0|  .|   |
     * `-----------------------------------------------------------' `-----------' `---------------'
     */
	/*
    KEYMAP(
    ESC, F1,  F2,  F3,  F4,  F5,  F6,  F7,  F8,  F9,  F10, F11, F12,           PSCR,SLCK,BRK,
    ESC, 1,   2,   3,   4,   5,   6,   7,   8,   9,   0,   MINS,EQL, BSPC,     INS, HOME,PGUP,    NLCK,PSLS,PAST,PMNS,
    TAB, Q,   W,   E,   R,   T,   Y,   U,   I,   O,   P,   LBRC,RBRC,BSLS,     DEL, END, PGDN,    P7,  P8,  P9,
    CAPS,A,   S,   D,   F,   G,   H,   J,   K,   L,   SCLN,QUOT,     ENT,                         P4,  P5,  P6,  PPLS,
    LSFT,Z,   X,   C,   V,   B,   N,   M,   COMM,DOT, SLSH,          RSFT,          UP,           P1,  P2,  P3,
    LCTL,LGUI,LALT,          SPC,                     RALT,RGUI,APP, RCTL,     LEFT,DOWN,RGHT,    P0,       PDOT,PENT
    ),*/

    /* 0: default
     *         ,-----------------------------------------------.
     *         |F13|F14|F15|F16|F17|F18|F19|F20|F21|F22|F23|F24|
     * ,---.   |-----------------------------------------------|     ,-----------.     ,-----------.
     * |Esc|   |F1 |F2 |F3 |F4 |F5 |F6 |F7 |F8 |F9 |F10|F11|F12|     |PrS|ScL|Pau|     |VDn|VUp|Mut|
     * `---'   `-----------------------------------------------'     `-----------'     `-----------'
     * ,-----------------------------------------------------------. ,-----------. ,---------------.
     * |  `|  1|  2|  3|  4|  5|  6|  7|  8|  9|  0|  -|  =|JPY|Bsp| |Ins|Hom|PgU| |NmL|  /|  *|  -|
     * |-----------------------------------------------------------| |-----------| |---------------|
     * |Tab  |  Q|  W|  E|  R|  T|  Y|  U|  I|  O|  P|  [|  ]|  \  | |Del|End|PgD| |  7|  8|  9|  +|
     * |-----------------------------------------------------------| `-----------' |---------------|
     * |CapsL |  A|  S|  D|  F|  G|  H|  J|  K|  L|  ;|  '| Return |               |  4|  5|  6|KP,|
     * |-----------------------------------------------------------|     ,---.     |---------------|
     * |Shft|  <|  Z|  X|  C|  V|  B|  N|  M|  ,|  .|  /| RO|Shift |     |Up |     |  1|  2|  3|Ent|
     * |-----------------------------------------------------------| ,-----------. |---------------|
     * |Ctl|Gui|Alt|MHEN|     Space      |HENK|KANA|Alt|Gui|App|Ctl| |Lef|Dow|Rig| |  #|  0|  .|KP=|
     * `-----------------------------------------------------------' `-----------' `---------------'
     */
    KEYMAP_FULL(
               BTN1,BTN2,F15, F16, F17, F18, F19, F20, F21, F22, F23, F24,
    ESC,       F1,  F2,  F3,  F4,  F5,  F6,  F7,  F8,  F9,  F10, F11, F12,                 PSCR,SLCK,BRK,           VOLD, VOLU, MUTE,
    GRV,       1,   2,   3,   4,   5,   6,   7,   8,   9,   0,   MINS,EQL, JYEN, BSPC,     INS, HOME,PGUP,    NLCK, PSLS, PAST, PMNS,
    TAB,       Q,   W,   E,   R,   T,   Y,   U,   I,   O,   P,   LBRC,RBRC,BSLS,           DEL, END, PGDN,    P7,   P8,   P9,   PPLS,
    CAPS,      A,   S,   D,   F,   G,   H,   J,   K,   L,   SCLN,QUOT,     ENT,                               P4,   P5,   P6,   PCMM,
    LSFT,NUBS, Z,   X,   C,   V,   B,   N,   M,   COMM,DOT, SLSH, RO,      RSFT,                 UP,          P1,   P2,   P3,   PENT,
    LCTL,LGUI,LALT, MHEN,    SPC,        HENK, KANA,  RALT,RGUI,APP, RCTL,                 LEFT,DOWN,RGHT,    PWR,  P0,   PDOT, PEQL
    ),
};

const action_t PROGMEM fn_actions[] = {
};

