#ifndef HAL_JOYSTICK_H
#define HAL_JOYSTICK_H

// Name: Prasanna Loganathan
// Student Number: 301576977

/// ============================================================================
/// joystick.h
/// ----------------------------------------------------------------------------
/// Header for joystick input handling via SPI ADC (e.g., MCP3208)
/// Provides direction enums, data structure, and function prototypes.
/// ============================================================================
#include <stdatomic.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/spi/spidev.h>

/// ============================================================================
/// Direction Enum
/// ----------------------------------------------------------------------------
/// Represents the possible logical directions detected from the joystick.
/// DIR_CENTER → no movement / deadzone
/// DIR_LEFT, DIR_RIGHT, DIR_UP, DIR_DOWN → directional input
/// ============================================================================
typedef enum {
    DIR_CENTER,
    DIR_LEFT,
    DIR_RIGHT,
    DIR_UP,
    DIR_DOWN
} Direction;

/// ============================================================================
/// Function Prototypes
/// ----------------------------------------------------------------------------
/// isCenter()      → Returns true if joystick is in neutral position.
/// getX() / getY() → Returns horizontal / vertical direction enums.
/// read_ch()       → Internal SPI channel read helper (static).
/// read_direction()→ Reads joystick and updates direction data structure.
/// ============================================================================

void joystick_init(atomic_int* runState);

// Returns true if joystick centered
bool isCenter(void);

// Returns horizontal direction (LEFT/RIGHT/CENTER)
Direction getX(void);

// Returns vertical direction (UP/DOWN/CENTER)
Direction getY(void);

// Low-level SPI ADC read (internal helper)
static int read_ch(int fd, int ch, uint32_t speed_hz);

// Reads both axes, updates internal joystick struct
int read_direction(int z);

/// ============================================================================
/// Joystick Data Structure
/// ----------------------------------------------------------------------------
/// Holds current joystick state:
/// - center : boolean (true if neutral)
/// - x      : horizontal direction
/// - y      : vertical direction
/// ============================================================================
struct joystick {
    bool center;
    Direction x;
    Direction y;
};

#endif