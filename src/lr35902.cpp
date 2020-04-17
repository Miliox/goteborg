/*
 * LR35902.cpp
 * Copyright (C) 2020 Emiliano Firmino <emiliano.firmino@gmail.com>
 *
 * Distributed under terms of the MIT license.
 */

#include "lr35902.h"

#include "alu.h"
#include "mmu.h"

using namespace goteborg;

ticks notImplementedInstruction(u8 opcode) {
    std::stringstream ss;
    ss << "Instruction not implemented: ";
    ss << std::uppercase << std::hex << std::setfill('0') << std::setw(2);
    ss << static_cast<int>(opcode);

    throw std::runtime_error(ss.str());
    return 0;
}

ticks notImplementedCBInstruction(u8 opcode) {
    std::stringstream ss;
    ss << "CB Instruction not implemented: ";
    ss << std::uppercase << std::hex << std::setfill('0') << std::setw(2);
    ss << static_cast<int>(opcode);

    throw std::runtime_error(ss.str());
    return 0;
}

LR35902::LR35902(MMU& mmu) : regs(), mmu(mmu),
    iset_(256, [&]() {
        return notImplementedInstruction(peek8());
    }),
    xset_(256, [&]() {
        return notImplementedCBInstruction(peek8());
    }) {

    populateInstructionSets();
}

ticks LR35902::cycle() {
    auto opcode = peek8();               // fetch
    auto instruction = iset_.at(opcode); // decode
    return instruction();                // execute
}

u8 LR35902::next8() {
    return mmu.read(regs.pc++);
}

u16 LR35902::next16() {
    u8 hsb = next8();
    u8 lsb = next8();
    return (hsb << 8) | lsb;
}

u8 LR35902::peek8() {
    return mmu.read(regs.pc);
}

u16 LR35902::peek16() {
    u8 hsb = mmu.read(regs.pc);
    u8 lsb = mmu.read(regs.pc + 1);
    return (hsb << 8) | lsb;
}

void LR35902::populateInstructionSets() {
    iset_.at(0x0e) = [&]() {
        regs.c = next8();
        return ticks(8);
    };

    iset_.at(0x11) = [&]() {
        regs.de = next16();
        return ticks(12);
    };

    iset_.at(0x20) = [&]() {
        regs.pc++;
        s8 offset = s8(next8());
        if ((regs.f & alu::kFZ) == 0) {
            regs.pc += offset;
            return ticks(12);
        }
        return ticks(8);
    };

    iset_.at(0x21) = [&]() {
        regs.pc++;
        regs.hl = next16();
        return ticks(12);
    };

    iset_.at(0x31) = [&]() {
        regs.pc++;
        regs.sp = next16();
        return ticks(12);
    };

    iset_.at(0x32) = [&]() {
        regs.pc++;
        mmu.write(regs.hl--, regs.a);
        return ticks(8);
    };

    iset_.at(0xaf) = [&]() {
        regs.pc++;
        alu::lxor(regs.f, regs.a, regs.a);
        return ticks(4);
    };

    iset_.at(0xcb) = [&]() {
        regs.pc++;
        return ticks(4) + xset_.at(next8())();
    };

    xset_.at(0x7c) = [&]() {
        alu::bit(regs.f, regs.h, 7);
        return ticks(8);
    };
}