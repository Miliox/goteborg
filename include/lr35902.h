/*
 * lr35902.h
 * Copyright (C) 2020 Emiliano Firmino <emiliano.firmino@gmail.com>
 *
 * Distributed under terms of the MIT license.
 */

#ifndef LR35902_H
#define LR35902_H

#include "registers.h"
#include "mmu.h"

#include <iomanip>
#include <sstream>

namespace goteborg {

const u64 kClockRate = 4'194'304;
const u64 kSuperClockRate = 4'295'454;
const u64 kColorClockRate = 8'388'608;

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

    Registers getRegisters();

    /**
     * Run fetch-decode-execute cycle
     */
    ticks cycle();

private:
    Registers r;
    MMU& mmu;

    std::vector<std::function<ticks()>> iset;
    std::vector<std::function<ticks()>> cb_iset;

};

} // namespace goteborg

#endif /* !LR35902_H */
