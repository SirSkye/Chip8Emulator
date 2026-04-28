#ifndef CHIP8_H
#define CHIP8_H

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>

struct chip8 {
    bool display[32][64];
    bool display_updt;
    uint8_t memory[4096];
    uint16_t I;
    uint8_t V[16];
    uint8_t delay_timer;
    uint8_t sound_timer;
    uint16_t pc;
    uint8_t sp;
    uint16_t stack[16];
    bool keyboard[16];

    #ifdef DEBUG
    bool EXIT;
    #endif
};

struct chip8 *chip8_create(void);
int chip8_load(struct chip8 *chip, const char* filename);
void chip8_emulate_cycle(struct chip8 *chip);
void chip8_decrement_timers(struct chip8 *chip);
void chip8_updt_key(struct chip8 *chip, int index, bool state);

#endif