/*
 * lr35902.h
 * Copyright (C) 2020 Emiliano Firmino <emiliano.firmino@gmail.com>
 *
 * Distributed under terms of the MIT license.
 */

#ifndef LR35902_H
#define LR35902_H

#include "common.hpp"
#include "registers.hpp"

#include <iomanip>
#include <sstream>

namespace gbg {

class MMU;
class Registers;

/**
 * SHARP LR35902 (Gameboy CPU)
 *
 * A simpler Zilog Z80.
 *
 * It contains the most of Z80 extended instruncions,
 * but contains only the Intel 8080 registers.
 *
 */
class LR35902 {
public:
    LR35902(MMU& mmu);

    /**
     * Registers
     */
    Registers regs;

    /**
     * Memory Manager Unit
     */
    MMU& mmu;

    /**
     * Run fetch-decode-execute cycle
     */
    ticks_t cycle();

private:
    std::vector<ticks_t (LR35902::*)()> iset_;

    void call(addr_t a);
    void ret();
    void rst(addr_t a);
    void push(u16& r);
    void pop(u16& reg);

    u8  next8();
    u16 next16();

    u8  peek8();
    u16 peek16();

    u8  read8(addr_t a);
    u16 read16(addr_t a);

    void write8(addr_t a, u8 v);
    void write16(addr_t a, u16 v);

    u8  zread8(u8 a);
    u16 zread16(u8 a);

    void zwrite8(u8 a, u8 v);
    void zwrite16(u8 a, u16 v);

    void populateInstructionSets();

    ticks_t notimpl();

    ticks_t opcode00();
    ticks_t opcode01();
    ticks_t opcode02();
    ticks_t opcode03();
    ticks_t opcode04();
    ticks_t opcode05();
    ticks_t opcode06();
    ticks_t opcode07();
    ticks_t opcode08();
    ticks_t opcode09();
    ticks_t opcode0A();
    ticks_t opcode0B();
    ticks_t opcode0C();
    ticks_t opcode0D();
    ticks_t opcode0E();
    ticks_t opcode0F();

    ticks_t opcode10();
    ticks_t opcode11();
    ticks_t opcode12();
    ticks_t opcode13();
    ticks_t opcode14();
    ticks_t opcode15();
    ticks_t opcode16();
    ticks_t opcode17();
    ticks_t opcode18();
    ticks_t opcode19();
    ticks_t opcode1A();
    ticks_t opcode1B();
    ticks_t opcode1C();
    ticks_t opcode1D();
    ticks_t opcode1E();
    ticks_t opcode1F();

    ticks_t opcode20();
    ticks_t opcode21();
    ticks_t opcode22();
    ticks_t opcode23();
    ticks_t opcode24();
    ticks_t opcode25();
    ticks_t opcode26();
    ticks_t opcode27();
    ticks_t opcode28();
    ticks_t opcode29();
    ticks_t opcode2A();
    ticks_t opcode2B();
    ticks_t opcode2C();
    ticks_t opcode2D();
    ticks_t opcode2E();
    ticks_t opcode2F();

    ticks_t opcode30();
    ticks_t opcode31();
    ticks_t opcode32();
    ticks_t opcode33();
    ticks_t opcode34();
    ticks_t opcode35();
    ticks_t opcode36();
    ticks_t opcode37();
    ticks_t opcode38();
    ticks_t opcode39();
    ticks_t opcode3A();
    ticks_t opcode3B();
    ticks_t opcode3C();
    ticks_t opcode3D();
    ticks_t opcode3E();
    ticks_t opcode3F();

    ticks_t opcode40();
    ticks_t opcode41();
    ticks_t opcode42();
    ticks_t opcode43();
    ticks_t opcode44();
    ticks_t opcode45();
    ticks_t opcode46();
    ticks_t opcode47();
    ticks_t opcode48();
    ticks_t opcode49();
    ticks_t opcode4A();
    ticks_t opcode4B();
    ticks_t opcode4C();
    ticks_t opcode4D();
    ticks_t opcode4E();
    ticks_t opcode4F();

    ticks_t opcode50();
    ticks_t opcode51();
    ticks_t opcode52();
    ticks_t opcode53();
    ticks_t opcode54();
    ticks_t opcode55();
    ticks_t opcode56();
    ticks_t opcode57();
    ticks_t opcode58();
    ticks_t opcode59();
    ticks_t opcode5A();
    ticks_t opcode5B();
    ticks_t opcode5C();
    ticks_t opcode5D();
    ticks_t opcode5E();
    ticks_t opcode5F();

    ticks_t opcode60();
    ticks_t opcode61();
    ticks_t opcode62();
    ticks_t opcode63();
    ticks_t opcode64();
    ticks_t opcode65();
    ticks_t opcode66();
    ticks_t opcode67();
    ticks_t opcode68();
    ticks_t opcode69();
    ticks_t opcode6A();
    ticks_t opcode6B();
    ticks_t opcode6C();
    ticks_t opcode6D();
    ticks_t opcode6E();
    ticks_t opcode6F();

    ticks_t opcode70();
    ticks_t opcode71();
    ticks_t opcode72();
    ticks_t opcode73();
    ticks_t opcode74();
    ticks_t opcode75();
    ticks_t opcode76();
    ticks_t opcode77();
    ticks_t opcode78();
    ticks_t opcode79();
    ticks_t opcode7A();
    ticks_t opcode7B();
    ticks_t opcode7C();
    ticks_t opcode7D();
    ticks_t opcode7E();
    ticks_t opcode7F();

    ticks_t opcode80();
    ticks_t opcode81();
    ticks_t opcode82();
    ticks_t opcode83();
    ticks_t opcode84();
    ticks_t opcode85();
    ticks_t opcode86();
    ticks_t opcode87();
    ticks_t opcode88();
    ticks_t opcode89();
    ticks_t opcode8A();
    ticks_t opcode8B();
    ticks_t opcode8C();
    ticks_t opcode8D();
    ticks_t opcode8E();
    ticks_t opcode8F();

    ticks_t opcode90();
    ticks_t opcode91();
    ticks_t opcode92();
    ticks_t opcode93();
    ticks_t opcode94();
    ticks_t opcode95();
    ticks_t opcode96();
    ticks_t opcode97();
    ticks_t opcode98();
    ticks_t opcode99();
    ticks_t opcode9A();
    ticks_t opcode9B();
    ticks_t opcode9C();
    ticks_t opcode9D();
    ticks_t opcode9E();
    ticks_t opcode9F();

    ticks_t opcodeA0();
    ticks_t opcodeA1();
    ticks_t opcodeA2();
    ticks_t opcodeA3();
    ticks_t opcodeA4();
    ticks_t opcodeA5();
    ticks_t opcodeA6();
    ticks_t opcodeA7();
    ticks_t opcodeA8();
    ticks_t opcodeA9();
    ticks_t opcodeAA();
    ticks_t opcodeAB();
    ticks_t opcodeAC();
    ticks_t opcodeAD();
    ticks_t opcodeAE();
    ticks_t opcodeAF();

    ticks_t opcodeB0();
    ticks_t opcodeB1();
    ticks_t opcodeB2();
    ticks_t opcodeB3();
    ticks_t opcodeB4();
    ticks_t opcodeB5();
    ticks_t opcodeB6();
    ticks_t opcodeB7();
    ticks_t opcodeB8();
    ticks_t opcodeB9();
    ticks_t opcodeBA();
    ticks_t opcodeBB();
    ticks_t opcodeBC();
    ticks_t opcodeBD();
    ticks_t opcodeBE();
    ticks_t opcodeBF();

    ticks_t opcodeC0();
    ticks_t opcodeC1();
    ticks_t opcodeC2();
    ticks_t opcodeC3();
    ticks_t opcodeC4();
    ticks_t opcodeC5();
    ticks_t opcodeC6();
    ticks_t opcodeC7();
    ticks_t opcodeC8();
    ticks_t opcodeC9();
    ticks_t opcodeCA();
    ticks_t opcodeCB();
    ticks_t opcodeCC();
    ticks_t opcodeCD();
    ticks_t opcodeCE();
    ticks_t opcodeCF();

    ticks_t opcodeD0();
    ticks_t opcodeD1();
    ticks_t opcodeD2();
    ticks_t opcodeD3();
    ticks_t opcodeD4();
    ticks_t opcodeD5();
    ticks_t opcodeD6();
    ticks_t opcodeD7();
    ticks_t opcodeD8();
    ticks_t opcodeD9();
    ticks_t opcodeDA();
    ticks_t opcodeDB();
    ticks_t opcodeDC();
    ticks_t opcodeDD();
    ticks_t opcodeDE();
    ticks_t opcodeDF();

    ticks_t opcodeE0();
    ticks_t opcodeE1();
    ticks_t opcodeE2();
    ticks_t opcodeE3();
    ticks_t opcodeE4();
    ticks_t opcodeE5();
    ticks_t opcodeE6();
    ticks_t opcodeE7();
    ticks_t opcodeE8();
    ticks_t opcodeE9();
    ticks_t opcodeEA();
    ticks_t opcodeEB();
    ticks_t opcodeEC();
    ticks_t opcodeED();
    ticks_t opcodeEE();
    ticks_t opcodeEF();

    ticks_t opcodeF0();
    ticks_t opcodeF1();
    ticks_t opcodeF2();
    ticks_t opcodeF3();
    ticks_t opcodeF4();
    ticks_t opcodeF5();
    ticks_t opcodeF6();
    ticks_t opcodeF7();
    ticks_t opcodeF8();
    ticks_t opcodeF9();
    ticks_t opcodeFA();
    ticks_t opcodeFB();
    ticks_t opcodeFC();
    ticks_t opcodeFD();
    ticks_t opcodeFE();
    ticks_t opcodeFF();

    ticks_t opcodeCB00();
    ticks_t opcodeCB01();
    ticks_t opcodeCB02();
    ticks_t opcodeCB03();
    ticks_t opcodeCB04();
    ticks_t opcodeCB05();
    ticks_t opcodeCB06();
    ticks_t opcodeCB07();
    ticks_t opcodeCB08();
    ticks_t opcodeCB09();
    ticks_t opcodeCB0A();
    ticks_t opcodeCB0B();
    ticks_t opcodeCB0C();
    ticks_t opcodeCB0D();
    ticks_t opcodeCB0E();
    ticks_t opcodeCB0F();

    ticks_t opcodeCB10();
    ticks_t opcodeCB11();
    ticks_t opcodeCB12();
    ticks_t opcodeCB13();
    ticks_t opcodeCB14();
    ticks_t opcodeCB15();
    ticks_t opcodeCB16();
    ticks_t opcodeCB17();
    ticks_t opcodeCB18();
    ticks_t opcodeCB19();
    ticks_t opcodeCB1A();
    ticks_t opcodeCB1B();
    ticks_t opcodeCB1C();
    ticks_t opcodeCB1D();
    ticks_t opcodeCB1E();
    ticks_t opcodeCB1F();

    ticks_t opcodeCB20();
    ticks_t opcodeCB21();
    ticks_t opcodeCB22();
    ticks_t opcodeCB23();
    ticks_t opcodeCB24();
    ticks_t opcodeCB25();
    ticks_t opcodeCB26();
    ticks_t opcodeCB27();
    ticks_t opcodeCB28();
    ticks_t opcodeCB29();
    ticks_t opcodeCB2A();
    ticks_t opcodeCB2B();
    ticks_t opcodeCB2C();
    ticks_t opcodeCB2D();
    ticks_t opcodeCB2E();
    ticks_t opcodeCB2F();

    ticks_t opcodeCB30();
    ticks_t opcodeCB31();
    ticks_t opcodeCB32();
    ticks_t opcodeCB33();
    ticks_t opcodeCB34();
    ticks_t opcodeCB35();
    ticks_t opcodeCB36();
    ticks_t opcodeCB37();
    ticks_t opcodeCB38();
    ticks_t opcodeCB39();
    ticks_t opcodeCB3A();
    ticks_t opcodeCB3B();
    ticks_t opcodeCB3C();
    ticks_t opcodeCB3D();
    ticks_t opcodeCB3E();
    ticks_t opcodeCB3F();

    ticks_t opcodeCB40();
    ticks_t opcodeCB41();
    ticks_t opcodeCB42();
    ticks_t opcodeCB43();
    ticks_t opcodeCB44();
    ticks_t opcodeCB45();
    ticks_t opcodeCB46();
    ticks_t opcodeCB47();
    ticks_t opcodeCB48();
    ticks_t opcodeCB49();
    ticks_t opcodeCB4A();
    ticks_t opcodeCB4B();
    ticks_t opcodeCB4C();
    ticks_t opcodeCB4D();
    ticks_t opcodeCB4E();
    ticks_t opcodeCB4F();

    ticks_t opcodeCB50();
    ticks_t opcodeCB51();
    ticks_t opcodeCB52();
    ticks_t opcodeCB53();
    ticks_t opcodeCB54();
    ticks_t opcodeCB55();
    ticks_t opcodeCB56();
    ticks_t opcodeCB57();
    ticks_t opcodeCB58();
    ticks_t opcodeCB59();
    ticks_t opcodeCB5A();
    ticks_t opcodeCB5B();
    ticks_t opcodeCB5C();
    ticks_t opcodeCB5D();
    ticks_t opcodeCB5E();
    ticks_t opcodeCB5F();

    ticks_t opcodeCB60();
    ticks_t opcodeCB61();
    ticks_t opcodeCB62();
    ticks_t opcodeCB63();
    ticks_t opcodeCB64();
    ticks_t opcodeCB65();
    ticks_t opcodeCB66();
    ticks_t opcodeCB67();
    ticks_t opcodeCB68();
    ticks_t opcodeCB69();
    ticks_t opcodeCB6A();
    ticks_t opcodeCB6B();
    ticks_t opcodeCB6C();
    ticks_t opcodeCB6D();
    ticks_t opcodeCB6E();
    ticks_t opcodeCB6F();

    ticks_t opcodeCB70();
    ticks_t opcodeCB71();
    ticks_t opcodeCB72();
    ticks_t opcodeCB73();
    ticks_t opcodeCB74();
    ticks_t opcodeCB75();
    ticks_t opcodeCB76();
    ticks_t opcodeCB77();
    ticks_t opcodeCB78();
    ticks_t opcodeCB79();
    ticks_t opcodeCB7A();
    ticks_t opcodeCB7B();
    ticks_t opcodeCB7C();
    ticks_t opcodeCB7D();
    ticks_t opcodeCB7E();
    ticks_t opcodeCB7F();

    ticks_t opcodeCB80();
    ticks_t opcodeCB81();
    ticks_t opcodeCB82();
    ticks_t opcodeCB83();
    ticks_t opcodeCB84();
    ticks_t opcodeCB85();
    ticks_t opcodeCB86();
    ticks_t opcodeCB87();
    ticks_t opcodeCB88();
    ticks_t opcodeCB89();
    ticks_t opcodeCB8A();
    ticks_t opcodeCB8B();
    ticks_t opcodeCB8C();
    ticks_t opcodeCB8D();
    ticks_t opcodeCB8E();
    ticks_t opcodeCB8F();

    ticks_t opcodeCB90();
    ticks_t opcodeCB91();
    ticks_t opcodeCB92();
    ticks_t opcodeCB93();
    ticks_t opcodeCB94();
    ticks_t opcodeCB95();
    ticks_t opcodeCB96();
    ticks_t opcodeCB97();
    ticks_t opcodeCB98();
    ticks_t opcodeCB99();
    ticks_t opcodeCB9A();
    ticks_t opcodeCB9B();
    ticks_t opcodeCB9C();
    ticks_t opcodeCB9D();
    ticks_t opcodeCB9E();
    ticks_t opcodeCB9F();

    ticks_t opcodeCBA0();
    ticks_t opcodeCBA1();
    ticks_t opcodeCBA2();
    ticks_t opcodeCBA3();
    ticks_t opcodeCBA4();
    ticks_t opcodeCBA5();
    ticks_t opcodeCBA6();
    ticks_t opcodeCBA7();
    ticks_t opcodeCBA8();
    ticks_t opcodeCBA9();
    ticks_t opcodeCBAA();
    ticks_t opcodeCBAB();
    ticks_t opcodeCBAC();
    ticks_t opcodeCBAD();
    ticks_t opcodeCBAE();
    ticks_t opcodeCBAF();

    ticks_t opcodeCBB0();
    ticks_t opcodeCBB1();
    ticks_t opcodeCBB2();
    ticks_t opcodeCBB3();
    ticks_t opcodeCBB4();
    ticks_t opcodeCBB5();
    ticks_t opcodeCBB6();
    ticks_t opcodeCBB7();
    ticks_t opcodeCBB8();
    ticks_t opcodeCBB9();
    ticks_t opcodeCBBA();
    ticks_t opcodeCBBB();
    ticks_t opcodeCBBC();
    ticks_t opcodeCBBD();
    ticks_t opcodeCBBE();
    ticks_t opcodeCBBF();

    ticks_t opcodeCBC0();
    ticks_t opcodeCBC1();
    ticks_t opcodeCBC2();
    ticks_t opcodeCBC3();
    ticks_t opcodeCBC4();
    ticks_t opcodeCBC5();
    ticks_t opcodeCBC6();
    ticks_t opcodeCBC7();
    ticks_t opcodeCBC8();
    ticks_t opcodeCBC9();
    ticks_t opcodeCBCA();
    ticks_t opcodeCBCB();
    ticks_t opcodeCBCC();
    ticks_t opcodeCBCD();
    ticks_t opcodeCBCE();
    ticks_t opcodeCBCF();

    ticks_t opcodeCBD0();
    ticks_t opcodeCBD1();
    ticks_t opcodeCBD2();
    ticks_t opcodeCBD3();
    ticks_t opcodeCBD4();
    ticks_t opcodeCBD5();
    ticks_t opcodeCBD6();
    ticks_t opcodeCBD7();
    ticks_t opcodeCBD8();
    ticks_t opcodeCBD9();
    ticks_t opcodeCBDA();
    ticks_t opcodeCBDB();
    ticks_t opcodeCBDC();
    ticks_t opcodeCBDD();
    ticks_t opcodeCBDE();
    ticks_t opcodeCBDF();

    ticks_t opcodeCBE0();
    ticks_t opcodeCBE1();
    ticks_t opcodeCBE2();
    ticks_t opcodeCBE3();
    ticks_t opcodeCBE4();
    ticks_t opcodeCBE5();
    ticks_t opcodeCBE6();
    ticks_t opcodeCBE7();
    ticks_t opcodeCBE8();
    ticks_t opcodeCBE9();
    ticks_t opcodeCBEA();
    ticks_t opcodeCBEB();
    ticks_t opcodeCBEC();
    ticks_t opcodeCBED();
    ticks_t opcodeCBEE();
    ticks_t opcodeCBEF();

    ticks_t opcodeCBF0();
    ticks_t opcodeCBF1();
    ticks_t opcodeCBF2();
    ticks_t opcodeCBF3();
    ticks_t opcodeCBF4();
    ticks_t opcodeCBF5();
    ticks_t opcodeCBF6();
    ticks_t opcodeCBF7();
    ticks_t opcodeCBF8();
    ticks_t opcodeCBF9();
    ticks_t opcodeCBFA();
    ticks_t opcodeCBFB();
    ticks_t opcodeCBFC();
    ticks_t opcodeCBFD();
    ticks_t opcodeCBFE();
    ticks_t opcodeCBFF();
};

} // namespace gbg

#endif /* !LR35902_H */
