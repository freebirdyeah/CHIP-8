#include <stdio.h>
#include <stdint.h>
#include "raylib.h"
#include <string.h>


#define SCREEN_WIDTH 64
#define SCREEN_HEIGHT 32
#define MEMORY_SIZE 4096
#define FONT_START_ADDRESS 0x050
#define PROGRAM_START 0x200


void init(unsigned char *memory, unsigned char *V, uint16_t *PC, uint16_t *I, uint16_t *SP, uint16_t *stack, int *flag, unsigned char *gfx) {
      
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
}


void load_ch8(char filepath[], unsigned char *memory) {
    
    FILE *f = fopen(filepath, "rb");
    if (!f) {
        perror("Failed to open ROM");
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


int main() {
    
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

    // Display
    unsigned char gfx[SCREEN_WIDTH * SCREEN_HEIGHT];

    int draw_flag = 0;

    init(memory, V, &PC, &I, &SP, stack, &draw_flag, gfx);
    load_ch8("bin/2-ibm-logo.ch8", memory);

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
    ///////////////////////////////////////////////////////////////////

    SetTargetFPS(60);
    
    while (!WindowShouldClose())
    {

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
}