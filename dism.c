
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/resource.h>

void print_byte(uint8_t byte) {
    for (int i = 7; i >= 0; --i) {
        printf("%d", byte >> i & 1);
    }
    printf("\n");
}

int main(int argc, char* argv[]) {
    if (argc == 1) {
        printf("Binary file has to be specified \n");
        return 1;
    }
    char* opcodes[0xff] = {};
    char* short_reg[0x8] = {"al", "cl", "dl", "bl", "ah", "ch", "dh", "bh"};
    char* long_reg[0x8] = {"ax", "cx", "dx", "bx", "sp", "bp", "si", "di"};
    char* mem_reg[0x8] = {"[bx + si]", "[bx + di]", "[bp + si]", "[bp + di]", "[si]", "[di]", "[bp]", "[bx]"};
    char* mem_displ_reg[0x8] = {"[bx + si + %d]", "[bx + di + %d]", "[bp + si + %d]", "[bp + di + %d]",
                                "[si + %d]",      "[di + %d]",      "[bp + %d]",      "[bx + %d]"};

    opcodes[0b100010] = "mov";
    opcodes[0b1100011] = "mov";
    opcodes[0b1011] = "mov";

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
        uint8_t opcode = buf[i] >> 2;
        if (opcodes[opcode] != NULL) {
            uint8_t d, w;
            w = buf[i] & 1;
            d = (buf[i] >> 1) & 1;
            i++;

            uint8_t mode_mask = 0b11;
            uint8_t reg_mask = 0b111;

            uint8_t mode = buf[i] >> 6 & mode_mask;
            if (mode == 0b11) {
                // register to register move
                uint8_t source_reg = d ? buf[i] >> 3 & reg_mask : buf[i] & reg_mask;
                uint8_t dest_reg = d ? buf[i] & reg_mask : buf[i] >> 3 & reg_mask;

                sprintf(command_stream[stream_i++], "%s %s, %s", opcodes[opcode],
                        w ? long_reg[source_reg] : short_reg[source_reg], w ? long_reg[dest_reg] : short_reg[dest_reg]);
            }
            if (mode == 0) {
                // register to memory
                uint8_t reg = buf[i] >> 3 & reg_mask;
                uint8_t rm = buf[i] & reg_mask;
                if (rm == 0b110) {
                    // 16 bit displacement special case
                    i += 2;
                }
                if (d) {
                    sprintf(command_stream[stream_i++], "%s %s, %s", opcodes[opcode],
                            w ? long_reg[reg] : short_reg[reg], mem_reg[rm]);
                } else {
                    sprintf(command_stream[stream_i++], "%s %s, %s", opcodes[opcode], mem_reg[rm],
                            w ? long_reg[reg] : short_reg[reg]);
                }
            }
            if (mode == 0b01) {
                // memory 8bit displacement
                uint8_t reg = buf[i] >> 3 & reg_mask;
                uint8_t rm = buf[i] & reg_mask;
                i += 1;
                uint8_t displacement = buf[i];
                char displ_reg[20];
                if (displacement == 0) {
                    strcpy(displ_reg, mem_reg[rm]);
                } else {
                    sprintf(displ_reg, mem_displ_reg[rm], displacement);
                }
                if (d) {
                    sprintf(command_stream[stream_i++], "%s %s, %s", opcodes[opcode],
                            w ? long_reg[reg] : short_reg[reg], displ_reg);
                } else {
                    sprintf(command_stream[stream_i++], "%s %s, %s", opcodes[opcode], displ_reg,
                            w ? long_reg[reg] : short_reg[reg]);
                }
            }
            if (mode == 0b10) {
                // memory 16 bit displacement
                uint8_t reg = buf[i] >> 3 & reg_mask;
                uint8_t rm = buf[i] & reg_mask;
                uint16_t displacement = (buf[i + 2] << 8) + buf[i + 1];
                char displ_reg[20];
                if (displacement == 0) {
                    strcpy(displ_reg, mem_reg[rm]);
                } else {
                    sprintf(displ_reg, mem_displ_reg[rm], displacement);
                }
                if (d) {
                    sprintf(command_stream[stream_i++], "%s %s, %s", opcodes[opcode],
                            w ? long_reg[reg] : short_reg[reg], displ_reg);
                } else {
                    sprintf(command_stream[stream_i++], "%s %s, %s", opcodes[opcode], displ_reg,
                            w ? long_reg[reg] : short_reg[reg]);
                }
                i += 2;
            }
        }
        opcode = buf[i] >> 4;
        if (opcodes[opcode] != NULL) {
            // immediate to register
            uint8_t reg_mask = 0b111;
            uint8_t w = buf[i] >> 3 & 1;
            uint8_t reg = buf[i] & reg_mask;
            uint16_t data;
            if (w && buf[i + 2] != 0) {
                data = (buf[i + 2] << 8) + buf[i + 1];
                i += 2;
            } else {
                data = buf[i + 1];
                i += 1;
            }
            sprintf(command_stream[stream_i++], "%s %s, %d", opcodes[opcode], w ? long_reg[reg] : short_reg[reg], data);
        }
    }
    for (uint32_t i = 0; i < stream_i; ++i) {
        printf("%s \n", command_stream[i]);
    }
    return 0;
}
