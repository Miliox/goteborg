/*
 * mmuimpl-tests.cpp
 * Copyright (C) 2020 Emiliano Firmino <emiliano.firmino@gmail.com>
 *
 * Distributed under terms of the MIT license.
 */

#include "catch2/catch.hpp"
#include "mmuimpl.hpp"

using namespace gbg;

TEST_CASE("Simple store/load", "[MMUImpl]") {
  MMUImpl mmu;
  REQUIRE(mmu.read(0) == 255);
  REQUIRE(mmu.read(65'535) == 255);

  mmu.write(65'535, 100);
  REQUIRE(mmu.read(65'535) == 100);
}

TEST_CASE("Echo store/load", "[MMUImpl]") {
  MMUImpl mmu;
  mmu.write(0xe000, 100);
  REQUIRE(mmu.read(0xc000) == 100);
}