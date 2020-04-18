/*
 * emulator.cpp
 * Copyright (C) 2020 Emiliano Firmino <emiliano.firmino@gmail.com>
 *
 * Distributed under terms of the MIT license.
 */

#include "emulator.h"

#include <exception>

#include <SFML/System/FileInputStream.hpp>

using namespace goteborg;

Emulator::Emulator() : mmu(), cpu(mmu) {

}

LR35902& Emulator::getCPU() {
    return cpu;
}

MMU& Emulator::getMMU() {
    return mmu;
}

void Emulator::reset() {
    sf::FileInputStream bios;
    if (!bios.open("bios.bin")) {
        throw std::runtime_error("error: cannot load bios");
    }

    buffer_t data(bios.getSize(), 0xff);
    bios.read(reinterpret_cast<void*>(&data.front()), data.size());

    mmu.write(0, data);
}

ticks_t Emulator::step() {
    return cpu.cycle();
}
