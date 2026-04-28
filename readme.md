# Chip8 Emulator
This is my chip8 emulator written in C. It has all opcodes for the original chip8 implemented.
Audio and visuals are implemented with SDL3.

---

## Compiling
```bash
	gcc src\main.c src\chip8.c -o main -L<SDL_LIB_DIR> -I<SDL_INCLUDES_DIR> -lSDL3
```

---

# Resources Used
http://devernay.free.fr/hacks/chip8/C8TECH10.HTM

https://tobiasvl.github.io/blog/write-a-chip-8-emulator/

---

# Things I Learned
uint8_t over char:
- char is not guaranteed to be 8 bits - 1 byte long whereas uint8_t is guaranteed
- int8_t is the signed version

presetting massive amounts of data in RAM:
- Define a const array or something similar then use memcpy at startup to copy into ram.
- Manual is sad af

Updating on hertz:
- To decrement flags on 60HZ independent of the instruction cycle, keep track of the time in ms. See how much time has elapsed. Depending on that updt variable as necessary

Switching on opcode:
- They don't manually create variables for each part of the instruction. Instead they evaluate it with bitwise &(bitmasksing) and then switch case on the value

It's ok if ROM is looping:
- Sometimes draws something on screen and to make it stay, it runs an infinite loop

Underflow on max/min:
- Subtract then took the min but what would not make 0 as it underflows to 255 before it could be detected:
- ```c
chip->delay_timer = max(0, chip->delay_timer - 1);
  ```
  