#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <cstdint>
#include <fstream>
#include <random>
#include <cmath>

#include <SDL2/SDL.h>

bool beep = false;

struct CPU
{
    uint8_t memory[4096] = {}; // This is where all data is stored sperate from the registers

    uint8_t V[16] = {}; // The 16 registers for storing data

    uint16_t PC = 0x200;    // Program counter will keep track of what instruction is currently running always starts at 0x200 for the chip-8
                            // The PC just keeps track of where execution should go next.
    uint16_t I = 0; // This can store the memory address of data

    uint16_t stack[16] = {}; // The stack holds the return addresses for subrutines
    uint8_t SP = 0; // Will keep track of where in position the stack is going to run next

    uint8_t delayTimer = 0; // This will be the timer of the cpu
    uint8_t soundTimer = 0; // This will be the timer for the sound
    // Both will be at 60hz and count down

    bool keys[16] = {}; // will keep track of key input

    bool display[64][32] = {}; // this represents the display for the CHIP-8 emulator
};

const uint8_t fontset[80] =
{
    // 0
    0xF0, 0x90, 0x90, 0x90, 0xF0,

    // 1
    0x20, 0x60, 0x20, 0x20, 0x70,

    // 2
    0xF0, 0x10, 0xF0, 0x80, 0xF0,

    // 3
    0xF0, 0x10, 0xF0, 0x10, 0xF0,

    // 4
    0x90, 0x90, 0xF0, 0x10, 0x10,

    // 5
    0xF0, 0x80, 0xF0, 0x10, 0xF0,

    // 6
    0xF0, 0x80, 0xF0, 0x90, 0xF0,

    // 7
    0xF0, 0x10, 0x20, 0x40, 0x40,

    // 8
    0xF0, 0x90, 0xF0, 0x90, 0xF0,

    // 9
    0xF0, 0x90, 0xF0, 0x10, 0xF0,

    // A
    0xF0, 0x90, 0xF0, 0x90, 0x90,

    // B
    0xE0, 0x90, 0xE0, 0x90, 0xE0,

    // C
    0xF0, 0x80, 0x80, 0x80, 0xF0,

    // D
    0xE0, 0x90, 0x90, 0x90, 0xE0,

    // E
    0xF0, 0x80, 0xF0, 0x80, 0xF0,

    // F
    0xF0, 0x80, 0xF0, 0x80, 0x80
};
//CPU cpu{}; // Intialize the cpu object
// FIX ROM LOADING ERROR

void loadRomFile(CPU& cpu, const std::string& filename)
{
    std::ifstream file(filename, std::ios::binary);

    if (!file)
    {
        std::cout << "Could not open ROM\n";
        return;
    }

    file.seekg(0, std::ios::end);
    std::size_t size = file.tellg();
    file.seekg(0, std::ios::beg);

    file.read(reinterpret_cast<char*>(&cpu.memory[0x200]), size);
}

uint16_t getOpcode(CPU& cpu)
{
    uint16_t opcode =
        (cpu.memory[cpu.PC] << 8) | cpu.memory[cpu.PC + 1];  // Fetches the curren opcode from memory

    cpu.PC += 2; // Moves the program counter to the next instruction
    return opcode;
}

void executeOpcode(CPU& cpu, uint16_t opcode)
{
    // Extract information from the opcode
    // F = 1111 acts as mask to use AND bitwise and get what you want
    uint8_t x = (opcode & 0x0F00) >> 8; // Gets the 2nd nibble in the opcode example: 0000 |here| 0000 0000
    uint8_t y = (opcode & 0x00F0) >> 4; // Gets the 3rd nibble in the opcode example: 0000 0000 |here| 0000
    uint8_t n = opcode & 0x000F;        // Gets the 4th or last nibble in the opcode example: 0000 0000 0000 |here| 
    uint8_t nn = opcode & 0x00FF;       // Gets the last 2 nibbles in the opcode example: 0000 0000 |here| |here|
    uint16_t nnn = opcode & 0x0FFF;     //Gets the last 3 nibbles in the opcode example: 0000 |here| |here| |here|

    switch (opcode & 0xF000) // This gets the first nibble of the opcode to determine which family the instruction is in
    {
        case 0x0000:
        {
            switch (opcode)
            {
                case 0x00E0:
                {
                    // 00E0 - CLS
                    // Clear the display.
                    
                    // will implement when we get the display thing working

                    for(int y = 0; y < 32; y++)
                    {
                        for(int x = 0; x < 64; x++)
                        {
                            cpu.display[x][y] = false; // runs through all pixels in buffer and sets to false
                        }
                    }

                }
                break;

                case 0x00EE:
                {
                    // 00EE - RET
                    // Return from a subroutine.
                    // The interpreter sets the program counter to the address at the top of the stack, then
                    // subtracts 1 from the stack pointer.
                    cpu.PC = cpu.stack[--cpu.SP]; // stack pointer is supposed to always point to top of stack
                    

                }
                break;
            }
            // std::cout << "0 family";

            break;
        }
        case 0x1000:
        {
            // std::cout << "1 family";
            // 1nnn - JP addr
            // Jump to location nnn.
            // The interpreter sets the program counter to nnn
            cpu.PC = nnn;
            break;
        }
        case 0x2000:
        {
            // std::cout << "2 family";
            // 2nnn - CALL addr
            //The interpreter increments the stack pointer, then puts the current PC on the top of
            //the stack. The PC is then set to nnn
            cpu.stack[cpu.SP] = cpu.PC;
            cpu.SP++;
            cpu.PC = nnn;
            break;
        }    
        case 0x3000:
        {
            // std::cout << "3 family";

            // 3xnn - SE Vx, byte
            // Skip next instruction if Vx = nn.
            // The interpreter compares register Vx to nn, and if they are equal, increments the
            // program counter by 2
            if (cpu.V[x] == nn) // Compares if Vx and nn are equal
            {
                cpu.PC += 2; // If they are equal it skips the next instruction by increasing the program counter by 2 which skips one instruction
            }
            break; 
        }
        case 0x4000:
        {
            // std::cout << "4 family";

            // 4xnn - SNE Vx, byte
            // Skip next instruction if Vx != nn.
            // The interpreter compares register Vx to nn, and if they are not equal, increments the
            // program counter by 2

            if (cpu.V[x] != nn)
            {
                cpu.PC += 2;
            }
            break;
        }
        case 0x5000:
        {
            // std::cout << "5 family";

            // 5xy0 - SE Vx, Vy
            // Skip next instruction if Vx = Vy.
            // The interpreter compares register Vx to register Vy, and if they are equal, increments
            // the program counter by 2

            if (cpu.V[x] == cpu.V[y])
            {
                cpu.PC += 2;
            }
            break;
        }
        case 0x6000:
        {
            // std::cout << "6 family";

            // 6xnn - LD Vx, byte
            // Set Vx = nn.
            // The interpreter puts the value kk into register Vx

            cpu.V[x] = nn;

            break;
        }
        case 0x7000:
        {
            // std::cout << "7 family";

            // 7xnn - ADD Vx, byte
            // Set Vx = Vx + nn.
            // Adds the value nn to the value of register Vx, then stores the result in Vx

            cpu.V[x] = cpu.V[x] + nn;
            break;
        }
        case 0x8000:
        {
            // std::cout << "8 family";

            switch (opcode & 0x000F)
            {
                case 0x0:
                {
                    // 8xy0 - LD Vx, Vy
                    // Set Vx = Vy.
                    // Stores the value of register Vy in register Vx
                    cpu.V[x] = cpu.V[y];
                    break;
                }

                case 0x1:
                {
                    // 8xy1 - OR Vx, Vy
                    // Set Vx = Vx OR Vy.
                    // Performs a bitwise OR on the values of Vx and Vy, then stores the result in Vx. A
                    // bitwise OR compares the corrseponding bits from two values, and if either bit is 1,
                    // then the same bit in the result is also 1. Otherwise, it is 0 
                    cpu.V[x] |= cpu.V[y];
                    break;
                }

                case 0x2:
                {
                    // 8xy2 - AND Vx, Vy
                    // Set Vx = Vx AND Vy.
                    // Performs a bitwise AND on the values of Vx and Vy, then stores the result in Vx. A
                    // bitwise AND compares the corrseponding bits from two values, and if both bits are 1,
                    // then the same bit in the result is also 1. Otherwise, it is 0

                    cpu.V[x] &= cpu.V[y];
                    break;
                }

                case 0x3:
                {
                    // 8xy3 - XOR Vx, Vy
                    // Set Vx = Vx XOR Vy.
                    // Performs a bitwise exclusive OR on the values of Vx and Vy, then stores the result in
                    // Vx. An exclusive OR compares the corrseponding bits from two values, and if the bits
                    // are not both the same, then the corresponding bit in the result is set to 1.
                    // Otherwise, it is 0
                    cpu.V[x] ^= cpu.V[y];
                    break;
                }

                case 0x4:
                {
                    // 8xy4 - ADD Vx, Vy
                    // Set Vx = Vx + Vy, set VF = carry.
                    // The values of Vx and Vy are added together. If the result is greater than 8 bits
                    // (i.e., > 255,) VF is set to 1, otherwise 0. Only the lowest 8 bits of the result are
                    // kept, and stored in Vx
                    
                    uint16_t sum = cpu.V[x] + cpu.V[y];
                    
                    if (sum > 255)
                        cpu.V[0xF] = 1;
                    else 
                        cpu.V[0xF] = 0;

                    cpu.V[x] = sum;
                    break;                    
                }

                case 0x5:
                {
                    // 8xy5 - SUB Vx, Vy
                    // Set Vx = Vx - Vy, set VF = NOT borrow.
                    // If Vx > Vy, then VF is set to 1, otherwise 0. Then Vy is subtracted from Vx, and the
                    // results stored in Vx.

                    if (cpu.V[x] > cpu.V[y])
                        cpu.V[0xF] = 1;
                    else 
                        cpu.V[0xF] = 0;

                    uint16_t sum = cpu.V[x] - cpu.V[y];
                    cpu.V[x] = sum;
                    break;
                }

                case 0x6:
                {
                    // 8xy6 - SHR Vx {, Vy}
                    // Set Vx = Vx SHR 1.
                    // If the least-significant bit of Vx is 1, then VF is set to 1, otherwise 0. Then Vx is
                    // divided by 2
                    if (cpu.V[x] & 0x1)
                    {
                        cpu.V[0xF] = 1;
                    } else
                    {
                        cpu.V[0xF] = 0;
                    }
                    cpu.V[x] /= 2;
                    break;
                }

                case 0x7:
                {
                    // 8xy7 - SUBN Vx, Vy
                    // Set Vx = Vy - Vx, set VF = NOT borrow.
                    // If Vy > Vx, then VF is set to 1, otherwise 0. Then Vx is subtracted from Vy, and the
                    // results stored in Vx.

                    if (cpu.V[y] > cpu.V[x])
                        cpu.V[0xF] = 1;
                    else 
                        cpu.V[0xF] = 0;

                    uint16_t sum = cpu.V[y] - cpu.V[x];
                    cpu.V[x] = sum;
                    break;
                }

                case 0xE:
                {
                    //8xyE - SHL Vx {, Vy}
                    // Set Vx = Vx SHL 1.
                    // If the most-significant bit of Vx is 1, then VF is set to 1, otherwise to 0. Then Vx
                    // is multiplied by 2
                    if (cpu.V[x] & 0x80)
                    {
                        cpu.V[0xF] = 1;
                    } else
                    {
                        cpu.V[0xF] = 0;
                    }
                    cpu.V[x] *= 2;
                    
                    break;
                }
            }
            break;
        }
        
            
        case 0x9000:
        {    
            // std::cout << "9 family";
            switch (opcode & 0x000F)
            {
                case 0x0:
                {
                    // 9xy0 - SNE Vx, Vy
                    // Skip next instruction if Vx != Vy.
                    // The values of Vx and Vy are compared, and if they are not equal, the program counter
                    // is increased by 2

                    if (cpu.V[x] != cpu.V[y])
                        cpu.PC += 2;

                    break;
                }
            }
            break;
        }
        case 0xA000:
        {
            // std::cout << "A family";

            // Annn - LD I, addr
            // Set I = nnn.
            // The value of register I is set to nnn.

            cpu.I = nnn;
            break;
        }
        case 0xB000:
        {
            // std::cout << "B family";
            // Bnnn - JP V0, addr
            // Jump to location nnn + V0.
            // The program counter is set to nnn plus the value of V0

            cpu.PC = nnn + cpu.V[0];
            break;
        }
        case 0xC000:
        {
            // std::cout << "C family";
            // Cxkk - RND Vx, byte
            // Set Vx = random byte AND nn.
            // The interpreter generates a random number from 0 to 255, which is then ANDed with the
            // value nn. The results are stored in Vx. See instruction 8xy2 for more information on
            // AND
            // 1. Obtain a random seed from the hardware
            std::random_device rd;

            // 2. Seed the Mersenne Twister engine
            std::mt19937 gen(rd());

            // 3. Define the range 
            std::uniform_int_distribution<int> distrib(0, 255);

            // 4. Generate and print the random number
            uint8_t random_num = distrib(gen);
            cpu.V[x] = random_num & nn;
            break;
        }

        case 0xD000:
        {
            // std::cout << "D family";

            // Dxyn - DRW Vx, Vy, nibble
            // Display n-byte sprite starting at memory location I at (Vx, Vy), set VF = collision.
            // The interpreter reads n bytes from memory, starting at the address stored in I. These
            // bytes are then displayed as sprites on screen at coordinates (Vx, Vy). Sprites are
            // XORed onto the existing screen. If this causes any pixels to be erased, VF is set to
            // 1, otherwise it is set to 0. If the sprite is positioned so part of it is outside the
            // coordinates of the display, it wraps around to the opposite side of the screen. See
            // instruction 8xy3 for more information on XOR, and section 2.4, Display, for more
            // information on the Chip-8 screen and sprites.

            // read each byte of sprite
            for (int row = 0; row < n; row++)
            {
                uint8_t sprite_byte = cpu.memory[cpu.I + row]; // one row

                for (int col = 0; col < 8; col++) // one colum
                {
                    if (sprite_byte & (0x80 >> col))
                    {
                        int scrx = cpu.V[x] + col;
                        int scry = cpu.V[y] + row;

                        if (cpu.display[scrx][scry])
                            cpu.V[0xF] = 1;

                        cpu.display[scrx][scry] ^= true; //XORs the bit
                    }
                }
            }

            break;
        }
        case 0xE000:
        {
            // std::cout << "E family";

            switch (opcode & 0x00FF)
            {
                case 0x009E:
                {
                    // Ex9E - SKP Vx
                    // Skip next instruction if key with the value of Vx is pressed.
                    // Checks the keyboard, and if the key corresponding to the value of Vx is currently in
                    // the down position, PC is increased by 2

                    if (cpu.keys[cpu.V[x]] == true)
                    {
                        cpu.PC += 2;
                    }

                    break;
                }
                case 0x00A1:
                {
                    // ExA1 - SKNP Vx
                    // Skip next instruction if key with the value of Vx is not pressed.
                    // Checks the keyboard, and if the key corresponding to the value of Vx is currently in
                    // the up position, PC is increased by 2
                    if (cpu.keys[cpu.V[x]] == false)
                    {
                        cpu.PC += 2;
                    }

                    break;
                }
            }
            break;
        }
        case 0xF000:
        {
            // std::cout << "F family";

            switch (opcode & 0x00FF)
            {
                case 0x0007:
                {
                    // Fx07 - LD Vx, DT
                    // Set Vx = delay timer value.
                    // The value of DT is placed into Vx

                    cpu.V[x] = cpu.delayTimer;
                    break;
                }

                case 0x000A:
                {
                    // Fx0A - LD Vx, K
                    // Wait for a key press, store the value of the key in Vx.
                    // All execution stops until a key is pressed, then the value of that key is stored in
                    // Vx.

                    bool keyPressed = false;

                    for (int k = 0; k < 16; k++)
                    {
                        if (cpu.keys[k])
                        {
                            cpu.V[x] = k;
                            keyPressed = true;
                            break;
                        }
                    }

                    if (!keyPressed)
                    {
                        cpu.PC -= 2;
                    }
                    break;
                }

                case 0x0015:
                {
                    // Fx15 - LD DT, Vx
                    // Set delay timer = Vx.
                    // DT is set equal to the value of Vx.

                    cpu.delayTimer = cpu.V[x];
                    break;
                }

                case 0x0018:
                {
                    // Fx18 - LD ST, Vx
                    // Set sound timer = Vx.
                    // ST is set equal to the value of Vx.

                    cpu.soundTimer = cpu.V[x];
                    break;
                }

                case 0x001E:
                {
                    // Fx1E - ADD I, Vx
                    // Set I = I + Vx.
                    // The values of I and Vx are added, and the results are stored in I

                    cpu.I = cpu.I + cpu.V[x];
                    break;
                }

                case 0x0029:
                {
                    // Fx29 - LD F, Vx
                    // Set I = location of sprite for digit Vx.
                    // The value of I is set to the location for the hexadecimal sprite corresponding to the
                    // value of Vx. See section 2.4, Display, for more information on the Chip-8 hexadecimal
                    // font.
                    cpu.I = 0x50 + (cpu.V[x] * 5);

                    break;
                }

                case 0x0033:
                {
                    // Fx33 - LD B, Vx
                    // Store BCD representation of Vx in memory locations I, I+1, and I+2.
                    // The interpreter takes the decimal value of Vx, and places the hundreds digit in memory
                    // at location in I, the tens digit at location I+1, and the ones digit at location I+2.

                    cpu.memory[cpu.I]     = cpu.V[x] / 100; // if Vx was = 123 it would become 1, 2, 3
                    cpu.memory[cpu.I + 1] = (cpu.V[x] / 10) % 10;
                    cpu.memory[cpu.I + 2] = cpu.V[x] % 10;
                    break;
                }

                case 0x0055:
                {
                    // Fx55 - LD [I], Vx
                    // Store registers V0 through Vx in memory starting at location I.
                    // The interpreter copies the values of registers V0 through Vx into memory, starting at
                    // the address in I

                    for (int m = 0; m <= x; m++) // loops through all registers
                    {
                        cpu.memory[cpu.I + m] = cpu.V[m];
                        
                    }
                    break;
                }

                case 0x0065:
                {
                    // Fx65 - LD Vx, [I]
                    // Read registers V0 through Vx from memory starting at location I.
                    // The interpreter reads values from memory starting at location I into registers V0
                    // through Vx.
                    for (int m = 0; m <= x; m++) // loops through all registers
                    {
                        cpu.V[m] = cpu.memory[cpu.I + m];
                        
                    }
                    break;
                }
            }
            break;
        }
    }
}


void audioCallback(void* userdata, Uint8* stream, int len)
{
    static float phase = 0.0f;

    float* buffer = (float*)stream;
    int samples = len / sizeof(float);

    for (int i = 0; i < samples; i++)
    {
        if (beep) // checks if sound timer is >0
        {
            buffer[i] = (phase < 0.5f) ? 0.1f : -0.1f;

            phase += 440.0f / 48000.0f; // generate a 440hz wave

            if (phase >= 1.0f)
                phase -= 1.0f;
        }
        else
        {
            buffer[i] = 0.0f;
        }
    }
}


int main(int argv, char* args[])
{
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) != 0)
    {
        printf("SDL_Init Error: %\n", SDL_GetError());
        return 1;
    }

    SDL_Window* window = SDL_CreateWindow(
        "Chip-8 Emulator",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        640 * 2,
        320 * 2,
        SDL_WINDOW_SHOWN
    );

    if (window == nullptr)
    {
        std::cout << SDL_GetError() << "\n";

        SDL_Quit();
        return 1;
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

    if (window == nullptr)
    {
        std::cout << SDL_GetError() << "\n";

        SDL_Quit();
        return 1;
    }
    
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255); 
    SDL_RenderClear(renderer); // clear window

    SDL_AudioSpec spec{}; // audio
    spec.freq = 48000;
    spec.format = AUDIO_F32SYS;
    spec.channels = 1;
    spec.samples = 1024;
    spec.callback = audioCallback;

    SDL_OpenAudio(&spec, nullptr);
    SDL_PauseAudio(0); // Start audio

    CPU cpu{}; // initialize cpu object

    // load sprites into memory

    for (int i = 0; i < 80; i++)
    {
        cpu.memory[0x50 + i] = fontset[i];
    }
    std::string romPath = "ROM programs\\";
    romPath += args[1];
    //std::string romFile = args[1];
    // load file into memory
    loadRomFile(cpu, romPath);

    // need to draw pixels based on the cpu.display

    //------------------------MAIN LOOP--------------------------------
    // TO ADD: update timers

    bool running = true;
    uint32_t lastTimerUpdate = SDL_GetTicks();
    SDL_Event event;
    
    while (running)
    {

        // windows error handling

        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_QUIT)
            {
                running = false;
            }

            // keyboard input

            int keypressed = event.key.keysym.sym; // gets key pressed

            if (event.type == SDL_KEYDOWN)
            {
                
                // key mapping i want these keys
                // 1 2 3 4   this corosponds to the 
                // q w e r   real chip 8 keys
                // a s d f 
                // z x c v 

                // chip 8 original
                // 1 2 3 c 
                // 4 5 6 d 
                // 7 8 9 e 
                // a 0 b f 
                // it would be kind of cool to have the original 
                // keyboard for this

                // key mapping:

                // 1 key: 49   		49  50  51  52      this will be the key layout
                // 2 key: 50    		
                // 3 key: 51   		113 119 101 114
                // 4 key: 52
                //                  97  115 100 102
                // Q key: 113  
                // W key: 119  		122 120 99  118
                // E key: 101  
                // R key: 114  

                // A key: 97  
                // S key: 115  
                // D key: 100  
                // F key: 102  

                // Z key: 122  
                // X key: 120  
                // C key: 99  
                // V key: 118
                
                switch (keypressed)
                {
                    case 49: // key 1 pressed
                        cpu.keys[1] = true;
                        break;
                    
                    case 50: // key 2 pressed
                        cpu.keys[2] = true;
                        break;

                    case 51: // key 3 pressed
                        cpu.keys[3] = true;
                        break;

                    case 52: // key 4 pressed
                        cpu.keys[0xC] = true;
                        break;

                    case 113: // key Q pressed
                        cpu.keys[4] = true;
                        break;

                    case 119: // key W pressed
                        cpu.keys[5] = true;
                        break;
                    
                    case 101: // key E pressed
                        cpu.keys[6] = true;
                        break;

                    case 114: // key R pressed
                        cpu.keys[0xD] = true;
                        break;

                    case 97: // key A pressed
                        cpu.keys[7] = true;
                        break;

                    case 115: // key S pressed
                        cpu.keys[8] = true;
                        break;

                    case 100: // key D pressed
                        cpu.keys[9] = true;
                        break;

                    case 102: // key F pressed
                        cpu.keys[0xE] = true;
                        break;

                    case 122: // key Z pressed
                        cpu.keys[0xA] = true;
                        break;

                    case 120: // key X pressed
                        cpu.keys[0] = true;
                        break;

                    case 99: // key C pressed
                        cpu.keys[0xB] = true;
                        break;

                    case 118: // key V pressed
                        cpu.keys[0xF] = true;
                        break;
                }   
                //std::cout << cpu.keys[1] << "\n";
            } else if (event.type == SDL_KEYUP)
            {
                switch (keypressed)
                {
                    case 49: // key 1 unpressed
                        cpu.keys[1] = false;
                        break;
                    
                    case 50: // key 2 unpressed
                        cpu.keys[2] = false;
                        break;

                    case 51: // key 3 unpressed
                        cpu.keys[3] = false;
                        break;

                    case 52: // key 4 unpressed
                        cpu.keys[0xC] = false;
                        break;

                    case 113: // key Q unpressed
                        cpu.keys[4] = false;
                        break;

                    case 119: // key W unpressed
                        cpu.keys[5] = false;
                        break;
                    
                    case 101: // key E unpressed
                        cpu.keys[6] = false;
                        break;

                    case 114: // key R unpressed
                        cpu.keys[0xD] = false;
                        break;

                    case 97: // key A unpressed
                        cpu.keys[7] = false;
                        break;

                    case 115: // key S unpressed
                        cpu.keys[8] = false;
                        break;

                    case 100: // key D unpressed
                        cpu.keys[9] = false;
                        break;

                    case 102: // key F unpressed
                        cpu.keys[0xE] = false;
                        break;

                    case 122: // key Z unpressed
                        cpu.keys[0xA] = false;
                        break;

                    case 120: // key X unpressed
                        cpu.keys[0] = false;
                        break;

                    case 99: // key C unpressed
                        cpu.keys[0xB] = false;
                        break;

                    case 118: // key V unpressed
                        cpu.keys[0xF] = false;
                        break;
                }
            }
            //std::cout << cpu.keys[1] << "\n";
        }
        // std::cout << std::hex
        //   << "PC=" << cpu.PC
        //   << " OP=" << getOpcode(cpu)
        //   << " I=" << cpu.I
        //   << '\n';
        // execute opcode 10x per frame
        for (int x = 0; x < 10; x++)
        {
            executeOpcode(cpu, getOpcode(cpu));
        }
        // update timers 
        if (SDL_GetTicks() - lastTimerUpdate >= 1000 / 60)
        {

            lastTimerUpdate = SDL_GetTicks();

            if (cpu.delayTimer > 0) // if the  timer is greater than 0 it is lowered by 1 which will be about 1 per second
            {
                cpu.delayTimer -= 1;
            }

            if (cpu.soundTimer > 0)
            {
                beep = true;
                cpu.soundTimer -= 1;
            }else
            {
                beep = false;
            }


        }
        // draw display

        for(int y = 0; y < 32; y++)
        {
            for(int x = 0; x < 64; x++)
            {               
                SDL_Rect pixel = { // create a scaled pixel
                    x * 20, // x
                    y * 20, //y
                    20, // width
                    20, // height
                };
                
                // cpu.display[x][y] = ((x + y) % 2 == 0); // makes every other pixel = true 
                
                if (cpu.display[x][y] == true) // goes through whole display if true will draw a white square
                {
                    SDL_SetRenderDrawColor(renderer, 255, 150, 55, 255); // can change color later
                    SDL_RenderFillRect(renderer, &pixel);               
                }
                else if (cpu.display[x][y] == false) // if false draws black one dont really need elif
                {
                    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
                    SDL_RenderFillRect(renderer, &pixel);
                }
            }
        }
        // present display
        SDL_RenderPresent(renderer);
        SDL_Delay(16); // 60fps
    }
    SDL_Quit();
    return 0;
}