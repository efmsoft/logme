#pragma once

// We define strings below using macro to allow usage like
//   printf(ANSI_BLUE "my str");

#define ANSI_BLACK            "\x1b[0;30m"
#define ANSI_BLUE             "\x1b[0;34m"
#define ANSI_GREEN            "\x1b[0;32m"
#define ANSI_CYAN             "\x1b[0;36m"
#define ANSI_RED              "\x1b[0;31m"
#define ANSI_PURPLE           "\x1b[0;35m"
#define ANSI_BROWN            "\x1b[0;33m"
#define ANSI_GRAY             "\x1b[0;37m"
#define ANSI_LIGHT_GRAY       "\x1b[1;30m"
#define ANSI_LIGHT_BLUE       "\x1b[1;34m"
#define ANSI_LIGHT_GREEN      "\x1b[1;32m"
#define ANSI_LIGHT_CYAN       "\x1b[1;36m"
#define ANSI_LIGHT_RED        "\x1b[1;31m"
#define ANSI_LIGHT_PURPLE     "\x1b[1;35m"
#define ANSI_YELLOW           "\x1b[1;33m"
#define ANSI_WHITE            "\x1b[1;37m"

#define ANSI_BG_BLACK         "\x1b[0;40m"
#define ANSI_BG_BLUE          "\x1b[0;44m"
#define ANSI_BG_GREEN         "\x1b[0;42m"
#define ANSI_BG_CYAN          "\x1b[0;46m"
#define ANSI_BG_RED           "\x1b[0;41m"
#define ANSI_BG_PURPLE        "\x1b[0;45m"
#define ANSI_BG_BROWN         "\x1b[0;43m"
#define ANSI_BG_GRAY          "\x1b[0;47m"
#define ANSI_BG_LIGHT_GRAY    "\x1b[1;40m"
#define ANSI_BG_LIGHT_BLUE    "\x1b[1;44m"
#define ANSI_BG_LIGHT_GREEN   "\x1b[1;42m"
#define ANSI_BG_LIGHT_CYAN    "\x1b[1;46m"
#define ANSI_BG_LIGHT_RED     "\x1b[1;41m"
#define ANSI_BG_LIGHT_PURPLE  "\x1b[1;45m"
#define ANSI_BG_YELLOW        "\x1b[1;43m"
#define ANSI_BG_WHITE         "\x1b[1;47m"

#define ANSI_RESET            "\x1b[0m"

// ---------------------------------------------------------------------------
// Text formatting effects (supported with ENABLE_VIRTUAL_TERMINAL_PROCESSING)
// ---------------------------------------------------------------------------

#define ANSI_BOLD             "\x1b[1m"   // Bold / bright
#define ANSI_DIM              "\x1b[2m"   // Dim / faint
#define ANSI_ITALIC           "\x1b[3m"   // Italic
#define ANSI_UNDERLINE        "\x1b[4m"   // Underline
#define ANSI_BLINK            "\x1b[5m"   // Slow blink
#define ANSI_RAPID_BLINK      "\x1b[6m"   // Rapid blink
#define ANSI_REVERSE          "\x1b[7m"   // Invert foreground/background
#define ANSI_HIDDEN           "\x1b[8m"   // Hidden text (invisible)
#define ANSI_STRIKE           "\x1b[9m"   // Strikethrough

// Reset individual attributes
#define ANSI_BOLD_OFF         "\x1b[22m"
#define ANSI_DIM_OFF          "\x1b[22m"
#define ANSI_ITALIC_OFF       "\x1b[23m"
#define ANSI_UNDERLINE_OFF    "\x1b[24m"
#define ANSI_BLINK_OFF        "\x1b[25m"
#define ANSI_REVERSE_OFF      "\x1b[27m"
#define ANSI_HIDDEN_OFF       "\x1b[28m"
#define ANSI_STRIKE_OFF       "\x1b[29m"

// Double underline (some terminals may not support it)
#define ANSI_DOUBLE_UNDERLINE "\x1b[21m"

// Overline (Win10+ supports it)
#define ANSI_OVERLINE         "\x1b[53m"
#define ANSI_OVERLINE_OFF     "\x1b[55m"
