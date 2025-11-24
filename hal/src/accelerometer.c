#include "accelerometer.h"
#include <stdatomic.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <math.h>
#include <pthread.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <sys/ioctl.h>
#include <linux/spi/spidev.h>

// ---------- CONFIG (change if wiring differs) ----------
#define ACCEL_SPI_DEV   "/dev/spidev0.0"

// ADXL335 on MCP3208:
//   X -> CH2, Y -> CH3, Z -> CH4
#define ACCEL_CH_X      2
#define ACCEL_CH_Y      3
#define ACCEL_CH_Z      4

// ADC reference voltage (3.3 V)
static const double VREF = 3.3;

// ADXL335 typical characteristics
static const double MIDPOINT   = 1.65;   // ~0 g level for X/Y, ~1 g for Z (after gravity)
static const double SENSITIVITY = 0.300; // V per g

// Hit detection parameters
// Tune these if needed.
static const double X_THRESH_G = 0.5;   // |x| > 0.5 g = hit
static const double Y_THRESH_G = 0.5;   // |y| > 0.5 g = hit
static const double Z_THRESH_G = 0.5;   // |z - 1.0| > 0.5 g = hit (since gravity ≈ 1 g)

static const double X_RELEASE_G = 0.2;  // must drop below this to re-arm
static const double Y_RELEASE_G = 0.2;
static const double Z_RELEASE_G = 0.2;

// Minimum time between hits on the same axis (ms)
static const long long HIT_REFRACT_MS = 120;

// Poll rate (10 ms = 100 Hz)
static const useconds_t POLL_USEC = 10000;
// ------------------------------------------------------

// Shared state
static int accel_fd = -1;
static pthread_t accel_thread;
static atomic_int *accel_runFlag = NULL;

static wavedata_t *sound_bass   = NULL;
static wavedata_t *sound_snare  = NULL;
static wavedata_t *sound_hihat  = NULL;

// --------- small time helper ---------
static long long now_ms(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (long long)ts.tv_sec * 1000LL + ts.tv_nsec / 1000000LL;
}

// --------- low-level ADC read (MCP3208) ---------
static int accel_read_adc_ch(int ch, uint32_t speed_hz)
{
    uint8_t tx[3] = {
        (uint8_t)(0x06 | ((ch & 0x04) >> 2)),  // start bit + single-ended
        (uint8_t)((ch & 0x03) << 6),           // channel bits
        0x00
    };
    uint8_t rx[3] = {0};

    struct spi_ioc_transfer tr = {
        .tx_buf = (unsigned long)tx,
        .rx_buf = (unsigned long)rx,
        .len = 3,
        .speed_hz = speed_hz,
        .bits_per_word = 8,
        .cs_change = 0
    };

    if (ioctl(accel_fd, SPI_IOC_MESSAGE(1), &tr) < 1) {
        perror("accel spi xfer");
        return -1;
    }

    // MCP3208: 12-bit result: [rx1 low 4 bits][rx2 8 bits]
    int value = ((rx[1] & 0x0F) << 8) | rx[2];
    return value;
}

// --------- thread: poll accelerometer + trigger sounds ---------
static void *accel_thread_func(void *arg)
{
    (void)arg;

    uint32_t speed = 400000;  // 400 kHz

    bool x_armed = true, y_armed = true, z_armed = true;
    long long lastHitX = 0, lastHitY = 0, lastHitZ = 0;

    while (atomic_load(accel_runFlag)) {
        int raw_x = accel_read_adc_ch(ACCEL_CH_X, speed);
        int raw_y = accel_read_adc_ch(ACCEL_CH_Y, speed);
        int raw_z = accel_read_adc_ch(ACCEL_CH_Z, speed);

        if (raw_x >= 0 && raw_y >= 0 && raw_z >= 0) {
            // Convert to volts
            double vx = (raw_x * VREF) / 4095.0;
            double vy = (raw_y * VREF) / 4095.0;
            double vz = (raw_z * VREF) / 4095.0;

            // Convert to g
            double gx = (vx - MIDPOINT) / SENSITIVITY;
            double gy = (vy - MIDPOINT) / SENSITIVITY;
            // Z has gravity ~1 g in resting orientation
            double gz = ((vz - MIDPOINT) / SENSITIVITY) + 1.0;

            long long t = now_ms();

            // ----- X axis → snare -----
            double ax = fabs(gx);
            if (x_armed && ax > X_THRESH_G &&
                (t - lastHitX) >= HIT_REFRACT_MS) {
                if (sound_snare != NULL) {
                    AudioMixer_queueSound(sound_snare);
                }
                lastHitX = t;
                x_armed = false;
            } else if (!x_armed && ax < X_RELEASE_G) {
                x_armed = true;
            }

            // ----- Y axis → hi-hat -----
            double ay = fabs(gy);
            if (y_armed && ay > Y_THRESH_G &&
                (t - lastHitY) >= HIT_REFRACT_MS) {
                if (sound_hihat != NULL) {
                    AudioMixer_queueSound(sound_hihat);
                }
                lastHitY = t;
                y_armed = false;
            } else if (!y_armed && ay < Y_RELEASE_G) {
                y_armed = true;
            }

            // ----- Z axis → bass drum -----
            double az = fabs(gz - 1.0);  // deviation from 1 g
            if (z_armed && az > Z_THRESH_G &&
                (t - lastHitZ) >= HIT_REFRACT_MS) {
                if (sound_bass != NULL) {
                    AudioMixer_queueSound(sound_bass);
                }
                lastHitZ = t;
                z_armed = false;
            } else if (!z_armed && az < Z_RELEASE_G) {
                z_armed = true;
            }
        }

        usleep(POLL_USEC);
    }

    return NULL;
}

// --------- public API ---------
void accelometer_init(atomic_int *runFlag,
                      wavedata_t *bass,
                      wavedata_t *snare,
                      wavedata_t *hihat)
{
    accel_runFlag = runFlag;
    sound_bass  = bass;
    sound_snare = snare;
    sound_hihat = hihat;

    accel_fd = open(ACCEL_SPI_DEV, O_RDWR);
    if (accel_fd < 0) {
        perror("accel open spidev");
        return;
    }

    uint8_t mode = 0;
    uint8_t bits = 8;
    uint32_t speed = 400000;

    if (ioctl(accel_fd, SPI_IOC_WR_MODE, &mode) == -1) {
        perror("accel spi mode");
    }
    if (ioctl(accel_fd, SPI_IOC_WR_BITS_PER_WORD, &bits) == -1) {
        perror("accel spi bpw");
    }
    if (ioctl(accel_fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed) == -1) {
        perror("accel spi speed");
    }

    pthread_create(&accel_thread, NULL, accel_thread_func, NULL);
}

void accelometer_stop(void)
{
    if (accel_fd >= 0) {
        // accel_runFlag should already be set to 0 by main
        pthread_join(accel_thread, NULL);
        close(accel_fd);
        accel_fd = -1;
    }
}
