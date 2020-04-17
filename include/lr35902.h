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
    ticks cycle();

private:
    std::vector<std::function<ticks()>> iset_;
    std::vector<std::function<ticks()>> xset_;

    u8  next8();
    u16 next16();

    u8  peek8();
    u16 peek16();

    void populateInstructionSets();
};

} // namespace goteborg

#endif /* !LR35902_H */
