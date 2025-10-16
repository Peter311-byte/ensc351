#ifndef HAL_LED_H
#define HAL_LED_H

// Name: Prasanna Loganathan
// Student Number: 301576977


/// ============================================================================
/// led.h
/// ----------------------------------------------------------------------------
/// Header file for LED control functions.
/// Provides prototypes to control onboard LEDs via sysfs.
/// ============================================================================

#include <stdbool.h>

/// ============================================================================
/// Function Prototypes
/// ----------------------------------------------------------------------------
/// led_initiate()            → (Optional) Initialize LED system (not implemented here)
/// led_setGreenBrightness()  → Turn green LED ON/OFF (true = ON, false = OFF)
/// led_setRedBrightness()    → Turn red LED ON/OFF (true = ON, false = OFF)
/// led_heartbeat()           → Set LED trigger mode to “heartbeat” (blinks automatically)
/// ============================================================================

// Optional initialization stub (declared for completeness)
void led_initiate(void);

// Control Green LED brightness (manual ON/OFF)
void led_setGreenBrightness(bool brightness);

// Control Red LED brightness (manual ON/OFF)
void led_setRedBrightness(bool brightness);

// Set LED trigger to "heartbeat" mode
void led_heartbeat(void);

#endif
