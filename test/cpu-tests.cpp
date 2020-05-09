/*
 * cpu-tests.cpp
 * Copyright (C) 2020 Emiliano Firmino <emiliano.firmino@gmail.com>
 *
 * Distributed under terms of the MIT license.
 */

#include "catch2/catch.hpp"
#include "cpu.hpp"
#include "mmuimpl.hpp"

using namespace gbg;

constexpr const char *kTag = "[CPU]";
constexpr const size_t kBiosSize = 256;

TEST_CASE("OPCODE 00: NOP", kTag) {
  MMUImpl mmu;
  Cpu cpu(mmu);

  buffer_t bios(kBiosSize, 0);
  mmu.loadBios(bios);

  Registers r = cpu.regs;
  REQUIRE(r.pc == 0x0000);
  auto ticks = cpu.cycle();
  REQUIRE(cpu.regs.pc == 0x0001);
  REQUIRE(cpu.regs.af == r.af);
  REQUIRE(cpu.regs.bc == r.bc);
  REQUIRE(cpu.regs.de == r.de);
  REQUIRE(cpu.regs.hl == r.hl);
  REQUIRE(ticks == 4);
}

TEST_CASE("OPCODE 01: LD BC,d16", kTag) {
  MMUImpl mmu;
  Cpu cpu(mmu);

  buffer_t bios(kBiosSize, 0);
  bios.at(0) = 0x01;
  bios.at(1) = 0xcd;
  bios.at(2) = 0xab;
  mmu.loadBios(bios);

  Registers r = cpu.regs;
  REQUIRE(r.pc == 0x0000);
  auto ticks = cpu.cycle();
  REQUIRE(cpu.regs.pc == 0x0003);
  REQUIRE(cpu.regs.af == r.af);
  REQUIRE(cpu.regs.bc == 0xabcd);
  REQUIRE(cpu.regs.de == r.de);
  REQUIRE(cpu.regs.hl == r.hl);
  REQUIRE(ticks == 12);
}

TEST_CASE("OPCODE 02: LD (BC),A", kTag) {
  MMUImpl mmu;
  Cpu cpu(mmu);

  buffer_t bios(kBiosSize, 0);
  bios.at(0) = 0x02;
  mmu.loadBios(bios);

  cpu.regs.a = 0x99;
  cpu.regs.bc = 0xc000;
  Registers r = cpu.regs;
  REQUIRE(r.pc == 0x0000);
  auto ticks = cpu.cycle();
  REQUIRE(cpu.regs.pc == 0x0001);
  REQUIRE(cpu.regs.af == r.af);
  REQUIRE(cpu.regs.bc == r.bc);
  REQUIRE(cpu.regs.de == r.de);
  REQUIRE(cpu.regs.hl == r.hl);
  REQUIRE(ticks == 8);
  REQUIRE(mmu.read(0xc000) == 0x99);
}