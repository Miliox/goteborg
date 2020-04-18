/*
 * lr35902.h
 * Copyright (C) 2020 Emiliano Firmino <emiliano.firmino@gmail.com>
 *
 * Distributed under terms of the MIT license.
 */

#ifndef LR35902_H
#define LR35902_H

#include "common.h"
#include "registers.h"

#include <iomanip>
#include <sstream>

namespace goteborg {

class MMU;
class Registers;

const u64 kClockRate = 4'194'304;       // Classic Gameboy
const u64 kSuperClockRate = 4'295'454;  // Super Gameboy
const u64 kColorClockRate = 8'388'608;  // Gameboy Color

/**
 * SHARP LR35902 (Gameboy CPU)
 *
 * A simpler Zilog Z80.
 *
 * It contains the most of Z80 extended instruncions,
 * but contains only the Intel 8080 registers.
 *
 */
class LR35902 {
public:
    LR35902(MMU& mmu);

    /**
     * Registers
     */
    Registers regs;

    /**
     * Memory Manager Unit
     */
    MMU& mmu;

    /**
     * Run fetch-decode-execute cycle
     */
    ticks_t cycle();

private:
    std::vector<std::function<ticks_t()>> iset_;

    void call(addr_t a);
    void ret();
    void rst(addr_t a);
    void push(u16& r);
    void pop(u16& reg);

    u8  next8();
    u16 next16();

    u8  peek8();
    u16 peek16();

    u8  read8(addr_t a);
    u16 read16(addr_t a);

    void write8(addr_t a, u8 v);
    void write16(addr_t a, u16 v);

    u8  zread8(u8 a);
    u16 zread16(u8 a);

    void zwrite8(u8 a, u8 v);
    void zwrite16(u8 a, u16 v);

    void populateInstructionSets();
};

} // namespace goteborg

#endif /* !LR35902_H */
