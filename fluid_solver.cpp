#include "mats.hpp"
#include "fluid_solver.h"

struct FluidSolver::Impl{
    //grids { pressure , velocity , dye , RGBA buffer }
    Field<Vec2f> pressure , pressure_next;
    Field<Vec2f> velocity , velocity_next;
    Field<Vec3u8> dye;
    Field<Vec4u8> color_buffer; //RGBA
    
    Vec3u8 color;  // dye color
    float decay;    // color decay
    float time_stamp ; // time_stamp;
    
    //external_force = 0

    FluidSolver::Impl(std::size_t row, std::size_t col) 
    : pressure(row , col) , pressure_next(row,col) 
    , velocity(row, col) , velocity_next(row,col) 
    , dye(row,  col ) , color_buffer(row ,col){}
};

FluidSolver::FluidSolver(std::size_t shape_x, std::size_t shape_y)
: m_impl(std::make_unique<Impl>(shape_x , shape_y)){
    m_impl->color = {255,0,0};
    m_impl->decay = 0.98;
    m_impl->time_stamp = 0.04;
}

void FluidSolver::SolveStep(){}

void FluidSolver::Advection(){}

void FluidSolver::Reset(){}

void FluidSolver::ExternalForce(){}

void FluidSolver::Projection(){}

void FluidSolver::UdpateVelocity(){}

void FluidSolver::UpdateDye(){}