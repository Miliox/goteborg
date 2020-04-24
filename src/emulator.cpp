/*
 * emulator.cpp
 * Copyright (C) 2020 Emiliano Firmino <emiliano.firmino@gmail.com>
 *
 * Distributed under terms of the MIT license.
 */

#include "emulator.h"

#include <exception>

#include <SFML/System/FileInputStream.hpp>

using namespace gbg;

Emulator::Emulator(u8 fps) : mmu_(), gpu_(mmu_), cpu_(mmu_), counter_(0), frameDuration_(kClockRate / fps) {

}

void Emulator::reset() {
    sf::FileInputStream bios;
    if (!bios.open("bios.bin")) {
        throw std::runtime_error("error: cannot load bios");
    }

    buffer_t data(bios.getSize(), 0xff);
    bios.read(reinterpret_cast<void*>(&data.front()), data.size());

    mmu_.write(0, data);
}

void Emulator::render(sf::RenderTarget& renderer) {
    gpu_.render(renderer);
}

void Emulator::nextFrame() {
    while (counter_ < frameDuration_) {
        auto t = cpu_.cycle();
        assert(t != 0 && "never halt");
        gpu_.step(t);
        counter_ += t;
    }
    counter_ -= frameDuration_;
}
