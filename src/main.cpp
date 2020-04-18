/*
 * main.cpp
 * Copyright (C) 2020 Emiliano Firmino <emiliano.firmino@gmail.com>
 *
 * Distributed under terms of the MIT license.
 */

#include "main.h"
#include "emulator.h"
#include "alu.h"

#include <fstream>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <inttypes.h>
#include <unistd.h>

#include <imgui.h>
#include <imgui-SFML.h>

#include <nlohmann/json.hpp>

#include <SFML/Graphics/CircleShape.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/System/Clock.hpp>
#include <SFML/System/FileInputStream.hpp>
#include <SFML/Window/Event.hpp>

using namespace goteborg;

void setCurrentWorkingDirectory(const char* appName);
nlohmann::json loadDisasmData();

int main(int argc, char** argv) {
    setCurrentWorkingDirectory(argv[0]);

    auto disasm = loadDisasmData();

    sf::RenderWindow window(sf::VideoMode(640, 480), "Goteborg");
    window.setFramerateLimit(60);
    ImGui::SFML::Init(window);

    Emulator emulator;
    emulator.reset();

    auto& r = emulator.getCPU().regs;
    auto& mmu = emulator.getMMU();

    ticks_t elapsedTicks = 0;

    sf::Clock deltaClock;
    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            ImGui::SFML::ProcessEvent(event);

            if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Return) {
                elapsedTicks += emulator.step();
            }

            if (event.type == sf::Event::Closed) {
                window.close();
            }
        }

        ImGui::SFML::Update(window, deltaClock.restart());

        ImGui::Begin("Debugger");

        ImGui::Text("PC   FLAGS    A  F  B  C  D  E  H  L  AF   BC   DE   HL   SP");
        //           0000 ZNHC---- 00 00 00 00 00 00 00 00 0000 0000 0000 0000 0000
        ImGui::Text("%04x %c%c%c%c%c%c%c%c %02X %02X %02X %02X %02X %02X %02X %02X %04x %04x %04x %04x %04x",
                    r.pc,
                    (r.f & 0b1000'0000) ? 'Z' : '-',
                    (r.f & 0b0100'0000) ? 'N' : '-',
                    (r.f & 0b0010'0000) ? 'H' : '-',
                    (r.f & 0b0001'0000) ? 'C' : '-',
                    (r.f & 0b0000'1000) ? '1' : '-',
                    (r.f & 0b0000'0100) ? '1' : '-',
                    (r.f & 0b0000'0010) ? '1' : '-',
                    (r.f & 0b0000'0001) ? '1' : '-',
                    r.a, r.f, r.b, r.c, r.d, r.e, r.h, r.l, r.af, r.bc, r.de, r.hl, r.sp);

        addr_t addr = r.pc;
        for (int i = 0; i < 16; i++) {
            size_t opcode = mmu.read(addr);

            if (opcode == 0xcb) {
                auto op = disasm.at(opcode);
                std::string format = std::string("%04x: ") + std::string(op.at("format"));
                ImGui::Text(format.c_str(), addr);

                i += 1;
                addr += 1;
                opcode = 0x100 + mmu.read(addr);

                if (i >= 16) {
                    break;
                }
            }

            auto op = disasm.at(opcode);
            int len = op.at("length");
            auto text = std::string("%04x: ") + std::string(op.at("format"));

            if (len == 1) {
                ImGui::Text(text.c_str(), addr);
            }
            else if (len == 2) {
                ImGui::Text(text.c_str(), addr,mmu.read(addr + 1));
            }
            else if (len == 3) {
                ImGui::Text(text.c_str(), addr, mmu.read(addr + 2), mmu.read(addr + 1));
            }
            else {
                ImGui::Text(text.c_str(), addr);
            }

            addr += op.at("length").get<int>();
        }

        ImGui::Text("Ticks: %017" PRIu64, elapsedTicks);

        ImGui::End();

        window.clear();
        ImGui::SFML::Render(window);
        window.display();
    }

    ImGui::SFML::Shutdown();
    return 0;
}

void setCurrentWorkingDirectory(const char* appName) {
    std::string workdir(appName);
    workdir = workdir.substr(0, workdir.rfind('/'));
    workdir = workdir.substr(0, workdir.rfind('/'));
    workdir += "/Resources";
    chdir(workdir.c_str());
}

nlohmann::json loadDisasmData() {
    std::ifstream ifs("disasm.json");
    return nlohmann::json::parse(ifs);
}