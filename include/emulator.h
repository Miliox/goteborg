/*
 * emulator.h
 * Copyright (C) 2020 Emiliano Firmino <emiliano.firmino@gmail.com>
 *
 * Distributed under terms of the MIT license.
 */

#ifndef EMULATOR_H
#define EMULATOR_H

#include "common.h"
#include "lr35902.h"
#include "mmuimpl.h"

namespace goteborg {

class Emulator {
public:
    Emulator();

    void reset();
    ticks_t step();

    LR35902& getCPU();
    MMU&     getMMU();

private:
    MMUImpl mmu;
    LR35902 cpu;
};

}

#endif /* !EMULATOR_H */
