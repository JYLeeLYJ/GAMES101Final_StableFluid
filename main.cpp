#include "gui.h"
#include <iostream>
#include <Eigen/Eigen>

int main(){
    auto gui = GUI{600,600};
    while(true){
        if(gui.ProcessMessage()) break;
        gui.Render([]{
            
        });
        gui.Update();
    }
}
