#include "gui.h"
#include "fluid_solver.h"
#include <iostream>
#include <Eigen/Eigen>

int main(){
    auto gui = GUI{600,600};
    auto solver = FluidSolver{600,600};
    while(true){
        if(gui.ProcessMessage()) break;
        solver.SolveStep();
        gui.UpdateFrameBuffer(solver.GetColors());
        //Render GUI
        gui.Render([]{
            
        });
        gui.Update();
    }
}
