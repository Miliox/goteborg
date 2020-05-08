/*
 * LR35902.cpp
 * Copyright (C) 2020 Emiliano Firmino <emiliano.firmino@gmail.com>
 *
 * Distributed under terms of the MIT license.
 */

#include "lr35902.hpp"

#include "alu.hpp"
#include "mmu.hpp"

#include <iomanip>
#include <iostream>

using namespace gbg;

LR35902::LR35902(MMU &mmu) : regs(), mmu(mmu), iset_(512, &LR35902::notimpl) {
  populateInstructionSets();
}

ticks_t LR35902::cycle() {
  auto opcode = peek8();               // fetch
  auto instruction = iset_.at(opcode); // decode
  return (this->*instruction)();       // execute
}

u8 LR35902::next8() { return read8(regs.pc++); }

u16 LR35902::next16() {
  auto data = read16(regs.pc);
  regs.pc += 2;
  return data;
}

u8 LR35902::peek8() { return read8(regs.pc); }

u16 LR35902::peek16() { return read16(regs.pc); }

u8 LR35902::read8(addr_t a) { return mmu.read(a); }

u16 LR35902::read16(addr_t a) {
  u8 lsb = mmu.read(a);
  u8 hsb = mmu.read(a + 1);
  return (hsb << 8) | lsb;
}

void LR35902::write8(addr_t a, u8 v) { mmu.write(a, v); }

void LR35902::write16(addr_t a, u16 v) {
  mmu.write(a, (v >> 8) & 0xff);
  mmu.write(a + 1, v & 0xff);
}

u8 LR35902::zread8(u8 a) { return read8(0xff00 + a); }

u16 LR35902::zread16(u8 a) { return read16(0xff00 + a); }

void LR35902::zwrite8(u8 a, u8 v) { write8(0xff00 + a, v); }

void LR35902::zwrite16(u8 a, u16 v) { write16(0xff00 + a, v); }

void LR35902::call(addr_t a) {
  push(regs.pc);
  regs.pc = a;
}

void LR35902::rst(addr_t a) {
  push(regs.pc);
  regs.pc = a;
}

void LR35902::ret() { pop(regs.pc); }

void LR35902::push(u16 &reg) {
  u8 hsb = reg >> 8;
  u8 lsb = reg;
  write8(regs.sp--, lsb);
  write8(regs.sp--, hsb);
}

void LR35902::pop(u16 &reg) {
  u8 hsb = read8(++regs.sp);
  u8 lsb = read8(++regs.sp);
  reg = (hsb << 8) | lsb;
}

ticks_t LR35902::notimpl() {
  std::stringstream ss;
  ss << "Not implemented: ";
  ss << std::uppercase << std::hex << std::setfill('0') << std::setw(2);
  ss << static_cast<int>(peek8());

  throw std::runtime_error(ss.str());
  return 0;
}

// NOP
ticks_t LR35902::opcode00() {
  regs.pc++;
  return 4;
}

// LD BC,d16
ticks_t LR35902::opcode01() {
  regs.pc++;
  regs.bc = next16();
  return 12;
}

// LD (BC),A
ticks_t LR35902::opcode02() {
  regs.pc++;
  write8(regs.bc, regs.a);
  return 8;
}

// INC BC
ticks_t LR35902::opcode03() {
  regs.pc++;
  alu::inc16(regs.f, regs.bc);
  return 8;
}

// INC B
ticks_t LR35902::opcode04() {
  regs.pc++;
  alu::inc8(regs.f, regs.b);
  return 4;
}

// DEC B
ticks_t LR35902::opcode05() {
  regs.pc++;
  alu::dec8(regs.f, regs.b);
  return 4;
}

//  LD B,d8
ticks_t LR35902::opcode06() {
  regs.pc++;
  regs.b = next8();
  return 8;
}

// RLCA
ticks_t LR35902::opcode07() {
  regs.pc++;
  alu::rlc(regs.f, regs.a);
  return 4;
}

// LD (a16),SP
ticks_t LR35902::opcode08() {
  regs.pc++;
  u16 addr = next16();
  write16(addr, regs.sp);
  return 20;
}

// ADD HL,BC
ticks_t LR35902::opcode09() {
  regs.pc++;
  alu::add16(regs.f, regs.hl, regs.bc);
  return 8;
}

// LD A,(BC)
ticks_t LR35902::opcode0A() {
  regs.pc++;
  regs.a = read16(regs.bc);
  return 8;
}

// DEC BC
ticks_t LR35902::opcode0B() {
  regs.pc++;
  alu::dec16(regs.f, regs.bc);
  return 8;
}

// INC C
ticks_t LR35902::opcode0C() {
  regs.pc++;
  alu::inc8(regs.f, regs.c);
  return 4;
}

// DEC C
ticks_t LR35902::opcode0D() {
  regs.pc++;
  alu::dec8(regs.f, regs.c);
  return 4;
}

// LD C,d8
ticks_t LR35902::opcode0E() {
  regs.pc++;
  regs.c = next8();
  return 8;
}

// RRCA
ticks_t LR35902::opcode0F() {
  regs.pc++;
  alu::rrc(regs.f, regs.a);
  return 4;
}

// STOP 0
ticks_t LR35902::opcode10() {
  regs.pc++;
  return 4;
}

// LD DE,d16
ticks_t LR35902::opcode11() {
  regs.pc++;
  regs.de = next16();
  return 12;
}

// LD (DE),A
ticks_t LR35902::opcode12() {
  regs.pc++;
  write8(regs.de, regs.a);
  return 8;
}

// INC DE
ticks_t LR35902::opcode13() {
  regs.pc++;
  alu::inc16(regs.f, regs.de);
  return 8;
}

// INC D
ticks_t LR35902::opcode14() {
  regs.pc++;
  alu::inc8(regs.f, regs.d);
  return 4;
}

// DEC D
ticks_t LR35902::opcode15() {
  regs.pc++;
  alu::dec8(regs.f, regs.d);
  return 4;
}

// LD D,d8
ticks_t LR35902::opcode16() {
  regs.pc++;
  regs.d = next8();
  return 8;
}

// RLA
ticks_t LR35902::opcode17() {
  regs.pc++;
  alu::rl(regs.f, regs.a);
  return 4;
}

// JR r8
ticks_t LR35902::opcode18() {
  regs.pc++;

  s32 aux = s32(regs.pc) + s8(next8());

  regs.pc = aux & 0xffff;
  return 12;
}

// ADD HL,DE
ticks_t LR35902::opcode19() {
  regs.pc++;
  alu::add16(regs.f, regs.hl, regs.de);
  return 8;
}

// LD A,(DE)
ticks_t LR35902::opcode1A() {
  regs.pc++;
  regs.a = read8(regs.de);
  return 8;
}

// DEC DE
ticks_t LR35902::opcode1B() {
  regs.pc++;
  alu::dec16(regs.f, regs.de);
  return 8;
}

// INC E
ticks_t LR35902::opcode1C() {
  regs.pc++;
  alu::inc8(regs.f, regs.e);
  return 4;
}

// DEC E
ticks_t LR35902::opcode1D() {
  regs.pc++;
  alu::dec8(regs.f, regs.e);
  return 4;
}

// LD E,d8
ticks_t LR35902::opcode1E() {
  regs.pc++;
  regs.e = next8();
  return 8;
}

// RRA
ticks_t LR35902::opcode1F() {
  regs.pc++;
  alu::rr(regs.f, regs.a);
  return 4;
}

// JR NZ,r8
ticks_t LR35902::opcode20() {
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
ticks_t LR35902::opcode21() {
  regs.pc++;
  regs.hl = next16();
  return 12;
}

// LD (HL+),A
ticks_t LR35902::opcode22() {
  regs.pc++;
  write8(regs.hl++, regs.a);
  return 8;
}

// INC HL
ticks_t LR35902::opcode23() {
  regs.pc++;
  alu::inc16(regs.f, regs.hl);
  return 8;
}

// INC H
ticks_t LR35902::opcode24() {
  regs.pc++;
  alu::inc8(regs.f, regs.h);
  return 4;
}

// DEC H
ticks_t LR35902::opcode25() {
  regs.pc++;
  alu::dec8(regs.f, regs.h);
  return 4;
}

// LD H,d8
ticks_t LR35902::opcode26() {
  regs.pc++;
  regs.h = next8();
  return 8;
}

// DAA
ticks_t LR35902::opcode27() {
  regs.pc++;
  alu::daa(regs.f, regs.a);
  return 4;
}

// JR Z,r8
ticks_t LR35902::opcode28() {
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
ticks_t LR35902::opcode29() {
  regs.pc++;
  alu::add16(regs.f, regs.hl, regs.hl);
  return 8;
}

// LD A,(HL+)
ticks_t LR35902::opcode2A() {
  regs.pc++;
  regs.a = read8(regs.hl++);
  return 8;
}

// DEC HL
ticks_t LR35902::opcode2B() {
  regs.pc++;
  alu::dec16(regs.f, regs.hl);
  return 8;
}

// INC L
ticks_t LR35902::opcode2C() {
  regs.pc++;
  alu::inc8(regs.f, regs.l);
  return 4;
}

// DEC L
ticks_t LR35902::opcode2D() {
  regs.pc++;
  alu::dec8(regs.f, regs.l);
  return 4;
}

// LD L,d8
ticks_t LR35902::opcode2E() {
  regs.pc++;
  regs.l = next8();
  return 8;
}

// CPL
ticks_t LR35902::opcode2F() {
  regs.pc++;
  alu::cpl(regs.f, regs.a);
  return 4;
}

// JR NC,r8
ticks_t LR35902::opcode30() {
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
ticks_t LR35902::opcode31() {
  regs.pc++;
  regs.sp = next16();
  return 12;
}

// LD (HL-),A
ticks_t LR35902::opcode32() {
  regs.pc++;
  write8(regs.hl--, regs.a);
  return 8;
}

// INC SP
ticks_t LR35902::opcode33() {
  regs.pc++;
  alu::inc16(regs.f, regs.sp);
  return 8;
}

// INC (HL)
ticks_t LR35902::opcode34() {
  regs.pc++;
  u8 v = read8(regs.hl);
  alu::inc8(regs.f, v);
  write8(regs.hl, v);
  return 12;
}

// DEC (HL)
ticks_t LR35902::opcode35() {
  regs.pc++;
  u8 v = read8(regs.hl);
  alu::dec8(regs.f, v);
  write8(regs.hl, v);
  return 12;
}

// LD (HL),d8
ticks_t LR35902::opcode36() {
  regs.pc++;
  u8 v = next8();
  write8(regs.hl, v);
  return 12;
}

// SCF
ticks_t LR35902::opcode37() {
  regs.pc++;
  alu::scf(regs.f);
  return 4;
}

// JR C,r8
ticks_t LR35902::opcode38() {
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
ticks_t LR35902::opcode39() {
  regs.pc++;
  alu::add16(regs.f, regs.hl, regs.sp);
  return 8;
}

// LD A,(HL-)
ticks_t LR35902::opcode3A() {
  regs.pc++;
  regs.a = read8(regs.hl--);
  return 8;
}

// DEC SP
ticks_t LR35902::opcode3B() {
  regs.pc++;
  alu::dec16(regs.f, regs.sp);
  return 8;
}

// INC A
ticks_t LR35902::opcode3C() {
  regs.pc++;
  alu::inc8(regs.f, regs.a);
  return 4;
}

// DEC A
ticks_t LR35902::opcode3D() {
  regs.pc++;
  alu::dec8(regs.f, regs.a);
  return 4;
}

// LD A,d8
ticks_t LR35902::opcode3E() {
  regs.pc++;
  regs.a = next8();
  return 8;
}

// CCF
ticks_t LR35902::opcode3F() {
  regs.pc++;
  alu::ccf(regs.f);
  return 4;
}

// LD B,B
ticks_t LR35902::opcode40() {
  regs.pc++;
  regs.b = regs.b;
  return 4;
}

// LD B,C
ticks_t LR35902::opcode41() {
  regs.pc++;
  regs.b = regs.c;
  return 4;
}

// LD B,D
ticks_t LR35902::opcode42() {
  regs.pc++;
  regs.b = regs.d;
  return 4;
}

// LD B,E
ticks_t LR35902::opcode43() {
  regs.pc++;
  regs.b = regs.e;
  return 4;
}

// LD B,H
ticks_t LR35902::opcode44() {
  regs.pc++;
  regs.b = regs.h;
  return 4;
}

// LD B,L
ticks_t LR35902::opcode45() {
  regs.pc++;
  regs.b = regs.l;
  return 4;
}

// LD B,(HL)
ticks_t LR35902::opcode46() {
  regs.pc++;
  regs.b = read8(regs.hl);
  return 8;
}

// LD B,A
ticks_t LR35902::opcode47() {
  regs.pc++;
  regs.b = regs.a;
  return 4;
}

// LD C,B
ticks_t LR35902::opcode48() {
  regs.pc++;
  regs.c = regs.b;
  return 4;
}

// LD C,C
ticks_t LR35902::opcode49() {
  regs.pc++;
  regs.c = regs.c;
  return 4;
}

// LD C,D
ticks_t LR35902::opcode4A() {
  regs.pc++;
  regs.c = regs.d;
  return 4;
}

// LD C,E
ticks_t LR35902::opcode4B() {
  regs.pc++;
  regs.c = regs.e;
  return 4;
}

// LD C,H
ticks_t LR35902::opcode4C() {
  regs.pc++;
  regs.c = regs.h;
  return 4;
}

// LD C,L
ticks_t LR35902::opcode4D() {
  regs.pc++;
  regs.c = regs.l;
  return 4;
}

// LD C,(HL)
ticks_t LR35902::opcode4E() {
  regs.pc++;
  regs.c = read8(regs.hl);
  return 8;
}

// LD C,A
ticks_t LR35902::opcode4F() {
  regs.pc++;
  regs.c = regs.a;
  return 4;
}

// LD D,B
ticks_t LR35902::opcode50() {
  regs.pc++;
  regs.d = regs.b;
  return 4;
}

// LD D,C
ticks_t LR35902::opcode51() {
  regs.pc++;
  regs.d = regs.c;
  return 4;
}

// LD D,D
ticks_t LR35902::opcode52() {
  regs.pc++;
  regs.d = regs.d;
  return 4;
}

// LD D,E
ticks_t LR35902::opcode53() {
  regs.pc++;
  regs.d = regs.e;
  return 4;
}

// LD D,H
ticks_t LR35902::opcode54() {
  regs.pc++;
  regs.d = regs.h;
  return 4;
}

// LD D,L
ticks_t LR35902::opcode55() {
  regs.pc++;
  regs.d = regs.l;
  return 4;
}

// LD D,(HL)
ticks_t LR35902::opcode56() {
  regs.pc++;
  regs.d = read8(regs.hl);
  return 8;
}

// LD D,A
ticks_t LR35902::opcode57() {
  regs.pc++;
  regs.d = regs.a;
  return 4;
}

// LD E,B
ticks_t LR35902::opcode58() {
  regs.pc++;
  regs.e = regs.b;
  return 4;
}

// LD E,C
ticks_t LR35902::opcode59() {
  regs.pc++;
  regs.e = regs.c;
  return 4;
}

// LD E,D
ticks_t LR35902::opcode5A() {
  regs.pc++;
  regs.e = regs.d;
  return 4;
}

// LD E,E
ticks_t LR35902::opcode5B() {
  regs.pc++;
  regs.e = regs.e;
  return 4;
}

// LD E,H
ticks_t LR35902::opcode5C() {
  regs.pc++;
  regs.e = regs.h;
  return 4;
}

// LD E,L
ticks_t LR35902::opcode5D() {
  regs.pc++;
  regs.e = regs.l;
  return 4;
}

// LD E,(HL)
ticks_t LR35902::opcode5E() {
  regs.pc++;
  regs.e = read8(regs.hl);
  return 8;
}

// LD E,A
ticks_t LR35902::opcode5F() {
  regs.pc++;
  regs.e = regs.a;
  return 4;
}

// LD H,B
ticks_t LR35902::opcode60() {
  regs.pc++;
  regs.h = regs.b;
  return 4;
}

// LD H,C
ticks_t LR35902::opcode61() {
  regs.pc++;
  regs.h = regs.c;
  return 4;
}

// LD H,D
ticks_t LR35902::opcode62() {
  regs.pc++;
  regs.h = regs.d;
  return 4;
}

// LD H,E
ticks_t LR35902::opcode63() {
  regs.pc++;
  regs.h = regs.e;
  return 4;
}

// LD H,H
ticks_t LR35902::opcode64() {
  regs.pc++;
  regs.h = regs.h;
  return 4;
}

// LD H,L
ticks_t LR35902::opcode65() {
  regs.pc++;
  regs.h = regs.l;
  return 4;
}

// LD H,(HL)
ticks_t LR35902::opcode66() {
  regs.pc++;
  regs.h = read8(regs.hl);
  return 8;
}

// LD H,A
ticks_t LR35902::opcode67() {
  regs.pc++;
  regs.h = regs.a;
  return 4;
}

// LD L,B
ticks_t LR35902::opcode68() {
  regs.pc++;
  regs.l = regs.b;
  return 4;
}

// LD L,C
ticks_t LR35902::opcode69() {
  regs.pc++;
  regs.l = regs.c;
  return 4;
}

// LD L,D
ticks_t LR35902::opcode6A() {
  regs.pc++;
  regs.l = regs.d;
  return 4;
}

// LD L,E
ticks_t LR35902::opcode6B() {
  regs.pc++;
  regs.l = regs.e;
  return 4;
}

// LD L,H
ticks_t LR35902::opcode6C() {
  regs.pc++;
  regs.l = regs.h;
  return 4;
}

// LD L,L
ticks_t LR35902::opcode6D() {
  regs.pc++;
  regs.l = regs.l;
  return 4;
}

// LD L,(HL)
ticks_t LR35902::opcode6E() {
  regs.pc++;
  regs.l = read8(regs.hl);
  return 8;
}

// LD L,A
ticks_t LR35902::opcode6F() {
  regs.pc++;
  regs.l = regs.a;
  return 4;
}

// LD (HL),B
ticks_t LR35902::opcode70() {
  regs.pc++;
  write8(regs.hl, regs.b);
  return 8;
}

// LD (HL),C
ticks_t LR35902::opcode71() {
  regs.pc++;
  write8(regs.hl, regs.c);
  return 8;
}

// LD (HL),D
ticks_t LR35902::opcode72() {
  regs.pc++;
  write8(regs.hl, regs.d);
  return 8;
}

// LD (HL),E
ticks_t LR35902::opcode73() {
  regs.pc++;
  write8(regs.hl, regs.e);
  return 8;
}

// LD (HL),H
ticks_t LR35902::opcode74() {
  regs.pc++;
  write8(regs.hl, regs.h);
  return 8;
}

// LD (HL),L
ticks_t LR35902::opcode75() {
  regs.pc++;
  write8(regs.hl, regs.l);
  return 8;
}

// HALT
ticks_t LR35902::opcode76() {
  // TODO: Detect interruption to resume execution
  return 4;
}

// LD (HL),A
ticks_t LR35902::opcode77() {
  regs.pc++;
  write8(regs.hl, regs.a);
  return 8;
}

// LD A,B
ticks_t LR35902::opcode78() {
  regs.pc++;
  regs.a = regs.b;
  return 4;
}

// LD A,C
ticks_t LR35902::opcode79() {
  regs.pc++;
  regs.a = regs.c;
  return 4;
}

// LD A,D
ticks_t LR35902::opcode7A() {
  regs.pc++;
  regs.a = regs.d;
  return 4;
}

// LD A,E
ticks_t LR35902::opcode7B() {
  regs.pc++;
  regs.a = regs.e;
  return 4;
}

// LD A,H
ticks_t LR35902::opcode7C() {
  regs.pc++;
  regs.a = regs.h;
  return 4;
}

// LD A,L
ticks_t LR35902::opcode7D() {
  regs.pc++;
  regs.a = regs.l;
  return 4;
}

// LD A,(HL)
ticks_t LR35902::opcode7E() {
  regs.pc++;
  regs.a = read8(regs.hl);
  return 8;
}

// LD A,A
ticks_t LR35902::opcode7F() {
  regs.pc++;
  regs.a = regs.a;
  return 4;
}

// ADD A,B
ticks_t LR35902::opcode80() {
  regs.pc++;
  alu::add8(regs.f, regs.a, regs.b);
  return 4;
}

// ADD A,C
ticks_t LR35902::opcode81() {
  regs.pc++;
  alu::add8(regs.f, regs.a, regs.c);
  return 4;
}

// ADD A,D
ticks_t LR35902::opcode82() {
  regs.pc++;
  alu::add8(regs.f, regs.a, regs.d);
  return 4;
}

// ADD A,E
ticks_t LR35902::opcode83() {
  regs.pc++;
  alu::add8(regs.f, regs.a, regs.e);
  return 4;
}

// ADD A,H
ticks_t LR35902::opcode84() {
  regs.pc++;
  alu::add8(regs.f, regs.a, regs.h);
  return 4;
}

// ADD A,L
ticks_t LR35902::opcode85() {
  regs.pc++;
  alu::add8(regs.f, regs.a, regs.l);
  return 4;
}

// ADD A,(HL)
ticks_t LR35902::opcode86() {
  regs.pc++;
  alu::add8(regs.f, regs.a, read8(regs.hl));
  return 8;
}

// ADD A,A
ticks_t LR35902::opcode87() {
  regs.pc++;
  alu::add8(regs.f, regs.a, regs.a);
  return 4;
}

// ADC A,B
ticks_t LR35902::opcode88() {
  regs.pc++;
  alu::adc8(regs.f, regs.a, regs.b);
  return 4;
}

// ADC A,C
ticks_t LR35902::opcode89() {
  regs.pc++;
  alu::adc8(regs.f, regs.a, regs.c);
  return 4;
}

// ADC A,D
ticks_t LR35902::opcode8A() {
  regs.pc++;
  alu::adc8(regs.f, regs.a, regs.d);
  return 4;
}

// ADC A,E
ticks_t LR35902::opcode8B() {
  regs.pc++;
  alu::adc8(regs.f, regs.a, regs.e);
  return 4;
}

// ADC A,H
ticks_t LR35902::opcode8C() {
  regs.pc++;
  alu::adc8(regs.f, regs.a, regs.h);
  return 4;
}

// ADC A,L
ticks_t LR35902::opcode8D() {
  regs.pc++;
  alu::adc8(regs.f, regs.a, regs.l);
  return 4;
}

// ADC A,(HL)
ticks_t LR35902::opcode8E() {
  regs.pc++;
  alu::adc8(regs.f, regs.a, read8(regs.hl));
  return 8;
}

// ADC A,A
ticks_t LR35902::opcode8F() {
  regs.pc++;
  alu::adc8(regs.f, regs.a, regs.a);
  return 4;
}

// SUB B
ticks_t LR35902::opcode90() {
  regs.pc++;
  alu::sub8(regs.f, regs.a, regs.b);
  return 4;
}

// SUB C
ticks_t LR35902::opcode91() {
  regs.pc++;
  alu::sub8(regs.f, regs.a, regs.c);
  return 4;
}

// SUB D
ticks_t LR35902::opcode92() {
  regs.pc++;
  alu::sub8(regs.f, regs.a, regs.d);
  return 4;
}

// SUB E
ticks_t LR35902::opcode93() {
  regs.pc++;
  alu::sub8(regs.f, regs.a, regs.e);
  return 4;
}

// SUB H
ticks_t LR35902::opcode94() {
  regs.pc++;
  alu::sub8(regs.f, regs.a, regs.h);
  return 4;
}

// SUB L
ticks_t LR35902::opcode95() {
  regs.pc++;
  alu::sub8(regs.f, regs.a, regs.l);
  return 4;
}

// SUB (HL)
ticks_t LR35902::opcode96() {
  regs.pc++;
  alu::sub8(regs.f, regs.a, read8(regs.hl));
  return 4;
}

// SUB A
ticks_t LR35902::opcode97() {
  regs.pc++;
  alu::sub8(regs.f, regs.a, regs.a);
  return 4;
}

// SBC A,B
ticks_t LR35902::opcode98() {
  regs.pc++;
  alu::sbc8(regs.f, regs.a, regs.b);
  return 4;
}

// SBC A,C
ticks_t LR35902::opcode99() {
  regs.pc++;
  alu::sbc8(regs.f, regs.a, regs.c);
  return 4;
}

// SBC A,D
ticks_t LR35902::opcode9A() {
  regs.pc++;
  alu::sbc8(regs.f, regs.a, regs.d);
  return 4;
}

// SBC A,E
ticks_t LR35902::opcode9B() {
  regs.pc++;
  alu::sbc8(regs.f, regs.a, regs.e);
  return 4;
}

// SBC A,H
ticks_t LR35902::opcode9C() {
  regs.pc++;
  alu::sbc8(regs.f, regs.a, regs.h);
  return 4;
}

// SBC A,L
ticks_t LR35902::opcode9D() {
  regs.pc++;
  alu::sbc8(regs.f, regs.a, regs.l);
  return 4;
}

// SBC A,(HL)
ticks_t LR35902::opcode9E() {
  regs.pc++;
  alu::sbc8(regs.f, regs.a, read8(regs.hl));
  return 8;
}

// SBC A,A
ticks_t LR35902::opcode9F() {
  regs.pc++;
  alu::sbc8(regs.f, regs.a, regs.a);
  return 4;
}

// AND B
ticks_t LR35902::opcodeA0() {
  regs.pc++;
  alu::land(regs.f, regs.a, regs.b);
  return 4;
}

// AND C
ticks_t LR35902::opcodeA1() {
  regs.pc++;
  alu::land(regs.f, regs.a, regs.c);
  return 4;
}

// AND D
ticks_t LR35902::opcodeA2() {
  regs.pc++;
  alu::land(regs.f, regs.a, regs.d);
  return 4;
}

// AND E
ticks_t LR35902::opcodeA3() {
  regs.pc++;
  alu::land(regs.f, regs.a, regs.e);
  return 4;
}

// AND H
ticks_t LR35902::opcodeA4() {
  regs.pc++;
  alu::land(regs.f, regs.a, regs.h);
  return 4;
}

// AND L
ticks_t LR35902::opcodeA5() {
  regs.pc++;
  alu::land(regs.f, regs.a, regs.l);
  return 4;
}

// AND (HL)
ticks_t LR35902::opcodeA6() {
  regs.pc++;
  alu::land(regs.f, regs.a, read8(regs.hl));
  return 8;
}

// AND A
ticks_t LR35902::opcodeA7() {
  regs.pc++;
  alu::land(regs.f, regs.a, regs.a);
  return 4;
}

// XOR B
ticks_t LR35902::opcodeA8() {
  regs.pc++;
  alu::lxor(regs.f, regs.a, regs.b);
  return 4;
}

// XOR C
ticks_t LR35902::opcodeA9() {
  regs.pc++;
  alu::lxor(regs.f, regs.a, regs.c);
  return 4;
}

// XOR D
ticks_t LR35902::opcodeAA() {
  regs.pc++;
  alu::lxor(regs.f, regs.a, regs.d);
  return 4;
}

// XOR E
ticks_t LR35902::opcodeAB() {
  regs.pc++;
  alu::lxor(regs.f, regs.a, regs.e);
  return 4;
}

// XOR H
ticks_t LR35902::opcodeAC() {
  regs.pc++;
  alu::lxor(regs.f, regs.a, regs.h);
  return 4;
}

// XOR L
ticks_t LR35902::opcodeAD() {
  regs.pc++;
  alu::lxor(regs.f, regs.a, regs.l);
  return 4;
}

// XOR (HL)
ticks_t LR35902::opcodeAE() {
  regs.pc++;
  alu::lxor(regs.f, regs.a, read8(regs.hl));
  return 8;
}

// XOR A
ticks_t LR35902::opcodeAF() {
  regs.pc++;
  alu::lxor(regs.f, regs.a, regs.a);
  return 4;
}

// OR B
ticks_t LR35902::opcodeB0() {
  regs.pc++;
  alu::lor(regs.f, regs.a, regs.b);
  return 4;
}

// OR C
ticks_t LR35902::opcodeB1() {
  regs.pc++;
  alu::lor(regs.f, regs.a, regs.c);
  return 4;
}

// OR D
ticks_t LR35902::opcodeB2() {
  regs.pc++;
  alu::lor(regs.f, regs.a, regs.d);
  return 4;
}

// OR E
ticks_t LR35902::opcodeB3() {
  regs.pc++;
  alu::lor(regs.f, regs.a, regs.e);
  return 4;
}

// OR H
ticks_t LR35902::opcodeB4() {
  regs.pc++;
  alu::lor(regs.f, regs.a, regs.h);
  return 4;
}

// OR L
ticks_t LR35902::opcodeB5() {
  regs.pc++;
  alu::lor(regs.f, regs.a, regs.l);
  return 4;
}

// OR (HL)
ticks_t LR35902::opcodeB6() {
  regs.pc++;
  alu::lor(regs.f, regs.a, read8(regs.hl));
  return 8;
}

// OR A
ticks_t LR35902::opcodeB7() {
  regs.pc++;
  alu::lor(regs.f, regs.a, regs.a);
  return 4;
}

// CP B
ticks_t LR35902::opcodeB8() {
  regs.pc++;
  alu::lcp(regs.f, regs.a, regs.b);
  return 4;
}

// CP C
ticks_t LR35902::opcodeB9() {
  regs.pc++;
  alu::lcp(regs.f, regs.a, regs.c);
  return 4;
}

// CP D
ticks_t LR35902::opcodeBA() {
  regs.pc++;
  alu::lcp(regs.f, regs.a, regs.d);
  return 4;
}

// CP E
ticks_t LR35902::opcodeBB() {
  regs.pc++;
  alu::lcp(regs.f, regs.a, regs.e);
  return 4;
}

// CP H
ticks_t LR35902::opcodeBC() {
  regs.pc++;
  alu::lcp(regs.f, regs.a, regs.h);
  return 4;
}

// CP L
ticks_t LR35902::opcodeBD() {
  regs.pc++;
  alu::lcp(regs.f, regs.a, regs.l);
  return 4;
}

// CP (HL)
ticks_t LR35902::opcodeBE() {
  regs.pc++;
  alu::lcp(regs.f, regs.a, read8(regs.hl));
  return 8;
}

// CP A
ticks_t LR35902::opcodeBF() {
  regs.pc++;
  alu::lcp(regs.f, regs.a, regs.a);
  return 4;
}

// RET NZ
ticks_t LR35902::opcodeC0() {
  regs.pc++;
  if ((regs.f & alu::kFZ) == 0) {
    ret();
    return 20;
  }
  return 8;
}

// POP BC
ticks_t LR35902::opcodeC1() {
  regs.pc++;
  pop(regs.bc);
  return 12;
}

// JP NZ,a16
ticks_t LR35902::opcodeC2() {
  regs.pc++;
  u16 addr = next16();
  if ((regs.f & alu::kFZ) == 0) {
    regs.pc = addr;
    return 16;
  }
  return 12;
}

// JP a16
ticks_t LR35902::opcodeC3() {
  regs.pc++;
  regs.pc = next16();
  return 12;
}

// CALL NZ,a16
ticks_t LR35902::opcodeC4() {
  regs.pc++;
  u16 addr = next16();
  if ((regs.f & alu::kFZ) == 0) {
    call(addr);
    return 24;
  }
  return 12;
}

// PUSH BC
ticks_t LR35902::opcodeC5() {
  regs.pc++;
  push(regs.bc);
  return 16;
}

// ADD A,d8
ticks_t LR35902::opcodeC6() {
  regs.pc++;
  alu::add8(regs.f, regs.a, next8());
  return 8;
}

// RST 00H
ticks_t LR35902::opcodeC7() {
  regs.pc++;
  rst(0x00);
  return 16;
}

// RET Z
ticks_t LR35902::opcodeC8() {
  regs.pc++;
  if ((regs.f & alu::kFZ) != 0) {
    ret();
    return 20;
  }
  return 8;
}

// RET
ticks_t LR35902::opcodeC9() {
  regs.pc++;
  ret();
  return 16;
}

// JP Z,a16
ticks_t LR35902::opcodeCA() {
  regs.pc++;
  u16 addr = next16();
  if ((regs.f & alu::kFZ) != 0) {
    regs.pc = addr;
    return 16;
  }
  return 12;
}

// PREFIX CB
ticks_t LR35902::opcodeCB() {
  regs.pc++;

  auto opcode = peek8() + 0x100;       // fetch
  auto instruction = iset_.at(opcode); // decode
  return 4 + (this->*instruction)();   // execute
}

// CALL Z,a16
ticks_t LR35902::opcodeCC() {
  regs.pc++;
  u16 addr = next16();
  if ((regs.f & alu::kFZ) != 0) {
    call(addr);
    return 12;
  }
  return 8;
}

// CALL a16
ticks_t LR35902::opcodeCD() {
  regs.pc++;
  call(next16());
  return 8;
}

// ADC A,d8
ticks_t LR35902::opcodeCE() {
  regs.pc++;
  alu::adc8(regs.f, regs.a, next8());
  return 8;
}

// RST 08H
ticks_t LR35902::opcodeCF() {
  regs.pc++;
  rst(0x08);
  return 16;
}

// RET NC
ticks_t LR35902::opcodeD0() {
  regs.pc++;
  if ((regs.f & alu::kFC) == 0) {
    ret();
    return 20;
  }
  return 8;
}

// POP DE
ticks_t LR35902::opcodeD1() {
  regs.pc++;
  pop(regs.de);
  return 12;
}

// JP NC,a16
ticks_t LR35902::opcodeD2() {
  regs.pc++;
  u16 addr = next16();
  if ((regs.f & alu::kFC) == 0) {
    regs.pc = addr;
    return 16;
  }
  return 12;
}

// NOP
ticks_t LR35902::opcodeD3() {
  regs.pc++;
  return 4;
}

// CALL NC,a16
ticks_t LR35902::opcodeD4() {
  regs.pc++;
  u16 addr = next16();
  if ((regs.f & alu::kFC) == 0) {
    call(addr);
    return 24;
  }
  return 12;
}

// PUSH DE
ticks_t LR35902::opcodeD5() {
  regs.pc++;
  push(regs.de);
  return 16;
}

// SUB d8
ticks_t LR35902::opcodeD6() {
  regs.pc++;
  alu::sub8(regs.f, regs.a, next8());
  return 8;
}

// RST 10H
ticks_t LR35902::opcodeD7() {
  regs.pc++;
  rst(0x10);
  return 16;
}

// RET C
ticks_t LR35902::opcodeD8() {
  regs.pc++;
  if ((regs.f & alu::kFC) != 0) {
    ret();
    return 20;
  }
  return 8;
}

// RETI
ticks_t LR35902::opcodeD9() {
  regs.pc++;
  ret();
  regs.ime = 1;
  return 16;
}

// JP C,a16
ticks_t LR35902::opcodeDA() {
  regs.pc++;
  u16 addr = next16();
  if ((regs.f & alu::kFC) != 0) {
    regs.pc = addr;
    return 16;
  }
  return 12;
}

// NOP
ticks_t LR35902::opcodeDB() {
  regs.pc++;
  return 4;
}

// CALL C,a16
ticks_t LR35902::opcodeDC() {
  regs.pc++;
  u16 addr = next16();
  if ((regs.f & alu::kFC) != 0) {
    call(addr);
    return 24;
  }
  return 12;
}

// NOP
ticks_t LR35902::opcodeDD() {
  regs.pc++;
  return 4;
}

// SBC A,d8
ticks_t LR35902::opcodeDE() {
  regs.pc++;
  alu::sbc8(regs.f, regs.a, next8());
  return 8;
}

// RST 18H
ticks_t LR35902::opcodeDF() {
  regs.pc++;
  rst(0x18);
  return 16;
}

// LDH (a8),A
ticks_t LR35902::opcodeE0() {
  regs.pc++;
  zwrite8(next8(), regs.a);
  return 12;
}

// POP HL
ticks_t LR35902::opcodeE1() {
  regs.pc++;
  pop(regs.hl);
  return 12;
}

// LD (C),A
ticks_t LR35902::opcodeE2() {
  regs.pc++;
  zwrite8(regs.c, regs.a);
  return 8;
}

// NOP
ticks_t LR35902::opcodeE3() {
  regs.pc++;
  return 4;
}

// NOP
ticks_t LR35902::opcodeE4() {
  regs.pc++;
  return 4;
}

// PUSH HL
ticks_t LR35902::opcodeE5() {
  regs.pc++;
  push(regs.hl);
  return 16;
}

// AND d8
ticks_t LR35902::opcodeE6() {
  regs.pc++;
  alu::land(regs.f, regs.a, next8());
  return 8;
}

// RST 20H
ticks_t LR35902::opcodeE7() {
  regs.pc++;
  rst(0x20);
  return 16;
}

// ADD SP,r8
ticks_t LR35902::opcodeE8() {
  regs.pc++;

  s32 aux = s32(regs.pc) + s8(next8());

  regs.pc = aux & 0xffff;
  return 16;
}

// JP (HL)
ticks_t LR35902::opcodeE9() {
  regs.pc++;
  regs.pc = regs.hl;
  return 4;
}

// LD (a16),A
ticks_t LR35902::opcodeEA() {
  regs.pc++;
  write8(next16(), regs.a);
  return 16;
}

// NOP
ticks_t LR35902::opcodeEB() {
  regs.pc++;
  return 4;
}

// NOP
ticks_t LR35902::opcodeEC() {
  regs.pc++;
  return 4;
}

// NOP
ticks_t LR35902::opcodeED() {
  regs.pc++;
  return 4;
}

// XOR d8
ticks_t LR35902::opcodeEE() {
  regs.pc++;
  alu::lxor(regs.f, regs.a, next8());
  return 8;
}

// RST 28H
ticks_t LR35902::opcodeEF() {
  regs.pc++;
  rst(0x28);
  return 16;
}

// LDH A,(a8)
ticks_t LR35902::opcodeF0() {
  regs.pc++;
  regs.a = zread8(next8());
  return 12;
}

// POP AF
ticks_t LR35902::opcodeF1() {
  regs.pc++;
  pop(regs.af);
  return 12;
}

// LD A,(C)
ticks_t LR35902::opcodeF2() {
  regs.pc++;
  regs.a = zread8(regs.c);
  return 8;
}

// DI
ticks_t LR35902::opcodeF3() {
  regs.pc++;
  regs.ime = 0;
  return 4;
}

// NOP
ticks_t LR35902::opcodeF4() {
  regs.pc++;
  return 4;
}

// PUSH AF
ticks_t LR35902::opcodeF5() {
  regs.pc++;
  push(regs.af);
  return 16;
}

// OR d8
ticks_t LR35902::opcodeF6() {
  regs.pc++;
  alu::lor(regs.f, regs.a, next8());
  return 8;
}

// RST 30H
ticks_t LR35902::opcodeF7() {
  regs.pc++;
  rst(0x30);
  return 16;
}

// LD HL,SP+r8
ticks_t LR35902::opcodeF8() {
  regs.pc++;

  s32 aux = s32(regs.sp) + s8(next8());

  regs.hl = aux & 0xffff;
  return 12;
}

// LD SP,HL
ticks_t LR35902::opcodeF9() {
  regs.pc++;
  regs.sp = regs.hl;
  return 8;
}

// LD A,(a16)
ticks_t LR35902::opcodeFA() {
  regs.pc++;
  regs.a = read8(next16());
  return 16;
}

// EI
ticks_t LR35902::opcodeFB() {
  regs.pc++;
  regs.ime = 1;
  return 4;
}

// NOP
ticks_t LR35902::opcodeFC() {
  regs.pc++;
  return 4;
}

// NOP
ticks_t LR35902::opcodeFD() {
  regs.pc++;
  return 4;
}

// CP d8
ticks_t LR35902::opcodeFE() {
  regs.pc++;
  alu::lcp(regs.f, regs.a, next8());
  return 8;
}

// RST 38H
ticks_t LR35902::opcodeFF() {
  regs.pc++;
  rst(0x38);
  return 16;
}

// RLC B
ticks_t LR35902::opcodeCB00() {
  regs.pc++;
  alu::rlc(regs.f, regs.b);
  return 8;
}

// RLC C
ticks_t LR35902::opcodeCB01() {
  regs.pc++;
  alu::rlc(regs.f, regs.c);
  return 8;
}

// RLC D
ticks_t LR35902::opcodeCB02() {
  regs.pc++;
  alu::rlc(regs.f, regs.d);
  return 8;
}

// RLC E
ticks_t LR35902::opcodeCB03() {
  regs.pc++;
  alu::rlc(regs.f, regs.e);
  return 8;
}

// RLC H
ticks_t LR35902::opcodeCB04() {
  regs.pc++;
  alu::rlc(regs.f, regs.h);
  return 8;
}

// RLC L
ticks_t LR35902::opcodeCB05() {
  regs.pc++;
  alu::rlc(regs.f, regs.l);
  return 8;
}

// RLC (HL)
ticks_t LR35902::opcodeCB06() {
  regs.pc++;
  u8 v = read8(regs.hl);
  alu::rlc(regs.f, v);
  write8(regs.hl, v);
  return 16;
}

// RLC A
ticks_t LR35902::opcodeCB07() {
  regs.pc++;
  alu::rlc(regs.f, regs.a);
  return 8;
}

// RRC B
ticks_t LR35902::opcodeCB08() {
  regs.pc++;
  alu::rrc(regs.f, regs.b);
  return 8;
}

// RRC C
ticks_t LR35902::opcodeCB09() {
  regs.pc++;
  alu::rrc(regs.f, regs.c);
  return 8;
}

// RRC D
ticks_t LR35902::opcodeCB0A() {
  regs.pc++;
  alu::rrc(regs.f, regs.d);
  return 8;
}

// RRC E
ticks_t LR35902::opcodeCB0B() {
  regs.pc++;
  alu::rrc(regs.f, regs.e);
  return 8;
}

// RRC H
ticks_t LR35902::opcodeCB0C() {
  regs.pc++;
  alu::rrc(regs.f, regs.h);
  return 8;
}

// RRC L
ticks_t LR35902::opcodeCB0D() {
  regs.pc++;
  alu::rrc(regs.f, regs.l);
  return 8;
}

// RRC (HL)
ticks_t LR35902::opcodeCB0E() {
  regs.pc++;
  u8 v = read8(regs.hl);
  alu::rrc(regs.f, v);
  write8(regs.hl, v);
  return 16;
}

// RRC A
ticks_t LR35902::opcodeCB0F() {
  regs.pc++;
  alu::rrc(regs.f, regs.a);
  return 8;
}

// RL B
ticks_t LR35902::opcodeCB10() {
  regs.pc++;
  alu::rl(regs.f, regs.b);
  return 8;
}

// RL C
ticks_t LR35902::opcodeCB11() {
  regs.pc++;
  alu::rl(regs.f, regs.c);
  return 8;
}

// RL D
ticks_t LR35902::opcodeCB12() {
  regs.pc++;
  alu::rl(regs.f, regs.d);
  return 8;
}

// RL E
ticks_t LR35902::opcodeCB13() {
  regs.pc++;
  alu::rl(regs.f, regs.e);
  return 8;
}

// RL H
ticks_t LR35902::opcodeCB14() {
  regs.pc++;
  alu::rl(regs.f, regs.h);
  return 8;
}

// RL L
ticks_t LR35902::opcodeCB15() {
  regs.pc++;
  alu::rl(regs.f, regs.l);
  return 8;
}

// RL (HL)
ticks_t LR35902::opcodeCB16() {
  regs.pc++;
  u8 v = read8(regs.hl);
  alu::rl(regs.f, v);
  write8(regs.hl, v);
  return 8;
}

// RL A
ticks_t LR35902::opcodeCB17() {
  regs.pc++;
  alu::rl(regs.f, regs.a);
  return 8;
}

// RR B
ticks_t LR35902::opcodeCB18() {
  regs.pc++;
  alu::rr(regs.f, regs.b);
  return 8;
}

// RR C
ticks_t LR35902::opcodeCB19() {
  regs.pc++;
  alu::rr(regs.f, regs.c);
  return 8;
}

// RR D
ticks_t LR35902::opcodeCB1A() {
  regs.pc++;
  alu::rr(regs.f, regs.d);
  return 8;
}

// RR E
ticks_t LR35902::opcodeCB1B() {
  regs.pc++;
  alu::rr(regs.f, regs.e);
  return 8;
}

// RR H
ticks_t LR35902::opcodeCB1C() {
  regs.pc++;
  alu::rr(regs.f, regs.h);
  return 8;
}

// RR L
ticks_t LR35902::opcodeCB1D() {
  regs.pc++;
  alu::rr(regs.f, regs.l);
  return 8;
}

// RR (HL)
ticks_t LR35902::opcodeCB1E() {
  regs.pc++;
  u8 v = read8(regs.hl);
  alu::rr(regs.f, v);
  write8(regs.hl, v);
  return 16;
}

// RR A
ticks_t LR35902::opcodeCB1F() {
  regs.pc++;
  alu::rr(regs.f, regs.a);
  return 8;
}

// SLA B
ticks_t LR35902::opcodeCB20() {
  regs.pc++;
  alu::sla(regs.f, regs.b);
  return 8;
}

// SLA C
ticks_t LR35902::opcodeCB21() {
  regs.pc++;
  alu::sla(regs.f, regs.c);
  return 8;
}

// SLA D
ticks_t LR35902::opcodeCB22() {
  regs.pc++;
  alu::sla(regs.f, regs.d);
  return 8;
}

// SLA E
ticks_t LR35902::opcodeCB23() {
  regs.pc++;
  alu::sla(regs.f, regs.e);
  return 8;
}

// SLA H
ticks_t LR35902::opcodeCB24() {
  regs.pc++;
  alu::sla(regs.f, regs.h);
  return 8;
}

// SLA L
ticks_t LR35902::opcodeCB25() {
  regs.pc++;
  alu::sla(regs.f, regs.l);
  return 8;
}

// SLA (HL)
ticks_t LR35902::opcodeCB26() {
  regs.pc++;
  u8 v = read8(regs.hl);
  alu::sla(regs.f, v);
  write8(regs.hl, v);
  return 16;
}

// SLA A
ticks_t LR35902::opcodeCB27() {
  regs.pc++;
  alu::sla(regs.f, regs.a);
  return 8;
}

// SRA B
ticks_t LR35902::opcodeCB28() {
  regs.pc++;
  alu::sra(regs.f, regs.b);
  return 8;
}

// SRA C
ticks_t LR35902::opcodeCB29() {
  regs.pc++;
  alu::sra(regs.f, regs.c);
  return 8;
}

// SRA D
ticks_t LR35902::opcodeCB2A() {
  regs.pc++;
  alu::sra(regs.f, regs.d);
  return 8;
}

// SRA E
ticks_t LR35902::opcodeCB2B() {
  regs.pc++;
  alu::sra(regs.f, regs.e);
  return 8;
}

// SRA H
ticks_t LR35902::opcodeCB2C() {
  regs.pc++;
  alu::sra(regs.f, regs.h);
  return 8;
}

// SRA L
ticks_t LR35902::opcodeCB2D() {
  regs.pc++;
  alu::sra(regs.f, regs.l);
  return 8;
}

// SRA (HL)
ticks_t LR35902::opcodeCB2E() {
  regs.pc++;
  u8 v = read8(regs.hl);
  alu::sra(regs.f, v);
  write8(regs.hl, v);
  return 16;
}

// SRA A
ticks_t LR35902::opcodeCB2F() {
  regs.pc++;
  alu::sra(regs.f, regs.b);
  return 8;
}

// SWAP B
ticks_t LR35902::opcodeCB30() {
  regs.pc++;
  alu::swap(regs.f, regs.b);
  return 8;
}

// SWAP C
ticks_t LR35902::opcodeCB31() {
  regs.pc++;
  alu::swap(regs.f, regs.c);
  return 8;
}

// SWAP D
ticks_t LR35902::opcodeCB32() {
  regs.pc++;
  alu::swap(regs.f, regs.d);
  return 8;
}

// SWAP E
ticks_t LR35902::opcodeCB33() {
  regs.pc++;
  alu::swap(regs.f, regs.e);
  return 8;
}

// SWAP H
ticks_t LR35902::opcodeCB34() {
  regs.pc++;
  alu::swap(regs.f, regs.h);
  return 8;
}

// SWAP L
ticks_t LR35902::opcodeCB35() {
  regs.pc++;
  alu::swap(regs.f, regs.l);
  return 8;
}

// SWAP (HL)
ticks_t LR35902::opcodeCB36() {
  regs.pc++;
  u8 v = read8(regs.hl);
  alu::swap(regs.f, v);
  write8(regs.hl, v);
  return 16;
}

// SWAP A
ticks_t LR35902::opcodeCB37() {
  regs.pc++;
  alu::swap(regs.f, regs.a);
  return 8;
}

// SRL B
ticks_t LR35902::opcodeCB38() {
  regs.pc++;
  alu::srl(regs.f, regs.b);
  return 8;
}

// SRL C
ticks_t LR35902::opcodeCB39() {
  regs.pc++;
  alu::srl(regs.f, regs.c);
  return 8;
}

// SRL D
ticks_t LR35902::opcodeCB3A() {
  regs.pc++;
  alu::srl(regs.f, regs.d);
  return 8;
}

// SRL E
ticks_t LR35902::opcodeCB3B() {
  regs.pc++;
  alu::srl(regs.f, regs.e);
  return 8;
}

// SRL H
ticks_t LR35902::opcodeCB3C() {
  regs.pc++;
  alu::srl(regs.f, regs.h);
  return 8;
}

// SRL L
ticks_t LR35902::opcodeCB3D() {
  regs.pc++;
  alu::srl(regs.f, regs.l);
  return 8;
}

// SRL (HL)
ticks_t LR35902::opcodeCB3E() {
  regs.pc++;
  u8 v = read8(regs.hl);
  alu::srl(regs.f, v);
  write8(regs.hl, v);
  return 16;
}

// SRL A
ticks_t LR35902::opcodeCB3F() {
  regs.pc++;
  alu::srl(regs.f, regs.a);
  return 8;
}

// BIT 0,B
ticks_t LR35902::opcodeCB40() {
  regs.pc++;
  alu::bit(regs.f, regs.b, 0);
  return 8;
}

// BIT 0,C
ticks_t LR35902::opcodeCB41() {
  regs.pc++;
  alu::bit(regs.f, regs.c, 0);
  return 8;
}

// BIT 0,D
ticks_t LR35902::opcodeCB42() {
  regs.pc++;
  alu::bit(regs.f, regs.d, 0);
  return 8;
}

// BIT 0,E
ticks_t LR35902::opcodeCB43() {
  regs.pc++;
  alu::bit(regs.f, regs.e, 0);
  return 8;
}

// BIT 0,H
ticks_t LR35902::opcodeCB44() {
  regs.pc++;
  alu::bit(regs.f, regs.h, 0);
  return 8;
}

// BIT 0,L
ticks_t LR35902::opcodeCB45() {
  regs.pc++;
  alu::bit(regs.f, regs.l, 0);
  return 8;
}

// BIT 0,(HL)
ticks_t LR35902::opcodeCB46() {
  regs.pc++;
  u8 v = read8(regs.hl);
  alu::bit(regs.f, v, 0);
  write8(regs.hl, v);
  return 16;
}

// BIT 0,A
ticks_t LR35902::opcodeCB47() {
  regs.pc++;
  alu::bit(regs.f, regs.a, 0);
  return 8;
}

// BIT 1,B
ticks_t LR35902::opcodeCB48() {
  regs.pc++;
  alu::bit(regs.f, regs.b, 1);
  return 8;
}

// BIT 1,C
ticks_t LR35902::opcodeCB49() {
  regs.pc++;
  alu::bit(regs.f, regs.c, 1);
  return 8;
}

// BIT 1,D
ticks_t LR35902::opcodeCB4A() {
  regs.pc++;
  alu::bit(regs.f, regs.d, 1);
  return 8;
}

// BIT 1,E
ticks_t LR35902::opcodeCB4B() {
  regs.pc++;
  alu::bit(regs.f, regs.e, 1);
  return 8;
}

// BIT 1,H
ticks_t LR35902::opcodeCB4C() {
  regs.pc++;
  alu::bit(regs.f, regs.h, 1);
  return 8;
}

// BIT 1,L
ticks_t LR35902::opcodeCB4D() {
  regs.pc++;
  alu::bit(regs.f, regs.l, 1);
  return 8;
}

// BIT 1,(HL)
ticks_t LR35902::opcodeCB4E() {
  regs.pc++;
  u8 v = read8(regs.hl);
  alu::bit(regs.f, v, 1);
  write8(regs.hl, v);
  return 16;
}

// BIT 1,A
ticks_t LR35902::opcodeCB4F() {
  regs.pc++;
  alu::bit(regs.f, regs.a, 1);
  return 8;
}

// BIT 2,B
ticks_t LR35902::opcodeCB50() {
  regs.pc++;
  alu::bit(regs.f, regs.b, 2);
  return 8;
}

// BIT 2,C
ticks_t LR35902::opcodeCB51() {
  regs.pc++;
  alu::bit(regs.f, regs.c, 2);
  return 8;
}

// BIT 2,D
ticks_t LR35902::opcodeCB52() {
  regs.pc++;
  alu::bit(regs.f, regs.d, 2);
  return 8;
}

// BIT 2,E
ticks_t LR35902::opcodeCB53() {
  regs.pc++;
  alu::bit(regs.f, regs.e, 2);
  return 8;
}

// BIT 2,H
ticks_t LR35902::opcodeCB54() {
  regs.pc++;
  alu::bit(regs.f, regs.h, 2);
  return 8;
}

// BIT 2,L
ticks_t LR35902::opcodeCB55() {
  regs.pc++;
  alu::bit(regs.f, regs.l, 2);
  return 8;
}

// BIT 2,(HL)
ticks_t LR35902::opcodeCB56() {
  regs.pc++;
  u8 v = read8(regs.hl);
  alu::bit(regs.f, v, 2);
  write8(regs.hl, v);
  return 16;
}

// BIT 2,A
ticks_t LR35902::opcodeCB57() {
  regs.pc++;
  alu::bit(regs.f, regs.a, 2);
  return 8;
}

// BIT 3,B
ticks_t LR35902::opcodeCB58() {
  regs.pc++;
  alu::bit(regs.f, regs.b, 3);
  return 8;
}

// BIT 3,C
ticks_t LR35902::opcodeCB59() {
  regs.pc++;
  alu::bit(regs.f, regs.c, 3);
  return 8;
}

// BIT 3,D
ticks_t LR35902::opcodeCB5A() {
  regs.pc++;
  alu::bit(regs.f, regs.d, 3);
  return 8;
}

// BIT 3,E
ticks_t LR35902::opcodeCB5B() {
  regs.pc++;
  alu::bit(regs.f, regs.e, 3);
  return 8;
}

// BIT 3,H
ticks_t LR35902::opcodeCB5C() {
  regs.pc++;
  alu::bit(regs.f, regs.h, 3);
  return 8;
}

// BIT 3,L
ticks_t LR35902::opcodeCB5D() {
  regs.pc++;
  alu::bit(regs.f, regs.l, 3);
  return 8;
}

// BIT 3,(HL)
ticks_t LR35902::opcodeCB5E() {
  regs.pc++;
  u8 v = read8(regs.hl);
  alu::bit(regs.f, v, 3);
  write8(regs.hl, v);
  return 16;
}

// BIT 3,A
ticks_t LR35902::opcodeCB5F() {
  regs.pc++;
  alu::bit(regs.f, regs.a, 3);
  return 8;
}

// BIT 4,B
ticks_t LR35902::opcodeCB60() {
  regs.pc++;
  alu::bit(regs.f, regs.b, 4);
  return 8;
}

// BIT 4,C
ticks_t LR35902::opcodeCB61() {
  regs.pc++;
  alu::bit(regs.f, regs.c, 4);
  return 8;
}

// BIT 4,D
ticks_t LR35902::opcodeCB62() {
  regs.pc++;
  alu::bit(regs.f, regs.d, 4);
  return 8;
}

// BIT 4,E
ticks_t LR35902::opcodeCB63() {
  regs.pc++;
  alu::bit(regs.f, regs.e, 4);
  return 8;
}

// BIT 4,H
ticks_t LR35902::opcodeCB64() {
  regs.pc++;
  alu::bit(regs.f, regs.h, 4);
  return 8;
}

// BIT 4,L
ticks_t LR35902::opcodeCB65() {
  regs.pc++;
  alu::bit(regs.f, regs.l, 4);
  return 8;
}

// BIT 4,(HL)
ticks_t LR35902::opcodeCB66() {
  regs.pc++;
  u8 v = read8(regs.hl);
  alu::bit(regs.f, v, 4);
  write8(regs.hl, v);
  return 16;
}

// BIT 4,A
ticks_t LR35902::opcodeCB67() {
  regs.pc++;
  alu::bit(regs.f, regs.a, 4);
  return 8;
}

// BIT 5,B
ticks_t LR35902::opcodeCB68() {
  regs.pc++;
  alu::bit(regs.f, regs.b, 5);
  return 8;
}

// BIT 5,C
ticks_t LR35902::opcodeCB69() {
  regs.pc++;
  alu::bit(regs.f, regs.c, 5);
  return 8;
}

// BIT 5,D
ticks_t LR35902::opcodeCB6A() {
  regs.pc++;
  alu::bit(regs.f, regs.d, 5);
  return 8;
}

// BIT 5,E
ticks_t LR35902::opcodeCB6B() {
  regs.pc++;
  alu::bit(regs.f, regs.e, 5);
  return 8;
}

// BIT 5,H
ticks_t LR35902::opcodeCB6C() {
  regs.pc++;
  alu::bit(regs.f, regs.h, 5);
  return 8;
}

// BIT 5,L
ticks_t LR35902::opcodeCB6D() {
  regs.pc++;
  alu::bit(regs.f, regs.l, 5);
  return 8;
}

// BIT 5,(HL)
ticks_t LR35902::opcodeCB6E() {
  regs.pc++;
  u8 v = read8(regs.hl);
  alu::bit(regs.f, v, 5);
  write8(regs.hl, v);
  return 16;
}

// BIT 5,A
ticks_t LR35902::opcodeCB6F() {
  regs.pc++;
  alu::bit(regs.f, regs.a, 5);
  return 8;
}

// BIT 6,B
ticks_t LR35902::opcodeCB70() {
  regs.pc++;
  alu::bit(regs.f, regs.b, 6);
  return 8;
}

// BIT 6,C
ticks_t LR35902::opcodeCB71() {
  regs.pc++;
  alu::bit(regs.f, regs.c, 6);
  return 8;
}

// BIT 6,D
ticks_t LR35902::opcodeCB72() {
  regs.pc++;
  alu::bit(regs.f, regs.d, 6);
  return 8;
}

// BIT 6,E
ticks_t LR35902::opcodeCB73() {
  regs.pc++;
  alu::bit(regs.f, regs.e, 6);
  return 8;
}

// BIT 6,H
ticks_t LR35902::opcodeCB74() {
  regs.pc++;
  alu::bit(regs.f, regs.h, 6);
  return 8;
}

// BIT 6,L
ticks_t LR35902::opcodeCB75() {
  regs.pc++;
  alu::bit(regs.f, regs.l, 6);
  return 8;
}

// BIT 6,(HL)
ticks_t LR35902::opcodeCB76() {
  regs.pc++;
  u8 v = read8(regs.hl);
  alu::bit(regs.f, v, 6);
  write8(regs.hl, v);
  return 16;
}

// BIT 6,A
ticks_t LR35902::opcodeCB77() {
  regs.pc++;
  alu::bit(regs.f, regs.a, 6);
  return 8;
}

// BIT 7,B
ticks_t LR35902::opcodeCB78() {
  regs.pc++;
  alu::bit(regs.f, regs.b, 7);
  return 8;
}

// BIT 7,C
ticks_t LR35902::opcodeCB79() {
  regs.pc++;
  alu::bit(regs.f, regs.c, 7);
  return 8;
}

// BIT 7,D
ticks_t LR35902::opcodeCB7A() {
  regs.pc++;
  alu::bit(regs.f, regs.d, 7);
  return 8;
}

// BIT 7,E
ticks_t LR35902::opcodeCB7B() {
  regs.pc++;
  alu::bit(regs.f, regs.e, 7);
  return 8;
}

// BIT 7,H
ticks_t LR35902::opcodeCB7C() {
  regs.pc++;
  alu::bit(regs.f, regs.h, 7);
  return 8;
}

// BIT 7,L
ticks_t LR35902::opcodeCB7D() {
  regs.pc++;
  alu::bit(regs.f, regs.l, 7);
  return 8;
}

// BIT 7,(HL)
ticks_t LR35902::opcodeCB7E() {
  regs.pc++;
  u8 v = read8(regs.hl);
  alu::bit(regs.f, v, 7);
  write8(regs.hl, v);
  return 16;
}

// BIT 7,A
ticks_t LR35902::opcodeCB7F() {
  regs.pc++;
  alu::bit(regs.f, regs.a, 7);
  return 8;
}

// RES 0,B
ticks_t LR35902::opcodeCB80() {
  regs.pc++;
  alu::res(regs.f, regs.b, 0);
  return 8;
}

// RES 0,C
ticks_t LR35902::opcodeCB81() {
  regs.pc++;
  alu::res(regs.f, regs.c, 0);
  return 8;
}

// RES 0,D
ticks_t LR35902::opcodeCB82() {
  regs.pc++;
  alu::res(regs.f, regs.d, 0);
  return 8;
}

// RES 0,E
ticks_t LR35902::opcodeCB83() {
  regs.pc++;
  alu::res(regs.f, regs.e, 0);
  return 8;
}

// RES 0,H
ticks_t LR35902::opcodeCB84() {
  regs.pc++;
  alu::res(regs.f, regs.h, 0);
  return 8;
}

// RES 0,L
ticks_t LR35902::opcodeCB85() {
  regs.pc++;
  alu::res(regs.f, regs.l, 0);
  return 8;
}

// RES 0,(HL)
ticks_t LR35902::opcodeCB86() {
  regs.pc++;
  u8 v = read8(regs.hl);
  alu::res(regs.f, v, 0);
  write8(regs.hl, v);
  return 16;
}

// RES 0,A
ticks_t LR35902::opcodeCB87() {
  regs.pc++;
  alu::res(regs.f, regs.a, 0);
  return 8;
}

// RES 1,B
ticks_t LR35902::opcodeCB88() {
  regs.pc++;
  alu::res(regs.f, regs.b, 1);
  return 8;
}

// RES 1,C
ticks_t LR35902::opcodeCB89() {
  regs.pc++;
  alu::res(regs.f, regs.c, 1);
  return 8;
}

// RES 1,D
ticks_t LR35902::opcodeCB8A() {
  regs.pc++;
  alu::res(regs.f, regs.d, 1);
  return 8;
}

// RES 1,E
ticks_t LR35902::opcodeCB8B() {
  regs.pc++;
  alu::res(regs.f, regs.e, 1);
  return 8;
}

// RES 1,H
ticks_t LR35902::opcodeCB8C() {
  regs.pc++;
  alu::res(regs.f, regs.h, 1);
  return 8;
}

// RES 1,L
ticks_t LR35902::opcodeCB8D() {
  regs.pc++;
  alu::res(regs.f, regs.l, 1);
  return 8;
}

// RES 1,(HL)
ticks_t LR35902::opcodeCB8E() {
  regs.pc++;
  u8 v = read8(regs.hl);
  alu::res(regs.f, v, 1);
  write8(regs.hl, v);
  return 16;
}

// RES 1,A
ticks_t LR35902::opcodeCB8F() {
  regs.pc++;
  alu::res(regs.f, regs.a, 1);
  return 8;
}

// RES 2,B
ticks_t LR35902::opcodeCB90() {
  regs.pc++;
  alu::res(regs.f, regs.b, 2);
  return 8;
}

// RES 2,C
ticks_t LR35902::opcodeCB91() {
  regs.pc++;
  alu::res(regs.f, regs.c, 2);
  return 8;
}

// RES 2,D
ticks_t LR35902::opcodeCB92() {
  regs.pc++;
  alu::res(regs.f, regs.d, 2);
  return 8;
}

// RES 2,E
ticks_t LR35902::opcodeCB93() {
  regs.pc++;
  alu::res(regs.f, regs.e, 2);
  return 8;
}

// RES 2,H
ticks_t LR35902::opcodeCB94() {
  regs.pc++;
  alu::res(regs.f, regs.h, 2);
  return 8;
}

// RES 2,L
ticks_t LR35902::opcodeCB95() {
  regs.pc++;
  alu::res(regs.f, regs.l, 2);
  return 8;
}

// RES 2,(HL)
ticks_t LR35902::opcodeCB96() {
  regs.pc++;
  u8 v = read8(regs.hl);
  alu::res(regs.f, v, 2);
  write8(regs.hl, v);
  return 16;
}

// RES 2,A
ticks_t LR35902::opcodeCB97() {
  regs.pc++;
  alu::res(regs.f, regs.a, 2);
  return 8;
}

// RES 3,B
ticks_t LR35902::opcodeCB98() {
  regs.pc++;
  alu::res(regs.f, regs.b, 3);
  return 8;
}

// RES 3,C
ticks_t LR35902::opcodeCB99() {
  regs.pc++;
  alu::res(regs.f, regs.c, 3);
  return 8;
}

// RES 3,D
ticks_t LR35902::opcodeCB9A() {
  regs.pc++;
  alu::res(regs.f, regs.d, 3);
  return 8;
}

// RES 3,E
ticks_t LR35902::opcodeCB9B() {
  regs.pc++;
  alu::res(regs.f, regs.e, 3);
  return 8;
}

// RES 3,H
ticks_t LR35902::opcodeCB9C() {
  regs.pc++;
  alu::res(regs.f, regs.h, 3);
  return 8;
}

// RES 3,L
ticks_t LR35902::opcodeCB9D() {
  regs.pc++;
  alu::res(regs.f, regs.l, 3);
  return 8;
}

// RES 3,(HL)
ticks_t LR35902::opcodeCB9E() {
  regs.pc++;
  u8 v = read8(regs.hl);
  alu::res(regs.f, v, 3);
  write8(regs.hl, v);
  return 16;
}

// RES 3,A
ticks_t LR35902::opcodeCB9F() {
  regs.pc++;
  alu::res(regs.f, regs.a, 3);
  return 8;
}

// RES 4,B
ticks_t LR35902::opcodeCBA0() {
  regs.pc++;
  alu::res(regs.f, regs.b, 4);
  return 8;
}

// RES 4,C
ticks_t LR35902::opcodeCBA1() {
  regs.pc++;
  alu::res(regs.f, regs.c, 4);
  return 8;
}

// RES 4,D
ticks_t LR35902::opcodeCBA2() {
  regs.pc++;
  alu::res(regs.f, regs.d, 4);
  return 8;
}

// RES 4,E
ticks_t LR35902::opcodeCBA3() {
  regs.pc++;
  alu::res(regs.f, regs.e, 4);
  return 8;
}

// RES 4,H
ticks_t LR35902::opcodeCBA4() {
  regs.pc++;
  alu::res(regs.f, regs.h, 4);
  return 8;
}

// RES 4,L
ticks_t LR35902::opcodeCBA5() {
  regs.pc++;
  alu::res(regs.f, regs.l, 4);
  return 8;
}

// RES 4,(HL)
ticks_t LR35902::opcodeCBA6() {
  regs.pc++;
  u8 v = read8(regs.hl);
  alu::res(regs.f, v, 4);
  write8(regs.hl, v);
  return 16;
}

// RES 4,A
ticks_t LR35902::opcodeCBA7() {
  regs.pc++;
  alu::res(regs.f, regs.a, 4);
  return 8;
}

// RES 5,B
ticks_t LR35902::opcodeCBA8() {
  regs.pc++;
  alu::res(regs.f, regs.b, 5);
  return 8;
}

// RES 5,C
ticks_t LR35902::opcodeCBA9() {
  regs.pc++;
  alu::res(regs.f, regs.c, 5);
  return 8;
}

// RES 5,D
ticks_t LR35902::opcodeCBAA() {
  regs.pc++;
  alu::res(regs.f, regs.d, 5);
  return 8;
}

// RES 5,E
ticks_t LR35902::opcodeCBAB() {
  regs.pc++;
  alu::res(regs.f, regs.e, 5);
  return 8;
}

// RES 5,H
ticks_t LR35902::opcodeCBAC() {
  regs.pc++;
  alu::res(regs.f, regs.h, 5);
  return 8;
}

// RES 5,L
ticks_t LR35902::opcodeCBAD() {
  regs.pc++;
  alu::res(regs.f, regs.l, 5);
  return 8;
}

// RES 5,(HL)
ticks_t LR35902::opcodeCBAE() {
  regs.pc++;
  u8 v = read8(regs.hl);
  alu::res(regs.f, v, 5);
  write8(regs.hl, v);
  return 16;
}

// RES 5,A
ticks_t LR35902::opcodeCBAF() {
  regs.pc++;
  alu::res(regs.f, regs.a, 5);
  return 8;
}

// RES 6,B
ticks_t LR35902::opcodeCBB0() {
  regs.pc++;
  alu::res(regs.f, regs.b, 6);
  return 8;
}

// RES 6,C
ticks_t LR35902::opcodeCBB1() {
  regs.pc++;
  alu::res(regs.f, regs.c, 6);
  return 8;
}

// RES 6,D
ticks_t LR35902::opcodeCBB2() {
  regs.pc++;
  alu::res(regs.f, regs.d, 6);
  return 8;
}

// RES 6,E
ticks_t LR35902::opcodeCBB3() {
  regs.pc++;
  alu::res(regs.f, regs.e, 6);
  return 8;
}

// RES 6,H
ticks_t LR35902::opcodeCBB4() {
  regs.pc++;
  alu::res(regs.f, regs.h, 6);
  return 8;
}

// RES 6,L
ticks_t LR35902::opcodeCBB5() {
  regs.pc++;
  alu::res(regs.f, regs.l, 6);
  return 8;
}

// RES 6,(HL)
ticks_t LR35902::opcodeCBB6() {
  regs.pc++;
  u8 v = read8(regs.hl);
  alu::res(regs.f, v, 6);
  write8(regs.hl, v);
  return 16;
}

// RES 6,A
ticks_t LR35902::opcodeCBB7() {
  regs.pc++;
  alu::res(regs.f, regs.a, 6);
  return 8;
}

// RES 7,B
ticks_t LR35902::opcodeCBB8() {
  regs.pc++;
  alu::res(regs.f, regs.b, 7);
  return 8;
}

// RES 7,C
ticks_t LR35902::opcodeCBB9() {
  regs.pc++;
  alu::res(regs.f, regs.c, 7);
  return 8;
}

// RES 7,D
ticks_t LR35902::opcodeCBBA() {
  regs.pc++;
  alu::res(regs.f, regs.d, 7);
  return 8;
}

// RES 7,E
ticks_t LR35902::opcodeCBBB() {
  regs.pc++;
  alu::res(regs.f, regs.e, 7);
  return 8;
}

// RES 7,H
ticks_t LR35902::opcodeCBBC() {
  regs.pc++;
  alu::res(regs.f, regs.h, 7);
  return 8;
}

// RES 7,L
ticks_t LR35902::opcodeCBBD() {
  regs.pc++;
  alu::res(regs.f, regs.l, 7);
  return 8;
}

// RES 7,(HL)
ticks_t LR35902::opcodeCBBE() {
  regs.pc++;
  u8 v = read8(regs.hl);
  alu::res(regs.f, v, 7);
  write8(regs.hl, v);
  return 16;
}

// RES 7,A
ticks_t LR35902::opcodeCBBF() {
  regs.pc++;
  alu::res(regs.f, regs.a, 7);
  return 8;
}

// SET 0,B
ticks_t LR35902::opcodeCBC0() {
  regs.pc++;
  alu::set(regs.f, regs.b, 0);
  return 8;
}

// SET 0,C
ticks_t LR35902::opcodeCBC1() {
  regs.pc++;
  alu::set(regs.f, regs.c, 0);
  return 8;
}

// SET 0,D
ticks_t LR35902::opcodeCBC2() {
  regs.pc++;
  alu::set(regs.f, regs.d, 0);
  return 8;
}

// SET 0,E
ticks_t LR35902::opcodeCBC3() {
  regs.pc++;
  alu::set(regs.f, regs.e, 0);
  return 8;
}

// SET 0,H
ticks_t LR35902::opcodeCBC4() {
  regs.pc++;
  alu::set(regs.f, regs.h, 0);
  return 8;
}

// SET 0,L
ticks_t LR35902::opcodeCBC5() {
  regs.pc++;
  alu::set(regs.f, regs.l, 0);
  return 8;
}

// SET 0,(HL)
ticks_t LR35902::opcodeCBC6() {
  regs.pc++;
  u8 v = read8(regs.hl);
  alu::set(regs.f, v, 0);
  write8(regs.hl, v);
  return 16;
}

// SET 0,A
ticks_t LR35902::opcodeCBC7() {
  regs.pc++;
  alu::set(regs.f, regs.a, 0);
  return 8;
}

// SET 1,B
ticks_t LR35902::opcodeCBC8() {
  regs.pc++;
  alu::set(regs.f, regs.b, 1);
  return 8;
}

// SET 1,C
ticks_t LR35902::opcodeCBC9() {
  regs.pc++;
  alu::set(regs.f, regs.c, 1);
  return 8;
}

// SET 1,D
ticks_t LR35902::opcodeCBCA() {
  regs.pc++;
  alu::set(regs.f, regs.d, 1);
  return 8;
}

// SET 1,E
ticks_t LR35902::opcodeCBCB() {
  regs.pc++;
  alu::set(regs.f, regs.e, 1);
  return 8;
}

// SET 1,H
ticks_t LR35902::opcodeCBCC() {
  regs.pc++;
  alu::set(regs.f, regs.h, 1);
  return 8;
}

// SET 1,L
ticks_t LR35902::opcodeCBCD() {
  regs.pc++;
  alu::set(regs.f, regs.l, 1);
  return 8;
}

// SET 1,(HL)
ticks_t LR35902::opcodeCBCE() {
  regs.pc++;
  u8 v = read8(regs.hl);
  alu::set(regs.f, v, 1);
  write8(regs.hl, v);
  return 16;
}

// SET 1,A
ticks_t LR35902::opcodeCBCF() {
  regs.pc++;
  alu::set(regs.f, regs.a, 1);
  return 8;
}

// SET 2,B
ticks_t LR35902::opcodeCBD0() {
  regs.pc++;
  alu::set(regs.f, regs.b, 2);
  return 8;
}

// SET 2,C
ticks_t LR35902::opcodeCBD1() {
  regs.pc++;
  alu::set(regs.f, regs.c, 2);
  return 8;
}

// SET 2,D
ticks_t LR35902::opcodeCBD2() {
  regs.pc++;
  alu::set(regs.f, regs.d, 2);
  return 8;
}

// SET 2,E
ticks_t LR35902::opcodeCBD3() {
  regs.pc++;
  alu::set(regs.f, regs.e, 2);
  return 8;
}

// SET 2,H
ticks_t LR35902::opcodeCBD4() {
  regs.pc++;
  alu::set(regs.f, regs.h, 2);
  return 8;
}

// SET 2,L
ticks_t LR35902::opcodeCBD5() {
  regs.pc++;
  alu::set(regs.f, regs.l, 2);
  return 8;
}

// SET 2,(HL)
ticks_t LR35902::opcodeCBD6() {
  regs.pc++;
  u8 v = read8(regs.hl);
  alu::set(regs.f, v, 2);
  write8(regs.hl, v);
  return 16;
}

// SET 2,A
ticks_t LR35902::opcodeCBD7() {
  regs.pc++;
  alu::set(regs.f, regs.a, 2);
  return 8;
}

// SET 3,B
ticks_t LR35902::opcodeCBD8() {
  regs.pc++;
  alu::set(regs.f, regs.b, 3);
  return 8;
}

// SET 3,C
ticks_t LR35902::opcodeCBD9() {
  regs.pc++;
  alu::set(regs.f, regs.c, 3);
  return 8;
}

// SET 3,D
ticks_t LR35902::opcodeCBDA() {
  regs.pc++;
  alu::set(regs.f, regs.d, 3);
  return 8;
}

// SET 3,E
ticks_t LR35902::opcodeCBDB() {
  regs.pc++;
  alu::set(regs.f, regs.e, 3);
  return 8;
}

// SET 3,H
ticks_t LR35902::opcodeCBDC() {
  regs.pc++;
  alu::set(regs.f, regs.h, 3);
  return 8;
}

// SET 3,L
ticks_t LR35902::opcodeCBDD() {
  regs.pc++;
  alu::set(regs.f, regs.l, 3);
  return 8;
}

// SET 3,(HL)
ticks_t LR35902::opcodeCBDE() {
  regs.pc++;
  u8 v = read8(regs.hl);
  alu::set(regs.f, v, 3);
  write8(regs.hl, v);
  return 16;
}

// SET 3,A
ticks_t LR35902::opcodeCBDF() {
  regs.pc++;
  alu::set(regs.f, regs.a, 3);
  return 8;
}

// SET 4,B
ticks_t LR35902::opcodeCBE0() {
  regs.pc++;
  alu::set(regs.f, regs.b, 4);
  return 8;
}

// SET 4,C
ticks_t LR35902::opcodeCBE1() {
  regs.pc++;
  alu::set(regs.f, regs.c, 4);
  return 8;
}

// SET 4,D
ticks_t LR35902::opcodeCBE2() {
  regs.pc++;
  alu::set(regs.f, regs.d, 4);
  return 8;
}

// SET 4,E
ticks_t LR35902::opcodeCBE3() {
  regs.pc++;
  alu::set(regs.f, regs.e, 4);
  return 8;
}

// SET 4,H
ticks_t LR35902::opcodeCBE4() {
  regs.pc++;
  alu::set(regs.f, regs.h, 4);
  return 8;
}

// SET 4,L
ticks_t LR35902::opcodeCBE5() {
  regs.pc++;
  alu::set(regs.f, regs.l, 4);
  return 8;
}

// SET 4,(HL)
ticks_t LR35902::opcodeCBE6() {
  regs.pc++;
  u8 v = read8(regs.hl);
  alu::set(regs.f, v, 4);
  write8(regs.hl, v);
  return 16;
}

// SET 4,A
ticks_t LR35902::opcodeCBE7() {
  regs.pc++;
  alu::set(regs.f, regs.a, 4);
  return 8;
}

// SET 5,B
ticks_t LR35902::opcodeCBE8() {
  regs.pc++;
  alu::set(regs.f, regs.b, 5);
  return 8;
}

// SET 5,C
ticks_t LR35902::opcodeCBE9() {
  regs.pc++;
  alu::set(regs.f, regs.c, 5);
  return 8;
}

// SET 5,D
ticks_t LR35902::opcodeCBEA() {
  regs.pc++;
  alu::set(regs.f, regs.d, 5);
  return 8;
}

// SET 5,E
ticks_t LR35902::opcodeCBEB() {
  regs.pc++;
  alu::set(regs.f, regs.e, 5);
  return 8;
}

// SET 5,H
ticks_t LR35902::opcodeCBEC() {
  regs.pc++;
  alu::set(regs.f, regs.h, 5);
  return 8;
}

// SET 5,L
ticks_t LR35902::opcodeCBED() {
  regs.pc++;
  alu::set(regs.f, regs.l, 5);
  return 8;
}

// SET 5,(HL)
ticks_t LR35902::opcodeCBEE() {
  regs.pc++;
  u8 v = read8(regs.hl);
  alu::set(regs.f, v, 5);
  write8(regs.hl, v);
  return 16;
}

// SET 5,A
ticks_t LR35902::opcodeCBEF() {
  regs.pc++;
  alu::set(regs.f, regs.a, 5);
  return 8;
}

// SET 6,B
ticks_t LR35902::opcodeCBF0() {
  regs.pc++;
  alu::set(regs.f, regs.b, 6);
  return 8;
}

// SET 6,C
ticks_t LR35902::opcodeCBF1() {
  regs.pc++;
  alu::set(regs.f, regs.c, 6);
  return 8;
}

// SET 6,D
ticks_t LR35902::opcodeCBF2() {
  regs.pc++;
  alu::set(regs.f, regs.d, 6);
  return 8;
}

// SET 6,E
ticks_t LR35902::opcodeCBF3() {
  regs.pc++;
  alu::set(regs.f, regs.e, 6);
  return 8;
}

// SET 6,H
ticks_t LR35902::opcodeCBF4() {
  regs.pc++;
  alu::set(regs.f, regs.h, 6);
  return 8;
}

// SET 6,L
ticks_t LR35902::opcodeCBF5() {
  regs.pc++;
  alu::set(regs.f, regs.l, 6);
  return 8;
}

// SET 6,(HL)
ticks_t LR35902::opcodeCBF6() {
  regs.pc++;
  u8 v = read8(regs.hl);
  alu::set(regs.f, v, 6);
  write8(regs.hl, v);
  return 16;
}

// SET 6,A
ticks_t LR35902::opcodeCBF7() {
  regs.pc++;
  alu::set(regs.f, regs.a, 6);
  return 8;
}

// SET 7,B
ticks_t LR35902::opcodeCBF8() {
  regs.pc++;
  alu::set(regs.f, regs.b, 7);
  return 8;
}

// SET 7,C
ticks_t LR35902::opcodeCBF9() {
  regs.pc++;
  alu::set(regs.f, regs.c, 7);
  return 8;
}

// SET 7,D
ticks_t LR35902::opcodeCBFA() {
  regs.pc++;
  alu::set(regs.f, regs.d, 7);
  return 8;
}

// SET 7,E
ticks_t LR35902::opcodeCBFB() {
  regs.pc++;
  alu::set(regs.f, regs.e, 7);
  return 8;
}

// SET 7,H
ticks_t LR35902::opcodeCBFC() {
  regs.pc++;
  alu::set(regs.f, regs.h, 7);
  return 8;
}

// SET 7,L
ticks_t LR35902::opcodeCBFD() {
  regs.pc++;
  alu::set(regs.f, regs.l, 7);
  return 8;
}

// SET 7,(HL)
ticks_t LR35902::opcodeCBFE() {
  regs.pc++;
  u8 v = read8(regs.hl);
  alu::set(regs.f, v, 7);
  write8(regs.hl, v);
  return 16;
}

// SET 7,A
ticks_t LR35902::opcodeCBFF() {
  regs.pc++;
  alu::set(regs.f, regs.a, 7);
  return 8;
}

void LR35902::populateInstructionSets() {
  iset_.at(0x00) = &LR35902::opcode00;
  iset_.at(0x01) = &LR35902::opcode01;
  iset_.at(0x02) = &LR35902::opcode02;
  iset_.at(0x03) = &LR35902::opcode03;
  iset_.at(0x04) = &LR35902::opcode04;
  iset_.at(0x05) = &LR35902::opcode05;
  iset_.at(0x06) = &LR35902::opcode06;
  iset_.at(0x07) = &LR35902::opcode07;
  iset_.at(0x08) = &LR35902::opcode08;
  iset_.at(0x09) = &LR35902::opcode09;
  iset_.at(0x0a) = &LR35902::opcode0A;
  iset_.at(0x0b) = &LR35902::opcode0B;
  iset_.at(0x0c) = &LR35902::opcode0C;
  iset_.at(0x0d) = &LR35902::opcode0D;
  iset_.at(0x0e) = &LR35902::opcode0E;
  iset_.at(0x0f) = &LR35902::opcode0F;
  iset_.at(0x10) = &LR35902::opcode10;
  iset_.at(0x11) = &LR35902::opcode11;
  iset_.at(0x12) = &LR35902::opcode12;
  iset_.at(0x13) = &LR35902::opcode13;
  iset_.at(0x14) = &LR35902::opcode14;
  iset_.at(0x15) = &LR35902::opcode15;
  iset_.at(0x16) = &LR35902::opcode16;
  iset_.at(0x17) = &LR35902::opcode17;
  iset_.at(0x18) = &LR35902::opcode18;
  iset_.at(0x19) = &LR35902::opcode19;
  iset_.at(0x1a) = &LR35902::opcode1A;
  iset_.at(0x1b) = &LR35902::opcode1B;
  iset_.at(0x1c) = &LR35902::opcode1C;
  iset_.at(0x1d) = &LR35902::opcode1D;
  iset_.at(0x1e) = &LR35902::opcode1E;
  iset_.at(0x1f) = &LR35902::opcode1F;
  iset_.at(0x20) = &LR35902::opcode20;
  iset_.at(0x21) = &LR35902::opcode21;
  iset_.at(0x22) = &LR35902::opcode22;
  iset_.at(0x23) = &LR35902::opcode23;
  iset_.at(0x24) = &LR35902::opcode24;
  iset_.at(0x25) = &LR35902::opcode25;
  iset_.at(0x26) = &LR35902::opcode26;
  iset_.at(0x27) = &LR35902::opcode27;
  iset_.at(0x28) = &LR35902::opcode28;
  iset_.at(0x29) = &LR35902::opcode29;
  iset_.at(0x2a) = &LR35902::opcode2A;
  iset_.at(0x2b) = &LR35902::opcode2B;
  iset_.at(0x2c) = &LR35902::opcode2C;
  iset_.at(0x2d) = &LR35902::opcode2D;
  iset_.at(0x2e) = &LR35902::opcode2E;
  iset_.at(0x2f) = &LR35902::opcode2F;
  iset_.at(0x30) = &LR35902::opcode30;
  iset_.at(0x31) = &LR35902::opcode31;
  iset_.at(0x32) = &LR35902::opcode32;
  iset_.at(0x33) = &LR35902::opcode33;
  iset_.at(0x34) = &LR35902::opcode34;
  iset_.at(0x35) = &LR35902::opcode35;
  iset_.at(0x36) = &LR35902::opcode36;
  iset_.at(0x37) = &LR35902::opcode37;
  iset_.at(0x38) = &LR35902::opcode38;
  iset_.at(0x39) = &LR35902::opcode39;
  iset_.at(0x3a) = &LR35902::opcode3A;
  iset_.at(0x3b) = &LR35902::opcode3B;
  iset_.at(0x3c) = &LR35902::opcode3C;
  iset_.at(0x3d) = &LR35902::opcode3D;
  iset_.at(0x3e) = &LR35902::opcode3E;
  iset_.at(0x3f) = &LR35902::opcode3F;
  iset_.at(0x40) = &LR35902::opcode40;
  iset_.at(0x41) = &LR35902::opcode41;
  iset_.at(0x42) = &LR35902::opcode42;
  iset_.at(0x43) = &LR35902::opcode43;
  iset_.at(0x44) = &LR35902::opcode44;
  iset_.at(0x45) = &LR35902::opcode45;
  iset_.at(0x46) = &LR35902::opcode46;
  iset_.at(0x47) = &LR35902::opcode47;
  iset_.at(0x48) = &LR35902::opcode48;
  iset_.at(0x49) = &LR35902::opcode49;
  iset_.at(0x4a) = &LR35902::opcode4A;
  iset_.at(0x4b) = &LR35902::opcode4B;
  iset_.at(0x4c) = &LR35902::opcode4C;
  iset_.at(0x4d) = &LR35902::opcode4D;
  iset_.at(0x4e) = &LR35902::opcode4E;
  iset_.at(0x4f) = &LR35902::opcode4F;
  iset_.at(0x50) = &LR35902::opcode50;
  iset_.at(0x51) = &LR35902::opcode51;
  iset_.at(0x52) = &LR35902::opcode52;
  iset_.at(0x53) = &LR35902::opcode53;
  iset_.at(0x54) = &LR35902::opcode54;
  iset_.at(0x55) = &LR35902::opcode55;
  iset_.at(0x56) = &LR35902::opcode56;
  iset_.at(0x57) = &LR35902::opcode57;
  iset_.at(0x58) = &LR35902::opcode58;
  iset_.at(0x59) = &LR35902::opcode59;
  iset_.at(0x5a) = &LR35902::opcode5A;
  iset_.at(0x5b) = &LR35902::opcode5B;
  iset_.at(0x5c) = &LR35902::opcode5C;
  iset_.at(0x5d) = &LR35902::opcode5D;
  iset_.at(0x5e) = &LR35902::opcode5E;
  iset_.at(0x5f) = &LR35902::opcode5F;
  iset_.at(0x60) = &LR35902::opcode60;
  iset_.at(0x61) = &LR35902::opcode61;
  iset_.at(0x62) = &LR35902::opcode62;
  iset_.at(0x63) = &LR35902::opcode63;
  iset_.at(0x64) = &LR35902::opcode64;
  iset_.at(0x65) = &LR35902::opcode65;
  iset_.at(0x66) = &LR35902::opcode66;
  iset_.at(0x67) = &LR35902::opcode67;
  iset_.at(0x68) = &LR35902::opcode68;
  iset_.at(0x69) = &LR35902::opcode69;
  iset_.at(0x6a) = &LR35902::opcode6A;
  iset_.at(0x6b) = &LR35902::opcode6B;
  iset_.at(0x6c) = &LR35902::opcode6C;
  iset_.at(0x6d) = &LR35902::opcode6D;
  iset_.at(0x6e) = &LR35902::opcode6E;
  iset_.at(0x6f) = &LR35902::opcode6F;
  iset_.at(0x70) = &LR35902::opcode70;
  iset_.at(0x71) = &LR35902::opcode71;
  iset_.at(0x72) = &LR35902::opcode72;
  iset_.at(0x73) = &LR35902::opcode73;
  iset_.at(0x74) = &LR35902::opcode74;
  iset_.at(0x75) = &LR35902::opcode75;
  iset_.at(0x76) = &LR35902::opcode76;
  iset_.at(0x77) = &LR35902::opcode77;
  iset_.at(0x78) = &LR35902::opcode78;
  iset_.at(0x79) = &LR35902::opcode79;
  iset_.at(0x7a) = &LR35902::opcode7A;
  iset_.at(0x7b) = &LR35902::opcode7B;
  iset_.at(0x7c) = &LR35902::opcode7C;
  iset_.at(0x7d) = &LR35902::opcode7D;
  iset_.at(0x7e) = &LR35902::opcode7E;
  iset_.at(0x7f) = &LR35902::opcode7F;
  iset_.at(0x80) = &LR35902::opcode80;
  iset_.at(0x81) = &LR35902::opcode81;
  iset_.at(0x82) = &LR35902::opcode82;
  iset_.at(0x83) = &LR35902::opcode83;
  iset_.at(0x84) = &LR35902::opcode84;
  iset_.at(0x85) = &LR35902::opcode85;
  iset_.at(0x86) = &LR35902::opcode86;
  iset_.at(0x87) = &LR35902::opcode87;
  iset_.at(0x88) = &LR35902::opcode88;
  iset_.at(0x89) = &LR35902::opcode89;
  iset_.at(0x8a) = &LR35902::opcode8A;
  iset_.at(0x8b) = &LR35902::opcode8B;
  iset_.at(0x8c) = &LR35902::opcode8C;
  iset_.at(0x8d) = &LR35902::opcode8D;
  iset_.at(0x8e) = &LR35902::opcode8E;
  iset_.at(0x8f) = &LR35902::opcode8F;
  iset_.at(0x90) = &LR35902::opcode90;
  iset_.at(0x91) = &LR35902::opcode91;
  iset_.at(0x92) = &LR35902::opcode92;
  iset_.at(0x93) = &LR35902::opcode93;
  iset_.at(0x94) = &LR35902::opcode94;
  iset_.at(0x95) = &LR35902::opcode95;
  iset_.at(0x96) = &LR35902::opcode96;
  iset_.at(0x97) = &LR35902::opcode97;
  iset_.at(0x98) = &LR35902::opcode98;
  iset_.at(0x99) = &LR35902::opcode99;
  iset_.at(0x9a) = &LR35902::opcode9A;
  iset_.at(0x9b) = &LR35902::opcode9B;
  iset_.at(0x9c) = &LR35902::opcode9C;
  iset_.at(0x9d) = &LR35902::opcode9D;
  iset_.at(0x9e) = &LR35902::opcode9E;
  iset_.at(0x9f) = &LR35902::opcode9F;
  iset_.at(0xa0) = &LR35902::opcodeA0;
  iset_.at(0xa1) = &LR35902::opcodeA1;
  iset_.at(0xa2) = &LR35902::opcodeA2;
  iset_.at(0xa3) = &LR35902::opcodeA3;
  iset_.at(0xa4) = &LR35902::opcodeA4;
  iset_.at(0xa5) = &LR35902::opcodeA5;
  iset_.at(0xa6) = &LR35902::opcodeA6;
  iset_.at(0xa7) = &LR35902::opcodeA7;
  iset_.at(0xa8) = &LR35902::opcodeA8;
  iset_.at(0xa9) = &LR35902::opcodeA9;
  iset_.at(0xaa) = &LR35902::opcodeAA;
  iset_.at(0xab) = &LR35902::opcodeAB;
  iset_.at(0xac) = &LR35902::opcodeAC;
  iset_.at(0xad) = &LR35902::opcodeAD;
  iset_.at(0xae) = &LR35902::opcodeAE;
  iset_.at(0xaf) = &LR35902::opcodeAF;
  iset_.at(0xb0) = &LR35902::opcodeB0;
  iset_.at(0xb1) = &LR35902::opcodeB1;
  iset_.at(0xb2) = &LR35902::opcodeB2;
  iset_.at(0xb3) = &LR35902::opcodeB3;
  iset_.at(0xb4) = &LR35902::opcodeB4;
  iset_.at(0xb5) = &LR35902::opcodeB5;
  iset_.at(0xb6) = &LR35902::opcodeB6;
  iset_.at(0xb7) = &LR35902::opcodeB7;
  iset_.at(0xb8) = &LR35902::opcodeB8;
  iset_.at(0xb9) = &LR35902::opcodeB9;
  iset_.at(0xba) = &LR35902::opcodeBA;
  iset_.at(0xbb) = &LR35902::opcodeBB;
  iset_.at(0xbc) = &LR35902::opcodeBC;
  iset_.at(0xbd) = &LR35902::opcodeBD;
  iset_.at(0xbe) = &LR35902::opcodeBE;
  iset_.at(0xbf) = &LR35902::opcodeBF;
  iset_.at(0xc0) = &LR35902::opcodeC0;
  iset_.at(0xc1) = &LR35902::opcodeC1;
  iset_.at(0xc2) = &LR35902::opcodeC2;
  iset_.at(0xc3) = &LR35902::opcodeC3;
  iset_.at(0xc4) = &LR35902::opcodeC4;
  iset_.at(0xc5) = &LR35902::opcodeC5;
  iset_.at(0xc6) = &LR35902::opcodeC6;
  iset_.at(0xc7) = &LR35902::opcodeC7;
  iset_.at(0xc8) = &LR35902::opcodeC8;
  iset_.at(0xc9) = &LR35902::opcodeC9;
  iset_.at(0xca) = &LR35902::opcodeCA;
  iset_.at(0xcb) = &LR35902::opcodeCB;
  iset_.at(0xcc) = &LR35902::opcodeCC;
  iset_.at(0xcd) = &LR35902::opcodeCD;
  iset_.at(0xce) = &LR35902::opcodeCE;
  iset_.at(0xcf) = &LR35902::opcodeCF;
  iset_.at(0xd0) = &LR35902::opcodeD0;
  iset_.at(0xd1) = &LR35902::opcodeD1;
  iset_.at(0xd2) = &LR35902::opcodeD2;
  iset_.at(0xd3) = &LR35902::opcodeD3;
  iset_.at(0xd4) = &LR35902::opcodeD4;
  iset_.at(0xd5) = &LR35902::opcodeD5;
  iset_.at(0xd6) = &LR35902::opcodeD6;
  iset_.at(0xd7) = &LR35902::opcodeD7;
  iset_.at(0xd8) = &LR35902::opcodeD8;
  iset_.at(0xd9) = &LR35902::opcodeD9;
  iset_.at(0xda) = &LR35902::opcodeDA;
  iset_.at(0xdb) = &LR35902::opcodeDB;
  iset_.at(0xdc) = &LR35902::opcodeDC;
  iset_.at(0xdd) = &LR35902::opcodeDD;
  iset_.at(0xde) = &LR35902::opcodeDE;
  iset_.at(0xdf) = &LR35902::opcodeDF;
  iset_.at(0xe0) = &LR35902::opcodeE0;
  iset_.at(0xe1) = &LR35902::opcodeE1;
  iset_.at(0xe2) = &LR35902::opcodeE2;
  iset_.at(0xe3) = &LR35902::opcodeE3;
  iset_.at(0xe4) = &LR35902::opcodeE4;
  iset_.at(0xe5) = &LR35902::opcodeE5;
  iset_.at(0xe6) = &LR35902::opcodeE6;
  iset_.at(0xe7) = &LR35902::opcodeE7;
  iset_.at(0xe8) = &LR35902::opcodeE8;
  iset_.at(0xe9) = &LR35902::opcodeE9;
  iset_.at(0xea) = &LR35902::opcodeEA;
  iset_.at(0xeb) = &LR35902::opcodeEB;
  iset_.at(0xec) = &LR35902::opcodeEC;
  iset_.at(0xed) = &LR35902::opcodeED;
  iset_.at(0xee) = &LR35902::opcodeEE;
  iset_.at(0xef) = &LR35902::opcodeEF;
  iset_.at(0xf0) = &LR35902::opcodeF0;
  iset_.at(0xf1) = &LR35902::opcodeF1;
  iset_.at(0xf2) = &LR35902::opcodeF2;
  iset_.at(0xf3) = &LR35902::opcodeF3;
  iset_.at(0xf4) = &LR35902::opcodeF4;
  iset_.at(0xf5) = &LR35902::opcodeF5;
  iset_.at(0xf6) = &LR35902::opcodeF6;
  iset_.at(0xf7) = &LR35902::opcodeF7;
  iset_.at(0xf8) = &LR35902::opcodeF8;
  iset_.at(0xf9) = &LR35902::opcodeF9;
  iset_.at(0xfa) = &LR35902::opcodeFA;
  iset_.at(0xfb) = &LR35902::opcodeFB;
  iset_.at(0xfc) = &LR35902::opcodeFC;
  iset_.at(0xfd) = &LR35902::opcodeFD;
  iset_.at(0xfe) = &LR35902::opcodeFE;
  iset_.at(0xff) = &LR35902::opcodeFF;
  iset_.at(0x100) = &LR35902::opcodeCB00;
  iset_.at(0x101) = &LR35902::opcodeCB01;
  iset_.at(0x102) = &LR35902::opcodeCB02;
  iset_.at(0x103) = &LR35902::opcodeCB03;
  iset_.at(0x104) = &LR35902::opcodeCB04;
  iset_.at(0x105) = &LR35902::opcodeCB05;
  iset_.at(0x106) = &LR35902::opcodeCB06;
  iset_.at(0x107) = &LR35902::opcodeCB07;
  iset_.at(0x108) = &LR35902::opcodeCB08;
  iset_.at(0x109) = &LR35902::opcodeCB09;
  iset_.at(0x10a) = &LR35902::opcodeCB0A;
  iset_.at(0x10b) = &LR35902::opcodeCB0B;
  iset_.at(0x10c) = &LR35902::opcodeCB0C;
  iset_.at(0x10d) = &LR35902::opcodeCB0D;
  iset_.at(0x10e) = &LR35902::opcodeCB0E;
  iset_.at(0x10f) = &LR35902::opcodeCB0F;
  iset_.at(0x110) = &LR35902::opcodeCB10;
  iset_.at(0x111) = &LR35902::opcodeCB11;
  iset_.at(0x112) = &LR35902::opcodeCB12;
  iset_.at(0x113) = &LR35902::opcodeCB13;
  iset_.at(0x114) = &LR35902::opcodeCB14;
  iset_.at(0x115) = &LR35902::opcodeCB15;
  iset_.at(0x116) = &LR35902::opcodeCB16;
  iset_.at(0x117) = &LR35902::opcodeCB17;
  iset_.at(0x118) = &LR35902::opcodeCB18;
  iset_.at(0x119) = &LR35902::opcodeCB19;
  iset_.at(0x11a) = &LR35902::opcodeCB1A;
  iset_.at(0x11b) = &LR35902::opcodeCB1B;
  iset_.at(0x11c) = &LR35902::opcodeCB1C;
  iset_.at(0x11d) = &LR35902::opcodeCB1D;
  iset_.at(0x11e) = &LR35902::opcodeCB1E;
  iset_.at(0x11f) = &LR35902::opcodeCB1F;
  iset_.at(0x120) = &LR35902::opcodeCB20;
  iset_.at(0x121) = &LR35902::opcodeCB21;
  iset_.at(0x122) = &LR35902::opcodeCB22;
  iset_.at(0x123) = &LR35902::opcodeCB23;
  iset_.at(0x124) = &LR35902::opcodeCB24;
  iset_.at(0x125) = &LR35902::opcodeCB25;
  iset_.at(0x126) = &LR35902::opcodeCB26;
  iset_.at(0x127) = &LR35902::opcodeCB27;
  iset_.at(0x128) = &LR35902::opcodeCB28;
  iset_.at(0x129) = &LR35902::opcodeCB29;
  iset_.at(0x12a) = &LR35902::opcodeCB2A;
  iset_.at(0x12b) = &LR35902::opcodeCB2B;
  iset_.at(0x12c) = &LR35902::opcodeCB2C;
  iset_.at(0x12d) = &LR35902::opcodeCB2D;
  iset_.at(0x12e) = &LR35902::opcodeCB2E;
  iset_.at(0x12f) = &LR35902::opcodeCB2F;
  iset_.at(0x130) = &LR35902::opcodeCB30;
  iset_.at(0x131) = &LR35902::opcodeCB31;
  iset_.at(0x132) = &LR35902::opcodeCB32;
  iset_.at(0x133) = &LR35902::opcodeCB33;
  iset_.at(0x134) = &LR35902::opcodeCB34;
  iset_.at(0x135) = &LR35902::opcodeCB35;
  iset_.at(0x136) = &LR35902::opcodeCB36;
  iset_.at(0x137) = &LR35902::opcodeCB37;
  iset_.at(0x138) = &LR35902::opcodeCB38;
  iset_.at(0x139) = &LR35902::opcodeCB39;
  iset_.at(0x13a) = &LR35902::opcodeCB3A;
  iset_.at(0x13b) = &LR35902::opcodeCB3B;
  iset_.at(0x13c) = &LR35902::opcodeCB3C;
  iset_.at(0x13d) = &LR35902::opcodeCB3D;
  iset_.at(0x13e) = &LR35902::opcodeCB3E;
  iset_.at(0x13f) = &LR35902::opcodeCB3F;
  iset_.at(0x140) = &LR35902::opcodeCB40;
  iset_.at(0x141) = &LR35902::opcodeCB41;
  iset_.at(0x142) = &LR35902::opcodeCB42;
  iset_.at(0x143) = &LR35902::opcodeCB43;
  iset_.at(0x144) = &LR35902::opcodeCB44;
  iset_.at(0x145) = &LR35902::opcodeCB45;
  iset_.at(0x146) = &LR35902::opcodeCB46;
  iset_.at(0x147) = &LR35902::opcodeCB47;
  iset_.at(0x148) = &LR35902::opcodeCB48;
  iset_.at(0x149) = &LR35902::opcodeCB49;
  iset_.at(0x14a) = &LR35902::opcodeCB4A;
  iset_.at(0x14b) = &LR35902::opcodeCB4B;
  iset_.at(0x14c) = &LR35902::opcodeCB4C;
  iset_.at(0x14d) = &LR35902::opcodeCB4D;
  iset_.at(0x14e) = &LR35902::opcodeCB4E;
  iset_.at(0x14f) = &LR35902::opcodeCB4F;
  iset_.at(0x150) = &LR35902::opcodeCB50;
  iset_.at(0x151) = &LR35902::opcodeCB51;
  iset_.at(0x152) = &LR35902::opcodeCB52;
  iset_.at(0x153) = &LR35902::opcodeCB53;
  iset_.at(0x154) = &LR35902::opcodeCB54;
  iset_.at(0x155) = &LR35902::opcodeCB55;
  iset_.at(0x156) = &LR35902::opcodeCB56;
  iset_.at(0x157) = &LR35902::opcodeCB57;
  iset_.at(0x158) = &LR35902::opcodeCB58;
  iset_.at(0x159) = &LR35902::opcodeCB59;
  iset_.at(0x15a) = &LR35902::opcodeCB5A;
  iset_.at(0x15b) = &LR35902::opcodeCB5B;
  iset_.at(0x15c) = &LR35902::opcodeCB5C;
  iset_.at(0x15d) = &LR35902::opcodeCB5D;
  iset_.at(0x15e) = &LR35902::opcodeCB5E;
  iset_.at(0x15f) = &LR35902::opcodeCB5F;
  iset_.at(0x160) = &LR35902::opcodeCB60;
  iset_.at(0x161) = &LR35902::opcodeCB61;
  iset_.at(0x162) = &LR35902::opcodeCB62;
  iset_.at(0x163) = &LR35902::opcodeCB63;
  iset_.at(0x164) = &LR35902::opcodeCB64;
  iset_.at(0x165) = &LR35902::opcodeCB65;
  iset_.at(0x166) = &LR35902::opcodeCB66;
  iset_.at(0x167) = &LR35902::opcodeCB67;
  iset_.at(0x168) = &LR35902::opcodeCB68;
  iset_.at(0x169) = &LR35902::opcodeCB69;
  iset_.at(0x16a) = &LR35902::opcodeCB6A;
  iset_.at(0x16b) = &LR35902::opcodeCB6B;
  iset_.at(0x16c) = &LR35902::opcodeCB6C;
  iset_.at(0x16d) = &LR35902::opcodeCB6D;
  iset_.at(0x16e) = &LR35902::opcodeCB6E;
  iset_.at(0x16f) = &LR35902::opcodeCB6F;
  iset_.at(0x170) = &LR35902::opcodeCB70;
  iset_.at(0x171) = &LR35902::opcodeCB71;
  iset_.at(0x172) = &LR35902::opcodeCB72;
  iset_.at(0x173) = &LR35902::opcodeCB73;
  iset_.at(0x174) = &LR35902::opcodeCB74;
  iset_.at(0x175) = &LR35902::opcodeCB75;
  iset_.at(0x176) = &LR35902::opcodeCB76;
  iset_.at(0x177) = &LR35902::opcodeCB77;
  iset_.at(0x178) = &LR35902::opcodeCB78;
  iset_.at(0x179) = &LR35902::opcodeCB79;
  iset_.at(0x17a) = &LR35902::opcodeCB7A;
  iset_.at(0x17b) = &LR35902::opcodeCB7B;
  iset_.at(0x17c) = &LR35902::opcodeCB7C;
  iset_.at(0x17d) = &LR35902::opcodeCB7D;
  iset_.at(0x17e) = &LR35902::opcodeCB7E;
  iset_.at(0x17f) = &LR35902::opcodeCB7F;
  iset_.at(0x180) = &LR35902::opcodeCB80;
  iset_.at(0x181) = &LR35902::opcodeCB81;
  iset_.at(0x182) = &LR35902::opcodeCB82;
  iset_.at(0x183) = &LR35902::opcodeCB83;
  iset_.at(0x184) = &LR35902::opcodeCB84;
  iset_.at(0x185) = &LR35902::opcodeCB85;
  iset_.at(0x186) = &LR35902::opcodeCB86;
  iset_.at(0x187) = &LR35902::opcodeCB87;
  iset_.at(0x188) = &LR35902::opcodeCB88;
  iset_.at(0x189) = &LR35902::opcodeCB89;
  iset_.at(0x18a) = &LR35902::opcodeCB8A;
  iset_.at(0x18b) = &LR35902::opcodeCB8B;
  iset_.at(0x18c) = &LR35902::opcodeCB8C;
  iset_.at(0x18d) = &LR35902::opcodeCB8D;
  iset_.at(0x18e) = &LR35902::opcodeCB8E;
  iset_.at(0x18f) = &LR35902::opcodeCB8F;
  iset_.at(0x190) = &LR35902::opcodeCB90;
  iset_.at(0x191) = &LR35902::opcodeCB91;
  iset_.at(0x192) = &LR35902::opcodeCB92;
  iset_.at(0x193) = &LR35902::opcodeCB93;
  iset_.at(0x194) = &LR35902::opcodeCB94;
  iset_.at(0x195) = &LR35902::opcodeCB95;
  iset_.at(0x196) = &LR35902::opcodeCB96;
  iset_.at(0x197) = &LR35902::opcodeCB97;
  iset_.at(0x198) = &LR35902::opcodeCB98;
  iset_.at(0x199) = &LR35902::opcodeCB99;
  iset_.at(0x19a) = &LR35902::opcodeCB9A;
  iset_.at(0x19b) = &LR35902::opcodeCB9B;
  iset_.at(0x19c) = &LR35902::opcodeCB9C;
  iset_.at(0x19d) = &LR35902::opcodeCB9D;
  iset_.at(0x19e) = &LR35902::opcodeCB9E;
  iset_.at(0x19f) = &LR35902::opcodeCB9F;
  iset_.at(0x1a0) = &LR35902::opcodeCBA0;
  iset_.at(0x1a1) = &LR35902::opcodeCBA1;
  iset_.at(0x1a2) = &LR35902::opcodeCBA2;
  iset_.at(0x1a3) = &LR35902::opcodeCBA3;
  iset_.at(0x1a4) = &LR35902::opcodeCBA4;
  iset_.at(0x1a5) = &LR35902::opcodeCBA5;
  iset_.at(0x1a6) = &LR35902::opcodeCBA6;
  iset_.at(0x1a7) = &LR35902::opcodeCBA7;
  iset_.at(0x1a8) = &LR35902::opcodeCBA8;
  iset_.at(0x1a9) = &LR35902::opcodeCBA9;
  iset_.at(0x1aa) = &LR35902::opcodeCBAA;
  iset_.at(0x1ab) = &LR35902::opcodeCBAB;
  iset_.at(0x1ac) = &LR35902::opcodeCBAC;
  iset_.at(0x1ad) = &LR35902::opcodeCBAD;
  iset_.at(0x1ae) = &LR35902::opcodeCBAE;
  iset_.at(0x1af) = &LR35902::opcodeCBAF;
  iset_.at(0x1b0) = &LR35902::opcodeCBB0;
  iset_.at(0x1b1) = &LR35902::opcodeCBB1;
  iset_.at(0x1b2) = &LR35902::opcodeCBB2;
  iset_.at(0x1b3) = &LR35902::opcodeCBB3;
  iset_.at(0x1b4) = &LR35902::opcodeCBB4;
  iset_.at(0x1b5) = &LR35902::opcodeCBB5;
  iset_.at(0x1b6) = &LR35902::opcodeCBB6;
  iset_.at(0x1b7) = &LR35902::opcodeCBB7;
  iset_.at(0x1b8) = &LR35902::opcodeCBB8;
  iset_.at(0x1b9) = &LR35902::opcodeCBB9;
  iset_.at(0x1ba) = &LR35902::opcodeCBBA;
  iset_.at(0x1bb) = &LR35902::opcodeCBBB;
  iset_.at(0x1bc) = &LR35902::opcodeCBBC;
  iset_.at(0x1bd) = &LR35902::opcodeCBBD;
  iset_.at(0x1be) = &LR35902::opcodeCBBE;
  iset_.at(0x1bf) = &LR35902::opcodeCBBF;
  iset_.at(0x1c0) = &LR35902::opcodeCBC0;
  iset_.at(0x1c1) = &LR35902::opcodeCBC1;
  iset_.at(0x1c2) = &LR35902::opcodeCBC2;
  iset_.at(0x1c3) = &LR35902::opcodeCBC3;
  iset_.at(0x1c4) = &LR35902::opcodeCBC4;
  iset_.at(0x1c5) = &LR35902::opcodeCBC5;
  iset_.at(0x1c6) = &LR35902::opcodeCBC6;
  iset_.at(0x1c7) = &LR35902::opcodeCBC7;
  iset_.at(0x1c8) = &LR35902::opcodeCBC8;
  iset_.at(0x1c9) = &LR35902::opcodeCBC9;
  iset_.at(0x1ca) = &LR35902::opcodeCBCA;
  iset_.at(0x1cb) = &LR35902::opcodeCBCB;
  iset_.at(0x1cc) = &LR35902::opcodeCBCC;
  iset_.at(0x1cd) = &LR35902::opcodeCBCD;
  iset_.at(0x1ce) = &LR35902::opcodeCBCE;
  iset_.at(0x1cf) = &LR35902::opcodeCBCF;
  iset_.at(0x1d0) = &LR35902::opcodeCBD0;
  iset_.at(0x1d1) = &LR35902::opcodeCBD1;
  iset_.at(0x1d2) = &LR35902::opcodeCBD2;
  iset_.at(0x1d3) = &LR35902::opcodeCBD3;
  iset_.at(0x1d4) = &LR35902::opcodeCBD4;
  iset_.at(0x1d5) = &LR35902::opcodeCBD5;
  iset_.at(0x1d6) = &LR35902::opcodeCBD6;
  iset_.at(0x1d7) = &LR35902::opcodeCBD7;
  iset_.at(0x1d8) = &LR35902::opcodeCBD8;
  iset_.at(0x1d9) = &LR35902::opcodeCBD9;
  iset_.at(0x1da) = &LR35902::opcodeCBDA;
  iset_.at(0x1db) = &LR35902::opcodeCBDB;
  iset_.at(0x1dc) = &LR35902::opcodeCBDC;
  iset_.at(0x1dd) = &LR35902::opcodeCBDD;
  iset_.at(0x1de) = &LR35902::opcodeCBDE;
  iset_.at(0x1df) = &LR35902::opcodeCBDF;
  iset_.at(0x1e0) = &LR35902::opcodeCBE0;
  iset_.at(0x1e1) = &LR35902::opcodeCBE1;
  iset_.at(0x1e2) = &LR35902::opcodeCBE2;
  iset_.at(0x1e3) = &LR35902::opcodeCBE3;
  iset_.at(0x1e4) = &LR35902::opcodeCBE4;
  iset_.at(0x1e5) = &LR35902::opcodeCBE5;
  iset_.at(0x1e6) = &LR35902::opcodeCBE6;
  iset_.at(0x1e7) = &LR35902::opcodeCBE7;
  iset_.at(0x1e8) = &LR35902::opcodeCBE8;
  iset_.at(0x1e9) = &LR35902::opcodeCBE9;
  iset_.at(0x1ea) = &LR35902::opcodeCBEA;
  iset_.at(0x1eb) = &LR35902::opcodeCBEB;
  iset_.at(0x1ec) = &LR35902::opcodeCBEC;
  iset_.at(0x1ed) = &LR35902::opcodeCBED;
  iset_.at(0x1ee) = &LR35902::opcodeCBEE;
  iset_.at(0x1ef) = &LR35902::opcodeCBEF;
  iset_.at(0x1f0) = &LR35902::opcodeCBF0;
  iset_.at(0x1f1) = &LR35902::opcodeCBF1;
  iset_.at(0x1f2) = &LR35902::opcodeCBF2;
  iset_.at(0x1f3) = &LR35902::opcodeCBF3;
  iset_.at(0x1f4) = &LR35902::opcodeCBF4;
  iset_.at(0x1f5) = &LR35902::opcodeCBF5;
  iset_.at(0x1f6) = &LR35902::opcodeCBF6;
  iset_.at(0x1f7) = &LR35902::opcodeCBF7;
  iset_.at(0x1f8) = &LR35902::opcodeCBF8;
  iset_.at(0x1f9) = &LR35902::opcodeCBF9;
  iset_.at(0x1fa) = &LR35902::opcodeCBFA;
  iset_.at(0x1fb) = &LR35902::opcodeCBFB;
  iset_.at(0x1fc) = &LR35902::opcodeCBFC;
  iset_.at(0x1fd) = &LR35902::opcodeCBFD;
  iset_.at(0x1fe) = &LR35902::opcodeCBFE;
  iset_.at(0x1ff) = &LR35902::opcodeCBFF;
}