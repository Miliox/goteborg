/*
 * LR35902.cpp
 * Copyright (C) 2020 Emiliano Firmino <emiliano.firmino@gmail.com>
 *
 * Distributed under terms of the MIT license.
 */

#include "lr35902.h"

using namespace goteborg;

ticks notImplemented(u8 opcode) {
    std::stringstream ss;
    ss << "Instruction not supported: ";
    ss << std::uppercase << std::hex << std::setfill('0') << std::setw(2);
    ss << static_cast<int>(opcode);

    throw std::runtime_error(ss.str());
    return 0;
}

LR35902::LR35902(MMU& mmu) : r(), mmu(mmu),
    iset(256, [&]() { return notImplemented(mmu.read(r.pc)); }),
    cb_iset(256, [&]() { return notImplemented(mmu.read(r.pc)); }) {

    iset.at(0x31) = [&]() {
        r.pc++;
        r.sp  = mmu.read(r.pc++) << 8;
        r.sp += mmu.read(r.pc++);
        return ticks(12);
    };
}

Registers LR35902::getRegisters() {
    return r;
}

ticks LR35902::cycle() {
    auto opcode = mmu.read(r.pc);       // fetch
    auto instruction = iset.at(opcode); // decode
    return instruction();               // execute
}