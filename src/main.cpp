/*
 * main.cpp
 * Copyright (C) 2020 Emiliano Firmino <emiliano.firmino@gmail.com>
 *
 * Distributed under terms of the MIT license.
 */

#include "main.h"
#include "mmuimpl.h"
#include "lr35902.h"

#include <fstream>
#include <iostream>

using namespace goteborg;

int main(int argc, char** argv) {
    MMUImpl mmu;
    LR35902 cpu(mmu);

    std::fstream bios;
    bios.open("res/bios.bin", std::fstream::in | std::fstream::binary);

    if (!bios.is_open()) {
        std::cerr << "error: cannot load bios" << std::endl;
        return -1;
    }

    // Load bios
    addr a = 0;
    auto b = bios.get();
    while (!bios.eof()) {
        mmu.write(a, b);
        b = bios.get();
        a += 1;
    }
    bios.close();

    for (;;) {
        cpu.cycle();
    }

    return 0;
}
