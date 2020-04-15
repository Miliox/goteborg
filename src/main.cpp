/*
 * main.cpp
 * Copyright (C) 2020 Emiliano Firmino <emiliano.firmino@gmail.com>
 *
 * Distributed under terms of the MIT license.
 */

#include "main.h"
#include "mmuimpl.h"
#include "lr35902.h"

#include <fstream>
#include <iostream>

#include <imgui.h>
#include <imgui-SFML.h>

#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/System/Clock.hpp>
#include <SFML/Window/Event.hpp>
#include <SFML/Graphics/CircleShape.hpp>

using namespace goteborg;

int main(int argc, char** argv) {
    MMUImpl mmu;
    LR35902 cpu(mmu);

    /* TODO: Bundle BIOS Resource
    std::fstream bios;
    bios.open("res/bios.bin", std::fstream::in | std::fstream::binary);

    if (!bios.is_open()) {
        std::cerr << "error: cannot load bios" << std::endl;
        return -1;
    }

    // Load bios
    addr a = 0;
    auto b = bios.get();
    while (!bios.eof()) {
        mmu.write(a, b);
        b = bios.get();
        a += 1;
    }
    bios.close();
    */

    sf::RenderWindow window(sf::VideoMode(640, 480), "Goteborg");
    window.setFramerateLimit(60);
    ImGui::SFML::Init(window);

    sf::CircleShape shape(100.f);
    shape.setFillColor(sf::Color::Green);

    sf::Clock deltaClock;
    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            ImGui::SFML::ProcessEvent(event);

            if (event.type == sf::Event::Closed) {
                window.close();
            }
        }

        // TODO: Emulator Loop

        ImGui::SFML::Update(window, deltaClock.restart());

        ImGui::Begin("Hello, world!");
        ImGui::Button("Look at this pretty button");
        ImGui::End();

        window.clear();
        window.draw(shape);
        ImGui::SFML::Render(window);
        window.display();
    }

    ImGui::SFML::Shutdown();
    return 0;
}
