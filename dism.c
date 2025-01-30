
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
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
    char* short_reg[0x111] = {"al", "cl", "dl", "bl", "ah", "ch", "dh", "bh"};
    char* long_reg[0x111] = {"ax", "cx", "dx", "bx", "sp", "bp", "si", "di"};

    opcodes[0b100010] = "mov";

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
            w = (buf[i] & 0b1) == 0b1;
            d = (buf[i] & 0b10) == 0b10;
            i++;
            uint8_t source_reg = d ? buf[i] >> 3 & 0b111 : buf[i] & 0b111;
            uint8_t dest_reg = d ? buf[i] & 0b111 : buf[i] >> 3 & 0b111;

            sprintf(command_stream[stream_i++], "%s %s, %s", opcodes[opcode],
                    w ? long_reg[source_reg] : short_reg[source_reg], w ? long_reg[dest_reg] : short_reg[dest_reg]);
        }
    }
    for (uint32_t i = 0; i < stream_i; ++i) {
        printf("%s \n", command_stream[i]);
    }
    return 0;
}
