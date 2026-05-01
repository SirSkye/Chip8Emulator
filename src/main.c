// gcc main.c chip8.c -o main
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include "chip8.h"

#define SCALE 20
#define CHIP_HZ 700  // Instructions per second
#define TIMER_HZ 60 // Decrement per second
#define CHIP_DT (1000.0 / CHIP_HZ) // CHIP Deltatime
#define TIMER_DT (1000.0 / TIMER_HZ) // Timer Deltatime

#define SAMPLE_RATE 44100
#define BEEP_HZ 440 // Middle A

const uint32_t keymap[16] = {
    120,
    49,
    50,
    51,
    113,
    119,
    101,
    97,
    115,
    100,
    122,
    99,
    52,
    114,
    102,
    118,
};

void SDLCALL audio_callback(void *userdata, SDL_AudioStream *stream, int additional_amount, int total_amount) {
    static double phase = 0.0;
    double phase_step = (double)BEEP_HZ / SAMPLE_RATE;
    
    int samples_needed = additional_amount / sizeof(int16_t);
    int16_t *buf = malloc(additional_amount);

    for (int i = 0; i < samples_needed; i++) {
        buf[i] = (int16_t)(3000 * sin(2.0 * M_PI * phase));
        phase += phase_step;
        if (phase >= 1.0) phase -= 1.0;
    }

    SDL_PutAudioStreamData(stream, buf, additional_amount);
    free(buf);
}

int main(int argc, char *argv[]) {
    struct chip8 *chip = chip8_create();
    if (chip8_load(chip, ".\\programs\\tetris.ch8") != 0) {
        printf("CLOSING");
        free(chip);
        exit(1);
    }

    SDL_Window *window = NULL;
    SDL_Renderer *renderer = NULL;
    SDL_AudioStream *speaker = NULL;

    SDL_AudioSpec spec = {
        SDL_AUDIO_S16, 
        1, 
        SAMPLE_RATE 
    };

    int result = SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS | SDL_INIT_AUDIO);
    if (result < 0) {
        SDL_Log("SDL_Init error: %s\n", SDL_GetError());
        return -1;
    }

    window = SDL_CreateWindow("CHIP8", 64*SCALE, 32*SCALE, 0);
    if (window == NULL) {
        SDL_Log("SDL_CreateWindow error: %s\n", SDL_GetError());
        return -1;
    }

    renderer = SDL_CreateRenderer(window, NULL);
    if (renderer == NULL) {
        SDL_Log("SDL_CreateRenderer error: %s\n", SDL_GetError());
        return -1;
    }

    speaker = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, audio_callback, NULL);
    if (speaker == NULL) {
        SDL_Log("SDL_OpenAudioDeviceStream error: %s\n", SDL_GetError());
        return -1;
    }
    SDL_PauseAudioStreamDevice(speaker);

    SDL_Event event;
    int quit = 0;

    uint64_t time_last = SDL_GetTicks();
    double chip_accumulator = 0.0;
    double timer_accumulator = 0.0;

    bool sound_playing = false;
    
    while (!quit) {
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_EVENT_QUIT:
                    quit = 1;
                    break;
                case SDL_EVENT_KEY_DOWN:
                    for (int i = 0; i < 16; i++) {
                        if (keymap[i] == event.key.key) {
                            chip8_updt_key(chip, i, true);
                        }
                    }
                    break;
                case SDL_EVENT_KEY_UP:
                    for (int i = 0; i < 16; i++) {
                        if (keymap[i] == event.key.key) {
                            chip8_updt_key(chip, i, false);
                        }
                    }
                    break;
                default:
                    break;
            }
        }

        uint64_t time_now = SDL_GetTicks();
        chip_accumulator += time_now - time_last;
        timer_accumulator += time_now - time_last;
        time_last = time_now;
        while (chip_accumulator >= CHIP_DT) {
            chip8_emulate_cycle(chip);
            chip_accumulator -= CHIP_DT;
        }
        while (timer_accumulator >= TIMER_DT) {
            chip8_decrement_timers(chip);
            timer_accumulator -= TIMER_DT;
        }

        if (chip->sound_timer > 0) {
            if (!sound_playing) {
                SDL_ResumeAudioStreamDevice(speaker);
                sound_playing = true;
            }
        } else {
            if (sound_playing) {
                SDL_PauseAudioStreamDevice(speaker);
                sound_playing = false;
                SDL_ClearAudioStream(speaker); 
            }
        }

        if (chip->display_updt) {
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
            SDL_RenderClear(renderer);
            SDL_SetRenderDrawColor(renderer, 0xFF, 0xFF, 0xFF, SDL_ALPHA_OPAQUE);
            for (int i = 0; i < 32; i++) {
                for (int j = 0; j < 64; j++) {
                    if (chip->display[i][j]) {
                        SDL_FRect pixel = {j*SCALE, i*SCALE, SCALE, SCALE};
                        SDL_RenderFillRect(renderer, &pixel);
                    }
                }
            }
            SDL_RenderPresent(renderer);
            chip->display_updt = false;
        }
        
        SDL_Delay(1);

        #ifdef DEBUG
        if (chip->EXIT) {
            break;
        }
        #endif
    }
    
    FILE *file = fopen("memory.bin", "wb");
    if (file != NULL) {
        fwrite(chip->memory, sizeof(uint8_t), 4096, file);
        fclose(file);
    }
    file = fopen("display.bin", "wb");
    if (file != NULL) {
        fwrite(chip->display, sizeof(bool), 64*32, file);
        fclose(file);
    }
    file = fopen("registeres.bin", "wb");
    if (file != NULL) {
        fwrite(chip->V, sizeof(uint8_t), 16, file);
        fclose(file);
    }
    free(chip);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_DestroyAudioStream(speaker);
    SDL_Quit();
    return 0;
}