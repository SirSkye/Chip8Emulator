#include "chip8.h"

#define SHIFT_VX true // For 8XY6 and 8XYE. Depends on if Y should be considered
// #define DEBUG

#define min(a,b) (((a)<(b))?(a):(b))
#define max(a,b) (((a)>(b))?(a):(b))

static const uint8_t font_set[80] = {
    0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
    0x20, 0x60, 0x20, 0x20, 0x70, // 1
    0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
    0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
    0x90, 0x90, 0xF0, 0x10, 0x10, // 4
    0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
    0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
    0xF0, 0x10, 0x20, 0x40, 0x40, // 7
    0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
    0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
    0xF0, 0x90, 0xF0, 0x90, 0x90, // A
    0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
    0xF0, 0x80, 0x80, 0x80, 0xF0, // C
    0xE0, 0x90, 0x90, 0x90, 0xE0, // D
    0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
    0xF0, 0x80, 0xF0, 0x80, 0x80  // F
};

struct chip8 *chip8_create(void) {
    struct chip8 *chip = malloc(sizeof(struct chip8));

    memcpy(&chip->memory[0x50], font_set, sizeof(font_set));
    memset(chip->display, 0, sizeof(chip->display));
    memset(chip->V, 0, sizeof(chip->V));
    memset(chip->stack, 0, sizeof(chip->stack));
    memset(chip->keyboard, 0, sizeof(chip->keyboard));

    chip->pc = 0x200;
    chip->sp = 0;
    chip->I = 0;
    chip->delay_timer = 0;
    chip->sound_timer = 0;
    return chip;
}

int chip8_load(struct chip8 *chip, const char* filename) {
    FILE* f = fopen(filename, "rb");
    if (f == NULL) {
        printf("ERROR: Could not open file\n");
        return -1;
    }

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    if (size > (0xFFF - 0x200)) {
        printf("ERROR: ROM too large\n");
        return -1;
    }
    fseek(f, 0, SEEK_SET);

    uint8_t *buf = malloc(size);
    fread(buf, sizeof(uint8_t), size, f);
    fclose(f);

    memcpy(&chip->memory[0x200], buf, size);
    free(buf);
    return 0;
}

void chip8_emulate_cycle(struct chip8 *chip) {
    // FETCH
    uint16_t opcode = (chip->memory[chip->pc] << 8) | (chip->memory[chip->pc + 1]);
    chip->pc += 2;
    chip->display_updt = false;
    printf("OPCODE: %X\n", opcode);

    // DECODE EXECUTE
    switch (opcode & 0xF000) {
        case 0x0000:
            switch (opcode & 0x00FF) {
                // 00E0 - CLS
                case 0x00E0:
                    chip->display_updt = true;
                    memset(chip->display, 0, sizeof(chip->display));
                    break;
                // 00EE - RET
                case 0x00EE:
                    assert(chip->sp > 0);
                    chip->sp--;
                    chip->pc = chip->stack[chip->sp];
                    break;
                default:
                    printf("ERROR: Unknown Opcode %X\n", opcode);
                    exit(-1);
                    break;
            }
            break;
        // 1nnn - JP addr
        case 0x1000:
            chip->pc = opcode & 0x0FFF;
            #ifdef DEBUG
            if (getchar() == 'e')  {
                chip->EXIT = true;
            }
            #endif
            break;
        // 2nnn - CALL addr
        case 0x2000:
            chip->stack[chip->sp] = chip->pc;
            chip->sp++;
            chip->pc = opcode & 0x0FFF;
            break;
        // 3xkk - SE Vx, byte
        case 0x3000:
            if (chip->V[(opcode & 0x0F00) >> 8] == (opcode & 0x00FF)) {
                chip->pc += 2;
            }
            break;
        // 4xkk - SNE Vx, byte
        case 0x4000:
            if (chip->V[(opcode & 0x0F00) >> 8] != (opcode & 0x00FF)) {
                chip->pc += 2;
            }
            break;
        // 5xy0 - SE Vx, Vy
        case 0x5000:
            if (chip->V[(opcode & 0x0F00) >> 8] == chip->V[(opcode & 0x00F0) >> 4]) {
                chip->pc += 2;
            }
            break;
        // 6xkk - LD Vx, byte
        case 0x6000:
            chip->V[(opcode & 0x0F00) >> 8] = opcode & 0x00FF;
            break;
        // 7xkk - ADD Vx, byte
        case 0x7000:
            chip->V[(opcode & 0x0F00) >> 8] += (opcode & 0x00FF);
            break;
        case 0x8000:
            switch (opcode & 0x000F) {
                // 8xy0 - LD Vx, Vy
                case 0x0000:
                    chip->V[(opcode & 0x0F00) >> 8] = chip->V[(opcode & 0x00F0) >> 4];
                    break;
                // 8xy1 - OR Vx, Vy
                case 0x0001:
                    chip->V[(opcode & 0x0F00) >> 8] |= chip->V[(opcode & 0x00F0) >> 4];
                    break;
                // 8xy2 - AND Vx, Vy
                case 0x0002:
                    chip->V[(opcode & 0x0F00) >> 8] &= chip->V[(opcode & 0x00F0) >> 4];
                    break;
                // 8xy3 - XOR Vx, Vy
                case 0x0003:
                    chip->V[(opcode & 0x0F00) >> 8] ^= chip->V[(opcode & 0x00F0) >> 4];
                    break;
                // 8xy4 - ADD Vx, Vy
                case 0x0004:
                    unsigned int result = chip->V[(opcode & 0x0F00) >> 8] + chip->V[(opcode & 0x00F0) >> 4];
                    chip->V[(opcode & 0x0F00) >> 8] = result;
                    chip->V[0xF] = (result > 0xFF);
                    break;
                // 8xy5 - SUB Vx, Vy
                case 0x0005:
                    chip->V[0xF] = 0;
                    if (chip->V[(opcode & 0x0F00) >> 8] >= chip->V[(opcode & 0x00F0) >> 4]) {
                        chip->V[0xF] = 1;
                    }
                    chip->V[(opcode & 0x0F00) >> 8] -= chip->V[(opcode & 0x00F0) >> 4];
                    break;
                // 8xy6 - SHR Vx {, Vy}
                case 0x0006:
                    chip->V[0xF] = chip->V[(opcode & 0x0F00) >> 8] & 0x1;
                    if (SHIFT_VX) {
                        chip->V[(opcode & 0x0F00) >> 8] = chip->V[(opcode & 0x0F00) >> 8] >> 1;
                    } else {
                        chip->V[(opcode & 0x0F00) >> 8] = chip->V[(opcode & 0x00F0) >> 4] >> 1;
                    }
                    break;
                // 8xy7 - SUBN Vx, Vy
                case 0x0007:
                    chip->V[0xF] = 0;
                    if (chip->V[(opcode & 0x00F0) >> 4] >= chip->V[(opcode & 0x0F00) >> 8]) {
                        chip->V[0xF] = 1;
                    }
                    chip->V[(opcode & 0x0F00) >> 8] = chip->V[(opcode & 0x00F0) >> 4] - chip->V[(opcode & 0x0F00) >> 8];
                    break;
                // 8xyE - SHL Vx {, Vy}
                case 0x000E:
                    chip->V[0xF] = chip->V[(opcode & 0x0F00) >> 8] >> 7;
                    if (SHIFT_VX) {
                        chip->V[(opcode & 0x0F00) >> 8] = chip->V[(opcode & 0x0F00) >> 8] << 1;
                    } else {
                        chip->V[(opcode & 0x0F00) >> 8] = chip->V[(opcode & 0x00F0) >> 4] << 1;
                    }
                    break;
                default:
                    printf("ERROR: Unknown Opcode: %X\n", opcode);
                    exit(-1);
                    break;
            }
            break;
        // 9xy0 - SNE Vx, Vy
        case 0x9000:
            if (chip->V[(opcode & 0x0F00) >> 8] != chip->V[(opcode & 0x00F0) >> 4]) {
                chip->pc += 2;
            }
            break;
        // Annn - LD I, addr
        case 0xA000:
            chip->I = opcode & 0x0FFF;
            break;
        // Cxkk - RND Vx, byte
        case 0xC000:
            chip->V[(opcode & 0x0F00) >> 8] = (rand() & 0xFF) & (opcode & 0x00FF);
            break;
        // Dxyn - DRW Vx, Vy, nibble
        case 0xD000:
            chip->display_updt = true;
            unsigned int x = chip->V[(opcode & 0x0F00) >> 8] % 64;
            unsigned int y = chip->V[(opcode & 0x00F0) >> 4] % 32;
            uint8_t height = opcode & 0x000F;
            chip->V[0xF] = 0;
            for (int i = 0; i < height; i++) {
                if (y + i >= 32) break;
                uint8_t row = chip->memory[chip->I + i];
                for (int j = 0; j < 8; j++) {
                    if (x + j >= 64) break;
                    if (row & (0x80 >> j)) {
                        if (chip->display[y + i][x + j]) {
                            chip->V[0xF] = 1;
                        }
                        chip->display[y + i][x + j] ^= 1;
                    }
                }
            }
            break;
        case 0xE000:
            switch (opcode & 0x00FF) {
                // Ex9E - SKP Vx
                case 0x009E:
                    assert(chip->V[(opcode & 0x0F00) >> 8] <= 0xF);
                    if (chip->keyboard[chip->V[(opcode & 0x0F00) >> 8]]) {
                        chip->pc += 2;
                    }
                    break;
                // ExA1 - SKNP Vx
                case 0x00A1:
                    assert(chip->V[(opcode & 0x0F00) >> 8] <= 0xF);
                    if (!chip->keyboard[chip->V[(opcode & 0x0F00) >> 8]]) {
                        chip->pc += 2;
                    }
                    break;
            }
            break;
        case 0xF000:
            switch (opcode & 0x00FF) {
                // Fx07 - LD Vx, DT
                case 0x0007:
                    chip->V[(opcode & 0x0F00) >> 8] = chip->delay_timer;
                    break;
                // Fx0A - LD Vx, K
                case 0x000A:
                    bool key = false;
                    for (int i = 0; i < 16; i++) {
                        if (chip->keyboard[i]) {
                            chip->V[(opcode & 0x0F00) >> 8] = i;
                            key = true;
                        }
                    }
                    if (!key) {
                        chip->pc -= 2;
                    }
                    break;
                // Fx15 - LD DT, Vx
                case 0x0015:
                    chip->delay_timer = chip->V[(opcode & 0x0F00) >> 8];
                    break;
                // Fx18 - LD ST, Vx
                case 0x0018:
                    chip->sound_timer = chip->V[(opcode & 0x0F00) >> 8];
                    break;
                // Fx1E - ADD I, Vx
                case 0x001E:
                    chip->I += chip->V[(opcode & 0x0F00) >> 8];
                    break;
                // Fx29 - LD F, Vx
                case 0x0029:
                    chip->I = chip->V[(opcode & 0x0F00) >> 8] * 5 + 0x50;
                    break;
                // Fx33 - LD B, Vx
                case 0x0033:
                    int vx = chip->V[(opcode & 0x0F00) >> 8];
                    chip->memory[chip->I] = vx / 100;
                    chip->memory[chip->I + 1] = (vx / 10) % 10;
                    chip->memory[chip->I + 2] = vx % 10;
                    break;
                // Fx55 - LD [I], Vx
                case 0x0055:
                    int x = (opcode & 0x0F00) >> 8;
                    for (int i = 0; i <= x; i++) {
                        chip->memory[i + chip->I] = chip->V[i];
                    }
                    break;
                // Fx65 - LD Vx, [I]
                case 0x0065:
                    x = (opcode & 0x0F00) >> 8;
                    for (int i = 0; i <= x; i++) {
                        chip->V[i] = chip->memory[i + chip->I];
                    }
                    break;
                default:
                    printf("ERROR: Unknown Opcode %X\n", opcode);
                    exit(-1);
                    break;
            }
            break;
        default:
            printf("ERROR: Unknown Opcode: %X\n", opcode);
            exit(-1);
            break;
    }
}

void chip8_decrement_timers(struct chip8 *chip) {
    if (chip->delay_timer > 0) chip->delay_timer--;
    if (chip->sound_timer > 0) chip->sound_timer--;
}

void chip8_updt_key(struct chip8 *chip, int index, bool state) {
    chip->keyboard[index] = state;
}