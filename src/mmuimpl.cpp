/*
 * mmuimpl.cpp
 * Copyright (C) 2020 Emiliano Firmino <emiliano.firmino@gmail.com>
 *
 * Distributed under terms of the MIT license.
 */

#include "mmuimpl.hpp"
#include "interrupt.hpp"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <iomanip>
#include <utility>

using namespace gbg;

namespace MemAddr {
    static const addr_t kBiosROM      = 0x0000;
    static const addr_t kCartridgeROM = 0x0000;
    static const addr_t kVideoRAM     = 0x8000;
    static const addr_t kCartridgeRAM = 0xA000;
    static const addr_t kLowRAM       = 0xC000;
    static const addr_t kEchoRAM      = 0xE000;
    static const addr_t kOamRAM       = 0xFE00;
    static const addr_t kInvRAM       = 0xFEA0;
    static const addr_t kHwIO         = 0xFF00;
    static const addr_t kHighRAM      = 0xFF80;
}

namespace MemSize {
    static const size_t kBiosROM      = 0x0100;
    static const size_t kCartridgeROM = 0x8000;
    static const size_t kVideoRAM     = 0x2000;
    static const size_t kCartridgeRAM = 0x2000;
    static const size_t kLowRAM       = 0x2000;
    static const size_t kEchoRAM      = 0x1E00;
    static const size_t kOamRAM       = 0x00A0;
    static const size_t kInvRAM       = 0x0060;
    static const size_t kHwIO         = 0x0080;
    static const size_t kHighRAM      = 0x0080;
}

static_assert(MemAddr::kBiosROM == 0, "bios should be at start of address space");
static_assert((MemAddr::kBiosROM + MemSize::kBiosROM) == 0x100, "bios must be 256 bytes long");

static_assert(MemAddr::kBiosROM == 0);
static_assert(MemAddr::kCartridgeROM == MemAddr::kBiosROM);
static_assert((MemAddr::kCartridgeROM + MemSize::kCartridgeROM) == MemAddr::kVideoRAM);
static_assert((MemAddr::kVideoRAM     + MemSize::kVideoRAM)     == MemAddr::kCartridgeRAM);
static_assert((MemAddr::kCartridgeRAM + MemSize::kCartridgeRAM) == MemAddr::kLowRAM);
static_assert((MemAddr::kLowRAM       + MemSize::kLowRAM)       == MemAddr::kEchoRAM);
static_assert((MemAddr::kEchoRAM      + MemSize::kEchoRAM)      == MemAddr::kOamRAM);
static_assert((MemAddr::kOamRAM       + MemSize::kOamRAM)       == MemAddr::kInvRAM);
static_assert((MemAddr::kInvRAM       + MemSize::kInvRAM)       == MemAddr::kHwIO);
static_assert((MemAddr::kHwIO         + MemSize::kHwIO)         == MemAddr::kHighRAM);
static_assert((MemAddr::kHighRAM      + MemSize::kHighRAM)      == 0x10000);

MMUImpl::MMUImpl() : MMU(),
    bios_(MemSize::kBiosROM, 0xff),
    crom_(MemSize::kCartridgeROM, 0xff),
    vram_(MemSize::kVideoRAM, 0xff),
    cram_(MemSize::kCartridgeRAM, 0xff),
    lram_(MemSize::kLowRAM, 0xff),
    oram_(MemSize::kOamRAM, 0xff),
    hwio_(MemSize::kHwIO, 0),
    hram_(MemSize::kHighRAM, 0xff),
    timer_(0),
    divider_(0) {

}

void MMUImpl::loadBios(const buffer_t& bios) {
    if (bios.size() != 256) {
        throw std::runtime_error("bios must be 256 bytes long");
    }

    bios_ = bios;
}

void MMUImpl::loadCartridge(const buffer_t& rom) {
    auto result = div(rom.size(), 32 * 1024);
    if (rom.size() == 0 && result.rem != 0) {
        throw std::runtime_error("cartridge rom must be multiple of 32Kb");
    }
    crom_ = rom;
}

u8 MMUImpl::read(addr_t src) {
    if (src < (MemAddr::kBiosROM + MemSize::kBiosROM) && hwio_.at(0x50) != 1) {
        src -= MemAddr::kBiosROM;
        return bios_.at(src);
    }

    static_assert(MemSize::kCartridgeRAM < MemAddr::kVideoRAM);

    if (src < (MemAddr::kCartridgeROM + MemSize::kCartridgeROM)) {
        src -= MemAddr::kCartridgeROM;
        return crom_.at(src);
    }

    static_assert(MemSize::kVideoRAM < MemAddr::kCartridgeRAM);

    if (src < (MemAddr::kVideoRAM + MemSize::kVideoRAM)) {
        src -= MemAddr::kVideoRAM;
        return vram_.at(src);
    }

    static_assert(MemAddr::kCartridgeRAM < MemAddr::kLowRAM);

    if (src < (MemAddr::kCartridgeRAM + MemSize::kCartridgeRAM)) {
        src -= MemAddr::kCartridgeRAM;
        return cram_.at(src);
    }

    static_assert(MemAddr::kLowRAM < MemAddr::kEchoRAM);

    if (src < (MemAddr::kLowRAM + MemSize::kLowRAM)) {
        src -= MemAddr::kLowRAM;
        return lram_.at(src);
    }

    static_assert(MemAddr::kEchoRAM < MemAddr::kOamRAM);

    if (src < (MemAddr::kEchoRAM + MemSize::kEchoRAM)) {
        src -= MemAddr::kEchoRAM;
        return lram_.at(src);
    }

    static_assert(MemAddr::kOamRAM < MemAddr::kInvRAM);

    if (src < (MemAddr::kOamRAM + MemSize::kOamRAM)) {
        src -= MemAddr::kOamRAM;
        return oram_.at(src);
    }

    static_assert(MemAddr::kInvRAM < MemAddr::kHwIO);

    if (src < (MemAddr::kInvRAM + MemSize::kInvRAM)) {
        return 0x00;
    }

    if (src < (MemAddr::kHwIO + MemSize::kHwIO)) {
        src -= MemAddr::kHwIO;
        return hwio_.at(src);
    }

    return 0xff;
}

static const ticks_t kTimerFrequencies[4] = {4096, 262144, 65536, 16384};

static const ticks_t kTimerDuration[4] = {
    kClockRate / kTimerFrequencies[0],
    kClockRate / kTimerFrequencies[1],
    kClockRate / kTimerFrequencies[2],
    kClockRate / kTimerFrequencies[3]
};

static const ticks_t kDividerDuration = 16384;

static const u8 kHwIoIndexTimerDivider = 0x04;
static const u8 kHwIoIndexTimerCounter = 0x05;
static const u8 kHwIoIndexTimerModulo  = 0x06;
static const u8 kHwIoIndexTimerControl = 0x07;
static const u8 kHwIoIndexInterruptFlag = 0x0f;

static const u8 kTimerControlStartFlag = 0x04;
static const u8 kTimerControlClockSelectMask = 0x03;

void MMUImpl::step(ticks_t ticks) {
    // Todo: DMA

    // Divider
    divider_ += ticks;
    if (divider_ >= kDividerDuration) {
        divider_ -= kDividerDuration;
        hwio_.at(kHwIoIndexTimerDivider) += 1;
    }

    // Timer
    u8 timerControl = hwio_.at(kHwIoIndexTimerControl);
    u8 timerRunning = timerControl & kTimerControlStartFlag;
    if (timerRunning) {
        u8 clockSelect = timerControl & kTimerControlClockSelectMask;

        timer_ += ticks;
        if (timer_ >= kTimerDuration[clockSelect]) {
            timer_ -= kTimerDuration[clockSelect];
            hwio_.at(kHwIoIndexTimerCounter) += 1;

            if (hwio_.at(kHwIoIndexTimerCounter) == 0) {
                hwio_.at(kHwIoIndexInterruptFlag) |= kTimerOverflowInterrupt;
                hwio_.at(kHwIoIndexTimerCounter) = hwio_.at(kHwIoIndexTimerModulo);
            }
        }
    } else {
        timer_ = 0;
    }
}

void MMUImpl::transfer(addr_t dst, addr_t src) {
    std::cout << "dma: src=" << std::hex << static_cast<u32>(src) << " dst=" << static_cast<u32>(dst) << "\n";
    for (addr_t i = 0; i < 160; i++) {
        write(dst + i, read(src + i));
    }
}

void MMUImpl::write(addr_t dst, u8 value) {
    static_assert(MemSize::kCartridgeRAM < MemAddr::kVideoRAM);

    if (dst < (MemAddr::kCartridgeROM + MemSize::kCartridgeROM)) {
        // read only
        return;
    }

    static_assert(MemSize::kVideoRAM < MemAddr::kCartridgeRAM);

    if (dst < (MemAddr::kVideoRAM + MemSize::kVideoRAM)) {
        dst -= MemAddr::kVideoRAM;
        vram_.at(dst) = value;
        return;
    }

    static_assert(MemAddr::kCartridgeRAM < MemAddr::kLowRAM);

    if (dst < (MemAddr::kCartridgeRAM + MemSize::kCartridgeRAM)) {
        dst -= MemAddr::kCartridgeRAM;
        cram_.at(dst) = value;
        return;
    }

    static_assert(MemAddr::kLowRAM < MemAddr::kEchoRAM);

    if (dst < (MemAddr::kLowRAM + MemSize::kLowRAM)) {
        dst -= MemAddr::kLowRAM;
        lram_.at(dst) = value;
        return;
    }

    static_assert(MemAddr::kEchoRAM < MemAddr::kOamRAM);

    if (dst < (MemAddr::kEchoRAM + MemSize::kEchoRAM)) {
        dst -= MemAddr::kEchoRAM;
        lram_.at(dst) = value;
        return;
    }

    static_assert(MemAddr::kOamRAM < MemAddr::kInvRAM);

    if (dst < (MemAddr::kOamRAM + MemSize::kOamRAM)) {
        dst -= MemAddr::kOamRAM;
        oram_.at(dst) = value;
        return;
    }

    static_assert(MemAddr::kInvRAM < MemAddr::kHwIO);

    if (dst < (MemAddr::kInvRAM + MemSize::kInvRAM)) {
        // read only
        return;
    }

    if (dst < (MemAddr::kHwIO + MemSize::kHwIO)) {
        dst -= MemAddr::kHwIO;

        if (dst == kHwIoIndexTimerDivider) {
            hwio_.at(dst) = 0;
            return;
        }

        hwio_.at(dst) = value;
        return;
    }
}
