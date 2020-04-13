/*
 * LR35902.cpp
 * Copyright (C) 2020 Emiliano Firmino <emiliano.firmino@gmail.com>
 *
 * Distributed under terms of the MIT license.
 */

#include "lr35902.h"

using namespace goteborg;

LR35902::LR35902(MMU& mmu) : r(), mmu(mmu),
    iset(256, &LR35902::not_implemented_error),
    cb_iset(256, &LR35902::not_implemented_error) {

}

u8 LR35902::cycle() {
    auto opcode = mmu.read(r.pc);       // fetch
    auto instruction = iset.at(opcode); // decode
    return (this->*instruction)();      // execute
}