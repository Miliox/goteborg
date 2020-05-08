/*
 * mmuimpl.h
 * Copyright (C) 2020 Emiliano Firmino <emiliano.firmino@gmail.com>
 *
 * Distributed under terms of the MIT license.
 */

#ifndef MMUIMPL_H
#define MMUIMPL_H

#include "mmu.hpp"

namespace gbg {

class MMUImpl : public MMU {
public:
  MMUImpl();
  virtual ~MMUImpl() = default;

  u8 read(addr_t src) override;

  void step(ticks_t ticks) override;
  void transfer(addr_t dst, addr_t src) override;
  void write(addr_t dst, u8 value) override;
  void write(addr_t dst, const buffer_t &data);

  void loadBios(const buffer_t &bios);
  void loadCartridge(const buffer_t &rom);

private:
  buffer_t bios_; // bios
  buffer_t crom_; // cartridge rom
  buffer_t vram_; // video ram
  buffer_t cram_; // cartridge ram
  buffer_t lram_; // low ram
  buffer_t oram_; // object attribute memory
  buffer_t hwio_; // hardware io
  buffer_t hram_; // high ram (zero memory)

  ticks_t timer_;
  ticks_t divider_;
};

} // namespace gbg

#endif /* !MMUIMPL_H */
