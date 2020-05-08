/*
 * emulator.h
 * Copyright (C) 2020 Emiliano Firmino <emiliano.firmino@gmail.com>
 *
 * Distributed under terms of the MIT license.
 */

#ifndef EMULATOR_H
#define EMULATOR_H

#include "common.hpp"

#include "gpu.hpp"
#include "lr35902.hpp"
#include "registers.hpp"
#include "mmuimpl.hpp"

#include <SFML/Graphics/RenderTarget.hpp>

namespace gbg {

class Emulator {
public:
    Emulator(u8 fps);

    void nextFrame();
    ticks_t nextTicks();

    void reset();
    void render(sf::RenderTarget& renderer);

    MMU& getMMU();
    Registers& getRegisters();

private:
    MMUImpl mmu_;
    Gpu     gpu_;
    LR35902 cpu_;

    ticks_t counter_;
    const ticks_t frameDuration_;
};

}

#endif /* !EMULATOR_H */
