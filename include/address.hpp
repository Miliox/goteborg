/*
 * address.h
 * Copyright (C) 2020 Emiliano Firmino <emiliano.firmino@gmail.com>
 *
 * Distributed under terms of the MIT license.
 */

#ifndef ADDRESS_H
#define ADDRESS_H

#include "common.hpp"

namespace gbg {

enum Address : addr_t {
    HwIoJoypad = 0xff00,
    HwIoSerialData = 0xff01,
    HwIoSerialControl = 0xff02,
    HwIoDivider = 0xff04,
    HwIoTimerCounter = 0xff05,
    HwIoTimerModulo = 0xff06,
    HwIoTimerControl = 0xff07,
    HwIoInterruptFlags = 0xff0f,
    HwIoSoundRegister10 = 0xff10,
    HwIoSoundRegister11 = 0xff11,
    HwIoSoundRegister12 = 0xff12,
    HwIoSoundRegister13 = 0xff13,
    HwIoSoundRegister14 = 0xff14,
    HwIoSoundRegister21 = 0xff16,
    HwIoSoundRegister22 = 0xff17,
    HwIoSoundRegister23 = 0xff18,
    HwIoSoundRegister24 = 0xff19,
    HwIoSoundRegister30 = 0xff1a,
    HwIoSoundRegister31 = 0xff1b,
    HwIoSoundRegister32 = 0xff1c,
    HwIoSoundRegister33 = 0xff1d,
    HwIoSoundRegister34 = 0xff1e,
    HwIoSoundRegister41 = 0xff20,
    HwIoSoundRegister42 = 0xff21,
    HwIoSoundRegister43 = 0xff22,
    HwIoSoundRegister44 = 0xff23,
    HwIoSoundRegister50 = 0xff24,
    HwIoSoundRegister51 = 0xff25,
    HwIoSoundRegister52 = 0xff26,
    HwIoLcdControl = 0xff40,
    HwIoLcdStatus = 0xff41,
    HwIoScrollY = 0xff42,
    HwIoScrollX = 0xff43,
    HwIoCurrentScanline = 0xff44,
    HwIoComparisonScanline = 0xff45,
    HwIoBackgroundPalette = 0xff47,
    HwIoSpritePalette0 = 0xff48,
    HwIoSpritePalette1 = 0xff49,
    HwIoWindowPositionY = 0xff4a,
    HwIoWindowPositionX = 0xff4b,
    HwIoDMATransferControl = 0xff46,
    HwIoInterruptSwitch = 0xffff
};

}

#endif /* !ADDRESS_H */
