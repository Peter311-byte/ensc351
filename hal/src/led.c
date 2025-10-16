#include "led.h"
#include <stdio.h>
#include <stdlib.h>

// Name: Prasanna Loganathan
//Student Number: 301576977

/// ============================================================================
/// LED Control for BeagleBone
/// ----------------------------------------------------------------------------
/// This file directly writes to sysfs LED device files to control the onboard
/// LEDs via user-space file I/O.
/// ============================================================================

/// ---------------------------------------------------------------------------
/// Sysfs LED paths
/// (Adjust paths depending on board and LED naming)
/// ---------------------------------------------------------------------------
#define LED_GREEN_BRIGHT "/sys/class/leds/ACT/brightness"   // Green LED brightness file
#define LED_TRIGGER      "/sys/class/leds/ACT/trigger"       // LED trigger file (for heartbeat)
#define LED_RED_BRIGHT   "/sys/class/leds/PWR/brightness"    // Red LED brightness file

/// ============================================================================
/// led_setGreenBrightness()
/// - Controls the green LED (ACT) brightness manually.
/// - brightness = true → ON (1)
/// - brightness = false → OFF (0)
/// - Opens the sysfs brightness file, writes 1 or 0, then closes it.
/// ============================================================================
void led_setGreenBrightness(bool brightness){
    FILE* file = fopen(LED_GREEN_BRIGHT, "w");
    if (!file) return;  // fail silently if file cannot be opened
    fprintf(file, "%d", brightness ? 1 : 0);
    fclose(file);
}

/// ============================================================================
/// led_setRedBrightness()
/// - Controls the red LED (PWR) brightness manually.
/// - brightness = true → ON (1)
/// - brightness = false → OFF (0)
/// - Opens the sysfs brightness file, writes 1 or 0, then closes it.
/// ============================================================================
void led_setRedBrightness(bool brightness){
    FILE* file = fopen(LED_RED_BRIGHT, "w");
    if (!file) return;  // fail silently if file cannot be opened
    fprintf(file, "%d", brightness ? 1 : 0);
    fclose(file);
}

/// ============================================================================
/// led_heartbeat()
/// - Sets the LED trigger to "heartbeat" mode.
/// - The LED then blinks automatically, synchronized to CPU load.
/// - This is a standard kernel LED trigger mode.
/// ============================================================================
void led_heartbeat(void){
    FILE* triggerfile = fopen(LED_TRIGGER, "w");
    fprintf(triggerfile, "heartbeat");
    fclose(triggerfile);
}
