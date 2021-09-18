#pragma once

#include <memory>

class FluidSolver{
public:
    explicit FluidSolver(std::size_t, std::size_t ); 
    FluidSolver(const FluidSolver & ) = delete;
    FluidSolver & operator=(const FluidSolver & ) = delete;
    
    void Reset();
    void Advection();
    void ExternalForce();
    void Projection();
    void UpdateVelocity();
    void UpdateDye();
    void SolveStep();
private:

private :
    struct Impl ;
    std::size_t m_shape_x ;
    std::size_t m_shape_y ;
    std::unique_ptr<Impl> m_impl;
};