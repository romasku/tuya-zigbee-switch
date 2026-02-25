#ifndef BUILD_DATE_YYYYMMDD_H
#define BUILD_DATE_YYYYMMDD_H

/* Helper to check the month letters in __DATE__ */
#define _MDATE_IS(c0, c1, c2) \
        (__DATE__[0] == (c0) && __DATE__[1] == (c1) && __DATE__[2] == (c2))

/* Month number (1..12) derived from the three-letter English month */
#define BUILD_MONTH_NUM                                \
        (_MDATE_IS('J', 'a', 'n')   ? 1                \
   : _MDATE_IS('F', 'e', 'b') ? 2                      \
   : _MDATE_IS('M', 'a', 'r') ? 3                      \
   : _MDATE_IS('A', 'p', 'r') ? 4                      \
   : _MDATE_IS('M', 'a', 'y') ? 5                      \
   : _MDATE_IS('J', 'u', 'n') ? 6                      \
   : _MDATE_IS('J', 'u', 'l') ? 7                      \
   : _MDATE_IS('A', 'u', 'g') ? 8                      \
   : _MDATE_IS('S', 'e', 'p') ? 9                      \
   : _MDATE_IS('O', 'c', 't') ? 10                     \
   : _MDATE_IS('N', 'o', 'v') ? 11                     \
   : _MDATE_IS('D', 'e', 'c') ? 12                     \
                              : 0 /* unknown format */ \
        )

/* Year digits (YYYY) are at positions 7..10 in __DATE__ */
#define BUILD_YEAR_CH0    __DATE__[7]
#define BUILD_YEAR_CH1    __DATE__[8]
#define BUILD_YEAR_CH2    __DATE__[9]
#define BUILD_YEAR_CH3    __DATE__[10]

/* Day digits (DD). Note: single-digit days have a leading space at pos 4. */
#define BUILD_DAY_CH0     (__DATE__[4] == ' ' ? '0' : __DATE__[4])
#define BUILD_DAY_CH1     __DATE__[5]

/* Month digits (MM) from BUILD_MONTH_NUM */
#define BUILD_MON_CH0     ('0' + (BUILD_MONTH_NUM / 10))
#define BUILD_MON_CH1     ('0' + (BUILD_MONTH_NUM % 10))

/* Stringify helper macros */
#define STRINGIFY(x)          #x
#define STRINGIFY_VALUE(x)    STRINGIFY(x)

/* Convert BUILD_MONTH_NUM to two-digit string */
#define MONTH_TENS_CHAR    ('0' + (BUILD_MONTH_NUM) / 10)
#define MONTH_ONES_CHAR    ('0' + (BUILD_MONTH_NUM) % 10)

/* Convert single digits to characters with zero padding */
#define DAY_TENS_CHAR      (__DATE__[4] == ' ' ? '0' : __DATE__[4])
#define DAY_ONES_CHAR      __DATE__[5]

/* ---- Public artifacts ---- */

/* Function to initialize build date at runtime - works with any compiler */
static inline void zb_build_date_init(char *dest) {
    dest[0] = 8;               /* Length */
    dest[1] = __DATE__[7];     /* Year digit 1 */
    dest[2] = __DATE__[8];     /* Year digit 2 */
    dest[3] = __DATE__[9];     /* Year digit 3 */
    dest[4] = __DATE__[10];    /* Year digit 4 */
    dest[5] = MONTH_TENS_CHAR; /* Month tens */
    dest[6] = MONTH_ONES_CHAR; /* Month ones */
    dest[7] = DAY_TENS_CHAR;   /* Day tens */
    dest[8] = DAY_ONES_CHAR;   /* Day ones */
}

/* Static buffer that gets initialized once */
static char ZB_BUILD_DATE_YYYYMMDD[9] = { 0 };

#endif
