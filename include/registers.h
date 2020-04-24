/*
 * registers.h
 * Copyright (C) 2020 Emiliano Firmino <emiliano.firmino@gmail.com>
 *
 * Distributed under terms of the MIT license.
 */

#ifndef REGISTERS_H
#define REGISTERS_H

#include "common.h"

namespace gbg {

enum class Flags : u8 {
    ZERO   = 1 << 7,
    NEGATE = 1 << 6,
    HALF   = 1 << 5,
    CARRY  = 1 << 4,
    UNDEF3 = 1 << 3,
    UNDEF2 = 1 << 2,
    UNDEF1 = 1 << 1,
    UNDEF0 = 1 << 0
};

class Registers {
public:
    struct {
        union {
            struct {
                u8 f; // flag
                u8 a; // accumulator
            };
            u16 af;   // data/address
        };
    };

    struct {
        union {
            struct {
                u8 c;
                u8 b;
            };
            u16 bc;  // data/address
        };
    };

    struct {
        union {
            struct {
                u8 e;
                u8 d;
            };
            u16 de;  // data/address
        };
    };

    struct {
        union {
            struct {
                u8 l;
                u8 h;
            };
            u16 hl;  // data/address
        };
    };

    u16 sp;  // stack pointer
    u16 pc;  // program counter

    u8  ime;   // interrupt master enabled

    Registers() : af(0xffff), bc(0xffff), de(0xffff), hl(0xffff), sp(0xffff), pc(0), ime(0) {}
};

} // namespace gbg

#endif /* !REGISTERS_H */
