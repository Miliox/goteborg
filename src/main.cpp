/*
 * main.cpp
 * Copyright (C) 2020 Emiliano Firmino <emiliano.firmino@gmail.com>
 *
 * Distributed under terms of the MIT license.
 */

#include "main.h"
#include "address.h"
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
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Texture.hpp>
#include <SFML/System/Clock.hpp>
#include <SFML/System/FileInputStream.hpp>
#include <SFML/Window/Event.hpp>

using namespace gbg;

void setCurrentWorkingDirectory(const char* appName);
nlohmann::json loadDisasmData();

int main(int argc, char** argv) {
    setCurrentWorkingDirectory(argv[0]);

    auto disasm = loadDisasmData();

    u8 frameRate = 60;

    sf::RenderWindow window(sf::VideoMode(640, 480), "Goteborg");
    window.setFramerateLimit(frameRate);
    ImGui::SFML::Init(window);

    Emulator emulator(frameRate);
    emulator.reset();

    sf::Clock deltaClock;
    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            ImGui::SFML::ProcessEvent(event);

            if (event.type == sf::Event::Closed) {
                window.close();
            }
        }

        ImGui::SFML::Update(window, deltaClock.restart());
        emulator.nextFrame();

        window.clear();
        emulator.render(window);
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