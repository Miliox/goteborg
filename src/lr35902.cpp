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

ticks_t notImplementedInstruction(u8 opcode) {
    std::stringstream ss;
    ss << "Instruction not implemented: ";
    ss << std::uppercase << std::hex << std::setfill('0') << std::setw(2);
    ss << static_cast<int>(opcode);

    throw std::runtime_error(ss.str());
    return 0;
}

ticks_t notImplementedCBInstruction(u8 opcode) {
    std::stringstream ss;
    ss << "Instruction not implemented: CB ";
    ss << std::uppercase << std::hex << std::setfill('0') << std::setw(2);
    ss << static_cast<int>(opcode);

    throw std::runtime_error(ss.str());
    return 0;
}

LR35902::LR35902(MMU& mmu) : regs(), mmu(mmu),
    iset_(512, [&]() {
        return notImplementedInstruction(peek8());
    }) {
    
    for (size_t i = iset_.size() / 2; i < iset_.size(); i++) {
        iset_.at(i) = [&]() { return notImplementedCBInstruction(peek8()); };
    }

    populateInstructionSets();
}

ticks_t LR35902::cycle() {
    auto opcode = peek8();               // fetch
    auto instruction = iset_.at(opcode); // decode
    return instruction();                // execute
}

u8 LR35902::next8() {
    return read8(regs.pc++);
}

u16 LR35902::next16() {
    auto data = read16(regs.pc);
    regs.pc += 2;
    return data;
}

u8 LR35902::peek8() {
    return read8(regs.pc);
}

u16 LR35902::peek16() {
    return read16(regs.pc);
}

u8 LR35902::read8(addr_t a) {
    return mmu.read(a);
}

u16 LR35902::read16(addr_t a) {
    u8 hsb = mmu.read(a);
    u8 lsb = mmu.read(a + 1);
    return (hsb << 8) | lsb;
}

void LR35902::write8(addr_t a, u8 v) {
    mmu.write(a, v);
}

void LR35902::write16(addr_t a, u16 v) {
    mmu.write(a, (v >> 8) & 0xff);
    mmu.write(a+1, v & 0xff);
}

u8 LR35902::zread8(u8 a) {
    return read8(0xff00 + a);
}

u16 LR35902::zread16(u8 a) {
    return read16(0xff00 + a);
}

void LR35902::zwrite8(u8 a, u8 v) {
    write8(0xff00 + a, v);
}

void LR35902::zwrite16(u8 a, u16 v) {
    write16(0xff00 + a, v);
}

void LR35902::call(addr_t a) {
    push(regs.pc);
    regs.pc = a;
}

void LR35902::rst(addr_t a) {
    push(regs.pc);
    regs.pc = a;
}

void LR35902::ret() {
    pop(regs.pc);
}

void LR35902::push(u16& reg) {
    u8 hsb = reg >> 8;
    u8 lsb = reg;
    write8(regs.sp--, lsb);
    write8(regs.sp--, hsb);
}

void LR35902::pop(u16& reg) {
    u8 hsb = read8(++regs.sp);
    u8 lsb = read8(++regs.sp);
    reg = (hsb << 8) | lsb;
}

void LR35902::populateInstructionSets() {
    // NOP
    iset_.at(0x00) = [&]() {
        regs.pc++;
        return 4;
    };

    // LD BC,d16
    iset_.at(0x01) = [&]() {
        regs.pc++;
        regs.bc = next16();
        return 12;
    };

    // LD (BC),A
    iset_.at(0x02) = [&]() {
        regs.pc++;
        write8(regs.bc, regs.a);
        return 8;
    };

    // INC BC
    iset_.at(0x03) = [&]() {
        regs.pc++;
        alu::inc16(regs.f, regs.bc);
        return 8;
    };

    // INC B
    iset_.at(0x04) = [&]() {
        regs.pc++;
        alu::inc8(regs.f, regs.b);
        return 4;
    };

    // DEC B
    iset_.at(0x05) = [&]() {
        regs.pc++;
        alu::dec8(regs.f, regs.b);
        return 4;
    };

    // LD B,d8
    iset_.at(0x06) = [&]() {
        regs.pc++;
        regs.b = next8();
        return 8;
    };

    // RLCA
    iset_.at(0x07) = [&]() {
        regs.pc++;
        alu::rlc(regs.f, regs.a);
        return 4;
    };

    // LD (a16),SP
    iset_.at(0x08) = [&]() {
        regs.pc++;
        u16 addr = next16();
        write16(addr, regs.sp);
        return 20;
    };

    // ADD HL,BC
    iset_.at(0x09) = [&]() {
        regs.pc++;
        alu::add16(regs.f, regs.hl, regs.bc);
        return 8;
    };

    // LD A,(BC)
    iset_.at(0x0a) = [&]() {
        regs.pc++;
        regs.a = read16(regs.bc);
        return 8;
    };

    // DEC BC
    iset_.at(0x0b) = [&]() {
        regs.pc++;
        alu::dec16(regs.f, regs.bc);
        return 8;
    };

    // INC C
    iset_.at(0x0c) = [&]() {
        regs.pc++;
        alu::inc8(regs.f, regs.c);
        return 4;
    };

    // DEC C
    iset_.at(0x0d) = [&]() {
        regs.pc++;
        alu::dec8(regs.f, regs.c);
        return 4;
    };

    // LD C,d8
    iset_.at(0x0e) = [&]() {
        regs.pc++;
        regs.c = next8();
        return 8;
    };

    // RRCA
    iset_.at(0x0f) = [&]() {
        regs.pc++;
        alu::rrc(regs.f, regs.a);
        return 4;
    };


    // STOP 0
    iset_.at(0x10) = [&]() {
        regs.pc++;
        return 4;
    };

    // LD DE,d16
    iset_.at(0x11) = [&]() {
        regs.pc++;
        regs.de = next16();
        return 12;
    };

    // LD (DE),A
    iset_.at(0x12) = [&]() {
        regs.pc++;
        write8(regs.de, regs.a);
        return 8;
    };

    // INC DE
    iset_.at(0x13) = [&]() {
        regs.pc++;
        alu::inc16(regs.f, regs.de);
        return 8;
    };

    // INC D
    iset_.at(0x14) = [&]() {
        regs.pc++;
        alu::inc8(regs.f, regs.d);
        return 4;
    };

    // DEC D
    iset_.at(0x15) = [&]() {
        regs.pc++;
        alu::dec8(regs.f, regs.d);
        return 4;
    };

    // LD D,d8
    iset_.at(0x16) = [&]() {
        regs.pc++;
        regs.d = next8();
        return 8;
    };

    // RLA
    iset_.at(0x17) = [&]() {
        regs.pc++;
        alu::rl(regs.f, regs.a);
        return 4;
    };

    // JR r8
    iset_.at(0x18) = [&](){
        regs.pc++;
        regs.pc += s8(next8());
        return 12;
    };

    // ADD HL,DE
    iset_.at(0x19) = [&]() {
        regs.pc++;
        alu::add16(regs.f, regs.hl, regs.de);
        return 8;
    };

    // LD A,(DE)
    iset_.at(0x1a) = [&]() {
        regs.pc++;
        regs.a = read8(regs.de);
        return 8;
    };

    // DEC DE
    iset_.at(0x1b) = [&]() {
        regs.pc++;
        alu::dec16(regs.f, regs.de);
        return 8;
    };

    // INC E
    iset_.at(0x1c) = [&]() {
        regs.pc++;
        alu::inc8(regs.f, regs.e);
        return 4;
    };

    // DEC E
    iset_.at(0x1d) = [&]() {
        regs.pc++;
        alu::dec8(regs.f, regs.e);
        return 4;
    };

    // LD E,d8
    iset_.at(0x1e) = [&]() {
        regs.pc++;
        regs.e = next8();
        return 8;
    };

    // RRA
    iset_.at(0x1f) = [&]() {
        regs.pc++;
        alu::rr(regs.f, regs.a);
        return 4;
    };

    // JR NZ,r8
    iset_.at(0x20) = [&]() {
        regs.pc++;
        s8 offset = s8(next8());
        if ((regs.f & alu::kFZ) == 0) {
            regs.pc += offset;
            return 12;
        }
        return 8;
    };

    // LD HL,d16
    iset_.at(0x21) = [&]() {
        regs.pc++;
        regs.hl = next16();
        return 12;
    };

    // LD (HL+),A
    iset_.at(0x22) = [&]() {
        regs.pc++;
        write8(regs.hl++, regs.a);
        return 8;
    };

    // INC HL
    iset_.at(0x23) = [&]() {
        regs.pc++;
        alu::inc16(regs.f, regs.hl);
        return 8;
    };

    // INC H
    iset_.at(0x24) = [&]() {
        regs.pc++;
        alu::inc8(regs.f, regs.h);
        return 4;
    };

    // DEC H
    iset_.at(0x25) = [&]() {
        regs.pc++;
        alu::dec8(regs.f, regs.h);
        return 4;
    };

    // LD H,d8
    iset_.at(0x26) = [&]() {
        regs.pc++;
        regs.h = next8();
        return 8;
    };

    // DAA
    iset_.at(0x27) = [&]() {
        regs.pc++;
        alu::daa(regs.f, regs.a);
        return 4;
    };

    // JR Z,r8
    iset_.at(0x28) = [&]() {
        regs.pc++;
        s8 offset = s8(next8());
        if ((regs.f & alu::kFZ) != 0) {
            regs.pc += offset;
            return 12;
        }
        return 8;
    };

    // ADD HL,HL
    iset_.at(0x29) = [&]() {
        regs.pc++;
        alu::add16(regs.f, regs.hl, regs.hl);
        return 8;
    };

    // LD A,(HL+)
    iset_.at(0x2a) = [&]() {
        regs.pc++;
        regs.a = read8(regs.hl++);
        return 8;
    };

    // DEC HL
    iset_.at(0x2b) = [&]() {
        regs.pc++;
        alu::dec16(regs.f, regs.hl);
        return 8;
    };

    // INC L
    iset_.at(0x2c) = [&]() {
        regs.pc++;
        alu::inc8(regs.f, regs.l);
        return 4;
    };

    // DEC L
    iset_.at(0x2d) = [&]() {
        regs.pc++;
        alu::dec8(regs.f, regs.l);
        return 4;
    };

    // LD L,d8
    iset_.at(0x2e) = [&]() {
        regs.pc++;
        regs.l = next8();
        return 8;
    };

    // CPL
    iset_.at(0x2f) = [&]() {
        regs.pc++;
        alu::cpl(regs.f, regs.a);
        return 4;
    };

    // JR NC,r8
    iset_.at(0x30) = [&]() {
        regs.pc++;
        s8 offset = s8(next8());
        if ((regs.f & alu::kFC) == 0) {
            regs.pc += offset;
            return 12;
        }
        return 8;
    };

    // LD SP,d16
    iset_.at(0x31) = [&]() {
        regs.pc++;
        regs.sp = next16();
        return 12;
    };

    // LD (HL-),A
    iset_.at(0x32) = [&]() {
        regs.pc++;
        write8(regs.hl--, regs.a);
        return 8;
    };

    // INC SP
    iset_.at(0x33) = [&]() {
        regs.pc++;
        alu::inc16(regs.f, regs.sp);
        return 8;
    };

    // INC (HL)
    iset_.at(0x34) = [&]() {
        regs.pc++;
        u8 v = read8(regs.hl);
        alu::inc8(regs.f, v);
        write8(regs.hl, v);
        return 12;
    };

    // DEC (HL)
    iset_.at(0x35) = [&]() {
        regs.pc++;
        u8 v = read8(regs.hl);
        alu::dec8(regs.f, v);
        write8(regs.hl, v);
        return 12;
    };

    // LD (HL),d8
    iset_.at(0x36) = [&]() {
        regs.pc++;
        u8 v = next8();
        write8(regs.hl, v);
        return 12;
    };

    // SCF
    iset_.at(0x37) = [&]() {
        regs.pc++;
        alu::scf(regs.f);
        return 4;
    };

    // JR C,r8
    iset_.at(0x38) = [&]() {
        regs.pc++;
        s8 offset = s8(next8());
        if ((regs.f & alu::kFC) != 0) {
            regs.pc += offset;
            return 12;
        }
        return 8;
    };

    // ADD HL,SP
    iset_.at(0x39) = [&]() {
        regs.pc++;
        alu::add16(regs.f, regs.hl, regs.sp);
        return 8;
    };

    // LD A,(HL-)
    iset_.at(0x3a) = [&]() {
        regs.pc++;
        regs.a = read8(regs.hl--);
        return 8;
    };

    // DEC SP
    iset_.at(0x3b) = [&]() {
        regs.pc++;
        alu::dec16(regs.f, regs.sp);
        return 8;
    };

    // INC A
    iset_.at(0x3c) = [&]() {
        regs.pc++;
        alu::inc8(regs.f, regs.a);
        return 4;
    };

    // DEC A
    iset_.at(0x3d) = [&]() {
        regs.pc++;
        alu::dec8(regs.f, regs.a);
        return 4;
    };

    // LD A,d8
    iset_.at(0x3e) = [&]() {
        regs.pc++;
        regs.a = next8();
        return 8;
    };

    // CCF
    iset_.at(0x3f) = [&]() {
        regs.pc++;
        alu::ccf(regs.f);
        return 4;
    };

    // LD B,B
    iset_.at(0x40) = [&]() {
        regs.pc++;
        regs.b = regs.b;
        return 4;
    };

    // LD B,C
    iset_.at(0x41) = [&]() {
        regs.pc++;
        regs.b = regs.c;
        return 4;
    };

    // LD B,D
    iset_.at(0x42) = [&]() {
        regs.pc++;
        regs.b = regs.d;
        return 4;
    };

    // LD B,E
    iset_.at(0x43) = [&]() {
        regs.pc++;
        regs.b = regs.e;
        return 4;
    };

    // LD B,H
    iset_.at(0x44) = [&]() {
        regs.pc++;
        regs.b = regs.h;
        return 4;
    };

    // LD B,L
    iset_.at(0x45) = [&]() {
        regs.pc++;
        regs.b = regs.l;
        return 4;
    };

    // LD B,(HL)
    iset_.at(0x46) = [&]() {
        regs.pc++;
        regs.b = read8(regs.hl);
        return 8;
    };

    // LD B,A
    iset_.at(0x47) = [&]() {
        regs.pc++;
        regs.b = regs.a;
        return 4;
    };

    // LD C,B
    iset_.at(0x48) = [&]() {
        regs.pc++;
        regs.c = regs.b;
        return 4;
    };

    // LD C,C
    iset_.at(0x49) = [&]() {
        regs.pc++;
        regs.c = regs.c;
        return 4;
    };

    // LD C,D
    iset_.at(0x4a) = [&]() {
        regs.pc++;
        regs.c = regs.d;
        return 4;
    };

    // LD C,E
    iset_.at(0x4b) = [&]() {
        regs.pc++;
        regs.c = regs.e;
        return 4;
    };

    // LD C,H
    iset_.at(0x4c) = [&]() {
        regs.pc++;
        regs.c = regs.h;
        return 4;
    };

    // LD C,L
    iset_.at(0x4d) = [&]() {
        regs.pc++;
        regs.c = regs.l;
        return 4;
    };

    // LD C,(HL)
    iset_.at(0x4e) = [&]() {
        regs.pc++;
        regs.c = read8(regs.hl);
        return 8;
    };

    // LD C,A
    iset_.at(0x4f) = [&]() {
        regs.pc++;
        regs.c = regs.a;
        return 4;
    };


    // LD D,B
    iset_.at(0x50) = [&]() {
        regs.pc++;
        regs.d = regs.b;
        return 4;
    };

    // LD D,C
    iset_.at(0x51) = [&]() {
        regs.pc++;
        regs.d = regs.c;
        return 4;
    };

    // LD D,D
    iset_.at(0x52) = [&]() {
        regs.pc++;
        regs.d = regs.d;
        return 4;
    };

    // LD D,E
    iset_.at(0x53) = [&]() {
        regs.pc++;
        regs.d = regs.e;
        return 4;
    };

    // LD D,H
    iset_.at(0x54) = [&]() {
        regs.pc++;
        regs.d = regs.h;
        return 4;
    };

    // LD D,L
    iset_.at(0x55) = [&]() {
        regs.pc++;
        regs.d = regs.l;
        return 4;
    };

    // LD D,(HL)
    iset_.at(0x56) = [&]() {
        regs.pc++;
        regs.d = read8(regs.hl);
        return 8;
    };

    // LD D,A
    iset_.at(0x57) = [&]() {
        regs.pc++;
        regs.d = regs.a;
        return 4;
    };

    // LD E,B
    iset_.at(0x58) = [&]() {
        regs.pc++;
        regs.e = regs.b;
        return 4;
    };

    // LD E,C
    iset_.at(0x59) = [&]() {
        regs.pc++;
        regs.e = regs.c;
        return 4;
    };

    // LD E,D
    iset_.at(0x5a) = [&]() {
        regs.pc++;
        regs.e = regs.d;
        return 4;
    };

    // LD E,E
    iset_.at(0x5b) = [&]() {
        regs.pc++;
        regs.e = regs.e;
        return 4;
    };

    // LD E,H
    iset_.at(0x5c) = [&]() {
        regs.pc++;
        regs.e = regs.h;
        return 4;
    };

    // LD E,L
    iset_.at(0x5d) = [&]() {
        regs.pc++;
        regs.e = regs.l;
        return 4;
    };

    // LD E,(HL)
    iset_.at(0x5e) = [&]() {
        regs.pc++;
        regs.e = read8(regs.hl);
        return 8;
    };

    // LD E,A
    iset_.at(0x5f) = [&]() {
        regs.pc++;
        regs.e = regs.a;
        return 4;
    };

    // LD H,B
    iset_.at(0x60) = [&]() {
        regs.pc++;
        regs.h = regs.b;
        return 4;
    };

    // LD H,C
    iset_.at(0x61) = [&]() {
        regs.pc++;
        regs.h = regs.c;
        return 4;
    };

    // LD H,D
    iset_.at(0x62) = [&]() {
        regs.pc++;
        regs.h = regs.d;
        return 4;
    };

    // LD H,E
    iset_.at(0x63) = [&]() {
        regs.pc++;
        regs.h = regs.e;
        return 4;
    };

    // LD H,H
    iset_.at(0x64) = [&]() {
        regs.pc++;
        regs.h = regs.h;
        return 4;
    };

    // LD H,L
    iset_.at(0x65) = [&]() {
        regs.pc++;
        regs.h = regs.l;
        return 4;
    };

    // LD H,(HL)
    iset_.at(0x66) = [&]() {
        regs.pc++;
        regs.h = read8(regs.hl);
        return 8;
    };

    // LD H,A
    iset_.at(0x67) = [&]() {
        regs.pc++;
        regs.h = regs.a;
        return 4;
    };


    // LD L,B
    iset_.at(0x68) = [&]() {
        regs.pc++;
        regs.l = regs.b;
        return 4;
    };

    // LD L,C
    iset_.at(0x69) = [&]() {
        regs.pc++;
        regs.l = regs.c;
        return 4;
    };

    // LD L,D
    iset_.at(0x6a) = [&]() {
        regs.pc++;
        regs.l = regs.d;
        return 4;
    };

    // LD L,E
    iset_.at(0x6b) = [&]() {
        regs.pc++;
        regs.l = regs.e;
        return 4;
    };

    // LD L,H
    iset_.at(0x6c) = [&]() {
        regs.pc++;
        regs.l = regs.h;
        return 4;
    };

    // LD L,L
    iset_.at(0x6d) = [&]() {
        regs.pc++;
        regs.l = regs.l;
        return 4;
    };

    // LD L,(HL)
    iset_.at(0x6e) = [&]() {
        regs.pc++;
        regs.l = read8(regs.hl);
        return 8;
    };

    // LD L,A
    iset_.at(0x6f) = [&]() {
        regs.pc++;
        regs.l = regs.a;
        return 4;
    };


    // LD (HL),B
    iset_.at(0x70) = [&]() {
        regs.pc++;
        write8(regs.hl, regs.b);
        return 8;
    };

    // LD (HL),C
    iset_.at(0x71) = [&]() {
        regs.pc++;
        write8(regs.hl, regs.c);
        return 8;
    };

    // LD (HL),D
    iset_.at(0x72) = [&]() {
        regs.pc++;
        write8(regs.hl, regs.d);
        return 8;
    };

    // LD (HL),E
    iset_.at(0x73) = [&]() {
        regs.pc++;
        write8(regs.hl, regs.e);
        return 8;
    };

    // LD (HL),H
    iset_.at(0x74) = [&]() {
        regs.pc++;
        write8(regs.hl, regs.h);
        return 8;
    };

    // LD (HL),L
    iset_.at(0x75) = [&]() {
        regs.pc++;
        write8(regs.hl, regs.l);
        return 8;
    };

    // HALT
    iset_.at(0x76) = [&]() {
        // TODO: Detect interruption to resume execution
        return 4;
    };

    // LD (HL),A
    iset_.at(0x77) = [&]() {
        regs.pc++;
        write8(regs.hl, regs.a);
        return 8;
    };

    // LD A,B
    iset_.at(0x78) = [&]() {
        regs.pc++;
        regs.a = regs.b;
        return 4;
    };

    // LD A,C
    iset_.at(0x79) = [&]() {
        regs.pc++;
        regs.a = regs.c;
        return 4;
    };

    // LD A,D
    iset_.at(0x7a) = [&]() {
        regs.pc++;
        regs.a = regs.d;
        return 4;
    };

    // LD A,E
    iset_.at(0x7b) = [&]() {
        regs.pc++;
        regs.a = regs.e;
        return 4;
    };

    // LD A,H
    iset_.at(0x7c) = [&]() {
        regs.pc++;
        regs.a = regs.h;
        return 4;
    };

    // LD A,L
    iset_.at(0x7d) = [&]() {
        regs.pc++;
        regs.a = regs.l;
        return 4;
    };

    // LD A,(HL)
    iset_.at(0x7e) = [&]() {
        regs.pc++;
        regs.a = read8(regs.hl);
        return 8;
    };

    // LD A,A
    iset_.at(0x7f) = [&]() {
        regs.pc++;
        regs.a = regs.a;
        return 4;
    };

    // ADD A,B
    iset_.at(0x80) = [&]() {
        regs.pc++;
        alu::add8(regs.f, regs.a, regs.b);
        return 4;
    };

    // ADD A,C
    iset_.at(0x81) = [&]() {
        regs.pc++;
        alu::add8(regs.f, regs.a, regs.c);
        return 4;
    };

    // ADD A,D
    iset_.at(0x82) = [&]() {
        regs.pc++;
        alu::add8(regs.f, regs.a, regs.d);
        return 4;
    };

    // ADD A,E
    iset_.at(0x83) = [&]() {
        regs.pc++;
        alu::add8(regs.f, regs.a, regs.e);
        return 4;
    };

    // ADD A,H
    iset_.at(0x84) = [&]() {
        regs.pc++;
        alu::add8(regs.f, regs.a, regs.h);
        return 4;
    };

    // ADD A,L
    iset_.at(0x85) = [&]() {
        regs.pc++;
        alu::add8(regs.f, regs.a, regs.l);
        return 4;
    };

    // ADD A,(HL)
    iset_.at(0x86) = [&]() {
        regs.pc++;
        alu::add8(regs.f, regs.a, read8(regs.hl));
        return 8;
    };

    // ADD A,A
    iset_.at(0x87) = [&]() {
        regs.pc++;
        alu::add8(regs.f, regs.a, regs.a);
        return 4;
    };

    // ADC A,B
    iset_.at(0x88) = [&]() {
        regs.pc++;
        alu::adc8(regs.f, regs.a, regs.b);
        return 4;
    };

    // ADC A,C
    iset_.at(0x89) = [&]() {
        regs.pc++;
        alu::adc8(regs.f, regs.a, regs.c);
        return 4;
    };

    // ADC A,D
    iset_.at(0x8a) = [&]() {
        regs.pc++;
        alu::adc8(regs.f, regs.a, regs.d);
        return 4;
    };

    // ADC A,E
    iset_.at(0x8b) = [&]() {
        regs.pc++;
        alu::adc8(regs.f, regs.a, regs.e);
        return 4;
    };

    // ADC A,H
    iset_.at(0x8c) = [&]() {
        regs.pc++;
        alu::adc8(regs.f, regs.a, regs.h);
        return 4;
    };

    // ADC A,L
    iset_.at(0x8d) = [&]() {
        regs.pc++;
        alu::adc8(regs.f, regs.a, regs.l);
        return 4;
    };

    // ADC A,(HL)
    iset_.at(0x8e) = [&]() {
        regs.pc++;
        alu::adc8(regs.f, regs.a, read8(regs.hl));
        return 8;
    };

    // ADC A,A
    iset_.at(0x8f) = [&]() {
        regs.pc++;
        alu::adc8(regs.f, regs.a, regs.a);
        return 4;
    };


    // SUB B
    iset_.at(0x90) = [&]() {
        regs.pc++;
        alu::sub8(regs.f, regs.a, regs.b);
        return 4;
    };

    // SUB C
    iset_.at(0x91) = [&]() {
        regs.pc++;
        alu::sub8(regs.f, regs.a, regs.c);
        return 4;
    };

    // SUB D
    iset_.at(0x92) = [&]() {
        regs.pc++;
        alu::sub8(regs.f, regs.a, regs.d);
        return 4;
    };

    // SUB E
    iset_.at(0x93) = [&]() {
        regs.pc++;
        alu::sub8(regs.f, regs.a, regs.e);
        return 4;
    };

    // SUB H
    iset_.at(0x94) = [&]() {
        regs.pc++;
        alu::sub8(regs.f, regs.a, regs.h);
        return 4;
    };

    // SUB L
    iset_.at(0x95) = [&]() {
        regs.pc++;
        alu::sub8(regs.f, regs.a, regs.l);
        return 4;
    };

    // SUB (HL)
    iset_.at(0x96) = [&]() {
        regs.pc++;
        alu::sub8(regs.f, regs.a, read8(regs.hl));
        return 4;
    };

    // SUB A
    iset_.at(0x97) = [&]() {
        regs.pc++;
        alu::sub8(regs.f, regs.a, regs.a);
        return 4;
    };

    // SBC A,B
    iset_.at(0x98) = [&]() {
        regs.pc++;
        alu::sbc8(regs.f, regs.a, regs.b);
        return 4;
    };

    // SBC A,C
    iset_.at(0x99) = [&]() {
        regs.pc++;
        alu::sbc8(regs.f, regs.a, regs.c);
        return 4;
    };

    // SBC A,D
    iset_.at(0x9a) = [&]() {
        regs.pc++;
        alu::sbc8(regs.f, regs.a, regs.d);
        return 4;
    };

    // SBC A,E
    iset_.at(0x9b) = [&]() {
        regs.pc++;
        alu::sbc8(regs.f, regs.a, regs.e);
        return 4;
    };

    // SBC A,H
    iset_.at(0x9c) = [&]() {
        regs.pc++;
        alu::sbc8(regs.f, regs.a, regs.h);
        return 4;
    };

    // SBC A,L
    iset_.at(0x9d) = [&]() {
        regs.pc++;
        alu::sbc8(regs.f, regs.a, regs.l);
        return 4;
    };

    // SBC A,(HL)
    iset_.at(0x9e) = [&]() {
        regs.pc++;
        alu::sbc8(regs.f, regs.a, read8(regs.hl));
        return 8;
    };

    // SBC A,A
    iset_.at(0x9f) = [&]() {
        regs.pc++;
        alu::sbc8(regs.f, regs.a, regs.a);
        return 4;
    };


    // AND B
    iset_.at(0xa0) = [&]() {
        regs.pc++;
        alu::land(regs.f, regs.a, regs.b);
        return 4;
    };

    // AND C
    iset_.at(0xa1) = [&]() {
        regs.pc++;
        alu::land(regs.f, regs.a, regs.c);
        return 4;
    };

    // AND D
    iset_.at(0xa2) = [&]() {
        regs.pc++;
        alu::land(regs.f, regs.a, regs.d);
        return 4;
    };

    // AND E
    iset_.at(0xa3) = [&]() {
        regs.pc++;
        alu::land(regs.f, regs.a, regs.e);
        return 4;
    };

    // AND H
    iset_.at(0xa4) = [&]() {
        regs.pc++;
        alu::land(regs.f, regs.a, regs.h);
        return 4;
    };

    // AND L
    iset_.at(0xa5) = [&]() {
        regs.pc++;
        alu::land(regs.f, regs.a, regs.l);
        return 4;
    };

    // AND (HL)
    iset_.at(0xa6) = [&]() {
        regs.pc++;
        alu::land(regs.f, regs.a, read8(regs.hl));
        return 8;
    };

    // AND A
    iset_.at(0xa7) = [&]() {
        regs.pc++;
        alu::land(regs.f, regs.a, regs.a);
        return 4;
    };

    // XOR B
    iset_.at(0xa8) = [&]() {
        regs.pc++;
        alu::lxor(regs.f, regs.a, regs.b);
        return 4;
    };

    // XOR C
    iset_.at(0xa9) = [&]() {
        regs.pc++;
        alu::lxor(regs.f, regs.a, regs.c);
        return 4;
    };

    // XOR D
    iset_.at(0xaa) = [&]() {
        regs.pc++;
        alu::lxor(regs.f, regs.a, regs.d);
        return 4;
    };

    // XOR E
    iset_.at(0xab) = [&]() {
        regs.pc++;
        alu::lxor(regs.f, regs.a, regs.e);
        return 4;
    };

    // XOR H
    iset_.at(0xac) = [&]() {
        regs.pc++;
        alu::lxor(regs.f, regs.a, regs.h);
        return 4;
    };

    // XOR L
    iset_.at(0xad) = [&]() {
        regs.pc++;
        alu::lxor(regs.f, regs.a, regs.l);
        return 4;
    };

    // XOR (HL)
    iset_.at(0xae) = [&]() {
        regs.pc++;
        alu::lxor(regs.f, regs.a, read8(regs.hl));
        return 8;
    };

    // XOR A
    iset_.at(0xaf) = [&]() {
        regs.pc++;
        alu::lxor(regs.f, regs.a, regs.a);
        return 4;
    };


    // OR B
    iset_.at(0xb0) = [&]() {
        regs.pc++;
        alu::lor(regs.f, regs.a, regs.b);
        return 4;
    };

    // OR C
    iset_.at(0xb1) = [&]() {
        regs.pc++;
        alu::lor(regs.f, regs.a, regs.c);
        return 4;
    };

    // OR D
    iset_.at(0xb2) = [&]() {
        regs.pc++;
        alu::lor(regs.f, regs.a, regs.d);
        return 4;
    };

    // OR E
    iset_.at(0xb3) = [&]() {
        regs.pc++;
        alu::lor(regs.f, regs.a, regs.e);
        return 4;
    };

    // OR H
    iset_.at(0xb4) = [&]() {
        regs.pc++;
        alu::lor(regs.f, regs.a, regs.h);
        return 4;
    };

    // OR L
    iset_.at(0xb5) = [&]() {
        regs.pc++;
        alu::lor(regs.f, regs.a, regs.l);
        return 4;
    };

    // OR (HL)
    iset_.at(0xb6) = [&]() {
        regs.pc++;
        alu::lor(regs.f, regs.a, read8(regs.hl));
        return 8;
    };

    // OR A
    iset_.at(0xb7) = [&]() {
        regs.pc++;
        alu::lor(regs.f, regs.a, regs.a);
        return 4;
    };

    // CP B
    iset_.at(0xb8) = [&]() {
        regs.pc++;
        alu::lcp(regs.f, regs.a, regs.b);
        return 4;
    };

    // CP C
    iset_.at(0xb9) = [&]() {
        regs.pc++;
        alu::lcp(regs.f, regs.a, regs.c);
        return 4;
    };

    // CP D
    iset_.at(0xba) = [&]() {
        regs.pc++;
        alu::lcp(regs.f, regs.a, regs.d);
        return 4;
    };

    // CP E
    iset_.at(0xbb) = [&]() {
        regs.pc++;
        alu::lcp(regs.f, regs.a, regs.e);
        return 4;
    };

    // CP H
    iset_.at(0xbc) = [&]() {
        regs.pc++;
        alu::lcp(regs.f, regs.a, regs.h);
        return 4;
    };

    // CP L
    iset_.at(0xbd) = [&]() {
        regs.pc++;
        alu::lcp(regs.f, regs.a, regs.l);
        return 4;
    };

    // CP (HL)
    iset_.at(0xbe) = [&]() {
        regs.pc++;
        alu::lcp(regs.f, regs.a, read8(regs.hl));
        return 8;
    };

    // CP A
    iset_.at(0xbf) = [&]() {
        regs.pc++;
        alu::lcp(regs.f, regs.a, regs.a);
        return 4;
    };

    // RET NZ
    iset_.at(0xc0) = [&]() {
        regs.pc++;
        if ((regs.f & alu::kFZ) == 0) {
            ret();
            return 20;
        }
        return 8;
    };

    // POP BC
    iset_.at(0xc1) = [&]() {
        regs.pc++;
        pop(regs.bc);
        return 12;
    };

    // JP NZ,a16
    iset_.at(0xc2) = [&]() {
        regs.pc++;
        u16 addr = next16();
        if ((regs.f & alu::kFZ) == 0) {
            regs.pc = addr;
            return 16;
        }
        return 12;
    };

    // JP a16
    iset_.at(0xc3) = [&]() {
        regs.pc++;
        regs.pc = next16();
        return 12;
    };

    // CALL NZ,a16
    iset_.at(0xc4) = [&]() {
        regs.pc++;
        u16 addr = next16();
        if ((regs.f & alu::kFZ) == 0) {
            call(addr);
            return 24;
        }
        return 12;
    };

    // PUSH BC
    iset_.at(0xc5) = [&]() {
        regs.pc++;
        push(regs.bc);
        return 16;
    };

    // ADD A,d8
    iset_.at(0xc6) = [&]() {
        regs.pc++;
        alu::add8(regs.f, regs.a, next8());
        return 8;
    };

    // RST 00H
    iset_.at(0xc7) = [&]() {
        regs.pc++;
        rst(0x00);
        return 16;
    };

    // RET Z
    iset_.at(0xc8) = [&]() {
        regs.pc++;
        if ((regs.f & alu::kFZ) != 0) {
            ret();
            return 20;
        }
        return 8;
    };

    // RET
    iset_.at(0xc9) = [&]() {
        regs.pc++;
        ret();
        return 16;
    };

    // JP Z,a16
    iset_.at(0xca) = [&]() {
        regs.pc++;
        u16 addr = next16();
        if ((regs.f & alu::kFZ) != 0) {
            regs.pc = addr;
            return 16;
        }
        return 12;
    };

    // PREFIX CB
    iset_.at(0xcb) = [&]() {
        regs.pc++;
        u8 opcode = next8();
        return 4 + iset_.at(0x100 | opcode)();
    };

    // CALL Z,a16
    iset_.at(0xcc) = [&]() {
        regs.pc++;
        u16 addr = next16();
        if ((regs.f & alu::kFZ) != 0) {
            call(addr);
            return 12;
        }
        return 8;
    };

    // CALL a16
    iset_.at(0xcd) = [&]() {
        regs.pc++;
        call(next16());
        return 8;
    };

    // ADC A,d8
    iset_.at(0xce) = [&]() {
        regs.pc++;
        alu::adc8(regs.f, regs.a, next8());
        return 8;
    };

    // RST 08H
    iset_.at(0xcf) = [&]() {
        regs.pc++;
        rst(0x08);
        return 16;
    };


    // RET NC
    iset_.at(0xd0) = [&]() {
        regs.pc++;
        if ((regs.f & alu::kFC) == 0) {
            ret();
            return 20;
        }
        return 8;
    };

    // POP DE
    iset_.at(0xd1) = [&]() {
        regs.pc++;
        pop(regs.de);
        return 12;
    };

    // JP NC,a16
    iset_.at(0xd2) = [&]() {
        regs.pc++;
        u16 addr = next16();
        if ((regs.f & alu::kFC) == 0) {
            regs.pc = addr;
            return 16;
        }
        return 12;
    };

    // NOP
    iset_.at(0xd3) = [&]() {
        regs.pc++;
        return 4;
    };

    // CALL NC,a16
    iset_.at(0xd4) = [&]() {
        regs.pc++;
        u16 addr = next16();
        if ((regs.f & alu::kFC) == 0) {
            call(addr);
            return 24;
        }
        return 12;
    };

    // PUSH DE
    iset_.at(0xd5) = [&]() {
        regs.pc++;
        push(regs.de);
        return 16;
    };

    // SUB d8
    iset_.at(0xd6) = [&]() {
        regs.pc++;
        alu::sub8(regs.f, regs.a, next8());
        return 8;
    };

    // RST 10H
    iset_.at(0xd7) = [&]() {
        regs.pc++;
        rst(0x10);
        return 16;
    };

    // RET C
    iset_.at(0xd8) = [&]() {
        regs.pc++;
        if ((regs.f & alu::kFC) != 0) {
            ret();
            return 20;
        }
        return 8;
    };

    // RETI
    iset_.at(0xd9) = [&]() {
        regs.pc++;
        ret();
        regs.ime = 1;
        return 16;
    };

    // JP C,a16
    iset_.at(0xda) = [&]() {
        regs.pc++;
        u16 addr = next16();
        if ((regs.f & alu::kFC) != 0) {
            regs.pc = addr;
            return 16;
        }
        return 12;
    };

    // NOP
    iset_.at(0xdb) = [&]() {
        regs.pc++;
        return 4;
    };

    // CALL C,a16
    iset_.at(0xdc) = [&]() {
        regs.pc++;
        u16 addr = next16();
        if ((regs.f & alu::kFC) != 0) {
            call(addr);
            return 24;
        }
        return 12;
    };

    // NOP
    iset_.at(0xdd) = [&]() {
        regs.pc++;
        return 4;
    };

    // SBC A,d8
    iset_.at(0xde) = [&]() {
        regs.pc++;
        alu::sbc8(regs.f, regs.a, next8());
        return 8;
    };

    // RST 18H
    iset_.at(0xdf) = [&]() {
        regs.pc++;
        rst(0x18);
        return 16;
    };

    // LDH (a8),A
    iset_.at(0xe0) = [&]() {
        regs.pc++;
        zwrite8(next8(), regs.a);
        return 12;
    };

    // POP HL
    iset_.at(0xe1) = [&]() {
        regs.pc++;
        pop(regs.hl);
        return 12;
    };

    // LD (C),A
    iset_.at(0xe2) = [&]() {
        regs.pc++;
        zwrite8(regs.c, regs.a);
        return 8;
    };

    // NOP
    iset_.at(0xe3) = [&]() {
        regs.pc++;
        return 4;
    };

    // NOP
    iset_.at(0xe4) = [&]() {
        regs.pc++;
        return 4;
    };

    // PUSH HL
    iset_.at(0xe5) = [&]() {
        regs.pc++;
        push(regs.hl);
        return 16;
    };

    // AND d8
    iset_.at(0xe6) = [&]() {
        regs.pc++;
        alu::land(regs.f, regs.a, next8());
        return 8;
    };

    // RST 20H
    iset_.at(0xe7) = [&]() {
        regs.pc++;
        rst(0x20);
        return 16;
    };

    // ADD SP,r8
    iset_.at(0xe8) = [&]() {
        regs.pc++;
        regs.pc += s8(next8());
        return 16;
    };

    // JP (HL)
    iset_.at(0xe9) = [&]() {
        regs.pc++;
        regs.pc = regs.hl;
        return 4;
    };

    // LD (a16),A
    iset_.at(0xea) = [&]() {
        regs.pc++;
        write8(next16(), regs.a);
        return 16;
    };

    // NOP
    iset_.at(0xeb) = [&]() {
        regs.pc++;
        return 4;
    };

    // NOP
    iset_.at(0xec) = [&]() {
        regs.pc++;
        return 4;
    };

    // NOP
    iset_.at(0xed) = [&]() {
        regs.pc++;
        return 4;
    };

    // XOR d8
    iset_.at(0xee) = [&]() {
        regs.pc++;
        alu::lxor(regs.f, regs.a, next8());
        return 8;
    };

    // RST 28H
    iset_.at(0xef) = [&]() {
        regs.pc++;
        rst(0x28);
        return 16;
    };

    // LDH A,(a8)
    iset_.at(0xf0) = [&]() {
        regs.pc++;
        regs.a = zread8(next8());
        return 12;
    };

    // POP AF
    iset_.at(0xf1) = [&]() {
        regs.pc++;
        pop(regs.af);
        return 12;
    };

    // LD A,(C)
    iset_.at(0xf2) = [&]() {
        regs.pc++;
        regs.a = zread8(regs.c);
        return 8;
    };

    // DI
    iset_.at(0xf3) = [&]() {
        regs.pc++;
        regs.ime = 0;
        return 4;
    };

    // NOP
    iset_.at(0xf4) = [&]() {
        regs.pc++;
        return 4;
    };

    // PUSH AF
    iset_.at(0xf5) = [&]() {
        regs.pc++;
        push(regs.af);
        return 16;
    };

    // OR d8
    iset_.at(0xf6) = [&]() {
        regs.pc++;
        alu::lor(regs.f, regs.a, next8());
        return 8;
    };

    // RST 30H
    iset_.at(0xf7) = [&]() {
        regs.pc++;
        rst(0x30);
        return 16;
    };

    // LD HL,SP+r8
    iset_.at(0xf8) = [&]() {
        regs.pc++;
        regs.hl = regs.pc + s8(next8());
        return 12;
    };

    // LD SP,HL
    iset_.at(0xf9) = [&]() {
        regs.pc++;
        regs.sp = regs.hl;
        return 8;
    };

    // LD A,(a16)
    iset_.at(0xfa) = [&]() {
        regs.pc++;
        regs.a = read8(next16());
        return 16;
    };

    // EI
    iset_.at(0xfb) = [&]() {
        regs.pc++;
        regs.ime = 1;
        return 4;
    };

    // NOP
    iset_.at(0xfc) = [&]() {
        regs.pc++;
        return 4;
    };

    // NOP
    iset_.at(0xfd) = [&]() {
        regs.pc++;
        return 4;
    };

    // CP d8
    iset_.at(0xfe) = [&]() {
        regs.pc++;
        alu::lcp(regs.f, regs.a, next8());
        return 8;
    };

    // RST 38H
    iset_.at(0xff) = [&]() {
        regs.pc++;
        rst(0x38);
        return 16;
    };

    // CB PREFIX

    // RLC B
    iset_.at(0x100) = [&]() {
        regs.pc++;
        alu::rlc(regs.f, regs.b);
        return 8;
    };

    // RLC C
    iset_.at(0x101) = [&]() {
        regs.pc++;
        alu::rlc(regs.f, regs.c);
        return 8;
    };

    // RLC D
    iset_.at(0x102) = [&]() {
        regs.pc++;
        alu::rlc(regs.f, regs.d);
        return 8;
    };

    // RLC E
    iset_.at(0x103) = [&]() {
        regs.pc++;
        alu::rlc(regs.f, regs.e);
        return 8;
    };

    // RLC H
    iset_.at(0x104) = [&]() {
        regs.pc++;
        alu::rlc(regs.f, regs.h);
        return 8;
    };

    // RLC L
    iset_.at(0x105) = [&]() {
        regs.pc++;
        alu::rlc(regs.f, regs.l);
        return 8;
    };

    // RLC (HL)
    iset_.at(0x106) = [&]() {
        regs.pc++;
        u8 v = read8(regs.hl);
        alu::rlc(regs.f, v);
        write8(regs.hl, v);
        return 16;
    };

    // RLC A
    iset_.at(0x107) = [&]() {
        regs.pc++;
        alu::rlc(regs.f, regs.a);
        return 8;
    };

    // RRC B
    iset_.at(0x108) = [&]() {
        regs.pc++;
        alu::rrc(regs.f, regs.b);
        return 8;
    };

    // RRC C
    iset_.at(0x109) = [&]() {
        regs.pc++;
        alu::rrc(regs.f, regs.c);
        return 8;
    };

    // RRC D
    iset_.at(0x10a) = [&]() {
        regs.pc++;
        alu::rrc(regs.f, regs.d);
        return 8;
    };

    // RRC E
    iset_.at(0x10b) = [&]() {
        regs.pc++;
        alu::rrc(regs.f, regs.e);
        return 8;
    };

    // RRC H
    iset_.at(0x10c) = [&]() {
        regs.pc++;
        alu::rrc(regs.f, regs.h);
        return 8;
    };

    // RRC L
    iset_.at(0x10d) = [&]() {
        regs.pc++;
        alu::rrc(regs.f, regs.l);
        return 8;
    };

    // RRC (HL)
    iset_.at(0x10e) = [&]() {
        regs.pc++;
        u8 v = read8(regs.hl);
        alu::rrc(regs.f, v);
        write8(regs.hl, v);
        return 16;
    };

    // RRC A
    iset_.at(0x10f) = [&]() {
        regs.pc++;
        alu::rrc(regs.f, regs.a);
        return 8;
    };

    // RL B
    iset_.at(0x110) = [&]() {
        regs.pc++;
        alu::rl(regs.f, regs.b);
        return 8;
    };

    // RL C
    iset_.at(0x111) = [&]() {
        regs.pc++;
        alu::rl(regs.f, regs.c);
        return 8;
    };

    // RL D
    iset_.at(0x112) = [&]() {
        regs.pc++;
        alu::rl(regs.f, regs.d);
        return 8;
    };

    // RL E
    iset_.at(0x113) = [&]() {
        regs.pc++;
        alu::rl(regs.f, regs.e);
        return 8;
    };

    // RL H
    iset_.at(0x114) = [&]() {
        regs.pc++;
        alu::rl(regs.f, regs.h);
        return 8;
    };

    // RL L
    iset_.at(0x115) = [&]() {
        regs.pc++;
        alu::rl(regs.f, regs.l);
        return 8;
    };

    // RL (HL)
    iset_.at(0x116) = [&]() {
        regs.pc++;
        u8 v = read8(regs.hl);
        alu::rl(regs.f, v);
        write8(regs.hl, v);
        return 8;
    };

    // RL A
    iset_.at(0x117) = [&]() {
        regs.pc++;
        alu::rl(regs.f, regs.a);
        return 8;
    };


    // RR B
    iset_.at(0x118) = [&]() {
        regs.pc++;
        alu::rr(regs.f, regs.b);
        return 8;
    };

    // RR C
    iset_.at(0x119) = [&]() {
        regs.pc++;
        alu::rr(regs.f, regs.c);
        return 8;
    };

    // RR D
    iset_.at(0x11a) = [&]() {
        regs.pc++;
        alu::rr(regs.f, regs.d);
        return 8;
    };

    // RR E
    iset_.at(0x11b) = [&]() {
        regs.pc++;
        alu::rr(regs.f, regs.e);
        return 8;
    };

    // RR H
    iset_.at(0x11c) = [&]() {
        regs.pc++;
        alu::rr(regs.f, regs.h);
        return 8;
    };

    // RR L
    iset_.at(0x11d) = [&]() {
        regs.pc++;
        alu::rr(regs.f, regs.l);
        return 8;
    };

    // RR (HL)
    iset_.at(0x11e) = [&]() {
        regs.pc++;
        u8 v = read8(regs.hl);
        alu::rr(regs.f, v);
        write8(regs.hl, v);
        return 16;
    };

    // RR A
    iset_.at(0x11f) = [&]() {
        regs.pc++;
        alu::rr(regs.f, regs.a);
        return 8;
    };

    // SLA B
    iset_.at(0x120) = [&]() {
        regs.pc++;
        alu::sla(regs.f, regs.b);
        return 8;
    };

    // SLA C
    iset_.at(0x121) = [&]() {
        regs.pc++;
        alu::sla(regs.f, regs.c);
        return 8;
    };

    // SLA D
    iset_.at(0x122) = [&]() {
        regs.pc++;
        alu::sla(regs.f, regs.d);
        return 8;
    };

    // SLA E
    iset_.at(0x123) = [&]() {
        regs.pc++;
        alu::sla(regs.f, regs.e);
        return 8;
    };

    // SLA H
    iset_.at(0x124) = [&]() {
        regs.pc++;
        alu::sla(regs.f, regs.h);
        return 8;
    };

    // SLA L
    iset_.at(0x125) = [&]() {
        regs.pc++;
        alu::sla(regs.f, regs.l);
        return 8;
    };

    // SLA (HL)
    iset_.at(0x126) = [&]() {
        regs.pc++;
        u8 v = read8(regs.hl);
        alu::sla(regs.f, v);
        write8(regs.hl, v);
        return 16;
    };

    // SLA A
    iset_.at(0x127) = [&]() {
        regs.pc++;
        alu::sla(regs.f, regs.a);
        return 8;
    };


    // SRA B
    iset_.at(0x128) = [&]() {
        regs.pc++;
        alu::sra(regs.f, regs.b);
        return 8;
    };

    // SRA C
    iset_.at(0x129) = [&]() {
        regs.pc++;
        alu::sra(regs.f, regs.c);
        return 8;
    };

    // SRA D
    iset_.at(0x12a) = [&]() {
        regs.pc++;
        alu::sra(regs.f, regs.d);
        return 8;
    };

    // SRA E
    iset_.at(0x12b) = [&]() {
        regs.pc++;
        alu::sra(regs.f, regs.e);
        return 8;
    };

    // SRA H
    iset_.at(0x12c) = [&]() {
        regs.pc++;
        alu::sra(regs.f, regs.h);
        return 8;
    };

    // SRA L
    iset_.at(0x12d) = [&]() {
        regs.pc++;
        alu::sra(regs.f, regs.l);
        return 8;
    };

    // SRA (HL)
    iset_.at(0x12e) = [&]() {
        regs.pc++;
        u8 v = read8(regs.hl);
        alu::sra(regs.f, v);
        write8(regs.hl, v);
        return 16;
    };

    // SRA A
    iset_.at(0x12f) = [&]() {
        regs.pc++;
        alu::sra(regs.f, regs.b);
        return 8;
    };

    // SWAP B
    iset_.at(0x130) = [&]() {
        regs.pc++;
        alu::swap(regs.f, regs.b);
        return 8;
    };

    // SWAP C
    iset_.at(0x131) = [&]() {
        regs.pc++;
        alu::swap(regs.f, regs.c);
        return 8;
    };

    // SWAP D
    iset_.at(0x132) = [&]() {
        regs.pc++;
        alu::swap(regs.f, regs.d);
        return 8;
    };

    // SWAP E
    iset_.at(0x133) = [&]() {
        regs.pc++;
        alu::swap(regs.f, regs.e);
        return 8;
    };

    // SWAP H
    iset_.at(0x134) = [&]() {
        regs.pc++;
        alu::swap(regs.f, regs.h);
        return 8;
    };

    // SWAP L
    iset_.at(0x135) = [&]() {
        regs.pc++;
        alu::swap(regs.f, regs.l);
        return 8;
    };

    // SWAP (HL)
    iset_.at(0x136) = [&]() {
        regs.pc++;
        u8 v = read8(regs.hl);
        alu::swap(regs.f, v);
        write8(regs.hl, v);
        return 16;
    };

    // SWAP A
    iset_.at(0x137) = [&]() {
        regs.pc++;
        alu::swap(regs.f, regs.a);
        return 8;
    };


    // SRL B
    iset_.at(0x138) = [&]() {
        regs.pc++;
        alu::srl(regs.f, regs.b);
        return 8;
    };

    // SRL C
    iset_.at(0x139) = [&]() {
        regs.pc++;
        alu::srl(regs.f, regs.c);
        return 8;
    };

    // SRL D
    iset_.at(0x13a) = [&]() {
        regs.pc++;
        alu::srl(regs.f, regs.d);
        return 8;
    };

    // SRL E
    iset_.at(0x13b) = [&]() {
        regs.pc++;
        alu::srl(regs.f, regs.e);
        return 8;
    };

    // SRL H
    iset_.at(0x13c) = [&]() {
        regs.pc++;
        alu::srl(regs.f, regs.h);
        return 8;
    };

    // SRL L
    iset_.at(0x13d) = [&]() {
        regs.pc++;
        alu::srl(regs.f, regs.l);
        return 8;
    };

    // SRL (HL)
    iset_.at(0x13e) = [&]() {
        regs.pc++;
        u8 v = read8(regs.hl);
        alu::srl(regs.f, v);
        write8(regs.hl, v);
        return 16;
    };

    // SRL A
    iset_.at(0x13f) = [&]() {
        regs.pc++;
        alu::srl(regs.f, regs.a);
        return 8;
    };

    {
        u16 opcode = 0x140;

        // RES [0x140~0x17F]
        assert(opcode == 0x140);

        for (u8 i = 0; i < 8; i ++) {
            iset_.at(opcode++) = [this,i]() {
                regs.pc++;
                alu::bit(regs.f, regs.b, i);
                return 8;
            };

            iset_.at(opcode++) = [this,i]() {
                regs.pc++;
                alu::bit(regs.f, regs.c, i);
                return 8;
            };

            iset_.at(opcode++) = [this,i]() {
                regs.pc++;
                alu::bit(regs.f, regs.d, i);
                return 8;
            };

            iset_.at(opcode++) = [this,i]() {
                regs.pc++;
                alu::bit(regs.f, regs.e, i);
                return 8;
            };

            iset_.at(opcode++) = [this,i]() {
                regs.pc++;
                alu::bit(regs.f, regs.h, i);
                return 8;
            };

            iset_.at(opcode++) = [this,i]() {
                regs.pc++;
                alu::bit(regs.f, regs.l, i);
                return 8;
            };

            iset_.at(opcode++) = [this,i]() {
                regs.pc++;
                u8 v = read8(regs.hl);
                alu::bit(regs.f, v, i);
                write8(regs.hl, v);
                return 16;
            };

            iset_.at(opcode++) = [this,i]() {
                regs.pc++;
                alu::bit(regs.f, regs.a, i);
                return 8;
            };
        }

        // RES [0x180~0x1BF]
        assert(opcode == 0x180);

        for (u8 i = 0; i < 8; i ++) {
            iset_.at(opcode++) = [this,i]() {
                regs.pc++;
                alu::res(regs.f, regs.b, i);
                return 8;
            };

            iset_.at(opcode++) = [this,i]() {
                regs.pc++;
                alu::res(regs.f, regs.c, i);
                return 8;
            };

            iset_.at(opcode++) = [this,i]() {
                regs.pc++;
                alu::res(regs.f, regs.d, i);
                return 8;
            };

            iset_.at(opcode++) = [this,i]() {
                regs.pc++;
                alu::res(regs.f, regs.e, i);
                return 8;
            };

            iset_.at(opcode++) = [this,i]() {
                regs.pc++;
                alu::res(regs.f, regs.h, i);
                return 8;
            };

            iset_.at(opcode++) = [this,i]() {
                regs.pc++;
                alu::res(regs.f, regs.l, i);
                return 8;
            };

            iset_.at(opcode++) = [this,i]() {
                regs.pc++;
                u8 v = read8(regs.hl);
                alu::res(regs.f, v, i);
                write8(regs.hl, v);
                return 16;
            };

            iset_.at(opcode++) = [this,i]() {
                regs.pc++;
                alu::res(regs.f, regs.a, i);
                return 8;
            };
        }

        // SET [0x1C0~0x1FF]
        assert(opcode == 0x1C0);

        for (u8 i = 0; i < 8; i ++) {
            iset_.at(opcode++) = [this,i]() {
                regs.pc++;
                alu::set(regs.f, regs.b, i);
                return 8;
            };

            iset_.at(opcode++) = [this,i]() {
                regs.pc++;
                alu::set(regs.f, regs.c, i);
                return 8;
            };

            iset_.at(opcode++) = [this,i]() {
                regs.pc++;
                alu::set(regs.f, regs.d, i);
                return 8;
            };

            iset_.at(opcode++) = [this,i]() {
                regs.pc++;
                alu::set(regs.f, regs.e, i);
                return 8;
            };

            iset_.at(opcode++) = [this,i]() {
                regs.pc++;
                alu::set(regs.f, regs.h, i);
                return 8;
            };

            iset_.at(opcode++) = [this,i]() {
                regs.pc++;
                alu::set(regs.f, regs.l, i);
                return 8;
            };

            iset_.at(opcode++) = [this,i]() {
                regs.pc++;
                u8 v = read8(regs.hl);
                alu::set(regs.f, v, i);
                write8(regs.hl, v);
                return 16;
            };

            iset_.at(opcode++) = [this,i]() {
                regs.pc++;
                alu::set(regs.f, regs.a, i);
                return 8;
            };
        }
    }
}