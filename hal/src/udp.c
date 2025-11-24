#include "udp.h"              // Header for this UDP interface
#include "audioMIxer.h"       // Header for audio mixing and playback
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>        // For socket programming
#include <pthread.h>          // For threading
#include <stdatomic.h>        // For atomic variables (thread-safe flags)

// Buffer size for receiving UDP messages
#define BUF_SIZE 1024

// Global variables
static int udp_sock = -1;             // UDP socket file descriptor
static pthread_t udp_thread;          // Thread to handle incoming UDP messages
static atomic_int *udp_app_running = NULL; // Pointer to an atomic int controlling app running state

// Pointers to sound data (to be initialized by main or setter functions)
static wavedata_t *udp_sound_bass = NULL;
static wavedata_t *udp_sound_snare = NULL;
static wavedata_t *udp_sound_hihat = NULL;
static atomic_int *udp_BPM = NULL;    // Pointer to shared tempo variable
static atomic_int *udp_state = NULL;  // Pointer to shared mode/state variable

// Function to parse and handle UDP commands
static void udp_handle_command(const char *cmd) {
    // Copy the incoming command to a local buffer to safely tokenize it
    char command[BUF_SIZE];
    strncpy(command, cmd, BUF_SIZE);
    command[BUF_SIZE - 1] = 0; // Ensure null termination

    // Tokenize the command string using space or newline as delimiters
    char *token = strtok(command, " \n");
    if (!token) return; // Ignore empty commands

    // Handle "mode" command: change drum beat mode
    if (strcmp(token, "mode") == 0) {
        token = strtok(NULL, " \n"); // Get mode value
        if (token && udp_state) {
            int mode = atoi(token);
            atomic_store(udp_state, mode); // Atomically update mode
            printf("[UDP] Set mode to %d\n", mode);
        }

    // Handle "volume" command: change audio output volume
    } else if (strcmp(token, "volume") == 0) {
        token = strtok(NULL, " \n");
        if (token) {
            int vol = atoi(token);
            if (vol >= 0 && vol <= 100) { // Ensure volume is in 0-100 range
                AudioMixer_setVolume(vol); 
                printf("[UDP] Set volume to %d\n", vol);
            }
        }

    // Handle "tempo" command: update beats per minute
    } else if (strcmp(token, "tempo") == 0) {
        token = strtok(NULL, " \n");
        if (token && udp_BPM) {
            int bpm = atoi(token);
            if (bpm > 0) { // Tempo must be positive
                atomic_store(udp_BPM, bpm);
                printf("[UDP] Set tempo to %d bpm\n", bpm);
            }
        }

    // Handle "play" command: play a specific sound
    } else if (strcmp(token, "play") == 0) {
        token = strtok(NULL, " \n");
        if (token) {
            // Queue the requested sound in the audio mixer
            if (strcmp(token, "bass") == 0 && udp_sound_bass) {
                AudioMixer_queueSound(udp_sound_bass);
            } else if (strcmp(token, "snare") == 0 && udp_sound_snare) {
                AudioMixer_queueSound(udp_sound_snare);
            } else if (strcmp(token, "hihat") == 0 && udp_sound_hihat) {
                AudioMixer_queueSound(udp_sound_hihat);
            }
        }

    // Handle "shutdown" command: request application shutdown
    } else if (strcmp(token, "shutdown") == 0) {
        if (udp_app_running) {
            atomic_store(udp_app_running, 0);
            printf("[UDP] Shutdown requested\n");
        }
    }
}

// Thread function that continuously listens for incoming UDP messages
static void* udp_listener_thread(void *arg) {
    (void)arg; // Unused parameter

    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_len = sizeof(client_addr);
    char buffer[BUF_SIZE];

    // Loop until the application is running
    while (udp_app_running && atomic_load(udp_app_running)) {
        // Receive UDP packets
        ssize_t recv_len = recvfrom(udp_sock, buffer, BUF_SIZE - 1, 0,
                                    (struct sockaddr*)&client_addr, &addr_len);
        if (recv_len > 0) {
            buffer[recv_len] = '\0'; // Null-terminate the received data
            udp_handle_command(buffer); // Process the command
        }
    }
    return NULL;
}

// Initialize UDP interface and start listener thread
void udp_init(unsigned short port) {
    udp_sock = socket(AF_INET, SOCK_DGRAM, 0); // Create UDP socket
    if (udp_sock < 0) {
        perror("[UDP] socket");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);    // Bind to specified port
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(udp_sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("[UDP] bind");
        close(udp_sock);
        exit(EXIT_FAILURE);
    }

    // Launch listener thread
    pthread_create(&udp_thread, NULL, udp_listener_thread, NULL);
    printf("[UDP] Listening on port %d\n", port);
}

// Clean up UDP resources
void udp_cleanup(void) {
    if (udp_sock >= 0) {
        close(udp_sock); // Close socket
        udp_sock = -1;
    }
    pthread_join(udp_thread, NULL); // Wait for thread to finish
}

// Setter functions to update shared state from other parts of the program
void udp_set_tempo(atomic_int *BPM, int newTempo) {
    udp_BPM = BPM;
    atomic_store(udp_BPM, newTempo);
}

void udp_set_mode(atomic_int *state, int newMode) {
    udp_state = state;
    atomic_store(udp_state, newMode);
}

void udp_set_volume(int newVolume) {
    AudioMixer_setVolume(newVolume);
}

void udp_play_sound(wavedata_t *sound) {
    AudioMixer_queueSound(sound);
}

void udp_shutdown(atomic_int *app_running) {
    udp_app_running = app_running;
    atomic_store(udp_app_running, 0);
}

// Setter to assign sound data to global pointers
void udp_set_sounds(wavedata_t *bass, wavedata_t *snare, wavedata_t *hihat) {
    udp_sound_bass = bass;
    udp_sound_snare = snare;
    udp_sound_hihat = hihat;
}

void udp_set_app_running(atomic_int *app_running) {
    udp_app_running = app_running;
}