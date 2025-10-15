#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>

#include "raylib.h"


#define SCREEN_WIDTH 64
#define SCREEN_HEIGHT 32
#define MEMORY_SIZE 4096
#define FONT_START_ADDRESS 0x050
#define PROGRAM_START 0x200


void init(unsigned char *memory, unsigned char *V, uint16_t *PC, uint16_t *I, uint16_t *SP, uint16_t *stack, int *flag, unsigned char *gfx, uint8_t *delay_timer, uint8_t *sound_timer) {
      
    for (int i = 0; i < MEMORY_SIZE; i++) {
        memory[i] = 0;        
    }

    for (int i = 0; i < 16; i++) {
        V[i] = 0;
    }

    *PC = PROGRAM_START;
    *I = 0;
    *SP = 0;
    
    for (int i = 0; i < 16; i++) {
        stack[i] = 0;
    }

    *flag = 0;

    for (int i = 0; i < SCREEN_WIDTH * SCREEN_HEIGHT; i++) {
        gfx[i] = 0;
    }
    
    // Sprite -> some graphic on the screen
    // need to represent chars 0-F, 5 px tall, 4 px wide -> 20 bits
    // use lower 4-bits of 5 bytes, for 16 character fonts we need 80 bytes to store all of em'
    // by convention, font data is stored in 0x050-0x09F
    
    unsigned char font_data[80] = {
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

    for (int i = 0; i < 80; i++) {
        memory[FONT_START_ADDRESS+ i] = font_data[i];
    }

    *delay_timer = 0;
    *sound_timer = 0;
}


void load_ch8(const char *filepath, unsigned char *memory) {
    
    FILE *f = fopen(filepath, "rb");
    if (!f) {
        perror("Failed to open ROM");
        exit(EXIT_FAILURE);
    }

    unsigned char byte;
    size_t count = 0;
    while (fread(&byte, 1, 1, f) == 1) {
        // `byte` now holds the next ROM byte
        printf("Byte %zu: 0x%02X\n", count, byte);

        // e.g. store into memory:
        if (PROGRAM_START + count < MEMORY_SIZE) memory[PROGRAM_START + count] = byte;
        ++count;
    }

    fclose(f);
}


int main(int argc, char *argv[]) {
    
    // 4096 bytes of RAM, 0x000 - 0x1FF is empty, ROM is loaded at 0x200
    unsigned char memory[MEMORY_SIZE];
    
    // 16 8-bit general purpose registers, V0-VE -> General, VF -> Carry
    unsigned char V[16];

    // points at the current instruction in memory, starts at 0x200
    uint16_t PC;
    
    // 16-bit index register, used to point at locations in memory
    uint16_t I;

    // Stack pointer
    uint16_t SP;
    
    // Stack is small, 16 two-byte entries
    uint16_t stack[16];

    // timers
    uint8_t delay_timer = 0;
    uint8_t sound_timer = 0;

    const char *rom_path = (argc > 1) ? argv[1] : "bin/2-ibm-logo.ch8";

    // Display
    unsigned char gfx[SCREEN_WIDTH * SCREEN_HEIGHT];

    int draw_flag = 0;

    init(memory, V, &PC, &I, &SP, stack, &draw_flag, gfx, &delay_timer, &sound_timer);
    load_ch8(rom_path, memory);

    ////////////////////////////////////////////////////////////////
    int scW = 800;
    int scH = 450;

    InitWindow(scW, scH, "CHIP-8");

    // thank you stack overflow: `https://stackoverflow.com/questions/76897194/how-to-display-an-image-from-an-array-using-raylib`
    unsigned char display_gfx[SCREEN_WIDTH * SCREEN_HEIGHT];
    for (int i = 0; i < SCREEN_WIDTH * SCREEN_HEIGHT; i++) display_gfx[i] = (gfx[i] % 2 == 0 ? 0 : 255);

    Image img = {
        .data = display_gfx,
        .width = SCREEN_WIDTH,
        .height = SCREEN_HEIGHT,
        .format = PIXELFORMAT_UNCOMPRESSED_GRAYSCALE,
        .mipmaps = 1
    };

    Texture2D texture = LoadTextureFromImage(img);

    InitAudioDevice();
    Sound beep_sound = LoadSound("assets/beep.wav");

    srand((unsigned) time(NULL));

    const int key_map[16] = {
        KEY_ONE, KEY_TWO, KEY_THREE, KEY_FOUR,
        KEY_Q, KEY_W, KEY_E, KEY_R,
        KEY_A, KEY_S, KEY_D, KEY_F,
        KEY_Z, KEY_X, KEY_C, KEY_V
    };

    bool chip_keys[16] = { false };
    ///////////////////////////////////////////////////////////////////

    SetTargetFPS(60);
    double timer_accumulator = 0.0;
    const double timer_interval = 1.0 / 60.0;

    while (!WindowShouldClose())
    {
        double frame_time = GetFrameTime();
        timer_accumulator += frame_time;
        while (timer_accumulator >= timer_interval) {
            if (delay_timer > 0) delay_timer--;
            if (sound_timer > 0) sound_timer--;
            timer_accumulator -= timer_interval;
        }

        if (sound_timer > 0) {
            if (!IsSoundPlaying(beep_sound)) PlaySound(beep_sound);
        } else if (IsSoundPlaying(beep_sound)) {
            StopSound(beep_sound);
        }

        for (int i = 0; i < 16; i++) {
            chip_keys[i] = IsKeyDown(key_map[i]);
        }

        // fetch 
        uint16_t opcode = (memory[PC] << 8) | memory[PC+1];
        // debugging
	    // printf("0x%04X\n", opcode);

        PC += 2;

        // decode and execute
        unsigned char X = (opcode & 0b0000111100000000) >> 8;   // used to look up some value in registers
        unsigned char Y = (opcode & 0b0000000011110000) >> 4;   // used to look up some value in registers
        unsigned char N = (opcode & 0b0000000000001111);
        unsigned char NN = (opcode & 0b0000000011111111);
        uint16_t NNN = (opcode & 0b0000111111111111);

        // 00E0 (clear screen)
        if (opcode == 0x00E0) {
            for (size_t i = 0; i < SCREEN_WIDTH * SCREEN_HEIGHT; i++) {
                gfx[i] = 0;
            }

            draw_flag = 1;
        }

        // 1NNN (jump)
        else if ((opcode & 0b1111000000000000) == 0x1000) {
            PC = NNN;
        }

        // 6XNN (set register VX)
        else if ((opcode & 0b1111000000000000) == 0x6000) {
            V[X] = NN;
        }

        // 7XNN (add value to register VX)
        else if ((opcode & 0b1111000000000000) == 0x7000) {
            V[X] += NN;
        }

        // ANNN (set index register I)
        else if ((opcode & 0b1111000000000000) == 0xA000) {
            I = NNN;
        }
        
        // DXYN (display/draw)
        else if ((opcode & 0b1111000000000000) == 0xD000) {
            unsigned char V_X = V[X];
            unsigned char V_Y = V[Y];
            V[0xF] = 0;

            for (int i = 0; i < N; i++) {
                
                for (int bit = 0; bit < 8; bit++) {
                    // start from the MSB of memory[I]
                    unsigned char pixel = (memory[I+i] >> (7 - bit)) & 1;
                    
                    // Only process if the sprite pixel is on
                    if (pixel == 1) {
                        int pos_x = (V_X + bit) % SCREEN_WIDTH;
                        int pos_y = (V_Y + i) % SCREEN_HEIGHT;
                        int index = pos_y * SCREEN_WIDTH + pos_x;
                        
                        // Check for collision
                        if (gfx[index] == 1) {
                            V[0xF] = 1;  // Set collision flag
                        }
                        
                        // XOR the pixel
                        gfx[index] ^= 1;
                    }
                }
            }

            draw_flag = 1;
        }

        // 2NNN (subroutine) PUSH TO STACK
        else if ((opcode & 0b1111000000000000) == 0x2000) {
            if (SP >= 16) {
                fprintf(stderr, "Stack overflow when calling subroutine\n");
                UnloadSound(beep_sound);
                CloseAudioDevice();
                UnloadTexture(texture);
                CloseWindow();
                return EXIT_FAILURE;
            }
            stack[SP] = PC;
            SP++;
            PC = NNN;
        }

        // 00EE (subroutine) POP FROM STACK
        else if (opcode == 0x00EE) {
            if (SP == 0) {
                fprintf(stderr, "Stack underflow when returning from subroutine\n");
                UnloadSound(beep_sound);
                CloseAudioDevice();
                UnloadTexture(texture);
                CloseWindow();
                return EXIT_FAILURE;
            }
            SP--;
            PC = stack[SP];
            stack[SP] = 0;
        }

        // 3XNN: skip conditionally
        else if ((opcode & 0b1111000000000000) == 0x3000) {
            if (V[X] == NN) {
                PC += 2;
            }
        }

        // 4XNN: skip conditionally
        else if ((opcode & 0b1111000000000000) == 0x4000) {
            if (V[X] != NN) {
                PC += 2;
            }
        }

        // 5XY0: skip conditionally
        else if ((opcode & 0b1111000000001111) == 0x5000) {
            if (V[X] == V[Y]) {
                PC += 2;
            }
        }

        // 9XY0: skip conditionally
        else if ((opcode & 0b1111000000001111) == 0x9000) {
            if (V[X] != V[Y]) {
                PC += 2;
            }
        }

        // 8XY0: set
        else if ((opcode & 0b1111000000001111) == 0x8000) {
            V[X] = V[Y];
        }

        // 8XY1: OR
        else if ((opcode & 0b1111000000001111) == 0x8001) {
            V[X] = V[X] | V[Y];
        }

        // 8XY2: AND
        else if ((opcode & 0b1111000000001111) == 0x8002) {
            V[X] = V[X] & V[Y];
        }

        // 8XY3: XOR
        else if ((opcode & 0b1111000000001111) == 0x8003) {
            V[X] = V[X] ^ V[Y];
        }

        // 8XY4: ADD
        else if ((opcode & 0b1111000000001111) == 0x8004) {
            uint16_t sum = V[X] + V[Y];
            V[0xF] = (sum > 0xFF) ? 1 : 0;
            V[X] = sum & 0xFF;
        }

        // 8XY5: Subtract
        else if ((opcode & 0b1111000000001111) == 0x8005) {
            V[0xF] = (V[X] >= V[Y]) ? 1 : 0;
            V[X] = (V[X] - V[Y]) & 0xFF;
        }

        // 8XY7: Subtract
        else if ((opcode & 0b1111000000001111) == 0x8007) {
            V[0xF] = (V[Y] >= V[X]) ? 1 : 0;
            V[X] = (V[Y] - V[X]) & 0xFF;
        }

        // 8XY6: shift right
        else if ((opcode & 0b1111000000001111) == 0x8006) {
            V[X] = V[Y];
            V[0xF] = V[X] & 1;
            V[X] = V[X] >> 1;
        }

        // 8XYE: shift left
        else if ((opcode & 0b1111000000001111) == 0x800E) {
            V[X] = V[Y];
            V[0xF] = V[X] & 1;
            V[X] = V[X] << 1;
        }

        // BNNN: jump with offset
        else if ((opcode & 0b1111000000000000) == 0xB000) {
            PC = NNN + V[0];
        }

        // CXNN: Random
        else if ((opcode & 0b1111000000000000) == 0xC000) {
            V[X] = (rand() % 256) & NN;
        }

        // EX9E: skip if key stored in VX is pressed
        else if ((opcode & 0b1111000011111111) == 0xE09E) {
            uint8_t key = V[X] & 0x0F;
            if (chip_keys[key]) {
                PC += 2;
            }
        }

        // EXA1: skip if key stored in VX is not pressed
        else if ((opcode & 0b1111000011111111) == 0xE0A1) {
            uint8_t key = V[X] & 0x0F;
            if (!chip_keys[key]) {
                PC += 2;
            }
        }

        // FX07: load delay timer into VX
        else if ((opcode & 0b1111000011111111) == 0xF007) {
            V[X] = delay_timer;
        }

        // FX15: set delay timer from VX
        else if ((opcode & 0b1111000011111111) == 0xF015) {
            delay_timer = V[X];
        }

        // FX18: set sound timer from VX
        else if ((opcode & 0b1111000011111111) == 0xF018) {
            sound_timer = V[X];
        }

        // FX1E: add VX to I with carry flag
        else if ((opcode & 0b1111000011111111) == 0xF01E) {
            uint16_t result = I + V[X];
            V[0xF] = (result > 0x0FFF) ? 1 : 0;
            I = result & 0x0FFF;
        }

        // FX0A: wait for key press and store
        else if ((opcode & 0b1111000011111111) == 0xF00A) {
            bool key_pressed = false;
            for (int i = 0; i < 16; i++) {
                if (IsKeyPressed(key_map[i])) {
                    V[X] = (unsigned char)i;
                    key_pressed = true;
                    break;
                }
            }
            if (!key_pressed) {
                PC -= 2;
            }
        }

        // FX29: set I to sprite location for digit in VX
        else if ((opcode & 0b1111000011111111) == 0xF029) {
            unsigned char digit = V[X] & 0x0F;
            I = FONT_START_ADDRESS + (digit * 5);
        }

        // FX33: store BCD of VX in memory
        else if ((opcode & 0b1111000011111111) == 0xF033) {
            unsigned char value = V[X];
            if (I + 2 < MEMORY_SIZE) {
                memory[I] = value / 100;
                memory[I + 1] = (value / 10) % 10;
                memory[I + 2] = value % 10;
            }
        }

        // FX55: store registers V0 through VX in memory starting at I
        else if ((opcode & 0b1111000011111111) == 0xF055) {
            if (I + X < MEMORY_SIZE) {
                for (int i = 0; i <= X; i++) {
                    memory[I + i] = V[i];
                }
                I += (uint16_t)(X + 1);
            }
        }

        // FX65: read registers V0 through VX from memory starting at I
        else if ((opcode & 0b1111000011111111) == 0xF065) {
            if (I + X < MEMORY_SIZE) {
                for (int i = 0; i <= X; i++) {
                    V[i] = memory[I + i];
                }
                I += (uint16_t)(X + 1);
            }
        }

        if (draw_flag) {
            for (int i = 0; i < SCREEN_WIDTH * SCREEN_HEIGHT; i++) display_gfx[i] = (gfx[i] % 2 == 0 ? 0 : 255);
            UpdateTexture(texture, display_gfx);
            draw_flag = 0;
        }
    
        // Draw, rectangles and drawtexturepro can be used to scale up the previous texture
        BeginDrawing();
        ClearBackground(BLACK);
        Rectangle src = { 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT };
        Rectangle dest = {
            0,
            0,
            scW,
            scH,
        };
        Vector2 origin = { 0, 0 };
        DrawTexturePro(texture, src, dest, origin, 0.0f, WHITE);
        EndDrawing();
    }

    UnloadSound(beep_sound);
    CloseAudioDevice();
    UnloadTexture(texture);
    CloseWindow();

    return 0;
}
