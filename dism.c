
#include <stdatomic.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/resource.h>

void print_byte(uint8_t byte);
size_t mov_register_to_register(int current_byte, uint8_t* buf, char* command_buf);
size_t mov_immediate_to_register(int current_byte, uint8_t* buf, char* command_buf);

typedef size_t (*HANDLE_PTR)(int, uint8_t*, char*);

const char* SHORT_REG[0x8] = {"al", "cl", "dl", "bl", "ah", "ch", "dh", "bh"};
const char* LONG_REG[0x8] = {"ax", "cx", "dx", "bx", "sp", "bp", "si", "di"};
const char* MEM_REG[0x8] = {"[bx + si]", "[bx + di]", "[bp + si]", "[bp + di]", "[si]", "[di]", "[bp]", "[bx]"};
const char* MEM_DISPL_REG[0x8] = {"[bx + si + %d]", "[bx + di + %d]", "[bp + si + %d]", "[bp + di + %d]",
                                  "[si + %d]",      "[di + %d]",      "[bp + %d]",      "[bx + %d]"};

int main(int argc, char* argv[]) {
    if (argc == 1) {
        printf("Binary file has to be specified \n");
        return 1;
    }
    HANDLE_PTR opcodes[0xff + 1] = {};

    opcodes[0x88] = &mov_register_to_register;
    opcodes[0x89] = &mov_register_to_register;
    opcodes[0x8a] = &mov_register_to_register;
    opcodes[0x8b] = &mov_register_to_register;
    opcodes[0x8c] = &mov_register_to_register;

    opcodes[0xb0] = &mov_immediate_to_register;
    opcodes[0xb1] = &mov_immediate_to_register;
    opcodes[0xb2] = &mov_immediate_to_register;
    opcodes[0xb3] = &mov_immediate_to_register;
    opcodes[0xb4] = &mov_immediate_to_register;
    opcodes[0xb5] = &mov_immediate_to_register;
    opcodes[0xb6] = &mov_immediate_to_register;
    opcodes[0xb7] = &mov_immediate_to_register;
    opcodes[0xb8] = &mov_immediate_to_register;
    opcodes[0xb9] = &mov_immediate_to_register;
    opcodes[0xba] = &mov_immediate_to_register;
    opcodes[0xbb] = &mov_immediate_to_register;
    opcodes[0xbc] = &mov_immediate_to_register;
    opcodes[0xbd] = &mov_immediate_to_register;
    opcodes[0xbe] = &mov_immediate_to_register;
    opcodes[0xbf] = &mov_immediate_to_register;

    FILE* handle = fopen(argv[1], "rb");
    if (handle == NULL) {
        printf("Failed to open input file");
        return 1;
    }

    uint8_t buf[1024];
    const size_t bytes_read = fread(buf, 1, 1024, handle);
    char command_stream[1024][30] = {{}};
    uint32_t stream_i = 0;

    for (int i = 0; i < bytes_read; ++i) {
        if (opcodes[buf[i]]) {
            size_t offset = (opcodes[buf[i]])(i, buf, command_stream[stream_i++]);
            i += offset;
        }
    }
    for (uint32_t i = 0; i < stream_i; ++i) {
        printf("%s \n", command_stream[i]);
    }
    return 0;
}

size_t mov_register_to_register(int current_byte, uint8_t* buf, char* command_buf) {
    size_t offset = 1;
    uint8_t first_byte, second_byte, dir, wide, rm, reg, mode;
    const char* source;
    const char* dst;
    uint8_t mode_mask = 0b11;
    uint8_t reg_mask = 0b111;

    first_byte = buf[current_byte];
    second_byte = buf[current_byte + 1];

    wide = first_byte & 1;
    dir = (first_byte >> 1) & 1;
    reg = second_byte >> 3 & reg_mask;
    rm = second_byte & reg_mask;
    mode = second_byte >> 6 & mode_mask;

    // register to register move
    if (mode == 0b11) {
        uint8_t source_reg = dir ? reg : rm;
        uint8_t dest_reg = dir ? rm : reg;
        source = wide ? LONG_REG[source_reg] : SHORT_REG[source_reg];
        dst = wide ? LONG_REG[dest_reg] : SHORT_REG[dest_reg];
    }
    // register to memory
    if (mode == 0) {
        if (rm == 0b110) {
            // 16 bit displacement special case
            offset += 2;
        }
        source = dir ? (wide ? LONG_REG[reg] : SHORT_REG[reg]) : MEM_REG[rm];
        dst = dir ? MEM_REG[rm] : (wide ? LONG_REG[reg] : SHORT_REG[reg]);
    }
    // memory 8bit displacement
    if (mode == 0b01) {
        offset += 1;
        uint8_t displacement = buf[current_byte + 2];
        char displ_reg[20];
        if (displacement == 0) {
            strcpy(displ_reg, MEM_REG[rm]);
        } else {
            sprintf(displ_reg, MEM_DISPL_REG[rm], displacement);
        }
        source = dir ? (wide ? LONG_REG[reg] : SHORT_REG[reg]) : displ_reg;
        dst = dir ? displ_reg : (wide ? LONG_REG[reg] : SHORT_REG[reg]);
    }
    // memory 16 bit displacement
    if (mode == 0b10) {
        offset += 2;
        uint16_t displacement = (buf[current_byte + 2] << 8) + buf[current_byte + 1];
        char displ_reg[20];
        if (displacement == 0) {
            strcpy(displ_reg, MEM_REG[rm]);
        } else {
            sprintf(displ_reg, MEM_DISPL_REG[rm], displacement);
        }
        source = dir ? (wide ? LONG_REG[reg] : SHORT_REG[reg]) : displ_reg;
        dst = dir ? displ_reg : (wide ? LONG_REG[reg] : SHORT_REG[reg]);
    }
    sprintf(command_buf, "mov %s, %s", source, dst);
    return offset;
}

size_t mov_immediate_to_register(int current_byte, uint8_t* buf, char* command_buf) {
    size_t offset = 0;
    uint8_t reg_mask = 0b111;
    uint8_t w = buf[current_byte] >> 3 & 1;
    uint8_t reg = buf[current_byte] & reg_mask;
    uint16_t data;
    if (w) {
        data = (buf[current_byte + 2] << 8) + buf[current_byte + 1];
        offset += 2;
    } else {
        data = buf[current_byte + 1];
        offset += 1;
    }
    sprintf(command_buf, "mov %s, %d", w ? LONG_REG[reg] : SHORT_REG[reg], data);
    return offset;
}

void print_byte(uint8_t byte) {
    for (int i = 7; i >= 0; --i) {
        printf("%d", byte >> i & 1);
    }
    printf("\n");
}
