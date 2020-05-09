/*
 * Cpu.cpp
 * Copyright (C) 2020 Emiliano Firmino <emiliano.firmino@gmail.com>
 *
 * Distributed under terms of the MIT license.
 */

#include "cpu.hpp"

#include "address.hpp"
#include "alu.hpp"
#include "interrupt.hpp"
#include "mmu.hpp"

#include <iomanip>
#include <iostream>

using namespace gbg;

Cpu::Cpu(MMU &mmu) : regs(), mmu(mmu), iset_(512, &Cpu::notimpl) {
  populateInstructionSets();
}

ticks_t Cpu::cycle() {
  auto opcode = peek8();               // fetch
  auto instruction = iset_.at(opcode); // decode
  auto ticks = (this->*instruction)(); // execute

  // Interruption handler
  if (regs.ime) {
    auto iflags = mmu.read(Address::HwIoInterruptFlags);
    auto iswitch = mmu.read(Address::HwIoInterruptSwitch);

    if (iflags & iswitch) {
      regs.ime = 0;
      if (iflags & Interrupt::kLcdVerticalBlankingInterrupt) {
        iflags &= ~Interrupt::kLcdVerticalBlankingInterrupt;
        rst(0x0040);
        ticks += 4;
      } else if (iflags & Interrupt::kLcdControllerInterrupt) {
        iflags &= ~Interrupt::kLcdControllerInterrupt;
        rst(0x0048);
        ticks += 4;
      } else if (iflags & Interrupt::kTimerOverflowInterrupt) {
        iflags &= ~Interrupt::kTimerOverflowInterrupt;
        rst(0x0050);
        ticks += 4;
      } else if (iflags & Interrupt::kSerialTransferCompleteInterrupt) {
        iflags &= ~Interrupt::kSerialTransferCompleteInterrupt;
        rst(0x0058);
        ticks += 4;
      } else if (iflags & Interrupt::kJoypadReleaseInterrupt) {
        iflags &= ~Interrupt::kJoypadReleaseInterrupt;
        rst(0x0060);
        ticks += 4;
      } else {
        assert(false);
      }

      mmu.write(Address::HwIoInterruptFlags, iflags);
    }
  }

  return ticks;
}

u8 Cpu::next8() { return read8(regs.pc++); }

u16 Cpu::next16() {
  auto data = read16(regs.pc);
  regs.pc += 2;
  return data;
}

u8 Cpu::peek8() { return read8(regs.pc); }

u16 Cpu::peek16() { return read16(regs.pc); }

u8 Cpu::read8(addr_t a) { return mmu.read(a); }

u16 Cpu::read16(addr_t a) {
  u8 lsb = mmu.read(a);
  u8 hsb = mmu.read(a + 1);
  return (hsb << 8) | lsb;
}

void Cpu::write8(addr_t a, u8 v) { mmu.write(a, v); }

void Cpu::write16(addr_t a, u16 v) {
  mmu.write(a, (v >> 8) & 0xff);
  mmu.write(a + 1, v & 0xff);
}

u8 Cpu::zread8(u8 a) { return read8(0xff00 + a); }

u16 Cpu::zread16(u8 a) { return read16(0xff00 + a); }

void Cpu::zwrite8(u8 a, u8 v) { write8(0xff00 + a, v); }

void Cpu::zwrite16(u8 a, u16 v) { write16(0xff00 + a, v); }

void Cpu::call(addr_t a) {
  push(regs.pc);
  regs.pc = a;
}

void Cpu::rst(addr_t a) {
  push(regs.pc);
  regs.pc = a;
}

void Cpu::ret() { pop(regs.pc); }

void Cpu::push(u16 &reg) {
  u8 hsb = reg >> 8;
  u8 lsb = reg;
  write8(regs.sp--, lsb);
  write8(regs.sp--, hsb);
}

void Cpu::pop(u16 &reg) {
  u8 hsb = read8(++regs.sp);
  u8 lsb = read8(++regs.sp);
  reg = (hsb << 8) | lsb;
}

ticks_t Cpu::notimpl() {
  std::stringstream ss;
  ss << "Not implemented: ";
  ss << std::uppercase << std::hex << std::setfill('0') << std::setw(2);
  ss << static_cast<int>(peek8());

  throw std::runtime_error(ss.str());
  return 0;
}

// NOP
ticks_t Cpu::opcode00() {
  regs.pc++;
  return 4;
}

// LD BC,d16
ticks_t Cpu::opcode01() {
  regs.pc++;
  regs.bc = next16();
  return 12;
}

// LD (BC),A
ticks_t Cpu::opcode02() {
  regs.pc++;
  write8(regs.bc, regs.a);
  return 8;
}

// INC BC
ticks_t Cpu::opcode03() {
  regs.pc++;
  alu::inc16(regs.f, regs.bc);
  return 8;
}

// INC B
ticks_t Cpu::opcode04() {
  regs.pc++;
  alu::inc8(regs.f, regs.b);
  return 4;
}

// DEC B
ticks_t Cpu::opcode05() {
  regs.pc++;
  alu::dec8(regs.f, regs.b);
  return 4;
}

//  LD B,d8
ticks_t Cpu::opcode06() {
  regs.pc++;
  regs.b = next8();
  return 8;
}

// RLCA
ticks_t Cpu::opcode07() {
  regs.pc++;
  alu::rlc(regs.f, regs.a);
  return 4;
}

// LD (a16),SP
ticks_t Cpu::opcode08() {
  regs.pc++;
  u16 addr = next16();
  write16(addr, regs.sp);
  return 20;
}

// ADD HL,BC
ticks_t Cpu::opcode09() {
  regs.pc++;
  alu::add16(regs.f, regs.hl, regs.bc);
  return 8;
}

// LD A,(BC)
ticks_t Cpu::opcode0A() {
  regs.pc++;
  regs.a = read16(regs.bc);
  return 8;
}

// DEC BC
ticks_t Cpu::opcode0B() {
  regs.pc++;
  alu::dec16(regs.f, regs.bc);
  return 8;
}

// INC C
ticks_t Cpu::opcode0C() {
  regs.pc++;
  alu::inc8(regs.f, regs.c);
  return 4;
}

// DEC C
ticks_t Cpu::opcode0D() {
  regs.pc++;
  alu::dec8(regs.f, regs.c);
  return 4;
}

// LD C,d8
ticks_t Cpu::opcode0E() {
  regs.pc++;
  regs.c = next8();
  return 8;
}

// RRCA
ticks_t Cpu::opcode0F() {
  regs.pc++;
  alu::rrc(regs.f, regs.a);
  return 4;
}

// STOP 0
ticks_t Cpu::opcode10() {
  regs.pc++;
  return 4;
}

// LD DE,d16
ticks_t Cpu::opcode11() {
  regs.pc++;
  regs.de = next16();
  return 12;
}

// LD (DE),A
ticks_t Cpu::opcode12() {
  regs.pc++;
  write8(regs.de, regs.a);
  return 8;
}

// INC DE
ticks_t Cpu::opcode13() {
  regs.pc++;
  alu::inc16(regs.f, regs.de);
  return 8;
}

// INC D
ticks_t Cpu::opcode14() {
  regs.pc++;
  alu::inc8(regs.f, regs.d);
  return 4;
}

// DEC D
ticks_t Cpu::opcode15() {
  regs.pc++;
  alu::dec8(regs.f, regs.d);
  return 4;
}

// LD D,d8
ticks_t Cpu::opcode16() {
  regs.pc++;
  regs.d = next8();
  return 8;
}

// RLA
ticks_t Cpu::opcode17() {
  regs.pc++;
  alu::rl(regs.f, regs.a);
  return 4;
}

// JR r8
ticks_t Cpu::opcode18() {
  regs.pc++;

  s8 ref = next8();
  s32 aux = s32(regs.pc) + ref;

  regs.pc = aux & 0xffff;
  return 12;
}

// ADD HL,DE
ticks_t Cpu::opcode19() {
  regs.pc++;
  alu::add16(regs.f, regs.hl, regs.de);
  return 8;
}

// LD A,(DE)
ticks_t Cpu::opcode1A() {
  regs.pc++;
  regs.a = read8(regs.de);
  return 8;
}

// DEC DE
ticks_t Cpu::opcode1B() {
  regs.pc++;
  alu::dec16(regs.f, regs.de);
  return 8;
}

// INC E
ticks_t Cpu::opcode1C() {
  regs.pc++;
  alu::inc8(regs.f, regs.e);
  return 4;
}

// DEC E
ticks_t Cpu::opcode1D() {
  regs.pc++;
  alu::dec8(regs.f, regs.e);
  return 4;
}

// LD E,d8
ticks_t Cpu::opcode1E() {
  regs.pc++;
  regs.e = next8();
  return 8;
}

// RRA
ticks_t Cpu::opcode1F() {
  regs.pc++;
  alu::rr(regs.f, regs.a);
  return 4;
}

// JR NZ,r8
ticks_t Cpu::opcode20() {
  regs.pc++;
  s8 offset = s8(next8());

  if ((regs.f & alu::kFZ) == 0) {
    s32 aux = s32(regs.pc) + offset;
    regs.pc = aux & 0xffff;
    return 12;
  }
  return 8;
}

// LD HL,d16
ticks_t Cpu::opcode21() {
  regs.pc++;
  regs.hl = next16();
  return 12;
}

// LD (HL+),A
ticks_t Cpu::opcode22() {
  regs.pc++;
  write8(regs.hl++, regs.a);
  return 8;
}

// INC HL
ticks_t Cpu::opcode23() {
  regs.pc++;
  alu::inc16(regs.f, regs.hl);
  return 8;
}

// INC H
ticks_t Cpu::opcode24() {
  regs.pc++;
  alu::inc8(regs.f, regs.h);
  return 4;
}

// DEC H
ticks_t Cpu::opcode25() {
  regs.pc++;
  alu::dec8(regs.f, regs.h);
  return 4;
}

// LD H,d8
ticks_t Cpu::opcode26() {
  regs.pc++;
  regs.h = next8();
  return 8;
}

// DAA
ticks_t Cpu::opcode27() {
  regs.pc++;
  alu::daa(regs.f, regs.a);
  return 4;
}

// JR Z,r8
ticks_t Cpu::opcode28() {
  regs.pc++;
  s8 offset = s8(next8());

  if ((regs.f & alu::kFZ) != 0) {
    s32 aux = s32(regs.pc) + offset;
    regs.pc = aux & 0xffff;
    return 12;
  }
  return 8;
}

// ADD HL,HL
ticks_t Cpu::opcode29() {
  regs.pc++;
  alu::add16(regs.f, regs.hl, regs.hl);
  return 8;
}

// LD A,(HL+)
ticks_t Cpu::opcode2A() {
  regs.pc++;
  regs.a = read8(regs.hl++);
  return 8;
}

// DEC HL
ticks_t Cpu::opcode2B() {
  regs.pc++;
  alu::dec16(regs.f, regs.hl);
  return 8;
}

// INC L
ticks_t Cpu::opcode2C() {
  regs.pc++;
  alu::inc8(regs.f, regs.l);
  return 4;
}

// DEC L
ticks_t Cpu::opcode2D() {
  regs.pc++;
  alu::dec8(regs.f, regs.l);
  return 4;
}

// LD L,d8
ticks_t Cpu::opcode2E() {
  regs.pc++;
  regs.l = next8();
  return 8;
}

// CPL
ticks_t Cpu::opcode2F() {
  regs.pc++;
  alu::cpl(regs.f, regs.a);
  return 4;
}

// JR NC,r8
ticks_t Cpu::opcode30() {
  regs.pc++;
  s8 offset = s8(next8());

  if ((regs.f & alu::kFC) == 0) {
    s32 aux = s32(regs.pc) + offset;

    regs.pc = aux & 0xffff;
    return 12;
  }
  return 8;
}

// LD SP,d16
ticks_t Cpu::opcode31() {
  regs.pc++;
  regs.sp = next16();
  return 12;
}

// LD (HL-),A
ticks_t Cpu::opcode32() {
  regs.pc++;
  write8(regs.hl--, regs.a);
  return 8;
}

// INC SP
ticks_t Cpu::opcode33() {
  regs.pc++;
  alu::inc16(regs.f, regs.sp);
  return 8;
}

// INC (HL)
ticks_t Cpu::opcode34() {
  regs.pc++;
  u8 v = read8(regs.hl);
  alu::inc8(regs.f, v);
  write8(regs.hl, v);
  return 12;
}

// DEC (HL)
ticks_t Cpu::opcode35() {
  regs.pc++;
  u8 v = read8(regs.hl);
  alu::dec8(regs.f, v);
  write8(regs.hl, v);
  return 12;
}

// LD (HL),d8
ticks_t Cpu::opcode36() {
  regs.pc++;
  u8 v = next8();
  write8(regs.hl, v);
  return 12;
}

// SCF
ticks_t Cpu::opcode37() {
  regs.pc++;
  alu::scf(regs.f);
  return 4;
}

// JR C,r8
ticks_t Cpu::opcode38() {
  regs.pc++;
  s8 offset = s8(next8());
  if ((regs.f & alu::kFC) != 0) {
    s32 aux = s32(regs.pc) + offset;
    regs.pc = aux & 0xffff;
    return 12;
  }
  return 8;
}

// ADD HL,SP
ticks_t Cpu::opcode39() {
  regs.pc++;
  alu::add16(regs.f, regs.hl, regs.sp);
  return 8;
}

// LD A,(HL-)
ticks_t Cpu::opcode3A() {
  regs.pc++;
  regs.a = read8(regs.hl--);
  return 8;
}

// DEC SP
ticks_t Cpu::opcode3B() {
  regs.pc++;
  alu::dec16(regs.f, regs.sp);
  return 8;
}

// INC A
ticks_t Cpu::opcode3C() {
  regs.pc++;
  alu::inc8(regs.f, regs.a);
  return 4;
}

// DEC A
ticks_t Cpu::opcode3D() {
  regs.pc++;
  alu::dec8(regs.f, regs.a);
  return 4;
}

// LD A,d8
ticks_t Cpu::opcode3E() {
  regs.pc++;
  regs.a = next8();
  return 8;
}

// CCF
ticks_t Cpu::opcode3F() {
  regs.pc++;
  alu::ccf(regs.f);
  return 4;
}

// LD B,B
ticks_t Cpu::opcode40() {
  regs.pc++;
  regs.b = regs.b;
  return 4;
}

// LD B,C
ticks_t Cpu::opcode41() {
  regs.pc++;
  regs.b = regs.c;
  return 4;
}

// LD B,D
ticks_t Cpu::opcode42() {
  regs.pc++;
  regs.b = regs.d;
  return 4;
}

// LD B,E
ticks_t Cpu::opcode43() {
  regs.pc++;
  regs.b = regs.e;
  return 4;
}

// LD B,H
ticks_t Cpu::opcode44() {
  regs.pc++;
  regs.b = regs.h;
  return 4;
}

// LD B,L
ticks_t Cpu::opcode45() {
  regs.pc++;
  regs.b = regs.l;
  return 4;
}

// LD B,(HL)
ticks_t Cpu::opcode46() {
  regs.pc++;
  regs.b = read8(regs.hl);
  return 8;
}

// LD B,A
ticks_t Cpu::opcode47() {
  regs.pc++;
  regs.b = regs.a;
  return 4;
}

// LD C,B
ticks_t Cpu::opcode48() {
  regs.pc++;
  regs.c = regs.b;
  return 4;
}

// LD C,C
ticks_t Cpu::opcode49() {
  regs.pc++;
  regs.c = regs.c;
  return 4;
}

// LD C,D
ticks_t Cpu::opcode4A() {
  regs.pc++;
  regs.c = regs.d;
  return 4;
}

// LD C,E
ticks_t Cpu::opcode4B() {
  regs.pc++;
  regs.c = regs.e;
  return 4;
}

// LD C,H
ticks_t Cpu::opcode4C() {
  regs.pc++;
  regs.c = regs.h;
  return 4;
}

// LD C,L
ticks_t Cpu::opcode4D() {
  regs.pc++;
  regs.c = regs.l;
  return 4;
}

// LD C,(HL)
ticks_t Cpu::opcode4E() {
  regs.pc++;
  regs.c = read8(regs.hl);
  return 8;
}

// LD C,A
ticks_t Cpu::opcode4F() {
  regs.pc++;
  regs.c = regs.a;
  return 4;
}

// LD D,B
ticks_t Cpu::opcode50() {
  regs.pc++;
  regs.d = regs.b;
  return 4;
}

// LD D,C
ticks_t Cpu::opcode51() {
  regs.pc++;
  regs.d = regs.c;
  return 4;
}

// LD D,D
ticks_t Cpu::opcode52() {
  regs.pc++;
  regs.d = regs.d;
  return 4;
}

// LD D,E
ticks_t Cpu::opcode53() {
  regs.pc++;
  regs.d = regs.e;
  return 4;
}

// LD D,H
ticks_t Cpu::opcode54() {
  regs.pc++;
  regs.d = regs.h;
  return 4;
}

// LD D,L
ticks_t Cpu::opcode55() {
  regs.pc++;
  regs.d = regs.l;
  return 4;
}

// LD D,(HL)
ticks_t Cpu::opcode56() {
  regs.pc++;
  regs.d = read8(regs.hl);
  return 8;
}

// LD D,A
ticks_t Cpu::opcode57() {
  regs.pc++;
  regs.d = regs.a;
  return 4;
}

// LD E,B
ticks_t Cpu::opcode58() {
  regs.pc++;
  regs.e = regs.b;
  return 4;
}

// LD E,C
ticks_t Cpu::opcode59() {
  regs.pc++;
  regs.e = regs.c;
  return 4;
}

// LD E,D
ticks_t Cpu::opcode5A() {
  regs.pc++;
  regs.e = regs.d;
  return 4;
}

// LD E,E
ticks_t Cpu::opcode5B() {
  regs.pc++;
  regs.e = regs.e;
  return 4;
}

// LD E,H
ticks_t Cpu::opcode5C() {
  regs.pc++;
  regs.e = regs.h;
  return 4;
}

// LD E,L
ticks_t Cpu::opcode5D() {
  regs.pc++;
  regs.e = regs.l;
  return 4;
}

// LD E,(HL)
ticks_t Cpu::opcode5E() {
  regs.pc++;
  regs.e = read8(regs.hl);
  return 8;
}

// LD E,A
ticks_t Cpu::opcode5F() {
  regs.pc++;
  regs.e = regs.a;
  return 4;
}

// LD H,B
ticks_t Cpu::opcode60() {
  regs.pc++;
  regs.h = regs.b;
  return 4;
}

// LD H,C
ticks_t Cpu::opcode61() {
  regs.pc++;
  regs.h = regs.c;
  return 4;
}

// LD H,D
ticks_t Cpu::opcode62() {
  regs.pc++;
  regs.h = regs.d;
  return 4;
}

// LD H,E
ticks_t Cpu::opcode63() {
  regs.pc++;
  regs.h = regs.e;
  return 4;
}

// LD H,H
ticks_t Cpu::opcode64() {
  regs.pc++;
  regs.h = regs.h;
  return 4;
}

// LD H,L
ticks_t Cpu::opcode65() {
  regs.pc++;
  regs.h = regs.l;
  return 4;
}

// LD H,(HL)
ticks_t Cpu::opcode66() {
  regs.pc++;
  regs.h = read8(regs.hl);
  return 8;
}

// LD H,A
ticks_t Cpu::opcode67() {
  regs.pc++;
  regs.h = regs.a;
  return 4;
}

// LD L,B
ticks_t Cpu::opcode68() {
  regs.pc++;
  regs.l = regs.b;
  return 4;
}

// LD L,C
ticks_t Cpu::opcode69() {
  regs.pc++;
  regs.l = regs.c;
  return 4;
}

// LD L,D
ticks_t Cpu::opcode6A() {
  regs.pc++;
  regs.l = regs.d;
  return 4;
}

// LD L,E
ticks_t Cpu::opcode6B() {
  regs.pc++;
  regs.l = regs.e;
  return 4;
}

// LD L,H
ticks_t Cpu::opcode6C() {
  regs.pc++;
  regs.l = regs.h;
  return 4;
}

// LD L,L
ticks_t Cpu::opcode6D() {
  regs.pc++;
  regs.l = regs.l;
  return 4;
}

// LD L,(HL)
ticks_t Cpu::opcode6E() {
  regs.pc++;
  regs.l = read8(regs.hl);
  return 8;
}

// LD L,A
ticks_t Cpu::opcode6F() {
  regs.pc++;
  regs.l = regs.a;
  return 4;
}

// LD (HL),B
ticks_t Cpu::opcode70() {
  regs.pc++;
  write8(regs.hl, regs.b);
  return 8;
}

// LD (HL),C
ticks_t Cpu::opcode71() {
  regs.pc++;
  write8(regs.hl, regs.c);
  return 8;
}

// LD (HL),D
ticks_t Cpu::opcode72() {
  regs.pc++;
  write8(regs.hl, regs.d);
  return 8;
}

// LD (HL),E
ticks_t Cpu::opcode73() {
  regs.pc++;
  write8(regs.hl, regs.e);
  return 8;
}

// LD (HL),H
ticks_t Cpu::opcode74() {
  regs.pc++;
  write8(regs.hl, regs.h);
  return 8;
}

// LD (HL),L
ticks_t Cpu::opcode75() {
  regs.pc++;
  write8(regs.hl, regs.l);
  return 8;
}

// HALT
ticks_t Cpu::opcode76() {
  // TODO: Detect interruption to resume execution
  return 4;
}

// LD (HL),A
ticks_t Cpu::opcode77() {
  regs.pc++;
  write8(regs.hl, regs.a);
  return 8;
}

// LD A,B
ticks_t Cpu::opcode78() {
  regs.pc++;
  regs.a = regs.b;
  return 4;
}

// LD A,C
ticks_t Cpu::opcode79() {
  regs.pc++;
  regs.a = regs.c;
  return 4;
}

// LD A,D
ticks_t Cpu::opcode7A() {
  regs.pc++;
  regs.a = regs.d;
  return 4;
}

// LD A,E
ticks_t Cpu::opcode7B() {
  regs.pc++;
  regs.a = regs.e;
  return 4;
}

// LD A,H
ticks_t Cpu::opcode7C() {
  regs.pc++;
  regs.a = regs.h;
  return 4;
}

// LD A,L
ticks_t Cpu::opcode7D() {
  regs.pc++;
  regs.a = regs.l;
  return 4;
}

// LD A,(HL)
ticks_t Cpu::opcode7E() {
  regs.pc++;
  regs.a = read8(regs.hl);
  return 8;
}

// LD A,A
ticks_t Cpu::opcode7F() {
  regs.pc++;
  regs.a = regs.a;
  return 4;
}

// ADD A,B
ticks_t Cpu::opcode80() {
  regs.pc++;
  alu::add8(regs.f, regs.a, regs.b);
  return 4;
}

// ADD A,C
ticks_t Cpu::opcode81() {
  regs.pc++;
  alu::add8(regs.f, regs.a, regs.c);
  return 4;
}

// ADD A,D
ticks_t Cpu::opcode82() {
  regs.pc++;
  alu::add8(regs.f, regs.a, regs.d);
  return 4;
}

// ADD A,E
ticks_t Cpu::opcode83() {
  regs.pc++;
  alu::add8(regs.f, regs.a, regs.e);
  return 4;
}

// ADD A,H
ticks_t Cpu::opcode84() {
  regs.pc++;
  alu::add8(regs.f, regs.a, regs.h);
  return 4;
}

// ADD A,L
ticks_t Cpu::opcode85() {
  regs.pc++;
  alu::add8(regs.f, regs.a, regs.l);
  return 4;
}

// ADD A,(HL)
ticks_t Cpu::opcode86() {
  regs.pc++;
  alu::add8(regs.f, regs.a, read8(regs.hl));
  return 8;
}

// ADD A,A
ticks_t Cpu::opcode87() {
  regs.pc++;
  alu::add8(regs.f, regs.a, regs.a);
  return 4;
}

// ADC A,B
ticks_t Cpu::opcode88() {
  regs.pc++;
  alu::adc8(regs.f, regs.a, regs.b);
  return 4;
}

// ADC A,C
ticks_t Cpu::opcode89() {
  regs.pc++;
  alu::adc8(regs.f, regs.a, regs.c);
  return 4;
}

// ADC A,D
ticks_t Cpu::opcode8A() {
  regs.pc++;
  alu::adc8(regs.f, regs.a, regs.d);
  return 4;
}

// ADC A,E
ticks_t Cpu::opcode8B() {
  regs.pc++;
  alu::adc8(regs.f, regs.a, regs.e);
  return 4;
}

// ADC A,H
ticks_t Cpu::opcode8C() {
  regs.pc++;
  alu::adc8(regs.f, regs.a, regs.h);
  return 4;
}

// ADC A,L
ticks_t Cpu::opcode8D() {
  regs.pc++;
  alu::adc8(regs.f, regs.a, regs.l);
  return 4;
}

// ADC A,(HL)
ticks_t Cpu::opcode8E() {
  regs.pc++;
  alu::adc8(regs.f, regs.a, read8(regs.hl));
  return 8;
}

// ADC A,A
ticks_t Cpu::opcode8F() {
  regs.pc++;
  alu::adc8(regs.f, regs.a, regs.a);
  return 4;
}

// SUB B
ticks_t Cpu::opcode90() {
  regs.pc++;
  alu::sub8(regs.f, regs.a, regs.b);
  return 4;
}

// SUB C
ticks_t Cpu::opcode91() {
  regs.pc++;
  alu::sub8(regs.f, regs.a, regs.c);
  return 4;
}

// SUB D
ticks_t Cpu::opcode92() {
  regs.pc++;
  alu::sub8(regs.f, regs.a, regs.d);
  return 4;
}

// SUB E
ticks_t Cpu::opcode93() {
  regs.pc++;
  alu::sub8(regs.f, regs.a, regs.e);
  return 4;
}

// SUB H
ticks_t Cpu::opcode94() {
  regs.pc++;
  alu::sub8(regs.f, regs.a, regs.h);
  return 4;
}

// SUB L
ticks_t Cpu::opcode95() {
  regs.pc++;
  alu::sub8(regs.f, regs.a, regs.l);
  return 4;
}

// SUB (HL)
ticks_t Cpu::opcode96() {
  regs.pc++;
  alu::sub8(regs.f, regs.a, read8(regs.hl));
  return 4;
}

// SUB A
ticks_t Cpu::opcode97() {
  regs.pc++;
  alu::sub8(regs.f, regs.a, regs.a);
  return 4;
}

// SBC A,B
ticks_t Cpu::opcode98() {
  regs.pc++;
  alu::sbc8(regs.f, regs.a, regs.b);
  return 4;
}

// SBC A,C
ticks_t Cpu::opcode99() {
  regs.pc++;
  alu::sbc8(regs.f, regs.a, regs.c);
  return 4;
}

// SBC A,D
ticks_t Cpu::opcode9A() {
  regs.pc++;
  alu::sbc8(regs.f, regs.a, regs.d);
  return 4;
}

// SBC A,E
ticks_t Cpu::opcode9B() {
  regs.pc++;
  alu::sbc8(regs.f, regs.a, regs.e);
  return 4;
}

// SBC A,H
ticks_t Cpu::opcode9C() {
  regs.pc++;
  alu::sbc8(regs.f, regs.a, regs.h);
  return 4;
}

// SBC A,L
ticks_t Cpu::opcode9D() {
  regs.pc++;
  alu::sbc8(regs.f, regs.a, regs.l);
  return 4;
}

// SBC A,(HL)
ticks_t Cpu::opcode9E() {
  regs.pc++;
  alu::sbc8(regs.f, regs.a, read8(regs.hl));
  return 8;
}

// SBC A,A
ticks_t Cpu::opcode9F() {
  regs.pc++;
  alu::sbc8(regs.f, regs.a, regs.a);
  return 4;
}

// AND B
ticks_t Cpu::opcodeA0() {
  regs.pc++;
  alu::land(regs.f, regs.a, regs.b);
  return 4;
}

// AND C
ticks_t Cpu::opcodeA1() {
  regs.pc++;
  alu::land(regs.f, regs.a, regs.c);
  return 4;
}

// AND D
ticks_t Cpu::opcodeA2() {
  regs.pc++;
  alu::land(regs.f, regs.a, regs.d);
  return 4;
}

// AND E
ticks_t Cpu::opcodeA3() {
  regs.pc++;
  alu::land(regs.f, regs.a, regs.e);
  return 4;
}

// AND H
ticks_t Cpu::opcodeA4() {
  regs.pc++;
  alu::land(regs.f, regs.a, regs.h);
  return 4;
}

// AND L
ticks_t Cpu::opcodeA5() {
  regs.pc++;
  alu::land(regs.f, regs.a, regs.l);
  return 4;
}

// AND (HL)
ticks_t Cpu::opcodeA6() {
  regs.pc++;
  alu::land(regs.f, regs.a, read8(regs.hl));
  return 8;
}

// AND A
ticks_t Cpu::opcodeA7() {
  regs.pc++;
  alu::land(regs.f, regs.a, regs.a);
  return 4;
}

// XOR B
ticks_t Cpu::opcodeA8() {
  regs.pc++;
  alu::lxor(regs.f, regs.a, regs.b);
  return 4;
}

// XOR C
ticks_t Cpu::opcodeA9() {
  regs.pc++;
  alu::lxor(regs.f, regs.a, regs.c);
  return 4;
}

// XOR D
ticks_t Cpu::opcodeAA() {
  regs.pc++;
  alu::lxor(regs.f, regs.a, regs.d);
  return 4;
}

// XOR E
ticks_t Cpu::opcodeAB() {
  regs.pc++;
  alu::lxor(regs.f, regs.a, regs.e);
  return 4;
}

// XOR H
ticks_t Cpu::opcodeAC() {
  regs.pc++;
  alu::lxor(regs.f, regs.a, regs.h);
  return 4;
}

// XOR L
ticks_t Cpu::opcodeAD() {
  regs.pc++;
  alu::lxor(regs.f, regs.a, regs.l);
  return 4;
}

// XOR (HL)
ticks_t Cpu::opcodeAE() {
  regs.pc++;
  alu::lxor(regs.f, regs.a, read8(regs.hl));
  return 8;
}

// XOR A
ticks_t Cpu::opcodeAF() {
  regs.pc++;
  alu::lxor(regs.f, regs.a, regs.a);
  return 4;
}

// OR B
ticks_t Cpu::opcodeB0() {
  regs.pc++;
  alu::lor(regs.f, regs.a, regs.b);
  return 4;
}

// OR C
ticks_t Cpu::opcodeB1() {
  regs.pc++;
  alu::lor(regs.f, regs.a, regs.c);
  return 4;
}

// OR D
ticks_t Cpu::opcodeB2() {
  regs.pc++;
  alu::lor(regs.f, regs.a, regs.d);
  return 4;
}

// OR E
ticks_t Cpu::opcodeB3() {
  regs.pc++;
  alu::lor(regs.f, regs.a, regs.e);
  return 4;
}

// OR H
ticks_t Cpu::opcodeB4() {
  regs.pc++;
  alu::lor(regs.f, regs.a, regs.h);
  return 4;
}

// OR L
ticks_t Cpu::opcodeB5() {
  regs.pc++;
  alu::lor(regs.f, regs.a, regs.l);
  return 4;
}

// OR (HL)
ticks_t Cpu::opcodeB6() {
  regs.pc++;
  alu::lor(regs.f, regs.a, read8(regs.hl));
  return 8;
}

// OR A
ticks_t Cpu::opcodeB7() {
  regs.pc++;
  alu::lor(regs.f, regs.a, regs.a);
  return 4;
}

// CP B
ticks_t Cpu::opcodeB8() {
  regs.pc++;
  alu::lcp(regs.f, regs.a, regs.b);
  return 4;
}

// CP C
ticks_t Cpu::opcodeB9() {
  regs.pc++;
  alu::lcp(regs.f, regs.a, regs.c);
  return 4;
}

// CP D
ticks_t Cpu::opcodeBA() {
  regs.pc++;
  alu::lcp(regs.f, regs.a, regs.d);
  return 4;
}

// CP E
ticks_t Cpu::opcodeBB() {
  regs.pc++;
  alu::lcp(regs.f, regs.a, regs.e);
  return 4;
}

// CP H
ticks_t Cpu::opcodeBC() {
  regs.pc++;
  alu::lcp(regs.f, regs.a, regs.h);
  return 4;
}

// CP L
ticks_t Cpu::opcodeBD() {
  regs.pc++;
  alu::lcp(regs.f, regs.a, regs.l);
  return 4;
}

// CP (HL)
ticks_t Cpu::opcodeBE() {
  regs.pc++;
  alu::lcp(regs.f, regs.a, read8(regs.hl));
  return 8;
}

// CP A
ticks_t Cpu::opcodeBF() {
  regs.pc++;
  alu::lcp(regs.f, regs.a, regs.a);
  return 4;
}

// RET NZ
ticks_t Cpu::opcodeC0() {
  regs.pc++;
  if ((regs.f & alu::kFZ) == 0) {
    ret();
    return 20;
  }
  return 8;
}

// POP BC
ticks_t Cpu::opcodeC1() {
  regs.pc++;
  pop(regs.bc);
  return 12;
}

// JP NZ,a16
ticks_t Cpu::opcodeC2() {
  regs.pc++;
  u16 addr = next16();
  if ((regs.f & alu::kFZ) == 0) {
    regs.pc = addr;
    return 16;
  }
  return 12;
}

// JP a16
ticks_t Cpu::opcodeC3() {
  regs.pc++;
  regs.pc = next16();
  return 12;
}

// CALL NZ,a16
ticks_t Cpu::opcodeC4() {
  regs.pc++;
  u16 addr = next16();
  if ((regs.f & alu::kFZ) == 0) {
    call(addr);
    return 24;
  }
  return 12;
}

// PUSH BC
ticks_t Cpu::opcodeC5() {
  regs.pc++;
  push(regs.bc);
  return 16;
}

// ADD A,d8
ticks_t Cpu::opcodeC6() {
  regs.pc++;
  alu::add8(regs.f, regs.a, next8());
  return 8;
}

// RST 00H
ticks_t Cpu::opcodeC7() {
  regs.pc++;
  rst(0x00);
  return 16;
}

// RET Z
ticks_t Cpu::opcodeC8() {
  regs.pc++;
  if ((regs.f & alu::kFZ) != 0) {
    ret();
    return 20;
  }
  return 8;
}

// RET
ticks_t Cpu::opcodeC9() {
  regs.pc++;
  ret();
  return 16;
}

// JP Z,a16
ticks_t Cpu::opcodeCA() {
  regs.pc++;
  u16 addr = next16();
  if ((regs.f & alu::kFZ) != 0) {
    regs.pc = addr;
    return 16;
  }
  return 12;
}

// PREFIX CB
ticks_t Cpu::opcodeCB() {
  regs.pc++;

  auto opcode = peek8() + 0x100;       // fetch
  auto instruction = iset_.at(opcode); // decode
  return 4 + (this->*instruction)();   // execute
}

// CALL Z,a16
ticks_t Cpu::opcodeCC() {
  regs.pc++;
  u16 addr = next16();
  if ((regs.f & alu::kFZ) != 0) {
    call(addr);
    return 12;
  }
  return 8;
}

// CALL a16
ticks_t Cpu::opcodeCD() {
  regs.pc++;
  call(next16());
  return 8;
}

// ADC A,d8
ticks_t Cpu::opcodeCE() {
  regs.pc++;
  alu::adc8(regs.f, regs.a, next8());
  return 8;
}

// RST 08H
ticks_t Cpu::opcodeCF() {
  regs.pc++;
  rst(0x08);
  return 16;
}

// RET NC
ticks_t Cpu::opcodeD0() {
  regs.pc++;
  if ((regs.f & alu::kFC) == 0) {
    ret();
    return 20;
  }
  return 8;
}

// POP DE
ticks_t Cpu::opcodeD1() {
  regs.pc++;
  pop(regs.de);
  return 12;
}

// JP NC,a16
ticks_t Cpu::opcodeD2() {
  regs.pc++;
  u16 addr = next16();
  if ((regs.f & alu::kFC) == 0) {
    regs.pc = addr;
    return 16;
  }
  return 12;
}

// NOP
ticks_t Cpu::opcodeD3() {
  regs.pc++;
  return 4;
}

// CALL NC,a16
ticks_t Cpu::opcodeD4() {
  regs.pc++;
  u16 addr = next16();
  if ((regs.f & alu::kFC) == 0) {
    call(addr);
    return 24;
  }
  return 12;
}

// PUSH DE
ticks_t Cpu::opcodeD5() {
  regs.pc++;
  push(regs.de);
  return 16;
}

// SUB d8
ticks_t Cpu::opcodeD6() {
  regs.pc++;
  alu::sub8(regs.f, regs.a, next8());
  return 8;
}

// RST 10H
ticks_t Cpu::opcodeD7() {
  regs.pc++;
  rst(0x10);
  return 16;
}

// RET C
ticks_t Cpu::opcodeD8() {
  regs.pc++;
  if ((regs.f & alu::kFC) != 0) {
    ret();
    return 20;
  }
  return 8;
}

// RETI
ticks_t Cpu::opcodeD9() {
  regs.pc++;
  ret();
  regs.ime = 1;
  return 16;
}

// JP C,a16
ticks_t Cpu::opcodeDA() {
  regs.pc++;
  u16 addr = next16();
  if ((regs.f & alu::kFC) != 0) {
    regs.pc = addr;
    return 16;
  }
  return 12;
}

// NOP
ticks_t Cpu::opcodeDB() {
  regs.pc++;
  return 4;
}

// CALL C,a16
ticks_t Cpu::opcodeDC() {
  regs.pc++;
  u16 addr = next16();
  if ((regs.f & alu::kFC) != 0) {
    call(addr);
    return 24;
  }
  return 12;
}

// NOP
ticks_t Cpu::opcodeDD() {
  regs.pc++;
  return 4;
}

// SBC A,d8
ticks_t Cpu::opcodeDE() {
  regs.pc++;
  alu::sbc8(regs.f, regs.a, next8());
  return 8;
}

// RST 18H
ticks_t Cpu::opcodeDF() {
  regs.pc++;
  rst(0x18);
  return 16;
}

// LDH (a8),A
ticks_t Cpu::opcodeE0() {
  regs.pc++;
  zwrite8(next8(), regs.a);
  return 12;
}

// POP HL
ticks_t Cpu::opcodeE1() {
  regs.pc++;
  pop(regs.hl);
  return 12;
}

// LD (C),A
ticks_t Cpu::opcodeE2() {
  regs.pc++;
  zwrite8(regs.c, regs.a);
  return 8;
}

// NOP
ticks_t Cpu::opcodeE3() {
  regs.pc++;
  return 4;
}

// NOP
ticks_t Cpu::opcodeE4() {
  regs.pc++;
  return 4;
}

// PUSH HL
ticks_t Cpu::opcodeE5() {
  regs.pc++;
  push(regs.hl);
  return 16;
}

// AND d8
ticks_t Cpu::opcodeE6() {
  regs.pc++;
  alu::land(regs.f, regs.a, next8());
  return 8;
}

// RST 20H
ticks_t Cpu::opcodeE7() {
  regs.pc++;
  rst(0x20);
  return 16;
}

// ADD SP,r8
ticks_t Cpu::opcodeE8() {
  regs.pc++;

  s8 value = next8();

  s32 aux = s32(regs.pc) + value;

  regs.pc = aux & 0xffff;
  return 16;
}

// JP (HL)
ticks_t Cpu::opcodeE9() {
  regs.pc++;
  regs.pc = regs.hl;
  return 4;
}

// LD (a16),A
ticks_t Cpu::opcodeEA() {
  regs.pc++;
  write8(next16(), regs.a);
  return 16;
}

// NOP
ticks_t Cpu::opcodeEB() {
  regs.pc++;
  return 4;
}

// NOP
ticks_t Cpu::opcodeEC() {
  regs.pc++;
  return 4;
}

// NOP
ticks_t Cpu::opcodeED() {
  regs.pc++;
  return 4;
}

// XOR d8
ticks_t Cpu::opcodeEE() {
  regs.pc++;
  alu::lxor(regs.f, regs.a, next8());
  return 8;
}

// RST 28H
ticks_t Cpu::opcodeEF() {
  regs.pc++;
  rst(0x28);
  return 16;
}

// LDH A,(a8)
ticks_t Cpu::opcodeF0() {
  regs.pc++;
  regs.a = zread8(next8());
  return 12;
}

// POP AF
ticks_t Cpu::opcodeF1() {
  regs.pc++;
  pop(regs.af);
  return 12;
}

// LD A,(C)
ticks_t Cpu::opcodeF2() {
  regs.pc++;
  regs.a = zread8(regs.c);
  return 8;
}

// DI
ticks_t Cpu::opcodeF3() {
  regs.pc++;
  regs.ime = 0;
  return 4;
}

// NOP
ticks_t Cpu::opcodeF4() {
  regs.pc++;
  return 4;
}

// PUSH AF
ticks_t Cpu::opcodeF5() {
  regs.pc++;
  push(regs.af);
  return 16;
}

// OR d8
ticks_t Cpu::opcodeF6() {
  regs.pc++;
  alu::lor(regs.f, regs.a, next8());
  return 8;
}

// RST 30H
ticks_t Cpu::opcodeF7() {
  regs.pc++;
  rst(0x30);
  return 16;
}

// LD HL,SP+r8
ticks_t Cpu::opcodeF8() {
  regs.pc++;
  s8 value = next8();
  s32 aux = s32(regs.sp) + value;

  regs.hl = aux & 0xffff;
  return 12;
}

// LD SP,HL
ticks_t Cpu::opcodeF9() {
  regs.pc++;
  regs.sp = regs.hl;
  return 8;
}

// LD A,(a16)
ticks_t Cpu::opcodeFA() {
  regs.pc++;
  regs.a = read8(next16());
  return 16;
}

// EI
ticks_t Cpu::opcodeFB() {
  regs.pc++;
  regs.ime = 1;
  return 4;
}

// NOP
ticks_t Cpu::opcodeFC() {
  regs.pc++;
  return 4;
}

// NOP
ticks_t Cpu::opcodeFD() {
  regs.pc++;
  return 4;
}

// CP d8
ticks_t Cpu::opcodeFE() {
  regs.pc++;
  alu::lcp(regs.f, regs.a, next8());
  return 8;
}

// RST 38H
ticks_t Cpu::opcodeFF() {
  regs.pc++;
  rst(0x38);
  return 16;
}

// RLC B
ticks_t Cpu::opcodeCB00() {
  regs.pc++;
  alu::rlc(regs.f, regs.b);
  return 8;
}

// RLC C
ticks_t Cpu::opcodeCB01() {
  regs.pc++;
  alu::rlc(regs.f, regs.c);
  return 8;
}

// RLC D
ticks_t Cpu::opcodeCB02() {
  regs.pc++;
  alu::rlc(regs.f, regs.d);
  return 8;
}

// RLC E
ticks_t Cpu::opcodeCB03() {
  regs.pc++;
  alu::rlc(regs.f, regs.e);
  return 8;
}

// RLC H
ticks_t Cpu::opcodeCB04() {
  regs.pc++;
  alu::rlc(regs.f, regs.h);
  return 8;
}

// RLC L
ticks_t Cpu::opcodeCB05() {
  regs.pc++;
  alu::rlc(regs.f, regs.l);
  return 8;
}

// RLC (HL)
ticks_t Cpu::opcodeCB06() {
  regs.pc++;
  u8 v = read8(regs.hl);
  alu::rlc(regs.f, v);
  write8(regs.hl, v);
  return 16;
}

// RLC A
ticks_t Cpu::opcodeCB07() {
  regs.pc++;
  alu::rlc(regs.f, regs.a);
  return 8;
}

// RRC B
ticks_t Cpu::opcodeCB08() {
  regs.pc++;
  alu::rrc(regs.f, regs.b);
  return 8;
}

// RRC C
ticks_t Cpu::opcodeCB09() {
  regs.pc++;
  alu::rrc(regs.f, regs.c);
  return 8;
}

// RRC D
ticks_t Cpu::opcodeCB0A() {
  regs.pc++;
  alu::rrc(regs.f, regs.d);
  return 8;
}

// RRC E
ticks_t Cpu::opcodeCB0B() {
  regs.pc++;
  alu::rrc(regs.f, regs.e);
  return 8;
}

// RRC H
ticks_t Cpu::opcodeCB0C() {
  regs.pc++;
  alu::rrc(regs.f, regs.h);
  return 8;
}

// RRC L
ticks_t Cpu::opcodeCB0D() {
  regs.pc++;
  alu::rrc(regs.f, regs.l);
  return 8;
}

// RRC (HL)
ticks_t Cpu::opcodeCB0E() {
  regs.pc++;
  u8 v = read8(regs.hl);
  alu::rrc(regs.f, v);
  write8(regs.hl, v);
  return 16;
}

// RRC A
ticks_t Cpu::opcodeCB0F() {
  regs.pc++;
  alu::rrc(regs.f, regs.a);
  return 8;
}

// RL B
ticks_t Cpu::opcodeCB10() {
  regs.pc++;
  alu::rl(regs.f, regs.b);
  return 8;
}

// RL C
ticks_t Cpu::opcodeCB11() {
  regs.pc++;
  alu::rl(regs.f, regs.c);
  return 8;
}

// RL D
ticks_t Cpu::opcodeCB12() {
  regs.pc++;
  alu::rl(regs.f, regs.d);
  return 8;
}

// RL E
ticks_t Cpu::opcodeCB13() {
  regs.pc++;
  alu::rl(regs.f, regs.e);
  return 8;
}

// RL H
ticks_t Cpu::opcodeCB14() {
  regs.pc++;
  alu::rl(regs.f, regs.h);
  return 8;
}

// RL L
ticks_t Cpu::opcodeCB15() {
  regs.pc++;
  alu::rl(regs.f, regs.l);
  return 8;
}

// RL (HL)
ticks_t Cpu::opcodeCB16() {
  regs.pc++;
  u8 v = read8(regs.hl);
  alu::rl(regs.f, v);
  write8(regs.hl, v);
  return 8;
}

// RL A
ticks_t Cpu::opcodeCB17() {
  regs.pc++;
  alu::rl(regs.f, regs.a);
  return 8;
}

// RR B
ticks_t Cpu::opcodeCB18() {
  regs.pc++;
  alu::rr(regs.f, regs.b);
  return 8;
}

// RR C
ticks_t Cpu::opcodeCB19() {
  regs.pc++;
  alu::rr(regs.f, regs.c);
  return 8;
}

// RR D
ticks_t Cpu::opcodeCB1A() {
  regs.pc++;
  alu::rr(regs.f, regs.d);
  return 8;
}

// RR E
ticks_t Cpu::opcodeCB1B() {
  regs.pc++;
  alu::rr(regs.f, regs.e);
  return 8;
}

// RR H
ticks_t Cpu::opcodeCB1C() {
  regs.pc++;
  alu::rr(regs.f, regs.h);
  return 8;
}

// RR L
ticks_t Cpu::opcodeCB1D() {
  regs.pc++;
  alu::rr(regs.f, regs.l);
  return 8;
}

// RR (HL)
ticks_t Cpu::opcodeCB1E() {
  regs.pc++;
  u8 v = read8(regs.hl);
  alu::rr(regs.f, v);
  write8(regs.hl, v);
  return 16;
}

// RR A
ticks_t Cpu::opcodeCB1F() {
  regs.pc++;
  alu::rr(regs.f, regs.a);
  return 8;
}

// SLA B
ticks_t Cpu::opcodeCB20() {
  regs.pc++;
  alu::sla(regs.f, regs.b);
  return 8;
}

// SLA C
ticks_t Cpu::opcodeCB21() {
  regs.pc++;
  alu::sla(regs.f, regs.c);
  return 8;
}

// SLA D
ticks_t Cpu::opcodeCB22() {
  regs.pc++;
  alu::sla(regs.f, regs.d);
  return 8;
}

// SLA E
ticks_t Cpu::opcodeCB23() {
  regs.pc++;
  alu::sla(regs.f, regs.e);
  return 8;
}

// SLA H
ticks_t Cpu::opcodeCB24() {
  regs.pc++;
  alu::sla(regs.f, regs.h);
  return 8;
}

// SLA L
ticks_t Cpu::opcodeCB25() {
  regs.pc++;
  alu::sla(regs.f, regs.l);
  return 8;
}

// SLA (HL)
ticks_t Cpu::opcodeCB26() {
  regs.pc++;
  u8 v = read8(regs.hl);
  alu::sla(regs.f, v);
  write8(regs.hl, v);
  return 16;
}

// SLA A
ticks_t Cpu::opcodeCB27() {
  regs.pc++;
  alu::sla(regs.f, regs.a);
  return 8;
}

// SRA B
ticks_t Cpu::opcodeCB28() {
  regs.pc++;
  alu::sra(regs.f, regs.b);
  return 8;
}

// SRA C
ticks_t Cpu::opcodeCB29() {
  regs.pc++;
  alu::sra(regs.f, regs.c);
  return 8;
}

// SRA D
ticks_t Cpu::opcodeCB2A() {
  regs.pc++;
  alu::sra(regs.f, regs.d);
  return 8;
}

// SRA E
ticks_t Cpu::opcodeCB2B() {
  regs.pc++;
  alu::sra(regs.f, regs.e);
  return 8;
}

// SRA H
ticks_t Cpu::opcodeCB2C() {
  regs.pc++;
  alu::sra(regs.f, regs.h);
  return 8;
}

// SRA L
ticks_t Cpu::opcodeCB2D() {
  regs.pc++;
  alu::sra(regs.f, regs.l);
  return 8;
}

// SRA (HL)
ticks_t Cpu::opcodeCB2E() {
  regs.pc++;
  u8 v = read8(regs.hl);
  alu::sra(regs.f, v);
  write8(regs.hl, v);
  return 16;
}

// SRA A
ticks_t Cpu::opcodeCB2F() {
  regs.pc++;
  alu::sra(regs.f, regs.b);
  return 8;
}

// SWAP B
ticks_t Cpu::opcodeCB30() {
  regs.pc++;
  alu::swap(regs.f, regs.b);
  return 8;
}

// SWAP C
ticks_t Cpu::opcodeCB31() {
  regs.pc++;
  alu::swap(regs.f, regs.c);
  return 8;
}

// SWAP D
ticks_t Cpu::opcodeCB32() {
  regs.pc++;
  alu::swap(regs.f, regs.d);
  return 8;
}

// SWAP E
ticks_t Cpu::opcodeCB33() {
  regs.pc++;
  alu::swap(regs.f, regs.e);
  return 8;
}

// SWAP H
ticks_t Cpu::opcodeCB34() {
  regs.pc++;
  alu::swap(regs.f, regs.h);
  return 8;
}

// SWAP L
ticks_t Cpu::opcodeCB35() {
  regs.pc++;
  alu::swap(regs.f, regs.l);
  return 8;
}

// SWAP (HL)
ticks_t Cpu::opcodeCB36() {
  regs.pc++;
  u8 v = read8(regs.hl);
  alu::swap(regs.f, v);
  write8(regs.hl, v);
  return 16;
}

// SWAP A
ticks_t Cpu::opcodeCB37() {
  regs.pc++;
  alu::swap(regs.f, regs.a);
  return 8;
}

// SRL B
ticks_t Cpu::opcodeCB38() {
  regs.pc++;
  alu::srl(regs.f, regs.b);
  return 8;
}

// SRL C
ticks_t Cpu::opcodeCB39() {
  regs.pc++;
  alu::srl(regs.f, regs.c);
  return 8;
}

// SRL D
ticks_t Cpu::opcodeCB3A() {
  regs.pc++;
  alu::srl(regs.f, regs.d);
  return 8;
}

// SRL E
ticks_t Cpu::opcodeCB3B() {
  regs.pc++;
  alu::srl(regs.f, regs.e);
  return 8;
}

// SRL H
ticks_t Cpu::opcodeCB3C() {
  regs.pc++;
  alu::srl(regs.f, regs.h);
  return 8;
}

// SRL L
ticks_t Cpu::opcodeCB3D() {
  regs.pc++;
  alu::srl(regs.f, regs.l);
  return 8;
}

// SRL (HL)
ticks_t Cpu::opcodeCB3E() {
  regs.pc++;
  u8 v = read8(regs.hl);
  alu::srl(regs.f, v);
  write8(regs.hl, v);
  return 16;
}

// SRL A
ticks_t Cpu::opcodeCB3F() {
  regs.pc++;
  alu::srl(regs.f, regs.a);
  return 8;
}

// BIT 0,B
ticks_t Cpu::opcodeCB40() {
  regs.pc++;
  alu::bit(regs.f, regs.b, 0);
  return 8;
}

// BIT 0,C
ticks_t Cpu::opcodeCB41() {
  regs.pc++;
  alu::bit(regs.f, regs.c, 0);
  return 8;
}

// BIT 0,D
ticks_t Cpu::opcodeCB42() {
  regs.pc++;
  alu::bit(regs.f, regs.d, 0);
  return 8;
}

// BIT 0,E
ticks_t Cpu::opcodeCB43() {
  regs.pc++;
  alu::bit(regs.f, regs.e, 0);
  return 8;
}

// BIT 0,H
ticks_t Cpu::opcodeCB44() {
  regs.pc++;
  alu::bit(regs.f, regs.h, 0);
  return 8;
}

// BIT 0,L
ticks_t Cpu::opcodeCB45() {
  regs.pc++;
  alu::bit(regs.f, regs.l, 0);
  return 8;
}

// BIT 0,(HL)
ticks_t Cpu::opcodeCB46() {
  regs.pc++;
  u8 v = read8(regs.hl);
  alu::bit(regs.f, v, 0);
  write8(regs.hl, v);
  return 16;
}

// BIT 0,A
ticks_t Cpu::opcodeCB47() {
  regs.pc++;
  alu::bit(regs.f, regs.a, 0);
  return 8;
}

// BIT 1,B
ticks_t Cpu::opcodeCB48() {
  regs.pc++;
  alu::bit(regs.f, regs.b, 1);
  return 8;
}

// BIT 1,C
ticks_t Cpu::opcodeCB49() {
  regs.pc++;
  alu::bit(regs.f, regs.c, 1);
  return 8;
}

// BIT 1,D
ticks_t Cpu::opcodeCB4A() {
  regs.pc++;
  alu::bit(regs.f, regs.d, 1);
  return 8;
}

// BIT 1,E
ticks_t Cpu::opcodeCB4B() {
  regs.pc++;
  alu::bit(regs.f, regs.e, 1);
  return 8;
}

// BIT 1,H
ticks_t Cpu::opcodeCB4C() {
  regs.pc++;
  alu::bit(regs.f, regs.h, 1);
  return 8;
}

// BIT 1,L
ticks_t Cpu::opcodeCB4D() {
  regs.pc++;
  alu::bit(regs.f, regs.l, 1);
  return 8;
}

// BIT 1,(HL)
ticks_t Cpu::opcodeCB4E() {
  regs.pc++;
  u8 v = read8(regs.hl);
  alu::bit(regs.f, v, 1);
  write8(regs.hl, v);
  return 16;
}

// BIT 1,A
ticks_t Cpu::opcodeCB4F() {
  regs.pc++;
  alu::bit(regs.f, regs.a, 1);
  return 8;
}

// BIT 2,B
ticks_t Cpu::opcodeCB50() {
  regs.pc++;
  alu::bit(regs.f, regs.b, 2);
  return 8;
}

// BIT 2,C
ticks_t Cpu::opcodeCB51() {
  regs.pc++;
  alu::bit(regs.f, regs.c, 2);
  return 8;
}

// BIT 2,D
ticks_t Cpu::opcodeCB52() {
  regs.pc++;
  alu::bit(regs.f, regs.d, 2);
  return 8;
}

// BIT 2,E
ticks_t Cpu::opcodeCB53() {
  regs.pc++;
  alu::bit(regs.f, regs.e, 2);
  return 8;
}

// BIT 2,H
ticks_t Cpu::opcodeCB54() {
  regs.pc++;
  alu::bit(regs.f, regs.h, 2);
  return 8;
}

// BIT 2,L
ticks_t Cpu::opcodeCB55() {
  regs.pc++;
  alu::bit(regs.f, regs.l, 2);
  return 8;
}

// BIT 2,(HL)
ticks_t Cpu::opcodeCB56() {
  regs.pc++;
  u8 v = read8(regs.hl);
  alu::bit(regs.f, v, 2);
  write8(regs.hl, v);
  return 16;
}

// BIT 2,A
ticks_t Cpu::opcodeCB57() {
  regs.pc++;
  alu::bit(regs.f, regs.a, 2);
  return 8;
}

// BIT 3,B
ticks_t Cpu::opcodeCB58() {
  regs.pc++;
  alu::bit(regs.f, regs.b, 3);
  return 8;
}

// BIT 3,C
ticks_t Cpu::opcodeCB59() {
  regs.pc++;
  alu::bit(regs.f, regs.c, 3);
  return 8;
}

// BIT 3,D
ticks_t Cpu::opcodeCB5A() {
  regs.pc++;
  alu::bit(regs.f, regs.d, 3);
  return 8;
}

// BIT 3,E
ticks_t Cpu::opcodeCB5B() {
  regs.pc++;
  alu::bit(regs.f, regs.e, 3);
  return 8;
}

// BIT 3,H
ticks_t Cpu::opcodeCB5C() {
  regs.pc++;
  alu::bit(regs.f, regs.h, 3);
  return 8;
}

// BIT 3,L
ticks_t Cpu::opcodeCB5D() {
  regs.pc++;
  alu::bit(regs.f, regs.l, 3);
  return 8;
}

// BIT 3,(HL)
ticks_t Cpu::opcodeCB5E() {
  regs.pc++;
  u8 v = read8(regs.hl);
  alu::bit(regs.f, v, 3);
  write8(regs.hl, v);
  return 16;
}

// BIT 3,A
ticks_t Cpu::opcodeCB5F() {
  regs.pc++;
  alu::bit(regs.f, regs.a, 3);
  return 8;
}

// BIT 4,B
ticks_t Cpu::opcodeCB60() {
  regs.pc++;
  alu::bit(regs.f, regs.b, 4);
  return 8;
}

// BIT 4,C
ticks_t Cpu::opcodeCB61() {
  regs.pc++;
  alu::bit(regs.f, regs.c, 4);
  return 8;
}

// BIT 4,D
ticks_t Cpu::opcodeCB62() {
  regs.pc++;
  alu::bit(regs.f, regs.d, 4);
  return 8;
}

// BIT 4,E
ticks_t Cpu::opcodeCB63() {
  regs.pc++;
  alu::bit(regs.f, regs.e, 4);
  return 8;
}

// BIT 4,H
ticks_t Cpu::opcodeCB64() {
  regs.pc++;
  alu::bit(regs.f, regs.h, 4);
  return 8;
}

// BIT 4,L
ticks_t Cpu::opcodeCB65() {
  regs.pc++;
  alu::bit(regs.f, regs.l, 4);
  return 8;
}

// BIT 4,(HL)
ticks_t Cpu::opcodeCB66() {
  regs.pc++;
  u8 v = read8(regs.hl);
  alu::bit(regs.f, v, 4);
  write8(regs.hl, v);
  return 16;
}

// BIT 4,A
ticks_t Cpu::opcodeCB67() {
  regs.pc++;
  alu::bit(regs.f, regs.a, 4);
  return 8;
}

// BIT 5,B
ticks_t Cpu::opcodeCB68() {
  regs.pc++;
  alu::bit(regs.f, regs.b, 5);
  return 8;
}

// BIT 5,C
ticks_t Cpu::opcodeCB69() {
  regs.pc++;
  alu::bit(regs.f, regs.c, 5);
  return 8;
}

// BIT 5,D
ticks_t Cpu::opcodeCB6A() {
  regs.pc++;
  alu::bit(regs.f, regs.d, 5);
  return 8;
}

// BIT 5,E
ticks_t Cpu::opcodeCB6B() {
  regs.pc++;
  alu::bit(regs.f, regs.e, 5);
  return 8;
}

// BIT 5,H
ticks_t Cpu::opcodeCB6C() {
  regs.pc++;
  alu::bit(regs.f, regs.h, 5);
  return 8;
}

// BIT 5,L
ticks_t Cpu::opcodeCB6D() {
  regs.pc++;
  alu::bit(regs.f, regs.l, 5);
  return 8;
}

// BIT 5,(HL)
ticks_t Cpu::opcodeCB6E() {
  regs.pc++;
  u8 v = read8(regs.hl);
  alu::bit(regs.f, v, 5);
  write8(regs.hl, v);
  return 16;
}

// BIT 5,A
ticks_t Cpu::opcodeCB6F() {
  regs.pc++;
  alu::bit(regs.f, regs.a, 5);
  return 8;
}

// BIT 6,B
ticks_t Cpu::opcodeCB70() {
  regs.pc++;
  alu::bit(regs.f, regs.b, 6);
  return 8;
}

// BIT 6,C
ticks_t Cpu::opcodeCB71() {
  regs.pc++;
  alu::bit(regs.f, regs.c, 6);
  return 8;
}

// BIT 6,D
ticks_t Cpu::opcodeCB72() {
  regs.pc++;
  alu::bit(regs.f, regs.d, 6);
  return 8;
}

// BIT 6,E
ticks_t Cpu::opcodeCB73() {
  regs.pc++;
  alu::bit(regs.f, regs.e, 6);
  return 8;
}

// BIT 6,H
ticks_t Cpu::opcodeCB74() {
  regs.pc++;
  alu::bit(regs.f, regs.h, 6);
  return 8;
}

// BIT 6,L
ticks_t Cpu::opcodeCB75() {
  regs.pc++;
  alu::bit(regs.f, regs.l, 6);
  return 8;
}

// BIT 6,(HL)
ticks_t Cpu::opcodeCB76() {
  regs.pc++;
  u8 v = read8(regs.hl);
  alu::bit(regs.f, v, 6);
  write8(regs.hl, v);
  return 16;
}

// BIT 6,A
ticks_t Cpu::opcodeCB77() {
  regs.pc++;
  alu::bit(regs.f, regs.a, 6);
  return 8;
}

// BIT 7,B
ticks_t Cpu::opcodeCB78() {
  regs.pc++;
  alu::bit(regs.f, regs.b, 7);
  return 8;
}

// BIT 7,C
ticks_t Cpu::opcodeCB79() {
  regs.pc++;
  alu::bit(regs.f, regs.c, 7);
  return 8;
}

// BIT 7,D
ticks_t Cpu::opcodeCB7A() {
  regs.pc++;
  alu::bit(regs.f, regs.d, 7);
  return 8;
}

// BIT 7,E
ticks_t Cpu::opcodeCB7B() {
  regs.pc++;
  alu::bit(regs.f, regs.e, 7);
  return 8;
}

// BIT 7,H
ticks_t Cpu::opcodeCB7C() {
  regs.pc++;
  alu::bit(regs.f, regs.h, 7);
  return 8;
}

// BIT 7,L
ticks_t Cpu::opcodeCB7D() {
  regs.pc++;
  alu::bit(regs.f, regs.l, 7);
  return 8;
}

// BIT 7,(HL)
ticks_t Cpu::opcodeCB7E() {
  regs.pc++;
  u8 v = read8(regs.hl);
  alu::bit(regs.f, v, 7);
  write8(regs.hl, v);
  return 16;
}

// BIT 7,A
ticks_t Cpu::opcodeCB7F() {
  regs.pc++;
  alu::bit(regs.f, regs.a, 7);
  return 8;
}

// RES 0,B
ticks_t Cpu::opcodeCB80() {
  regs.pc++;
  alu::res(regs.f, regs.b, 0);
  return 8;
}

// RES 0,C
ticks_t Cpu::opcodeCB81() {
  regs.pc++;
  alu::res(regs.f, regs.c, 0);
  return 8;
}

// RES 0,D
ticks_t Cpu::opcodeCB82() {
  regs.pc++;
  alu::res(regs.f, regs.d, 0);
  return 8;
}

// RES 0,E
ticks_t Cpu::opcodeCB83() {
  regs.pc++;
  alu::res(regs.f, regs.e, 0);
  return 8;
}

// RES 0,H
ticks_t Cpu::opcodeCB84() {
  regs.pc++;
  alu::res(regs.f, regs.h, 0);
  return 8;
}

// RES 0,L
ticks_t Cpu::opcodeCB85() {
  regs.pc++;
  alu::res(regs.f, regs.l, 0);
  return 8;
}

// RES 0,(HL)
ticks_t Cpu::opcodeCB86() {
  regs.pc++;
  u8 v = read8(regs.hl);
  alu::res(regs.f, v, 0);
  write8(regs.hl, v);
  return 16;
}

// RES 0,A
ticks_t Cpu::opcodeCB87() {
  regs.pc++;
  alu::res(regs.f, regs.a, 0);
  return 8;
}

// RES 1,B
ticks_t Cpu::opcodeCB88() {
  regs.pc++;
  alu::res(regs.f, regs.b, 1);
  return 8;
}

// RES 1,C
ticks_t Cpu::opcodeCB89() {
  regs.pc++;
  alu::res(regs.f, regs.c, 1);
  return 8;
}

// RES 1,D
ticks_t Cpu::opcodeCB8A() {
  regs.pc++;
  alu::res(regs.f, regs.d, 1);
  return 8;
}

// RES 1,E
ticks_t Cpu::opcodeCB8B() {
  regs.pc++;
  alu::res(regs.f, regs.e, 1);
  return 8;
}

// RES 1,H
ticks_t Cpu::opcodeCB8C() {
  regs.pc++;
  alu::res(regs.f, regs.h, 1);
  return 8;
}

// RES 1,L
ticks_t Cpu::opcodeCB8D() {
  regs.pc++;
  alu::res(regs.f, regs.l, 1);
  return 8;
}

// RES 1,(HL)
ticks_t Cpu::opcodeCB8E() {
  regs.pc++;
  u8 v = read8(regs.hl);
  alu::res(regs.f, v, 1);
  write8(regs.hl, v);
  return 16;
}

// RES 1,A
ticks_t Cpu::opcodeCB8F() {
  regs.pc++;
  alu::res(regs.f, regs.a, 1);
  return 8;
}

// RES 2,B
ticks_t Cpu::opcodeCB90() {
  regs.pc++;
  alu::res(regs.f, regs.b, 2);
  return 8;
}

// RES 2,C
ticks_t Cpu::opcodeCB91() {
  regs.pc++;
  alu::res(regs.f, regs.c, 2);
  return 8;
}

// RES 2,D
ticks_t Cpu::opcodeCB92() {
  regs.pc++;
  alu::res(regs.f, regs.d, 2);
  return 8;
}

// RES 2,E
ticks_t Cpu::opcodeCB93() {
  regs.pc++;
  alu::res(regs.f, regs.e, 2);
  return 8;
}

// RES 2,H
ticks_t Cpu::opcodeCB94() {
  regs.pc++;
  alu::res(regs.f, regs.h, 2);
  return 8;
}

// RES 2,L
ticks_t Cpu::opcodeCB95() {
  regs.pc++;
  alu::res(regs.f, regs.l, 2);
  return 8;
}

// RES 2,(HL)
ticks_t Cpu::opcodeCB96() {
  regs.pc++;
  u8 v = read8(regs.hl);
  alu::res(regs.f, v, 2);
  write8(regs.hl, v);
  return 16;
}

// RES 2,A
ticks_t Cpu::opcodeCB97() {
  regs.pc++;
  alu::res(regs.f, regs.a, 2);
  return 8;
}

// RES 3,B
ticks_t Cpu::opcodeCB98() {
  regs.pc++;
  alu::res(regs.f, regs.b, 3);
  return 8;
}

// RES 3,C
ticks_t Cpu::opcodeCB99() {
  regs.pc++;
  alu::res(regs.f, regs.c, 3);
  return 8;
}

// RES 3,D
ticks_t Cpu::opcodeCB9A() {
  regs.pc++;
  alu::res(regs.f, regs.d, 3);
  return 8;
}

// RES 3,E
ticks_t Cpu::opcodeCB9B() {
  regs.pc++;
  alu::res(regs.f, regs.e, 3);
  return 8;
}

// RES 3,H
ticks_t Cpu::opcodeCB9C() {
  regs.pc++;
  alu::res(regs.f, regs.h, 3);
  return 8;
}

// RES 3,L
ticks_t Cpu::opcodeCB9D() {
  regs.pc++;
  alu::res(regs.f, regs.l, 3);
  return 8;
}

// RES 3,(HL)
ticks_t Cpu::opcodeCB9E() {
  regs.pc++;
  u8 v = read8(regs.hl);
  alu::res(regs.f, v, 3);
  write8(regs.hl, v);
  return 16;
}

// RES 3,A
ticks_t Cpu::opcodeCB9F() {
  regs.pc++;
  alu::res(regs.f, regs.a, 3);
  return 8;
}

// RES 4,B
ticks_t Cpu::opcodeCBA0() {
  regs.pc++;
  alu::res(regs.f, regs.b, 4);
  return 8;
}

// RES 4,C
ticks_t Cpu::opcodeCBA1() {
  regs.pc++;
  alu::res(regs.f, regs.c, 4);
  return 8;
}

// RES 4,D
ticks_t Cpu::opcodeCBA2() {
  regs.pc++;
  alu::res(regs.f, regs.d, 4);
  return 8;
}

// RES 4,E
ticks_t Cpu::opcodeCBA3() {
  regs.pc++;
  alu::res(regs.f, regs.e, 4);
  return 8;
}

// RES 4,H
ticks_t Cpu::opcodeCBA4() {
  regs.pc++;
  alu::res(regs.f, regs.h, 4);
  return 8;
}

// RES 4,L
ticks_t Cpu::opcodeCBA5() {
  regs.pc++;
  alu::res(regs.f, regs.l, 4);
  return 8;
}

// RES 4,(HL)
ticks_t Cpu::opcodeCBA6() {
  regs.pc++;
  u8 v = read8(regs.hl);
  alu::res(regs.f, v, 4);
  write8(regs.hl, v);
  return 16;
}

// RES 4,A
ticks_t Cpu::opcodeCBA7() {
  regs.pc++;
  alu::res(regs.f, regs.a, 4);
  return 8;
}

// RES 5,B
ticks_t Cpu::opcodeCBA8() {
  regs.pc++;
  alu::res(regs.f, regs.b, 5);
  return 8;
}

// RES 5,C
ticks_t Cpu::opcodeCBA9() {
  regs.pc++;
  alu::res(regs.f, regs.c, 5);
  return 8;
}

// RES 5,D
ticks_t Cpu::opcodeCBAA() {
  regs.pc++;
  alu::res(regs.f, regs.d, 5);
  return 8;
}

// RES 5,E
ticks_t Cpu::opcodeCBAB() {
  regs.pc++;
  alu::res(regs.f, regs.e, 5);
  return 8;
}

// RES 5,H
ticks_t Cpu::opcodeCBAC() {
  regs.pc++;
  alu::res(regs.f, regs.h, 5);
  return 8;
}

// RES 5,L
ticks_t Cpu::opcodeCBAD() {
  regs.pc++;
  alu::res(regs.f, regs.l, 5);
  return 8;
}

// RES 5,(HL)
ticks_t Cpu::opcodeCBAE() {
  regs.pc++;
  u8 v = read8(regs.hl);
  alu::res(regs.f, v, 5);
  write8(regs.hl, v);
  return 16;
}

// RES 5,A
ticks_t Cpu::opcodeCBAF() {
  regs.pc++;
  alu::res(regs.f, regs.a, 5);
  return 8;
}

// RES 6,B
ticks_t Cpu::opcodeCBB0() {
  regs.pc++;
  alu::res(regs.f, regs.b, 6);
  return 8;
}

// RES 6,C
ticks_t Cpu::opcodeCBB1() {
  regs.pc++;
  alu::res(regs.f, regs.c, 6);
  return 8;
}

// RES 6,D
ticks_t Cpu::opcodeCBB2() {
  regs.pc++;
  alu::res(regs.f, regs.d, 6);
  return 8;
}

// RES 6,E
ticks_t Cpu::opcodeCBB3() {
  regs.pc++;
  alu::res(regs.f, regs.e, 6);
  return 8;
}

// RES 6,H
ticks_t Cpu::opcodeCBB4() {
  regs.pc++;
  alu::res(regs.f, regs.h, 6);
  return 8;
}

// RES 6,L
ticks_t Cpu::opcodeCBB5() {
  regs.pc++;
  alu::res(regs.f, regs.l, 6);
  return 8;
}

// RES 6,(HL)
ticks_t Cpu::opcodeCBB6() {
  regs.pc++;
  u8 v = read8(regs.hl);
  alu::res(regs.f, v, 6);
  write8(regs.hl, v);
  return 16;
}

// RES 6,A
ticks_t Cpu::opcodeCBB7() {
  regs.pc++;
  alu::res(regs.f, regs.a, 6);
  return 8;
}

// RES 7,B
ticks_t Cpu::opcodeCBB8() {
  regs.pc++;
  alu::res(regs.f, regs.b, 7);
  return 8;
}

// RES 7,C
ticks_t Cpu::opcodeCBB9() {
  regs.pc++;
  alu::res(regs.f, regs.c, 7);
  return 8;
}

// RES 7,D
ticks_t Cpu::opcodeCBBA() {
  regs.pc++;
  alu::res(regs.f, regs.d, 7);
  return 8;
}

// RES 7,E
ticks_t Cpu::opcodeCBBB() {
  regs.pc++;
  alu::res(regs.f, regs.e, 7);
  return 8;
}

// RES 7,H
ticks_t Cpu::opcodeCBBC() {
  regs.pc++;
  alu::res(regs.f, regs.h, 7);
  return 8;
}

// RES 7,L
ticks_t Cpu::opcodeCBBD() {
  regs.pc++;
  alu::res(regs.f, regs.l, 7);
  return 8;
}

// RES 7,(HL)
ticks_t Cpu::opcodeCBBE() {
  regs.pc++;
  u8 v = read8(regs.hl);
  alu::res(regs.f, v, 7);
  write8(regs.hl, v);
  return 16;
}

// RES 7,A
ticks_t Cpu::opcodeCBBF() {
  regs.pc++;
  alu::res(regs.f, regs.a, 7);
  return 8;
}

// SET 0,B
ticks_t Cpu::opcodeCBC0() {
  regs.pc++;
  alu::set(regs.f, regs.b, 0);
  return 8;
}

// SET 0,C
ticks_t Cpu::opcodeCBC1() {
  regs.pc++;
  alu::set(regs.f, regs.c, 0);
  return 8;
}

// SET 0,D
ticks_t Cpu::opcodeCBC2() {
  regs.pc++;
  alu::set(regs.f, regs.d, 0);
  return 8;
}

// SET 0,E
ticks_t Cpu::opcodeCBC3() {
  regs.pc++;
  alu::set(regs.f, regs.e, 0);
  return 8;
}

// SET 0,H
ticks_t Cpu::opcodeCBC4() {
  regs.pc++;
  alu::set(regs.f, regs.h, 0);
  return 8;
}

// SET 0,L
ticks_t Cpu::opcodeCBC5() {
  regs.pc++;
  alu::set(regs.f, regs.l, 0);
  return 8;
}

// SET 0,(HL)
ticks_t Cpu::opcodeCBC6() {
  regs.pc++;
  u8 v = read8(regs.hl);
  alu::set(regs.f, v, 0);
  write8(regs.hl, v);
  return 16;
}

// SET 0,A
ticks_t Cpu::opcodeCBC7() {
  regs.pc++;
  alu::set(regs.f, regs.a, 0);
  return 8;
}

// SET 1,B
ticks_t Cpu::opcodeCBC8() {
  regs.pc++;
  alu::set(regs.f, regs.b, 1);
  return 8;
}

// SET 1,C
ticks_t Cpu::opcodeCBC9() {
  regs.pc++;
  alu::set(regs.f, regs.c, 1);
  return 8;
}

// SET 1,D
ticks_t Cpu::opcodeCBCA() {
  regs.pc++;
  alu::set(regs.f, regs.d, 1);
  return 8;
}

// SET 1,E
ticks_t Cpu::opcodeCBCB() {
  regs.pc++;
  alu::set(regs.f, regs.e, 1);
  return 8;
}

// SET 1,H
ticks_t Cpu::opcodeCBCC() {
  regs.pc++;
  alu::set(regs.f, regs.h, 1);
  return 8;
}

// SET 1,L
ticks_t Cpu::opcodeCBCD() {
  regs.pc++;
  alu::set(regs.f, regs.l, 1);
  return 8;
}

// SET 1,(HL)
ticks_t Cpu::opcodeCBCE() {
  regs.pc++;
  u8 v = read8(regs.hl);
  alu::set(regs.f, v, 1);
  write8(regs.hl, v);
  return 16;
}

// SET 1,A
ticks_t Cpu::opcodeCBCF() {
  regs.pc++;
  alu::set(regs.f, regs.a, 1);
  return 8;
}

// SET 2,B
ticks_t Cpu::opcodeCBD0() {
  regs.pc++;
  alu::set(regs.f, regs.b, 2);
  return 8;
}

// SET 2,C
ticks_t Cpu::opcodeCBD1() {
  regs.pc++;
  alu::set(regs.f, regs.c, 2);
  return 8;
}

// SET 2,D
ticks_t Cpu::opcodeCBD2() {
  regs.pc++;
  alu::set(regs.f, regs.d, 2);
  return 8;
}

// SET 2,E
ticks_t Cpu::opcodeCBD3() {
  regs.pc++;
  alu::set(regs.f, regs.e, 2);
  return 8;
}

// SET 2,H
ticks_t Cpu::opcodeCBD4() {
  regs.pc++;
  alu::set(regs.f, regs.h, 2);
  return 8;
}

// SET 2,L
ticks_t Cpu::opcodeCBD5() {
  regs.pc++;
  alu::set(regs.f, regs.l, 2);
  return 8;
}

// SET 2,(HL)
ticks_t Cpu::opcodeCBD6() {
  regs.pc++;
  u8 v = read8(regs.hl);
  alu::set(regs.f, v, 2);
  write8(regs.hl, v);
  return 16;
}

// SET 2,A
ticks_t Cpu::opcodeCBD7() {
  regs.pc++;
  alu::set(regs.f, regs.a, 2);
  return 8;
}

// SET 3,B
ticks_t Cpu::opcodeCBD8() {
  regs.pc++;
  alu::set(regs.f, regs.b, 3);
  return 8;
}

// SET 3,C
ticks_t Cpu::opcodeCBD9() {
  regs.pc++;
  alu::set(regs.f, regs.c, 3);
  return 8;
}

// SET 3,D
ticks_t Cpu::opcodeCBDA() {
  regs.pc++;
  alu::set(regs.f, regs.d, 3);
  return 8;
}

// SET 3,E
ticks_t Cpu::opcodeCBDB() {
  regs.pc++;
  alu::set(regs.f, regs.e, 3);
  return 8;
}

// SET 3,H
ticks_t Cpu::opcodeCBDC() {
  regs.pc++;
  alu::set(regs.f, regs.h, 3);
  return 8;
}

// SET 3,L
ticks_t Cpu::opcodeCBDD() {
  regs.pc++;
  alu::set(regs.f, regs.l, 3);
  return 8;
}

// SET 3,(HL)
ticks_t Cpu::opcodeCBDE() {
  regs.pc++;
  u8 v = read8(regs.hl);
  alu::set(regs.f, v, 3);
  write8(regs.hl, v);
  return 16;
}

// SET 3,A
ticks_t Cpu::opcodeCBDF() {
  regs.pc++;
  alu::set(regs.f, regs.a, 3);
  return 8;
}

// SET 4,B
ticks_t Cpu::opcodeCBE0() {
  regs.pc++;
  alu::set(regs.f, regs.b, 4);
  return 8;
}

// SET 4,C
ticks_t Cpu::opcodeCBE1() {
  regs.pc++;
  alu::set(regs.f, regs.c, 4);
  return 8;
}

// SET 4,D
ticks_t Cpu::opcodeCBE2() {
  regs.pc++;
  alu::set(regs.f, regs.d, 4);
  return 8;
}

// SET 4,E
ticks_t Cpu::opcodeCBE3() {
  regs.pc++;
  alu::set(regs.f, regs.e, 4);
  return 8;
}

// SET 4,H
ticks_t Cpu::opcodeCBE4() {
  regs.pc++;
  alu::set(regs.f, regs.h, 4);
  return 8;
}

// SET 4,L
ticks_t Cpu::opcodeCBE5() {
  regs.pc++;
  alu::set(regs.f, regs.l, 4);
  return 8;
}

// SET 4,(HL)
ticks_t Cpu::opcodeCBE6() {
  regs.pc++;
  u8 v = read8(regs.hl);
  alu::set(regs.f, v, 4);
  write8(regs.hl, v);
  return 16;
}

// SET 4,A
ticks_t Cpu::opcodeCBE7() {
  regs.pc++;
  alu::set(regs.f, regs.a, 4);
  return 8;
}

// SET 5,B
ticks_t Cpu::opcodeCBE8() {
  regs.pc++;
  alu::set(regs.f, regs.b, 5);
  return 8;
}

// SET 5,C
ticks_t Cpu::opcodeCBE9() {
  regs.pc++;
  alu::set(regs.f, regs.c, 5);
  return 8;
}

// SET 5,D
ticks_t Cpu::opcodeCBEA() {
  regs.pc++;
  alu::set(regs.f, regs.d, 5);
  return 8;
}

// SET 5,E
ticks_t Cpu::opcodeCBEB() {
  regs.pc++;
  alu::set(regs.f, regs.e, 5);
  return 8;
}

// SET 5,H
ticks_t Cpu::opcodeCBEC() {
  regs.pc++;
  alu::set(regs.f, regs.h, 5);
  return 8;
}

// SET 5,L
ticks_t Cpu::opcodeCBED() {
  regs.pc++;
  alu::set(regs.f, regs.l, 5);
  return 8;
}

// SET 5,(HL)
ticks_t Cpu::opcodeCBEE() {
  regs.pc++;
  u8 v = read8(regs.hl);
  alu::set(regs.f, v, 5);
  write8(regs.hl, v);
  return 16;
}

// SET 5,A
ticks_t Cpu::opcodeCBEF() {
  regs.pc++;
  alu::set(regs.f, regs.a, 5);
  return 8;
}

// SET 6,B
ticks_t Cpu::opcodeCBF0() {
  regs.pc++;
  alu::set(regs.f, regs.b, 6);
  return 8;
}

// SET 6,C
ticks_t Cpu::opcodeCBF1() {
  regs.pc++;
  alu::set(regs.f, regs.c, 6);
  return 8;
}

// SET 6,D
ticks_t Cpu::opcodeCBF2() {
  regs.pc++;
  alu::set(regs.f, regs.d, 6);
  return 8;
}

// SET 6,E
ticks_t Cpu::opcodeCBF3() {
  regs.pc++;
  alu::set(regs.f, regs.e, 6);
  return 8;
}

// SET 6,H
ticks_t Cpu::opcodeCBF4() {
  regs.pc++;
  alu::set(regs.f, regs.h, 6);
  return 8;
}

// SET 6,L
ticks_t Cpu::opcodeCBF5() {
  regs.pc++;
  alu::set(regs.f, regs.l, 6);
  return 8;
}

// SET 6,(HL)
ticks_t Cpu::opcodeCBF6() {
  regs.pc++;
  u8 v = read8(regs.hl);
  alu::set(regs.f, v, 6);
  write8(regs.hl, v);
  return 16;
}

// SET 6,A
ticks_t Cpu::opcodeCBF7() {
  regs.pc++;
  alu::set(regs.f, regs.a, 6);
  return 8;
}

// SET 7,B
ticks_t Cpu::opcodeCBF8() {
  regs.pc++;
  alu::set(regs.f, regs.b, 7);
  return 8;
}

// SET 7,C
ticks_t Cpu::opcodeCBF9() {
  regs.pc++;
  alu::set(regs.f, regs.c, 7);
  return 8;
}

// SET 7,D
ticks_t Cpu::opcodeCBFA() {
  regs.pc++;
  alu::set(regs.f, regs.d, 7);
  return 8;
}

// SET 7,E
ticks_t Cpu::opcodeCBFB() {
  regs.pc++;
  alu::set(regs.f, regs.e, 7);
  return 8;
}

// SET 7,H
ticks_t Cpu::opcodeCBFC() {
  regs.pc++;
  alu::set(regs.f, regs.h, 7);
  return 8;
}

// SET 7,L
ticks_t Cpu::opcodeCBFD() {
  regs.pc++;
  alu::set(regs.f, regs.l, 7);
  return 8;
}

// SET 7,(HL)
ticks_t Cpu::opcodeCBFE() {
  regs.pc++;
  u8 v = read8(regs.hl);
  alu::set(regs.f, v, 7);
  write8(regs.hl, v);
  return 16;
}

// SET 7,A
ticks_t Cpu::opcodeCBFF() {
  regs.pc++;
  alu::set(regs.f, regs.a, 7);
  return 8;
}

void Cpu::populateInstructionSets() {
  iset_.at(0x00) = &Cpu::opcode00;
  iset_.at(0x01) = &Cpu::opcode01;
  iset_.at(0x02) = &Cpu::opcode02;
  iset_.at(0x03) = &Cpu::opcode03;
  iset_.at(0x04) = &Cpu::opcode04;
  iset_.at(0x05) = &Cpu::opcode05;
  iset_.at(0x06) = &Cpu::opcode06;
  iset_.at(0x07) = &Cpu::opcode07;
  iset_.at(0x08) = &Cpu::opcode08;
  iset_.at(0x09) = &Cpu::opcode09;
  iset_.at(0x0a) = &Cpu::opcode0A;
  iset_.at(0x0b) = &Cpu::opcode0B;
  iset_.at(0x0c) = &Cpu::opcode0C;
  iset_.at(0x0d) = &Cpu::opcode0D;
  iset_.at(0x0e) = &Cpu::opcode0E;
  iset_.at(0x0f) = &Cpu::opcode0F;
  iset_.at(0x10) = &Cpu::opcode10;
  iset_.at(0x11) = &Cpu::opcode11;
  iset_.at(0x12) = &Cpu::opcode12;
  iset_.at(0x13) = &Cpu::opcode13;
  iset_.at(0x14) = &Cpu::opcode14;
  iset_.at(0x15) = &Cpu::opcode15;
  iset_.at(0x16) = &Cpu::opcode16;
  iset_.at(0x17) = &Cpu::opcode17;
  iset_.at(0x18) = &Cpu::opcode18;
  iset_.at(0x19) = &Cpu::opcode19;
  iset_.at(0x1a) = &Cpu::opcode1A;
  iset_.at(0x1b) = &Cpu::opcode1B;
  iset_.at(0x1c) = &Cpu::opcode1C;
  iset_.at(0x1d) = &Cpu::opcode1D;
  iset_.at(0x1e) = &Cpu::opcode1E;
  iset_.at(0x1f) = &Cpu::opcode1F;
  iset_.at(0x20) = &Cpu::opcode20;
  iset_.at(0x21) = &Cpu::opcode21;
  iset_.at(0x22) = &Cpu::opcode22;
  iset_.at(0x23) = &Cpu::opcode23;
  iset_.at(0x24) = &Cpu::opcode24;
  iset_.at(0x25) = &Cpu::opcode25;
  iset_.at(0x26) = &Cpu::opcode26;
  iset_.at(0x27) = &Cpu::opcode27;
  iset_.at(0x28) = &Cpu::opcode28;
  iset_.at(0x29) = &Cpu::opcode29;
  iset_.at(0x2a) = &Cpu::opcode2A;
  iset_.at(0x2b) = &Cpu::opcode2B;
  iset_.at(0x2c) = &Cpu::opcode2C;
  iset_.at(0x2d) = &Cpu::opcode2D;
  iset_.at(0x2e) = &Cpu::opcode2E;
  iset_.at(0x2f) = &Cpu::opcode2F;
  iset_.at(0x30) = &Cpu::opcode30;
  iset_.at(0x31) = &Cpu::opcode31;
  iset_.at(0x32) = &Cpu::opcode32;
  iset_.at(0x33) = &Cpu::opcode33;
  iset_.at(0x34) = &Cpu::opcode34;
  iset_.at(0x35) = &Cpu::opcode35;
  iset_.at(0x36) = &Cpu::opcode36;
  iset_.at(0x37) = &Cpu::opcode37;
  iset_.at(0x38) = &Cpu::opcode38;
  iset_.at(0x39) = &Cpu::opcode39;
  iset_.at(0x3a) = &Cpu::opcode3A;
  iset_.at(0x3b) = &Cpu::opcode3B;
  iset_.at(0x3c) = &Cpu::opcode3C;
  iset_.at(0x3d) = &Cpu::opcode3D;
  iset_.at(0x3e) = &Cpu::opcode3E;
  iset_.at(0x3f) = &Cpu::opcode3F;
  iset_.at(0x40) = &Cpu::opcode40;
  iset_.at(0x41) = &Cpu::opcode41;
  iset_.at(0x42) = &Cpu::opcode42;
  iset_.at(0x43) = &Cpu::opcode43;
  iset_.at(0x44) = &Cpu::opcode44;
  iset_.at(0x45) = &Cpu::opcode45;
  iset_.at(0x46) = &Cpu::opcode46;
  iset_.at(0x47) = &Cpu::opcode47;
  iset_.at(0x48) = &Cpu::opcode48;
  iset_.at(0x49) = &Cpu::opcode49;
  iset_.at(0x4a) = &Cpu::opcode4A;
  iset_.at(0x4b) = &Cpu::opcode4B;
  iset_.at(0x4c) = &Cpu::opcode4C;
  iset_.at(0x4d) = &Cpu::opcode4D;
  iset_.at(0x4e) = &Cpu::opcode4E;
  iset_.at(0x4f) = &Cpu::opcode4F;
  iset_.at(0x50) = &Cpu::opcode50;
  iset_.at(0x51) = &Cpu::opcode51;
  iset_.at(0x52) = &Cpu::opcode52;
  iset_.at(0x53) = &Cpu::opcode53;
  iset_.at(0x54) = &Cpu::opcode54;
  iset_.at(0x55) = &Cpu::opcode55;
  iset_.at(0x56) = &Cpu::opcode56;
  iset_.at(0x57) = &Cpu::opcode57;
  iset_.at(0x58) = &Cpu::opcode58;
  iset_.at(0x59) = &Cpu::opcode59;
  iset_.at(0x5a) = &Cpu::opcode5A;
  iset_.at(0x5b) = &Cpu::opcode5B;
  iset_.at(0x5c) = &Cpu::opcode5C;
  iset_.at(0x5d) = &Cpu::opcode5D;
  iset_.at(0x5e) = &Cpu::opcode5E;
  iset_.at(0x5f) = &Cpu::opcode5F;
  iset_.at(0x60) = &Cpu::opcode60;
  iset_.at(0x61) = &Cpu::opcode61;
  iset_.at(0x62) = &Cpu::opcode62;
  iset_.at(0x63) = &Cpu::opcode63;
  iset_.at(0x64) = &Cpu::opcode64;
  iset_.at(0x65) = &Cpu::opcode65;
  iset_.at(0x66) = &Cpu::opcode66;
  iset_.at(0x67) = &Cpu::opcode67;
  iset_.at(0x68) = &Cpu::opcode68;
  iset_.at(0x69) = &Cpu::opcode69;
  iset_.at(0x6a) = &Cpu::opcode6A;
  iset_.at(0x6b) = &Cpu::opcode6B;
  iset_.at(0x6c) = &Cpu::opcode6C;
  iset_.at(0x6d) = &Cpu::opcode6D;
  iset_.at(0x6e) = &Cpu::opcode6E;
  iset_.at(0x6f) = &Cpu::opcode6F;
  iset_.at(0x70) = &Cpu::opcode70;
  iset_.at(0x71) = &Cpu::opcode71;
  iset_.at(0x72) = &Cpu::opcode72;
  iset_.at(0x73) = &Cpu::opcode73;
  iset_.at(0x74) = &Cpu::opcode74;
  iset_.at(0x75) = &Cpu::opcode75;
  iset_.at(0x76) = &Cpu::opcode76;
  iset_.at(0x77) = &Cpu::opcode77;
  iset_.at(0x78) = &Cpu::opcode78;
  iset_.at(0x79) = &Cpu::opcode79;
  iset_.at(0x7a) = &Cpu::opcode7A;
  iset_.at(0x7b) = &Cpu::opcode7B;
  iset_.at(0x7c) = &Cpu::opcode7C;
  iset_.at(0x7d) = &Cpu::opcode7D;
  iset_.at(0x7e) = &Cpu::opcode7E;
  iset_.at(0x7f) = &Cpu::opcode7F;
  iset_.at(0x80) = &Cpu::opcode80;
  iset_.at(0x81) = &Cpu::opcode81;
  iset_.at(0x82) = &Cpu::opcode82;
  iset_.at(0x83) = &Cpu::opcode83;
  iset_.at(0x84) = &Cpu::opcode84;
  iset_.at(0x85) = &Cpu::opcode85;
  iset_.at(0x86) = &Cpu::opcode86;
  iset_.at(0x87) = &Cpu::opcode87;
  iset_.at(0x88) = &Cpu::opcode88;
  iset_.at(0x89) = &Cpu::opcode89;
  iset_.at(0x8a) = &Cpu::opcode8A;
  iset_.at(0x8b) = &Cpu::opcode8B;
  iset_.at(0x8c) = &Cpu::opcode8C;
  iset_.at(0x8d) = &Cpu::opcode8D;
  iset_.at(0x8e) = &Cpu::opcode8E;
  iset_.at(0x8f) = &Cpu::opcode8F;
  iset_.at(0x90) = &Cpu::opcode90;
  iset_.at(0x91) = &Cpu::opcode91;
  iset_.at(0x92) = &Cpu::opcode92;
  iset_.at(0x93) = &Cpu::opcode93;
  iset_.at(0x94) = &Cpu::opcode94;
  iset_.at(0x95) = &Cpu::opcode95;
  iset_.at(0x96) = &Cpu::opcode96;
  iset_.at(0x97) = &Cpu::opcode97;
  iset_.at(0x98) = &Cpu::opcode98;
  iset_.at(0x99) = &Cpu::opcode99;
  iset_.at(0x9a) = &Cpu::opcode9A;
  iset_.at(0x9b) = &Cpu::opcode9B;
  iset_.at(0x9c) = &Cpu::opcode9C;
  iset_.at(0x9d) = &Cpu::opcode9D;
  iset_.at(0x9e) = &Cpu::opcode9E;
  iset_.at(0x9f) = &Cpu::opcode9F;
  iset_.at(0xa0) = &Cpu::opcodeA0;
  iset_.at(0xa1) = &Cpu::opcodeA1;
  iset_.at(0xa2) = &Cpu::opcodeA2;
  iset_.at(0xa3) = &Cpu::opcodeA3;
  iset_.at(0xa4) = &Cpu::opcodeA4;
  iset_.at(0xa5) = &Cpu::opcodeA5;
  iset_.at(0xa6) = &Cpu::opcodeA6;
  iset_.at(0xa7) = &Cpu::opcodeA7;
  iset_.at(0xa8) = &Cpu::opcodeA8;
  iset_.at(0xa9) = &Cpu::opcodeA9;
  iset_.at(0xaa) = &Cpu::opcodeAA;
  iset_.at(0xab) = &Cpu::opcodeAB;
  iset_.at(0xac) = &Cpu::opcodeAC;
  iset_.at(0xad) = &Cpu::opcodeAD;
  iset_.at(0xae) = &Cpu::opcodeAE;
  iset_.at(0xaf) = &Cpu::opcodeAF;
  iset_.at(0xb0) = &Cpu::opcodeB0;
  iset_.at(0xb1) = &Cpu::opcodeB1;
  iset_.at(0xb2) = &Cpu::opcodeB2;
  iset_.at(0xb3) = &Cpu::opcodeB3;
  iset_.at(0xb4) = &Cpu::opcodeB4;
  iset_.at(0xb5) = &Cpu::opcodeB5;
  iset_.at(0xb6) = &Cpu::opcodeB6;
  iset_.at(0xb7) = &Cpu::opcodeB7;
  iset_.at(0xb8) = &Cpu::opcodeB8;
  iset_.at(0xb9) = &Cpu::opcodeB9;
  iset_.at(0xba) = &Cpu::opcodeBA;
  iset_.at(0xbb) = &Cpu::opcodeBB;
  iset_.at(0xbc) = &Cpu::opcodeBC;
  iset_.at(0xbd) = &Cpu::opcodeBD;
  iset_.at(0xbe) = &Cpu::opcodeBE;
  iset_.at(0xbf) = &Cpu::opcodeBF;
  iset_.at(0xc0) = &Cpu::opcodeC0;
  iset_.at(0xc1) = &Cpu::opcodeC1;
  iset_.at(0xc2) = &Cpu::opcodeC2;
  iset_.at(0xc3) = &Cpu::opcodeC3;
  iset_.at(0xc4) = &Cpu::opcodeC4;
  iset_.at(0xc5) = &Cpu::opcodeC5;
  iset_.at(0xc6) = &Cpu::opcodeC6;
  iset_.at(0xc7) = &Cpu::opcodeC7;
  iset_.at(0xc8) = &Cpu::opcodeC8;
  iset_.at(0xc9) = &Cpu::opcodeC9;
  iset_.at(0xca) = &Cpu::opcodeCA;
  iset_.at(0xcb) = &Cpu::opcodeCB;
  iset_.at(0xcc) = &Cpu::opcodeCC;
  iset_.at(0xcd) = &Cpu::opcodeCD;
  iset_.at(0xce) = &Cpu::opcodeCE;
  iset_.at(0xcf) = &Cpu::opcodeCF;
  iset_.at(0xd0) = &Cpu::opcodeD0;
  iset_.at(0xd1) = &Cpu::opcodeD1;
  iset_.at(0xd2) = &Cpu::opcodeD2;
  iset_.at(0xd3) = &Cpu::opcodeD3;
  iset_.at(0xd4) = &Cpu::opcodeD4;
  iset_.at(0xd5) = &Cpu::opcodeD5;
  iset_.at(0xd6) = &Cpu::opcodeD6;
  iset_.at(0xd7) = &Cpu::opcodeD7;
  iset_.at(0xd8) = &Cpu::opcodeD8;
  iset_.at(0xd9) = &Cpu::opcodeD9;
  iset_.at(0xda) = &Cpu::opcodeDA;
  iset_.at(0xdb) = &Cpu::opcodeDB;
  iset_.at(0xdc) = &Cpu::opcodeDC;
  iset_.at(0xdd) = &Cpu::opcodeDD;
  iset_.at(0xde) = &Cpu::opcodeDE;
  iset_.at(0xdf) = &Cpu::opcodeDF;
  iset_.at(0xe0) = &Cpu::opcodeE0;
  iset_.at(0xe1) = &Cpu::opcodeE1;
  iset_.at(0xe2) = &Cpu::opcodeE2;
  iset_.at(0xe3) = &Cpu::opcodeE3;
  iset_.at(0xe4) = &Cpu::opcodeE4;
  iset_.at(0xe5) = &Cpu::opcodeE5;
  iset_.at(0xe6) = &Cpu::opcodeE6;
  iset_.at(0xe7) = &Cpu::opcodeE7;
  iset_.at(0xe8) = &Cpu::opcodeE8;
  iset_.at(0xe9) = &Cpu::opcodeE9;
  iset_.at(0xea) = &Cpu::opcodeEA;
  iset_.at(0xeb) = &Cpu::opcodeEB;
  iset_.at(0xec) = &Cpu::opcodeEC;
  iset_.at(0xed) = &Cpu::opcodeED;
  iset_.at(0xee) = &Cpu::opcodeEE;
  iset_.at(0xef) = &Cpu::opcodeEF;
  iset_.at(0xf0) = &Cpu::opcodeF0;
  iset_.at(0xf1) = &Cpu::opcodeF1;
  iset_.at(0xf2) = &Cpu::opcodeF2;
  iset_.at(0xf3) = &Cpu::opcodeF3;
  iset_.at(0xf4) = &Cpu::opcodeF4;
  iset_.at(0xf5) = &Cpu::opcodeF5;
  iset_.at(0xf6) = &Cpu::opcodeF6;
  iset_.at(0xf7) = &Cpu::opcodeF7;
  iset_.at(0xf8) = &Cpu::opcodeF8;
  iset_.at(0xf9) = &Cpu::opcodeF9;
  iset_.at(0xfa) = &Cpu::opcodeFA;
  iset_.at(0xfb) = &Cpu::opcodeFB;
  iset_.at(0xfc) = &Cpu::opcodeFC;
  iset_.at(0xfd) = &Cpu::opcodeFD;
  iset_.at(0xfe) = &Cpu::opcodeFE;
  iset_.at(0xff) = &Cpu::opcodeFF;
  iset_.at(0x100) = &Cpu::opcodeCB00;
  iset_.at(0x101) = &Cpu::opcodeCB01;
  iset_.at(0x102) = &Cpu::opcodeCB02;
  iset_.at(0x103) = &Cpu::opcodeCB03;
  iset_.at(0x104) = &Cpu::opcodeCB04;
  iset_.at(0x105) = &Cpu::opcodeCB05;
  iset_.at(0x106) = &Cpu::opcodeCB06;
  iset_.at(0x107) = &Cpu::opcodeCB07;
  iset_.at(0x108) = &Cpu::opcodeCB08;
  iset_.at(0x109) = &Cpu::opcodeCB09;
  iset_.at(0x10a) = &Cpu::opcodeCB0A;
  iset_.at(0x10b) = &Cpu::opcodeCB0B;
  iset_.at(0x10c) = &Cpu::opcodeCB0C;
  iset_.at(0x10d) = &Cpu::opcodeCB0D;
  iset_.at(0x10e) = &Cpu::opcodeCB0E;
  iset_.at(0x10f) = &Cpu::opcodeCB0F;
  iset_.at(0x110) = &Cpu::opcodeCB10;
  iset_.at(0x111) = &Cpu::opcodeCB11;
  iset_.at(0x112) = &Cpu::opcodeCB12;
  iset_.at(0x113) = &Cpu::opcodeCB13;
  iset_.at(0x114) = &Cpu::opcodeCB14;
  iset_.at(0x115) = &Cpu::opcodeCB15;
  iset_.at(0x116) = &Cpu::opcodeCB16;
  iset_.at(0x117) = &Cpu::opcodeCB17;
  iset_.at(0x118) = &Cpu::opcodeCB18;
  iset_.at(0x119) = &Cpu::opcodeCB19;
  iset_.at(0x11a) = &Cpu::opcodeCB1A;
  iset_.at(0x11b) = &Cpu::opcodeCB1B;
  iset_.at(0x11c) = &Cpu::opcodeCB1C;
  iset_.at(0x11d) = &Cpu::opcodeCB1D;
  iset_.at(0x11e) = &Cpu::opcodeCB1E;
  iset_.at(0x11f) = &Cpu::opcodeCB1F;
  iset_.at(0x120) = &Cpu::opcodeCB20;
  iset_.at(0x121) = &Cpu::opcodeCB21;
  iset_.at(0x122) = &Cpu::opcodeCB22;
  iset_.at(0x123) = &Cpu::opcodeCB23;
  iset_.at(0x124) = &Cpu::opcodeCB24;
  iset_.at(0x125) = &Cpu::opcodeCB25;
  iset_.at(0x126) = &Cpu::opcodeCB26;
  iset_.at(0x127) = &Cpu::opcodeCB27;
  iset_.at(0x128) = &Cpu::opcodeCB28;
  iset_.at(0x129) = &Cpu::opcodeCB29;
  iset_.at(0x12a) = &Cpu::opcodeCB2A;
  iset_.at(0x12b) = &Cpu::opcodeCB2B;
  iset_.at(0x12c) = &Cpu::opcodeCB2C;
  iset_.at(0x12d) = &Cpu::opcodeCB2D;
  iset_.at(0x12e) = &Cpu::opcodeCB2E;
  iset_.at(0x12f) = &Cpu::opcodeCB2F;
  iset_.at(0x130) = &Cpu::opcodeCB30;
  iset_.at(0x131) = &Cpu::opcodeCB31;
  iset_.at(0x132) = &Cpu::opcodeCB32;
  iset_.at(0x133) = &Cpu::opcodeCB33;
  iset_.at(0x134) = &Cpu::opcodeCB34;
  iset_.at(0x135) = &Cpu::opcodeCB35;
  iset_.at(0x136) = &Cpu::opcodeCB36;
  iset_.at(0x137) = &Cpu::opcodeCB37;
  iset_.at(0x138) = &Cpu::opcodeCB38;
  iset_.at(0x139) = &Cpu::opcodeCB39;
  iset_.at(0x13a) = &Cpu::opcodeCB3A;
  iset_.at(0x13b) = &Cpu::opcodeCB3B;
  iset_.at(0x13c) = &Cpu::opcodeCB3C;
  iset_.at(0x13d) = &Cpu::opcodeCB3D;
  iset_.at(0x13e) = &Cpu::opcodeCB3E;
  iset_.at(0x13f) = &Cpu::opcodeCB3F;
  iset_.at(0x140) = &Cpu::opcodeCB40;
  iset_.at(0x141) = &Cpu::opcodeCB41;
  iset_.at(0x142) = &Cpu::opcodeCB42;
  iset_.at(0x143) = &Cpu::opcodeCB43;
  iset_.at(0x144) = &Cpu::opcodeCB44;
  iset_.at(0x145) = &Cpu::opcodeCB45;
  iset_.at(0x146) = &Cpu::opcodeCB46;
  iset_.at(0x147) = &Cpu::opcodeCB47;
  iset_.at(0x148) = &Cpu::opcodeCB48;
  iset_.at(0x149) = &Cpu::opcodeCB49;
  iset_.at(0x14a) = &Cpu::opcodeCB4A;
  iset_.at(0x14b) = &Cpu::opcodeCB4B;
  iset_.at(0x14c) = &Cpu::opcodeCB4C;
  iset_.at(0x14d) = &Cpu::opcodeCB4D;
  iset_.at(0x14e) = &Cpu::opcodeCB4E;
  iset_.at(0x14f) = &Cpu::opcodeCB4F;
  iset_.at(0x150) = &Cpu::opcodeCB50;
  iset_.at(0x151) = &Cpu::opcodeCB51;
  iset_.at(0x152) = &Cpu::opcodeCB52;
  iset_.at(0x153) = &Cpu::opcodeCB53;
  iset_.at(0x154) = &Cpu::opcodeCB54;
  iset_.at(0x155) = &Cpu::opcodeCB55;
  iset_.at(0x156) = &Cpu::opcodeCB56;
  iset_.at(0x157) = &Cpu::opcodeCB57;
  iset_.at(0x158) = &Cpu::opcodeCB58;
  iset_.at(0x159) = &Cpu::opcodeCB59;
  iset_.at(0x15a) = &Cpu::opcodeCB5A;
  iset_.at(0x15b) = &Cpu::opcodeCB5B;
  iset_.at(0x15c) = &Cpu::opcodeCB5C;
  iset_.at(0x15d) = &Cpu::opcodeCB5D;
  iset_.at(0x15e) = &Cpu::opcodeCB5E;
  iset_.at(0x15f) = &Cpu::opcodeCB5F;
  iset_.at(0x160) = &Cpu::opcodeCB60;
  iset_.at(0x161) = &Cpu::opcodeCB61;
  iset_.at(0x162) = &Cpu::opcodeCB62;
  iset_.at(0x163) = &Cpu::opcodeCB63;
  iset_.at(0x164) = &Cpu::opcodeCB64;
  iset_.at(0x165) = &Cpu::opcodeCB65;
  iset_.at(0x166) = &Cpu::opcodeCB66;
  iset_.at(0x167) = &Cpu::opcodeCB67;
  iset_.at(0x168) = &Cpu::opcodeCB68;
  iset_.at(0x169) = &Cpu::opcodeCB69;
  iset_.at(0x16a) = &Cpu::opcodeCB6A;
  iset_.at(0x16b) = &Cpu::opcodeCB6B;
  iset_.at(0x16c) = &Cpu::opcodeCB6C;
  iset_.at(0x16d) = &Cpu::opcodeCB6D;
  iset_.at(0x16e) = &Cpu::opcodeCB6E;
  iset_.at(0x16f) = &Cpu::opcodeCB6F;
  iset_.at(0x170) = &Cpu::opcodeCB70;
  iset_.at(0x171) = &Cpu::opcodeCB71;
  iset_.at(0x172) = &Cpu::opcodeCB72;
  iset_.at(0x173) = &Cpu::opcodeCB73;
  iset_.at(0x174) = &Cpu::opcodeCB74;
  iset_.at(0x175) = &Cpu::opcodeCB75;
  iset_.at(0x176) = &Cpu::opcodeCB76;
  iset_.at(0x177) = &Cpu::opcodeCB77;
  iset_.at(0x178) = &Cpu::opcodeCB78;
  iset_.at(0x179) = &Cpu::opcodeCB79;
  iset_.at(0x17a) = &Cpu::opcodeCB7A;
  iset_.at(0x17b) = &Cpu::opcodeCB7B;
  iset_.at(0x17c) = &Cpu::opcodeCB7C;
  iset_.at(0x17d) = &Cpu::opcodeCB7D;
  iset_.at(0x17e) = &Cpu::opcodeCB7E;
  iset_.at(0x17f) = &Cpu::opcodeCB7F;
  iset_.at(0x180) = &Cpu::opcodeCB80;
  iset_.at(0x181) = &Cpu::opcodeCB81;
  iset_.at(0x182) = &Cpu::opcodeCB82;
  iset_.at(0x183) = &Cpu::opcodeCB83;
  iset_.at(0x184) = &Cpu::opcodeCB84;
  iset_.at(0x185) = &Cpu::opcodeCB85;
  iset_.at(0x186) = &Cpu::opcodeCB86;
  iset_.at(0x187) = &Cpu::opcodeCB87;
  iset_.at(0x188) = &Cpu::opcodeCB88;
  iset_.at(0x189) = &Cpu::opcodeCB89;
  iset_.at(0x18a) = &Cpu::opcodeCB8A;
  iset_.at(0x18b) = &Cpu::opcodeCB8B;
  iset_.at(0x18c) = &Cpu::opcodeCB8C;
  iset_.at(0x18d) = &Cpu::opcodeCB8D;
  iset_.at(0x18e) = &Cpu::opcodeCB8E;
  iset_.at(0x18f) = &Cpu::opcodeCB8F;
  iset_.at(0x190) = &Cpu::opcodeCB90;
  iset_.at(0x191) = &Cpu::opcodeCB91;
  iset_.at(0x192) = &Cpu::opcodeCB92;
  iset_.at(0x193) = &Cpu::opcodeCB93;
  iset_.at(0x194) = &Cpu::opcodeCB94;
  iset_.at(0x195) = &Cpu::opcodeCB95;
  iset_.at(0x196) = &Cpu::opcodeCB96;
  iset_.at(0x197) = &Cpu::opcodeCB97;
  iset_.at(0x198) = &Cpu::opcodeCB98;
  iset_.at(0x199) = &Cpu::opcodeCB99;
  iset_.at(0x19a) = &Cpu::opcodeCB9A;
  iset_.at(0x19b) = &Cpu::opcodeCB9B;
  iset_.at(0x19c) = &Cpu::opcodeCB9C;
  iset_.at(0x19d) = &Cpu::opcodeCB9D;
  iset_.at(0x19e) = &Cpu::opcodeCB9E;
  iset_.at(0x19f) = &Cpu::opcodeCB9F;
  iset_.at(0x1a0) = &Cpu::opcodeCBA0;
  iset_.at(0x1a1) = &Cpu::opcodeCBA1;
  iset_.at(0x1a2) = &Cpu::opcodeCBA2;
  iset_.at(0x1a3) = &Cpu::opcodeCBA3;
  iset_.at(0x1a4) = &Cpu::opcodeCBA4;
  iset_.at(0x1a5) = &Cpu::opcodeCBA5;
  iset_.at(0x1a6) = &Cpu::opcodeCBA6;
  iset_.at(0x1a7) = &Cpu::opcodeCBA7;
  iset_.at(0x1a8) = &Cpu::opcodeCBA8;
  iset_.at(0x1a9) = &Cpu::opcodeCBA9;
  iset_.at(0x1aa) = &Cpu::opcodeCBAA;
  iset_.at(0x1ab) = &Cpu::opcodeCBAB;
  iset_.at(0x1ac) = &Cpu::opcodeCBAC;
  iset_.at(0x1ad) = &Cpu::opcodeCBAD;
  iset_.at(0x1ae) = &Cpu::opcodeCBAE;
  iset_.at(0x1af) = &Cpu::opcodeCBAF;
  iset_.at(0x1b0) = &Cpu::opcodeCBB0;
  iset_.at(0x1b1) = &Cpu::opcodeCBB1;
  iset_.at(0x1b2) = &Cpu::opcodeCBB2;
  iset_.at(0x1b3) = &Cpu::opcodeCBB3;
  iset_.at(0x1b4) = &Cpu::opcodeCBB4;
  iset_.at(0x1b5) = &Cpu::opcodeCBB5;
  iset_.at(0x1b6) = &Cpu::opcodeCBB6;
  iset_.at(0x1b7) = &Cpu::opcodeCBB7;
  iset_.at(0x1b8) = &Cpu::opcodeCBB8;
  iset_.at(0x1b9) = &Cpu::opcodeCBB9;
  iset_.at(0x1ba) = &Cpu::opcodeCBBA;
  iset_.at(0x1bb) = &Cpu::opcodeCBBB;
  iset_.at(0x1bc) = &Cpu::opcodeCBBC;
  iset_.at(0x1bd) = &Cpu::opcodeCBBD;
  iset_.at(0x1be) = &Cpu::opcodeCBBE;
  iset_.at(0x1bf) = &Cpu::opcodeCBBF;
  iset_.at(0x1c0) = &Cpu::opcodeCBC0;
  iset_.at(0x1c1) = &Cpu::opcodeCBC1;
  iset_.at(0x1c2) = &Cpu::opcodeCBC2;
  iset_.at(0x1c3) = &Cpu::opcodeCBC3;
  iset_.at(0x1c4) = &Cpu::opcodeCBC4;
  iset_.at(0x1c5) = &Cpu::opcodeCBC5;
  iset_.at(0x1c6) = &Cpu::opcodeCBC6;
  iset_.at(0x1c7) = &Cpu::opcodeCBC7;
  iset_.at(0x1c8) = &Cpu::opcodeCBC8;
  iset_.at(0x1c9) = &Cpu::opcodeCBC9;
  iset_.at(0x1ca) = &Cpu::opcodeCBCA;
  iset_.at(0x1cb) = &Cpu::opcodeCBCB;
  iset_.at(0x1cc) = &Cpu::opcodeCBCC;
  iset_.at(0x1cd) = &Cpu::opcodeCBCD;
  iset_.at(0x1ce) = &Cpu::opcodeCBCE;
  iset_.at(0x1cf) = &Cpu::opcodeCBCF;
  iset_.at(0x1d0) = &Cpu::opcodeCBD0;
  iset_.at(0x1d1) = &Cpu::opcodeCBD1;
  iset_.at(0x1d2) = &Cpu::opcodeCBD2;
  iset_.at(0x1d3) = &Cpu::opcodeCBD3;
  iset_.at(0x1d4) = &Cpu::opcodeCBD4;
  iset_.at(0x1d5) = &Cpu::opcodeCBD5;
  iset_.at(0x1d6) = &Cpu::opcodeCBD6;
  iset_.at(0x1d7) = &Cpu::opcodeCBD7;
  iset_.at(0x1d8) = &Cpu::opcodeCBD8;
  iset_.at(0x1d9) = &Cpu::opcodeCBD9;
  iset_.at(0x1da) = &Cpu::opcodeCBDA;
  iset_.at(0x1db) = &Cpu::opcodeCBDB;
  iset_.at(0x1dc) = &Cpu::opcodeCBDC;
  iset_.at(0x1dd) = &Cpu::opcodeCBDD;
  iset_.at(0x1de) = &Cpu::opcodeCBDE;
  iset_.at(0x1df) = &Cpu::opcodeCBDF;
  iset_.at(0x1e0) = &Cpu::opcodeCBE0;
  iset_.at(0x1e1) = &Cpu::opcodeCBE1;
  iset_.at(0x1e2) = &Cpu::opcodeCBE2;
  iset_.at(0x1e3) = &Cpu::opcodeCBE3;
  iset_.at(0x1e4) = &Cpu::opcodeCBE4;
  iset_.at(0x1e5) = &Cpu::opcodeCBE5;
  iset_.at(0x1e6) = &Cpu::opcodeCBE6;
  iset_.at(0x1e7) = &Cpu::opcodeCBE7;
  iset_.at(0x1e8) = &Cpu::opcodeCBE8;
  iset_.at(0x1e9) = &Cpu::opcodeCBE9;
  iset_.at(0x1ea) = &Cpu::opcodeCBEA;
  iset_.at(0x1eb) = &Cpu::opcodeCBEB;
  iset_.at(0x1ec) = &Cpu::opcodeCBEC;
  iset_.at(0x1ed) = &Cpu::opcodeCBED;
  iset_.at(0x1ee) = &Cpu::opcodeCBEE;
  iset_.at(0x1ef) = &Cpu::opcodeCBEF;
  iset_.at(0x1f0) = &Cpu::opcodeCBF0;
  iset_.at(0x1f1) = &Cpu::opcodeCBF1;
  iset_.at(0x1f2) = &Cpu::opcodeCBF2;
  iset_.at(0x1f3) = &Cpu::opcodeCBF3;
  iset_.at(0x1f4) = &Cpu::opcodeCBF4;
  iset_.at(0x1f5) = &Cpu::opcodeCBF5;
  iset_.at(0x1f6) = &Cpu::opcodeCBF6;
  iset_.at(0x1f7) = &Cpu::opcodeCBF7;
  iset_.at(0x1f8) = &Cpu::opcodeCBF8;
  iset_.at(0x1f9) = &Cpu::opcodeCBF9;
  iset_.at(0x1fa) = &Cpu::opcodeCBFA;
  iset_.at(0x1fb) = &Cpu::opcodeCBFB;
  iset_.at(0x1fc) = &Cpu::opcodeCBFC;
  iset_.at(0x1fd) = &Cpu::opcodeCBFD;
  iset_.at(0x1fe) = &Cpu::opcodeCBFE;
  iset_.at(0x1ff) = &Cpu::opcodeCBFF;
}