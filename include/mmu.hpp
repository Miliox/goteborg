/*
 * mmu.h
 * Copyright (C) 2020 Emiliano Firmino <emiliano.firmino@gmail.com>
 *
 * Distributed under terms of the MIT license.
 */

#ifndef MMU_H
#define MMU_H

#include "common.hpp"

namespace gbg {

/**
 * Memory Management Unit
 * 
 * Interface all transfers to and from memory.
 */
class MMU {
public:
    MMU() = default;
    virtual ~MMU() = default;

    MMU(const MMU&) = delete;
    MMU& operator=(const MMU&) = delete;

    /**
     * Read value contained into address
     */
    virtual u8 read(addr_t src) = 0;

    /**
     * Execute internal dma transfer
     */
    virtual void step(ticks_t ticks) = 0;

    /**
     * Begin DMA transfer 
     */
    virtual void transfer(addr_t dst, addr_t src) = 0;

    /**
     * Write value to destination address
     */
    virtual void write(addr_t dst, u8 value) = 0;

};

} // namespace gbg

#endif /* !MMU_H */
