#include <iostream>
#include <fstream>
#include <vector>

int main() {
    std::ifstream file("roms/Tetris.ch8", std::ios::binary);
    std::vector<unsigned char> data((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    printf("Subroutine at 0x272:\n");
    for(size_t i=0x272-0x200; i<0x284-0x200; i+=2) {
        printf("0x%04lX: %02X%02X\n", i + 0x200, data[i], data[i+1]);
    }
    printf("Subroutine at 0x284:\n");
    for(size_t i=0x284-0x200; i<0x296-0x200; i+=2) {
        printf("0x%04lX: %02X%02X\n", i + 0x200, data[i], data[i+1]);
    }
    printf("Subroutine at 0x296:\n");
    for(size_t i=0x296-0x200; i<0x2A8-0x200; i+=2) {
        printf("0x%04lX: %02X%02X\n", i + 0x200, data[i], data[i+1]);
    }
    return 0;
}
