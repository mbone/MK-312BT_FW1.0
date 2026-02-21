/*
 * menu.h - Menu System and User Interface
 *
 * 4-button LCD menu system with mode selection, options,
 * power level, and advanced parameter adjustment screens.
 * Handles ramp-up intensity sequencing.
 */

#ifndef MENU_H
#define MENU_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Menu screen identifiers */
#define MENU_MAIN         0   /* Mode selection + status display */
#define MENU_OPTIONS      1   /* Settings submenu (6 options) */
#define MENU_POWER_LEVEL  2   /* Low/Normal/High power selection */
#define MENU_ADVANCED     3   /* Advanced parameter adjustment */
#define MENU_SPLIT        4   /* Split mode channel A/B selection */

/* Item counts for each menu */
#define OPTION_COUNT   7      /* Options menu items */
#define ADVANCED_COUNT 8      /* Advanced settings (Ramp Level through Pace) */
#define STATUS_COUNT   8      /* Status message strings */

/* Button events from the 4-button keypad */
typedef enum {
    BUTTON_NONE = 0,          /* No button pressed */
    BUTTON_UP,                /* PC6 - Right/Up button */
    BUTTON_DOWN,              /* PC4 - Left/Down button */
    BUTTON_OK,                /* PC5 - OK/Select button */
    BUTTON_MENU               /* PC7 - Menu button */
} ButtonEvent;

/* Current menu navigation state */
typedef struct {
    uint8_t current_menu;     /* Active screen (MENU_MAIN, etc.) */
    uint8_t menu_position;    /* Selected item index within current menu */
    bool edit_mode;           /* True when editing an advanced setting value */
} MenuState;

extern MenuState menu_state;

void menuInit(void);                          /* Init menu state + custom LCD chars */
void menuShowStartup(void);                   /* Display splash screen with battery */
void menuShowMode(uint8_t mode_index);        /* Refresh main mode display */
void menuHandleButton(ButtonEvent event);     /* Dispatch button to active screen */
void menuHandleRampUp(void);                  /* Advance ramp counter (call from loop) */
void menuStartRamp(void);                     /* Begin intensity ramp-up */
bool menuIsRampActive(void);                  /* Check if ramp is in progress */
bool menuIsOutputEnabled(void);               /* True when output is active */
uint8_t menuGetRampPercent(void);             /* Current ramp 0-100 for intensity scaling */
void menuStartOutput(void);                   /* Enable output + start ramp */
void menuStopOutput(void);                    /* Stop output (called on mode change) */

#ifdef __cplusplus
}
#endif

#endif
