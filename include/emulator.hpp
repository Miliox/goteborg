/*
 * emulator.h
 * Copyright (C) 2020 Emiliano Firmino <emiliano.firmino@gmail.com>
 *
 * Distributed under terms of the MIT license.
 */

#ifndef EMULATOR_H
#define EMULATOR_H

#include <SFML/Graphics/RenderTarget.hpp>

#include "common.hpp"
#include "cpu.hpp"
#include "gpu.hpp"
#include "mmuimpl.hpp"
#include "registers.hpp"

namespace gbg {

class Emulator {
public:
  Emulator(u8 fps);

  void nextFrame();
  ticks_t nextTicks();

  void reset();
  void render(sf::RenderTarget &renderer);

  MMU &getMMU();
  Registers &getRegisters();

private:
  MMUImpl mmu_;
  Gpu gpu_;
  Cpu cpu_;

  ticks_t counter_;
  const ticks_t frameDuration_;
};

} // namespace gbg

#endif /* !EMULATOR_H */
