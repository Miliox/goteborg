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

    MMUImpl mmu;
    LR35902 cpu(mmu);

    {
        sf::FileInputStream bios;
        if (!bios.open("bios.bin")) {
            std::cerr << "error: cannot load bios" << std::endl;
            return -1;
        }

        buffer data(bios.getSize(), 0xff);
        bios.read(reinterpret_cast<void*>(&data.front()), data.size());
        for (size_t addr = 0; addr < data.size(); addr++) {
            mmu.write(addr, data.at(addr));
        }
    }

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

        window.clear();
        window.draw(shape);
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