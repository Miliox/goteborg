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
#include <unistd.h>

#include <imgui.h>
#include <imgui-SFML.h>

#include <SFML/Graphics/CircleShape.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/System/Clock.hpp>
#include <SFML/System/FileInputStream.hpp>
#include <SFML/Window/Event.hpp>

using namespace goteborg;

void setCurrentWorkingDirectory(const char* appName);

int main(int argc, char** argv) {
    setCurrentWorkingDirectory(argv[0]);

    sf::RenderWindow window(sf::VideoMode(640, 480), "Goteborg");
    window.setFramerateLimit(60);
    ImGui::SFML::Init(window);

    Emulator emulator;
    emulator.reset();

    sf::Clock deltaClock;
    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            ImGui::SFML::ProcessEvent(event);

            if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Return) {
                emulator.step();
            }

            if (event.type == sf::Event::Closed) {
                window.close();
            }
        }

        ImGui::SFML::Update(window, deltaClock.restart());

        auto r = emulator.getCPU().getRegisters();
        ImGui::Begin("Debugger");

        ImGui::BeginChild("Registers");

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

        ImGui::EndChild();
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