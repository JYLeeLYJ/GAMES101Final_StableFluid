#include "gui.h"
#include "fluid_solver.h"
#include <iostream>
#include <Eigen/Eigen>
#include <chrono>
#include <format>

// TODO : 
// 1. frame buffer should directly write into d3d backgroud
// 2. GUI interactive { color picker , reset , gravity setting }
// 3. more example { taichi stable fluid }

int main(){
    auto gui = GUI{600,600};
    auto solver = FluidSolver{600,600};
    while(true){
        if(gui.ProcessMessage()) break;
        auto beg = std::chrono::high_resolution_clock::now();
        solver.SolveStep();
        gui.UpdateFrameBuffer(solver.GetColors());
        //Render GUI
        gui.Render([]{
            
        });
        gui.Update();
        auto end = std::chrono::high_resolution_clock::now();
        auto time_spend = std::chrono::duration_cast<std::chrono::milliseconds>(end - beg).count();
        gui.SetWindowsTitle(std::format("Stable Fluid (FPS:{})" , 1000 / time_spend));
    }
}
