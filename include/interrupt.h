/*
 * interrupt.h
 * Copyright (C) 2020 Emiliano Firmino <emiliano.firmino@gmail.com>
 *
 * Distributed under terms of the MIT license.
 */

#ifndef INTERRUPT_H
#define INTERRUPT_H

#include "common.h"

namespace gbg {

enum Interrupt : u8 {
    kJoypadReleaseInterrupt = 1 << 4,
    kSerialTransferCompleteInterrupt = 1 << 3,
    kTimerOverflowInterrupt = 1 << 2,
    kLcdControllerInterrupt = 1 << 1,
    kLcdVerticalBlankingInterrupt = 1 << 0
};

}

#endif /* !INTERRUPT_H */
