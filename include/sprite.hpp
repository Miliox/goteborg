/*
 * sprite.hpp
 * Copyright (C) 2020 Emiliano Firmino <emiliano.firmino@gmail.com>
 *
 * Distributed under terms of the MIT license.
 */

#ifndef SPRITE_H
#define SPRITE_H

#include "common.hpp"

#pragma pack(push, 1)
class Sprite {
private:
public:
  Sprite(u8 y = 0, u8 x = 0, u8 tile = 0, u8 flags = 0)
      : y(y), x(x), tile(tile), flags(flags) {}

  const u8 y;
  const u8 x;
  const u8 tile;
  const u8 flags;

  bool hasPriority() { return flags & 0x80; }
  bool isFlipY() { return flags & 0x40; }
  bool isFlipX() { return flags & 0x20; }
  bool isPalette1() { return flags & 0x10; }

  u8 screenX() { return x - 8; }
  u8 screenY() { return y - 16; }
};
#pragma pack(pop)

static_assert(sizeof(Sprite) == 4);

#endif // SPRITE_H