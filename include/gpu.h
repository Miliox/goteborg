/*
 * gpu.h
 * Copyright (C) 2020 Emiliano Firmino <emiliano.firmino@gmail.com>
 *
 * Distributed under terms of the MIT license.
 */

#ifndef GPU_H
#define GPU_H

#include "common.h"

#include <memory>

#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Texture.hpp>

namespace gbg {

class MMU;

class Gpu {
public:
    Gpu(MMU& mmu);

    void render(sf::RenderTarget& renderer);
    void step(ticks_t elapsedTicks);

private:
    static const size_t kPaletteSize = 4;

    static const size_t kColorComponentRed   = 0;
    static const size_t kColorComponentGreen = 1;
    static const size_t kColorComponentBlue  = 2;
    static const size_t kColorComponentAlpha = 3;
    static const size_t kColorComponentSize  = 4;

    MMU& mmu_;

    u8 mode_;
    u8 scanline_;

    ticks_t counter_;

    u8       palette_[kPaletteSize][kColorComponentSize];
    buffer_t pixels_;

    std::unique_ptr<sf::Texture> texture_;
    std::unique_ptr<sf::Sprite>  viewport_;

    u8 getMode();
    void setMode(u8 mode);

    u8 getScanline();
    void setScanline(u8 scanline);

    void clearScanline(u8 scanline);
    void renderScanlineBackground(u8 scanline);
    void renderScanlineSprites(u8 scanline);
};

}

#endif /* !GPU_H */
