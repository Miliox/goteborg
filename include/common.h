/*
 * common.h
 * Copyright (C) 2020 Emiliano Firmino <emiliano.firmino@gmail.com>
 *
 * Distributed under terms of the MIT license.
 */

#ifndef COMMON_H
#define COMMON_H

#include <cstdint>
#include <vector>

#define UNUSED(x) (void)(x)

typedef std::uint8_t  u8;
typedef std::uint16_t u16;
typedef std::uint32_t u32;
typedef std::uint64_t u64;

typedef std::int8_t   s8;
typedef std::int16_t  s16;
typedef std::int32_t  s32;
typedef std::int64_t  s64;

typedef std::uint16_t addr_t;
typedef std::uint64_t ticks_t;

typedef std::vector<u8> buffer_t;

#endif /* !COMMON_H */
