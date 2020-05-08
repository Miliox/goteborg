/*
 * emulator.cpp
 * Copyright (C) 2020 Emiliano Firmino <emiliano.firmino@gmail.com>
 *
 * Distributed under terms of the MIT license.
 */

#include "emulator.hpp"

#include <exception>

#include <SFML/System/FileInputStream.hpp>

using namespace gbg;

Emulator::Emulator(u8 fps)
    : mmu_(), gpu_(mmu_), cpu_(mmu_), counter_(0),
      frameDuration_(kClockRate / fps) {}

void Emulator::reset() {
  {
    sf::FileInputStream file;

    if (!file.open("bios.bin")) {
      throw std::runtime_error("error: cannot load bios");
    }

    buffer_t bios(file.getSize(), 0xff);
    file.read(bios.data(), bios.size());

    mmu_.loadBios(bios);
  }

  {
    sf::FileInputStream file;

    if (!file.open("cartridge.gb")) {
      throw std::runtime_error("error: cannot load cartridge");
    }

    buffer_t cartridge(file.getSize(), 0xff);
    file.read(cartridge.data(), cartridge.size());

    mmu_.loadCartridge(cartridge);
  }

  addr_t blogo = 0x00a8;
  addr_t clogo = 0x0104;
  for (addr_t i = 0; i < 48; i++) {
    auto bdata = mmu_.read(blogo + i);
    auto cdata = mmu_.read(clogo + i);
    if (bdata != cdata) {
      throw std::runtime_error("error: logo mismatch");
    }
  }
}

MMU &Emulator::getMMU() { return mmu_; }

Registers &Emulator::getRegisters() { return cpu_.regs; }

void Emulator::render(sf::RenderTarget &renderer) { gpu_.render(renderer); }

void Emulator::nextFrame() {
  while (counter_ < frameDuration_) {
    auto t = cpu_.cycle();

    if (t == 0) {
      // halt
      return;
    }

    mmu_.step(t);
    gpu_.step(t);

    counter_ += t;
  }
  counter_ -= frameDuration_;
}

ticks_t Emulator::nextTicks() {
  auto t = cpu_.cycle();

  if (t == 0) {
    // halt
    return 0;
  }

  mmu_.step(t);
  gpu_.step(t);

  return t;
}
