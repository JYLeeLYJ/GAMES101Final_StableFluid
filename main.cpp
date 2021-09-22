#include "gui.h"
#include "fluid_solver.h"
#include <imgui.h>
#include <iostream>
#include <Eigen/Eigen>
#include <chrono>
#include <format>

// TODO : 
// 1. frame buffer should directly write into d3d backgroud
// 2. GUI interactive { color picker , reset , gravity setting }
// 3. more example { taichi stable fluid }

using namespace std::chrono;
using namespace std::string_literals;

int main(){
    auto gui = GUI{600,600};
    auto solver = FluidSolver{600,600};
    // GUI states
    bool paused = false;
    bool reset = false;
    int cnt = 3;
    // main loop 
    while(true){
        if(gui.ProcessMessage(paused)) break;
        auto beg = high_resolution_clock::now();
        // solver step
        if(reset) solver.Reset() , reset = false;
        if(!paused) solver.SolveStep();
        // update ui & window
        gui.UpdateFrameBuffer(solver.GetColors());
        // Render GUI
        gui.Render([&]{
            auto flags = 
                ImGuiWindowFlags_NoResize |
                ImGuiWindowFlags_NoSavedSettings ;
            ImGui::Begin("Config" , nullptr , flags);
            if(ImGui::Button(paused ? "Continue" : " Pause ")) paused = !paused ;
            if(ImGui::Button("Reset")) reset = true;
            ImGui::End();
        });
        gui.Update();
        // time elapse
        auto end = high_resolution_clock::now();
        // update time info 
        auto time_spend = duration_cast<milliseconds>(end - beg).count();
        auto title = paused ? 
            "Stable Fluid (PAUSE)"s :
            std::format("Stable Fluid (FPS:{})" ,1000 / (time_spend + 1));
        gui.SetWindowsTitle(std::move(title));
    }
}
