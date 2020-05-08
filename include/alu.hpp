/*
 * alu.h
 * Copyright (C) 2020 Emiliano Firmino <emiliano.firmino@gmail.com>
 *
 * Distributed under terms of the MIT license.
 */

#ifndef ALU_H
#define ALU_H

#include "common.hpp"

namespace gbg {

namespace alu {

// flags
extern const u8 kFZ; // Flag Zero
extern const u8 kFN; // Flag Negative
extern const u8 kFH; // Flag Half Carry
extern const u8 kFC; // Flag Carry

// move operations

void ld8(u8 &dst, u8 src);
void ld16(u16 &dst, u16 src);

// arithmetic operations

void add8(u8 &flags, u8 &acc, u8 arg);
void adc8(u8 &flags, u8 &acc, u8 arg);
void sub8(u8 &flags, u8 &acc, u8 arg);
void sbc8(u8 &flags, u8 &acc, u8 arg);
void inc8(u8 &flags, u8 &acc);
void dec8(u8 &flags, u8 &acc);

void add16(u8 &flags, u16 &acc, u16 arg);
void sub16(u8 &flags, u16 &acc, u16 arg);
void inc16(u8 &flags, u16 &acc);
void dec16(u8 &flags, u16 &acc);

// logical operation

void land(u8 &flags, u8 &acc, u8 arg);
void lxor(u8 &flags, u8 &acc, u8 arg);
void lor(u8 &flags, u8 &acc, u8 arg);
void lcp(u8 &flags, u8 &acc, u8 arg);

// bit manipulation

void bit(u8 &flags, u8 &acc, u8 arg);
void set(u8 &flags, u8 &acc, u8 arg);
void res(u8 &flags, u8 &acc, u8 arg);
void cpl(u8 &flags, u8 &acc);

// bit rotation and shifts

void rl(u8 &flags, u8 &acc);
void rr(u8 &flags, u8 &acc);
void rlc(u8 &flags, u8 &acc);
void rrc(u8 &flags, u8 &acc);

void sla(u8 &flags, u8 &acc);
void sra(u8 &flags, u8 &acc);
void srl(u8 &flags, u8 &acc);

// misc

void ccf(u8 &flags);
void scf(u8 &flags);
void daa(u8 &flags, u8 &acc);
void swap(u8 &flags, u8 &acc);

} // namespace alu

} // namespace gbg

#endif /* !ALU_H */
