/*
 * gpu.cpp
 * Copyright (C) 2020 Emiliano Firmino <emiliano.firmino@gmail.com>
 *
 * Distributed under terms of the MIT license.
 */

#include "gpu.hpp"

#include "address.hpp"
#include "interrupt.hpp"
#include "mmu.hpp"

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

bool Gpu::isBackgroundEnable() {
    return mmu_.read(Address::HwIoLcdControl) & ControlFlags::kBackgroundDisplayEnable;
}

addr_t Gpu::getTileDataIndex(u8 line, u8 index) {
    u8 dataIndex = (line % kTileHeight) * 2;

    if (index < 128 || getTileDataAddr() == 0x8000) {
        dataIndex += index * kTileSize;
    } else {
        dataIndex -= (static_cast<s8>(index) * kTileSize) * -1;
    }

    return dataIndex;
}

addr_t Gpu::getTileDataAddr() {
    const u8 control = mmu_.read(Address::HwIoLcdControl);
    if (control & ControlFlags::kBackgroundWindowTileDataSelect) {
        return 0x8000;
    }
    return 0x9000;
}

addr_t Gpu::getTileMapAddr() {
    const u8 control = mmu_.read(Address::HwIoLcdControl);
    if (control & ControlFlags::kBackgroundTileMapDisplaySelect) {
        return 0x9C00;
    }
    return 0x9800;
}

addr_t Gpu::getScrollX() {
    return mmu_.read(Address::HwIoScrollX);
}

addr_t Gpu::getScrollY() {
    return mmu_.read(Address::HwIoScrollY);
}

addr_t Gpu::getWindowTileIndex(u8 windowX, u8 windowY) {
    addr_t windowTileIndex = 0;
    windowTileIndex += static_cast<addr_t>(windowX / kTileWidth);
    windowTileIndex += static_cast<addr_t>(windowY / kTileHeight) * kTilesPerRow;
    return windowTileIndex;
}

void Gpu::renderScanlineBackground(u8 scanline) {
    if (!isBackgroundEnable()) {
        return;
    }

    const u8 bgPalette = mmu_.read(Address::HwIoBackgroundPalette);

    const addr_t dataAddr = getTileDataAddr();
    const addr_t mapAddr = getTileMapAddr();

    const u8 scrollX = getScrollX();
    const u8 scrollY = getScrollY();

    const u8 screenY = scanline;
    const u8 windowY = screenY + scrollY;

    for (u8 screenX = 0; screenX < kDisplayWidth; screenX++) {
        u8 windowX = screenX + scrollX;
        u8 bgTileIndex = mmu_.read(mapAddr + getWindowTileIndex(windowX, windowY));

        addr_t bgTileDataAddr = dataAddr + getTileDataIndex(windowY, bgTileIndex);

        uint8_t lsb = mmu_.read(bgTileDataAddr + 0);
        uint8_t msb = mmu_.read(bgTileDataAddr + 1);

        size_t bitIndex = 7 - (screenX % 8);

        size_t palleteIndex = 0;
        palleteIndex += ((lsb >> bitIndex) & 0x01) ? 2 : 0;
        palleteIndex += ((msb >> bitIndex) & 0x01) ? 1 : 0;

        palleteIndex = (bgPalette >> (palleteIndex * 2)) & 0x3;

        size_t pos = (screenX + (scanline * kDisplayWidth)) * kColorComponentSize;

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
