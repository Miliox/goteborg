/*
 * main.cpp
 * Copyright (C) 2020 Emiliano Firmino <emiliano.firmino@gmail.com>
 *
 * Distributed under terms of the MIT license.
 */

#include "main.hpp"
#include "address.hpp"
#include "alu.hpp"
#include "emulator.hpp"

#include <fstream>
#include <inttypes.h>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <thread>
#include <time.h>
#include <unistd.h>

using namespace std::chrono;

#include <imgui-SFML.h>
#include <imgui.h>

#include <nlohmann/json.hpp>

#include <SFML/Graphics/CircleShape.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Texture.hpp>
#include <SFML/System/Clock.hpp>
#include <SFML/System/FileInputStream.hpp>
#include <SFML/Window/Event.hpp>

using namespace gbg;
using namespace std::chrono;

void setCurrentWorkingDirectory(const char *appName);
nlohmann::json loadDisasmData();

int main(int argc, char **argv) {
  UNUSED(argc);
  setCurrentWorkingDirectory(argv[0]);

  auto disasm = loadDisasmData();

  u8 frameRate = 60;

  s32 lastFrameCount = 0;
  s32 frameCounter = 0;
  s64 currentSecond = 0;

  bool running = true;

  sf::RenderWindow window(sf::VideoMode(640, 480), "Goteborg");
  // window.setFramerateLimit(frameRate);
  ImGui::SFML::Init(window);

  Emulator emulator(frameRate);
  emulator.reset();

  const auto kNanosPerFrame = nanoseconds(1'000'000'000 / frameRate);

  auto lastTs = high_resolution_clock::now();
  auto oversleep = nanoseconds(0);

  sf::Clock deltaClock;
  while (window.isOpen()) {
    sf::Event event;
    while (window.pollEvent(event)) {
      ImGui::SFML::ProcessEvent(event);
      if (event.type == sf::Event::KeyPressed &&
          event.key.code == sf::Keyboard::Return) {
        emulator.nextTicks();
      }

      if (event.type == sf::Event::KeyPressed &&
          event.key.code == sf::Keyboard::Space) {
        running = !running;
      }

      if (event.type == sf::Event::KeyPressed &&
          event.key.code == sf::Keyboard::Z) {
        if (emulator.getRegisters().f & alu::kFZ) {
          emulator.getRegisters().f &= ~alu::kFZ;
        } else {
          emulator.getRegisters().f |= alu::kFZ;
        }
      }

      if (event.type == sf::Event::Closed) {
        window.close();
      }
    }

    ImGui::SFML::Update(window, deltaClock.restart());

    if (running) {
      emulator.nextFrame();
      if (emulator.getRegisters().pc == 0x027e) {
        running = false;
      }
    }

    ImGui::Begin("Debugger");

    ImGui::Text("PC   FLAGS    A  F  B  C  D  E  H  L  AF   BC   DE   HL   SP");
    //           0000 ZNHC---- 00 00 00 00 00 00 00 00 0000 0000 0000 0000 0000

    auto &r = emulator.getRegisters();
    ImGui::Text(
        "%04x %c%c%c%c%c%c%c%c %02X %02X %02X %02X %02X %02X %02X %02X %04x "
        "%04x %04x %04x %04x",
        r.pc, (r.f & 0b1000'0000) ? 'Z' : '-', (r.f & 0b0100'0000) ? 'N' : '-',
        (r.f & 0b0010'0000) ? 'H' : '-', (r.f & 0b0001'0000) ? 'C' : '-',
        (r.f & 0b0000'1000) ? '1' : '-', (r.f & 0b0000'0100) ? '1' : '-',
        (r.f & 0b0000'0010) ? '1' : '-', (r.f & 0b0000'0001) ? '1' : '-', r.a,
        r.f, r.b, r.c, r.d, r.e, r.h, r.l, r.af, r.bc, r.de, r.hl, r.sp);

    addr_t addr = r.pc;
    for (int i = 0; i < 16; i++) {
      size_t opcode = emulator.getMMU().read(addr);

      if (opcode == 0xcb) {
        auto op = disasm.at(opcode);
        std::string format =
            std::string("%04x: ") + std::string(op.at("format"));
        ImGui::Text(format.c_str(), addr);

        i += 1;
        addr += 1;
        opcode = 0x100 + emulator.getMMU().read(addr);

        if (i >= 16) {
          break;
        }
      }

      auto op = disasm.at(opcode);
      int len = op.at("length");
      auto text = std::string("%04x: ") + std::string(op.at("format"));

      if (len == 1) {
        ImGui::Text(text.c_str(), addr);
      } else if (len == 2) {
        ImGui::Text(text.c_str(), addr, emulator.getMMU().read(addr + 1));
      } else if (len == 3) {
        ImGui::Text(text.c_str(), addr, emulator.getMMU().read(addr + 2),
                    emulator.getMMU().read(addr + 1));
      } else {
        ImGui::Text(text.c_str(), addr);
      }

      addr += op.at("length").get<int>();
    }

    s64 epoch = static_cast<s64>(time(nullptr));
    if (epoch == currentSecond) {
      frameCounter += 1;
    } else {
      currentSecond = epoch;
      lastFrameCount = frameCounter;
      frameCounter = 0;
    }

    ImGui::Text("fps: %d", lastFrameCount);

    ImGui::End();

    window.clear();
    emulator.render(window);
    ImGui::SFML::Render(window);
    window.display();

    auto now = high_resolution_clock::now();
    auto duty = duration_cast<nanoseconds>(now - lastTs);

    if ((duty + oversleep) < kNanosPerFrame) {
      auto delay = kNanosPerFrame - duty - oversleep;
      std::this_thread::sleep_for(nanoseconds(delay));

      // store oversleep to compesate for it on frame sync
      oversleep =
          duration_cast<nanoseconds>(high_resolution_clock::now() - now) -
          delay;
    } else {
      oversleep = nanoseconds(0);
    }

    lastTs = high_resolution_clock::now();
  }

  ImGui::SFML::Shutdown();
  return 0;
}

void setCurrentWorkingDirectory(const char *appName) {
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