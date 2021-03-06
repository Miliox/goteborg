/*
 * gpu.cpp
 * Copyright (C) 2020 Emiliano Firmino <emiliano.firmino@gmail.com>
 *
 * Distributed under terms of the MIT license.
 */

#include "gpu.hpp"

#include "address.hpp"
#include "interrupt.hpp"
#include "mmuimpl.hpp"
#include "sprite.hpp"

#include <algorithm>
#include <deque>
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
} // namespace Mode

namespace Duration {
static const ticks_t kHorizontalBlank = 204;
static const ticks_t kVerticalBlank = 456;
static const ticks_t kReadOAM = 80;
static const ticks_t kWriteToVRAM = 172;
} // namespace Duration

namespace ControlFlags {
static const u8 kDisplayEnable = 1 << 7;
static const u8 kWindowTileMapDisplaySelect = 1 << 6;
static const u8 kWindowDisplayEnable = 1 << 5;
static const u8 kBackgroundWindowTileDataSelect = 1 << 4;
static const u8 kBackgroundTileMapDisplaySelect = 1 << 3;
static const u8 kSpriteSizeSelect = 1 << 2;
static const u8 kSpriteDisplayEnable = 1 << 1;
static const u8 kBackgroundDisplayEnable = 1 << 0;
}; // namespace ControlFlags

namespace StatusFlags {
static const u8 kInterruptOnScanlineCoincidence = 1 << 6;
static const u8 kInterruptOnReadOAM = 1 << 5;
static const u8 kInterruptOnVerticalBlanking = 1 << 4;
static const u8 kScanlineCoincidenceFlag = 1 << 3;
static const u8 kInterruptOnHorizontalBlanking = 1 << 2;

static const u8 kModeMask = 0x3;
} // namespace StatusFlags

Gpu::Gpu(MMUImpl &mmu)
    : mmu_(mmu), mode_(Mode::kVerticalBlank), scanline_(0), counter_(0),
      palette_(), pixels_(kDisplaySize * kColorComponentSize, 0), texture_(),
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
  mmu_.write(Address::HwIoLcdStatus,
             mode_ | StatusFlags::kScanlineCoincidenceFlag);
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

        renderScanline();
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
      renderScanline();
    }
    break;
  }
}

void Gpu::render(sf::RenderTarget &renderer) { renderer.draw(*viewport_); }

u8 Gpu::getMode() { return (mmu_.read(Address::HwIoLcdStatus) & 0x3); }

void Gpu::setMode(u8 mode) {
  u8 status = mmu_.read(Address::HwIoLcdStatus);
  if (mode == Mode::kVerticalBlank) {
    auto flags = mmu_.read(Address::HwIoInterruptFlags);
    flags |= kLcdVerticalBlankingInterrupt;
    if (status & StatusFlags::kInterruptOnVerticalBlanking) {
      flags |= kLcdControllerInterrupt;
    }
    mmu_.write(Address::HwIoInterruptFlags, flags);
  } else if (mode == Mode::kHorizontalBlank &&
             (status & StatusFlags::kInterruptOnHorizontalBlanking)) {
    auto flags = mmu_.read(Address::HwIoInterruptFlags);
    flags |= kLcdControllerInterrupt;
    mmu_.write(Address::HwIoInterruptFlags, flags);
  } else if (mode == Mode::kReadOAM &&
             (status & StatusFlags::kInterruptOnReadOAM)) {
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
               mmu_.read(Address::HwIoInterruptFlags) |
                   kLcdControllerInterrupt);
  }

  mmu_.write(Address::HwIoCurrentScanline, scanline);
  mmu_.write(Address::HwIoLcdStatus, status);
}

void Gpu::clearScanline(u8 scanline) {
  size_t begin = scanline * kDisplayWidth * kColorComponentSize;
  size_t end = begin + kDisplayWidth * kColorComponentSize;
  for (size_t i = begin; i < end; i += kColorComponentSize) {
    for (size_t j = 0; j < kColorComponentSize; j++) {
      pixels_[i + j] = palette_[0][j];
    }
  }
}

bool Gpu::isBackgroundEnable() {
  return mmu_.read(Address::HwIoLcdControl) &
         ControlFlags::kBackgroundDisplayEnable;
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

addr_t Gpu::getScrollX() { return mmu_.read(Address::HwIoScrollX); }

addr_t Gpu::getScrollY() { return mmu_.read(Address::HwIoScrollY); }

addr_t Gpu::getWindowTileIndex(u8 windowX, u8 windowY) {
  addr_t windowTileIndex = 0;
  windowTileIndex += static_cast<addr_t>(windowX / kTileWidth);
  windowTileIndex += static_cast<addr_t>(windowY / kTileHeight) * kTilesPerRow;
  return windowTileIndex;
}

void Gpu::renderScanline() {
  auto scanline = getScanline();
  clearScanline(scanline);
  renderScanlineBackground(scanline);
  renderScanlineSprites(scanline);
}

void Gpu::renderScanlineBackground(u8 scanline) {
  if (!isBackgroundEnable()) {
    return;
  }

  const u8 scrollX = getScrollX();
  const u8 scrollY = getScrollY();

  const addr_t dataAddr = getTileDataAddr();
  const addr_t mapAddr = getTileMapAddr();

  const u8 windowY = scanline + scrollY;
  for (size_t column = 0; column < kDisplayWidth; column++) {
    const u8 windowX = column + scrollX;

    addr_t bgIndex =
        (windowY / kTileHeight) * kTilesPerRow + (windowX / kTileWidth);
    addr_t tileIndex = mmu_.read(mapAddr + bgIndex);

    addr_t tileAddr = dataAddr;
    if (dataAddr == 0x9000 && tileIndex >= 128) {
      tileIndex = (0xff - tileIndex) + 1;
      tileAddr -= (tileIndex * 16);
    } else {
      tileAddr += (tileIndex * 16);
    }
    tileAddr += (windowY % kTileHeight) * 2;

    uint8_t lsb = mmu_.read(tileAddr + 0);
    uint8_t msb = mmu_.read(tileAddr + 1);

    int palleteIndex = 0;
    int bitIndex = 7 - (windowX % 8);
    palleteIndex += ((lsb >> bitIndex) & 0x01) ? 2 : 0;
    palleteIndex += ((msb >> bitIndex) & 0x01) ? 1 : 0;

    palleteIndex =
        (mmu_.read(Address::HwIoBackgroundPalette) >> (palleteIndex * 2)) & 0x3;

    int pos = (column + (scanline * kDisplayWidth)) * kColorComponentSize;
    for (size_t i = 0; i < kColorComponentSize; i++) {
      pixels_.at(pos + i) = palette_[palleteIndex][i];
    }
  }
}

void Gpu::renderScanlineSprites(u8 scanline) {
  UNUSED(scanline);
  UNUSED(kTileSize);
  UNUSED(kTilesPerColumn);
  UNUSED(ControlFlags::kDisplayEnable);
  UNUSED(ControlFlags::kWindowDisplayEnable);
  UNUSED(ControlFlags::kWindowTileMapDisplaySelect);
  UNUSED(ControlFlags::kSpriteDisplayEnable);
  UNUSED(ControlFlags::kSpriteDisplayEnable);
  UNUSED(ControlFlags::kSpriteSizeSelect);

  const u8 control = mmu_.read(Address::HwIoLcdControl);

  bool is8x16 = control & ControlFlags::kSpriteSizeSelect;
  const u8 width = 8;
  const u8 height = is8x16 ? 16 : 8;

  auto oam = mmu_.getOAM();
  Sprite *allSprites = reinterpret_cast<Sprite *>(oam.data());
  auto count = oam.size() / sizeof(Sprite);

  std::deque<Sprite *> visibleSprites;
  for (size_t i = 0; i < count; i++) {
    Sprite *sprite = allSprites + i;

    if (sprite->x == 0 || sprite->x >= (160 + width) || sprite->y == 0 ||
        sprite->y >= (144 + 16)) {
      continue;
    }

    if ((sprite->screenY() > scanline) ||
        (sprite->screenY() + height) <= scanline) {
      continue;
    }

    if (sprite->hasPriority()) {
      visibleSprites.push_front(sprite);
    } else {
      visibleSprites.push_back(sprite);
    }
  }

  if (visibleSprites.size() > 10) {
    visibleSprites.resize(10);
  }

  const uint16_t kSpriteTileSize = 16;
  const uint16_t kSpriteTileLineSize = 2;
  const uint16_t kSpriteTileAddress = 0x8000;

  for (auto &sprite : visibleSprites) {
    addr_t tileNumber = is8x16 ? (sprite->tile & 0xfe) : sprite->tile;
    addr_t tileAddress = kSpriteTileAddress + (tileNumber * kSpriteTileSize);
    addr_t tileLine = scanline - sprite->screenY();

    if (sprite->isFlipY()) {
      tileLine = (height - 1) - tileLine;
    }

    addr_t tileLineAddress = tileAddress + (tileLine * kSpriteTileLineSize);

    u8 lsb = mmu_.read(tileLineAddress);
    u8 msb = mmu_.read(tileLineAddress + 1);

    u8 spritePallete =
        mmu_.read(sprite->isPalette1() ? Address::HwIoSpritePalette1
                                       : Address::HwIoSpritePalette0);

    for (size_t i = 0; i < width; i++) {
      if ((sprite->x + i) < width || (sprite->screenX() + i) >= kDisplayWidth) {
        // Pixel not visible
        continue;
      }

      int bitIndex = sprite->isFlipX() ? i : (7 - i);

      int spritePaletteIndex = 0;
      spritePaletteIndex += ((lsb >> bitIndex) & 0x01) ? 2 : 0;
      spritePaletteIndex += ((msb >> bitIndex) & 0x01) ? 1 : 0;

      int palleteIndex = (spritePallete >> (spritePaletteIndex * 2)) & 0x3;

      int column = sprite->screenX() + i;

      int pos = (column + (scanline * kDisplayWidth)) * kColorComponentSize;
      for (size_t i = 0; i < kColorComponentSize; i++) {
        pixels_.at(pos + i) = palette_[palleteIndex][i];
      }
    }
  }
}
