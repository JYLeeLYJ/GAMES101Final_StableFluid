#include "gui.h"
#include "fluid_solver.h"
#include <imgui.h>
#include <iostream>
#include <Eigen/Eigen>
#include <chrono>
#include <format>

// TODO : 
// 1. frame buffer should directly write into d3d backgroud
// 2. GUI interactive {gravity setting }
// 4. profiling

using namespace std::chrono;
using namespace std::string_literals;

int main(){
    auto config = FluidConfig{
        .jacobian_step = 100,
        .decay = 0.999,
        .time_step = 0.015,
        .gravity = {0,0},
    };
    auto gui = GUI{600,600};
    auto solver = FluidSolver{600,600, config};
    // GUI states
    bool paused = false;
    bool reset = false;
    bool update = false;
    bool setdecay = false;
    float color[3] = {1.0f , 0.f , 0.f};
    bool setcolor = false;
    // main loop 
    while(true){
        if(gui.ProcessMessage(paused)) break;
        if(ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Escape))) break;
        auto beg = high_resolution_clock::now();
        // solver step
        if(update) solver.SetConfig(config) , update = false;
        if(setcolor) solver.SetColor(color[0] , color[1] , color[2]) , setcolor = false;
        if(reset) solver.Reset() , reset = false;
        if(!paused) solver.SolveStep();
        // update ui & window
        gui.UpdateFrameBuffer(solver.GetColors());
        // Render GUI
        gui.Render([&]{
            // ImGui::ShowDemoWindow();
            constexpr int config_window_width = 270;
            ImGui::SetNextWindowPos({10,10});
            ImGui::SetNextWindowSize({config_window_width,0});
            auto flags = 
                ImGuiWindowFlags_NoScrollbar|
                ImGuiWindowFlags_NoMove |
                ImGuiWindowFlags_NoResize |
                ImGuiWindowFlags_NoSavedSettings ;
            ImGui::Begin("Config" , nullptr , flags);
            ImGui::AlignTextToFramePadding();
            ImGui::PushItemWidth(ImGui::GetFontSize() * -8);
            if(ImGui::Button(paused ? "Continue" : "Pause" )) 
                paused = !paused ;
            ImGui::SameLine();
            if(ImGui::Button("Reset" )) 
                reset = true;
            ImGui::Separator();
            ImGui::InputFloat("time stamp" , &config.time_step);
            ImGui::InputFloat("decay" , &config.decay);
            ImGui::InputInt("jacobian step" , &config.jacobian_step);
            ImGui::InputFloat2("gravity" , config.gravity);
            if(ImGui::Button("Update" )) 
                update = true;
            
            ImGui::Separator();
            auto picker_flag = 
                ImGuiColorEditFlags_NoSmallPreview |
                ImGuiColorEditFlags_RGB |
                ImGuiColorEditFlags_None;
            if(ImGui::ColorPicker3("dyeing color" , color ,picker_flag)){
                setcolor = true;
            }
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