/*
 * gpu.cpp
 * Copyright (C) 2020 Emiliano Firmino <emiliano.firmino@gmail.com>
 *
 * Distributed under terms of the MIT license.
 */

#include "gpu.h"

#include "address.h"
#include "interrupt.h"
#include "mmu.h"

#include <algorithm>
#include <sstream>

using namespace gbg;

static const u8 kTileSize = 16;
static const u8 kTileWidth = 8;
static const u8 kTileHeight = 8;
static const u8 kTilesPerRow = 32;
static const u8 kTilesPerColumn = 32;

static const size_t kDisplayWidth = 160;
static const size_t kDisplayHeight = 144;
static const size_t kDisplaySize = kDisplayWidth * kDisplayHeight;

static const u8 kVerticalBlankScanline = 143;
static const u8 kReadObjectAttributeMemoryScanline = 153;

namespace Mode {
    static const u8 kHorizontalBlank = 0;
    static const u8 kVerticalBlank = 1;
    static const u8 kReadOAM = 2;
    static const u8 kWriteToVRAM = 3;
}

namespace Duration {
    static const ticks_t kHorizontalBlank = 204;
    static const ticks_t kVerticalBlank = 456;
    static const ticks_t kReadOAM = 80;
    static const ticks_t kWriteToVRAM = 172;
}

namespace ControlFlags {
    static const u8 kDisplayEnable                  = 1 << 7;
    static const u8 kWindowTileMapDisplaySelect     = 1 << 6;
    static const u8 kWindowDisplayEnable            = 1 << 5;
    static const u8 kBackgroundWindowTileDataSelect = 1 << 4;
    static const u8 kBackgroundTileMapDisplaySelect = 1 << 3;
    static const u8 kSpriteSizeSelect               = 1 << 2;
    static const u8 kSpriteDisplayEnable            = 1 << 1;
    static const u8 kBackgroundDisplayEnable        = 1 << 0;
};

namespace StatusFlags {
    static const u8 kInterruptOnScanlineCoincidence = 1 << 6;
    static const u8 kInterruptOnReadOAM             = 1 << 5;
    static const u8 kInterruptOnVerticalBlanking    = 1 << 4;
    static const u8 kScanlineCoincidenceFlag        = 1 << 3;
    static const u8 kInterruptOnHorizontalBlanking  = 1 << 2;

    static const u8 kModeMask = 0x3;
}

Gpu::Gpu(MMU& mmu) :
    mmu_(mmu),
    mode_(Mode::kVerticalBlank),
    scanline_(0),
    counter_(0),
    palette_(),
    pixels_(kDisplaySize * kColorComponentSize, 0),
    texture_(),
    viewport_() {

    // #9BBC0FFF (RGBA)
    palette_[0][0] = 0x9B;
    palette_[0][1] = 0xBC;
    palette_[0][2] = 0x0F;
    palette_[0][3] = 0xFF; 

    // 8BAC0FFF (RGBA)
    palette_[1][0] = 0x8B;
    palette_[1][1] = 0xAC;
    palette_[1][2] = 0x0F;
    palette_[1][3] = 0xFF;

    // #306230FF (RGBA)
    palette_[2][0] = 0x30;
    palette_[2][1] = 0x62;
    palette_[2][2] = 0x30;
    palette_[2][3] = 0xFF;

    // #0F380FFF (RGBA)
    palette_[3][0] = 0x0F;
    palette_[3][1] = 0x38;
    palette_[3][2] = 0x0F;
    palette_[3][3] = 0xFF;

    for (u8 line = 0; line < kDisplayHeight; line++) {
        clearScanline(line);
    }

    texture_.reset(new sf::Texture());
    texture_->create(kDisplayWidth, kDisplayHeight);
    texture_->update(pixels_.data());

    viewport_.reset(new sf::Sprite(*texture_));

    mmu_.write(Address::HwIoScrollX, 0);
    mmu_.write(Address::HwIoScrollY, 0);
    mmu_.write(Address::HwIoCurrentScanline, 0);
    mmu_.write(Address::HwIoComparisonScanline, 0);
    mmu_.write(Address::HwIoLcdStatus, mode_ | StatusFlags::kScanlineCoincidenceFlag);
    mmu_.write(Address::HwIoLcdControl, 0);
}

void Gpu::step(ticks_t t) {
    counter_ += t;

    switch (getMode()) {
    case Mode::kHorizontalBlank:
        if (counter_ >= Duration::kHorizontalBlank) {
            counter_ -= Duration::kHorizontalBlank;

            auto scanline = getScanline();
            scanline += 1;
            setScanline(scanline);

            if (scanline >= kVerticalBlankScanline) {
                setMode(Mode::kVerticalBlank);
                texture_->update(pixels_.data());
            } else {
                setMode(Mode::kReadOAM);
            }

        }
        break;
    case Mode::kVerticalBlank:
        if (counter_ >= Duration::kVerticalBlank) {
            counter_ -= Duration::kVerticalBlank;

            auto scanline = getScanline();
            scanline += 1;
            setScanline(scanline);

            if (scanline > kReadObjectAttributeMemoryScanline) {
                setMode(Mode::kReadOAM);
                setScanline(0);
            }

        }
        break;
    case Mode::kReadOAM:
        if (counter_ >= Duration::kReadOAM) {
            counter_ -= Duration::kReadOAM;
            setMode(Mode::kWriteToVRAM);
        }
        break;
    case Mode::kWriteToVRAM:
        if (counter_ >= Duration::kWriteToVRAM) {
            counter_ -= Duration::kWriteToVRAM;
            setMode(Mode::kHorizontalBlank);

            auto scanline = getScanline();
            clearScanline(scanline);
            renderScanlineBackground(scanline);
            renderScanlineSprites(scanline);
        }
        break;
    }
}

void Gpu::render(sf::RenderTarget& renderer) {
    renderer.draw(*viewport_);
}

u8 Gpu::getMode() {
    return (mmu_.read(Address::HwIoLcdStatus) & 0x3);
}

void Gpu::setMode(u8 mode) {
    u8 status = mmu_.read(Address::HwIoLcdStatus);
    if (mode == Mode::kVerticalBlank) {
        auto flags = mmu_.read(Address::HwIoInterruptFlags);
        flags |= kLcdVerticalBlankingInterrupt;
        if (status & StatusFlags::kInterruptOnVerticalBlanking) {
            flags |= kLcdControllerInterrupt;
        }
        mmu_.write(Address::HwIoInterruptFlags, flags);
    }
    else if (mode == Mode::kHorizontalBlank && (status & StatusFlags::kInterruptOnHorizontalBlanking)) {
        auto flags = mmu_.read(Address::HwIoInterruptFlags);
        flags |= kLcdControllerInterrupt;
        mmu_.write(Address::HwIoInterruptFlags, flags);
    }
    else if (mode == Mode::kReadOAM && (status & StatusFlags::kInterruptOnReadOAM)) {
        auto flags = mmu_.read(Address::HwIoInterruptFlags);
        flags |= kLcdControllerInterrupt;
        mmu_.write(Address::HwIoInterruptFlags, flags);
    }

    status = mode | (status & ~StatusFlags::kModeMask);
    mmu_.write(Address::HwIoLcdStatus, status);
}

u8 Gpu::getScanline() {
    auto scanline = mmu_.read(Address::HwIoCurrentScanline);
    if (scanline_ != scanline) {
        scanline = 0;
        scanline_ = 0;
    }
    return scanline;
}

void Gpu::setScanline(u8 scanline) {
    scanline_ = scanline;

    auto status = mmu_.read(Address::HwIoLcdStatus);
    auto comparisonScanline = mmu_.read(Address::HwIoComparisonScanline);

    if (scanline == comparisonScanline) {
        status |= StatusFlags::kScanlineCoincidenceFlag;
    } else {
        status &= ~StatusFlags::kScanlineCoincidenceFlag;
    }

    if (status & StatusFlags::kInterruptOnScanlineCoincidence) {
        mmu_.write(Address::HwIoInterruptFlags,
                   mmu_.read(Address::HwIoInterruptFlags) | kLcdControllerInterrupt);
    }

    mmu_.write(Address::HwIoCurrentScanline, scanline);
    mmu_.write(Address::HwIoLcdStatus, status);
}

void Gpu::clearScanline(u8 scanline) {
    size_t begin = scanline * kDisplayWidth * kColorComponentSize;
    size_t end   = begin + kDisplayWidth * kColorComponentSize;
    for (size_t i = begin; i < end; i+= kColorComponentSize) {
        for (size_t j = 0; j < kColorComponentSize; j++) {
            pixels_[i + j] = palette_[0][j];
        }
    }
}

void Gpu::renderScanlineBackground(u8 scanline) {
    u8 control = mmu_.read(Address::HwIoLcdControl);

    if ((control & ControlFlags::kBackgroundDisplayEnable) == 0) {
        return;
    }

    u8 scrollY = mmu_.read(Address::HwIoScrollY);
    u8 bgPalette = mmu_.read(Address::HwIoBackgroundPalette);

    scanline += scrollY;

    if (scanline >= kDisplayHeight) {
        // off screen
        return;
    }

    addr_t bgAddr = (control & ControlFlags::kBackgroundTileMapDisplaySelect) ? 0x9c00 : 0x9800;

    for (int column = 0; column < kDisplayWidth; column++) {
        addr_t bgIndex = (scanline / kTileHeight) * kTilesPerRow;
        bgIndex += (column / kTileWidth);

        addr_t tileNumber = mmu_.read(bgAddr + bgIndex);

        addr_t tileAddr = (control & ControlFlags::kBackgroundWindowTileDataSelect) ? 0x8000 : 0x9000;

        if (tileAddr == 0x9000 && tileNumber >= 128) {
            tileNumber = (0xff - tileNumber) + 1;
            tileAddr -= (tileNumber * 16);
        } else {
            tileAddr += (tileNumber * 16);
        }
        tileAddr += (scanline % kTileHeight) * 2;

        uint8_t lsb = mmu_.read(tileAddr + 0);
        uint8_t msb = mmu_.read(tileAddr + 1);

        int bitIndex = 7 - (column % 8);

        int palleteIndex = 0;
        palleteIndex += ((lsb >> bitIndex) & 0x01) ? 2 : 0;
        palleteIndex += ((msb >> bitIndex) & 0x01) ? 1 : 0;

        palleteIndex = (bgPalette >> (palleteIndex * 2)) & 0x3;

        size_t pos = (column + (scanline * kDisplayWidth)) * kColorComponentSize;

        for (size_t i = 0; i < kColorComponentSize; i++) {
            pixels_.at(pos + i) = palette_[palleteIndex][i];
        }
    }

}

void Gpu::renderScanlineSprites(u8 scanline) {
    UNUSED(kTileSize);
    UNUSED(kTilesPerColumn);
    UNUSED(ControlFlags::kDisplayEnable);
    UNUSED(ControlFlags::kWindowDisplayEnable);
    UNUSED(ControlFlags::kWindowTileMapDisplaySelect);
    UNUSED(ControlFlags::kSpriteDisplayEnable);
    UNUSED(ControlFlags::kSpriteDisplayEnable);
    UNUSED(ControlFlags::kSpriteSizeSelect);
}
