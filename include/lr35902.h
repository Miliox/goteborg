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

    /**
     * Run fetch-decode-execute cycle
     */
    u8 cycle();

private:
    Registers r;

    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wunused-private-field"
    MMU& mmu;
    #pragma GCC diagnostic pop

    std::vector<u8 (LR35902::*)()> iset;
    std::vector<u8 (LR35902::*)()> cb_iset;

    u8 not_implemented_error() {
        auto opcode = mmu.read(r.pc);

        std::stringstream ss;
        ss << "Instruction not supported: ";
        ss << std::uppercase << std::hex << std::setfill('0') << std::setw(2);
        ss << static_cast<int>(opcode);

        throw std::runtime_error(ss.str());
        return 0;
    }
};

} // namespace goteborg

#endif /* !LR35902_H */
