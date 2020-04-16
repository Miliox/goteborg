/*
 * mmuimpl.cpp
 * Copyright (C) 2020 Emiliano Firmino <emiliano.firmino@gmail.com>
 *
 * Distributed under terms of the MIT license.
 */

#include "mmuimpl.h"

#include <algorithm>

using namespace goteborg;


MMUImpl::MMUImpl() : MMU(), mem(1 << 16, (1 << 8) - 1) {

}

MMUImpl::~MMUImpl() {

}

u8 MMUImpl::read(addr src) {
    src = resolve(src);
    return mem.at(src);
}

addr MMUImpl::resolve(addr a) {
    /*
    $FFFF             Interrupt Enable Flag
    $FF80-$FFFE     Zero Page - 127 bytes
    $FF00-$FF7F     Hardware I/O Registers
    $FEA0-$FEFF     Unusable Memory
    $FE00-$FE9F     OAM - Object Attribute Memory
    $E000-$FDFF     Echo RAM - Reserved, Do Not Use
    $D000-$DFFF     Internal RAM - Bank 1-7 (switchable - CGB only)
    $C000-$CFFF     Internal RAM - Bank 0 (fixed)
    $A000-$BFFF     Cartridge RAM (If Available)
    $9C00-$9FFF     BG Map Data 2
    $9800-$9BFF     BG Map Data 1
    $8000-$97FF     Character RAM
    $4000-$7FFF     Cartridge ROM - Switchable Banks 1-xx
    $0150-$3FFF     Cartridge ROM - Bank 0 (fixed)
    $0100-$014F     Cartridge Header Area
    $0000-$00FF     Restart and Interrupt Vectors
    */

    if (a >= 0xe000u && a <= 0xfdff) {
        // ECHO RAM => Internal RAM
        a -= 0xe000u;
        a += 0xc000u;
    }

    return a;
}

void MMUImpl::step(u8 ticks) {
    // Todo: DMA
}

void MMUImpl::transfer(addr dst, addr src) {
    dst = resolve(dst);
    src = resolve(src);

    // Naive DMA
    for (size_t i = 0; i < 160; i++) {
        mem.at(dst + i) =  mem.at(src + i);
    }
}

void MMUImpl::write(addr dst, u8 value) {
    dst = resolve(dst);
    mem.at(dst) = value;
}

void MMUImpl::write(addr dst, const buffer& data) {
    for (addr a = 0; a < data.size(); a++) {
        auto aux = resolve(dst + a);
        mem.at(aux) = data.at(a);
    }
}
