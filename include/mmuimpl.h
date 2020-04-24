/*
 * mmuimpl.h
 * Copyright (C) 2020 Emiliano Firmino <emiliano.firmino@gmail.com>
 *
 * Distributed under terms of the MIT license.
 */

#ifndef MMUIMPL_H
#define MMUIMPL_H

#include "mmu.h"

namespace gbg {

class MMUImpl : public MMU {
public:
    MMUImpl();
    virtual ~MMUImpl();

    u8 read(addr_t src) override;
    void step(u8 ticks) override;
    void transfer(addr_t dst, addr_t src) override;
    void write(addr_t dst, u8 value) override;
    void write(addr_t dst, const buffer_t& data);

private:
    /**
     * Simple flat storage layout
     */
    buffer_t mem;

    /**
     * Resolve real address
     */
    addr_t resolve(addr_t a);
};

}

#endif /* !MMUIMPL_H */
