/*
 * mmuimpl.h
 * Copyright (C) 2020 Emiliano Firmino <emiliano.firmino@gmail.com>
 *
 * Distributed under terms of the MIT license.
 */

#ifndef MMUIMPL_H
#define MMUIMPL_H

#include "mmu.h"

namespace goteborg {

class MMUImpl : MMU {
public:
    MMUImpl();
    virtual ~MMUImpl();

    u8 read(addr src) override;
    void step(u8 ticks) override;
    void transfer(addr dst, addr src) override;
    void write(addr dst, u8 value) override;

private:
    /**
     * Simple flat storage layout
     */
    buffer mem;

    /**
     * Resolve real address
     */
    addr resolve(addr a);
};

}

#endif /* !MMUIMPL_H */
