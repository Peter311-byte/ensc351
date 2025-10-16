#include "joystick.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/spi/spidev.h>

// Name: Prasanna Loganathan
//Student Number: 301576977

/// ============================================================================
/// Global joystick state structure (shared with getX/getY/isCenter)
/// ============================================================================
struct joystick j1;

/// ============================================================================
/// SPI channel read helper
/// - Reads 12-bit data from a specific MCP3208 ADC channel.
/// - tx/rx: SPI transfer buffers.
/// - Returns raw 12-bit ADC reading (0–4095) or -1 on failure.
/// ============================================================================
static int read_ch(int fd, int ch, uint32_t speed_hz){
   uint8_t tx[3] = { 
      (uint8_t)(0x06 | ((ch & 0x04) >> 2)),   // Start bit + single-ended mode
      (uint8_t)((ch & 0x03) << 6),            // Channel select bits
      0x00 
   };

   uint8_t rx[3] = { 0 };

   struct spi_ioc_transfer tr = {
     .tx_buf = (unsigned long)tx,
     .rx_buf = (unsigned long)rx,
     .len = 3,
     .speed_hz = speed_hz,
     .bits_per_word = 8,
     .cs_change = 0
   };

   // Perform SPI transaction
   if (ioctl(fd, SPI_IOC_MESSAGE(1), &tr) < 1) 
       return -1;

   // Combine received bytes into 12-bit value
   return ((rx[1] & 0x0F) << 8) | rx[2]; 
}

/// ============================================================================
/// read_direction()
/// - Main joystick sampling and direction detection logic.
/// - Reads both X and Y ADC channels.
/// - Normalizes readings around center point.
/// - Applies deadzone and threshold to map to direction enums.
/// Returns:
///   0  → success
///   1  → SPI/config error
///   2  → joystick not centered during first calibration
/// ============================================================================
int read_direction(int z){
   int fd = z;
   uint8_t mode = 0;         // SPI mode 0
   uint8_t bits = 8;         // 8 bits per word
   uint32_t speed = 250000;  // 250 kHz SPI speed
   double vref = 3.300;      // ADC reference voltage (same as joystick supply)

   // Direction tuning constants
   const double DEADZONE = 0.10;  // Ignore small movements around center
   const double THRESH   = 0.30;  // Threshold for directional classification

   // SPI configuration setup
   if (ioctl(fd, SPI_IOC_WR_MODE, &mode) == -1) { perror("mode"); return 1; }
   if (ioctl(fd, SPI_IOC_WR_BITS_PER_WORD, &bits) == -1) { perror("bpw"); return 1; }
   if (ioctl(fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed) == -1) { perror("speed"); return 1; }

   // Calibration: store center positions once
   static int center0 = -1, center1 = -1;

   // Read both channels (X, Y)
   int ch0 = read_ch(fd, 0, speed);  // X-axis
   int ch1 = read_ch(fd, 1, speed);  // Y-axis
   if (ch0 < 0 || ch1 < 0) { perror("spi"); return 1; }

   // Capture joystick center only once, ensure user is centered at startup
   if ((center0 < 0) && ((ch0>2070||ch0<2050)|| (ch1>2050||ch1<2030))){
       return 2;  // Not centered yet
   } else if(center0<0){
       center0 = ch0;
       center1 = ch1;
   }

   // Normalize ADC readings to roughly -1..+1 range
   double x = (ch0 - center0) / 2048.0;
   double y = (ch1 - center1) / 2048.0;

   // Apply deadzone (ignore slight offsets)
   if (x > -DEADZONE && x < DEADZONE) x = 0.0;
   if (y > -DEADZONE && y < DEADZONE) y = 0.0;

   // Classify direction using threshold
   Direction horiz =
       (x <= -THRESH) ? DIR_LEFT  :
       (x >=  THRESH) ? DIR_RIGHT : DIR_CENTER;

   Direction vert  =
       (y <= -THRESH) ? DIR_DOWN  :
       (y >=  THRESH) ? DIR_UP    : DIR_CENTER;

   // Store direction results
   j1.x = horiz;
   j1.y = vert;

   // Determine if joystick is centered (both axes)
   j1.center = (horiz == DIR_CENTER && vert == DIR_CENTER) ? 1 : 0;

   return 0;
}

/// ============================================================================
/// Direction getters (interface for main game)
/// - getX(): return current X-axis direction
/// - getY(): return current Y-axis direction
/// - isCenter(): return true if joystick centered
/// ============================================================================
Direction getX(void){
    return j1.x;
}

Direction getY(void){
    return j1.y;
}

bool isCenter(void){
    return j1.center;
}
