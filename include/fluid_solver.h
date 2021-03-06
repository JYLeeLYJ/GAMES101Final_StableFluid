#pragma once

#include <memory>
#include <span>
#include "global.h"


struct FluidConfig{
    int jacobian_step;
    float decay ;
    float time_step;
    float gravity[2];
};

class FluidSolver{
public:
    explicit FluidSolver(std::size_t, std::size_t , const FluidConfig& ); 
    ~FluidSolver();
    FluidSolver(const FluidSolver & ) = delete;
    FluidSolver & operator=(const FluidSolver & ) = delete;

    void SolveStep();
    void Reset();
    void SetColor(float r, float g , float b );
    void SetConfig(const FluidConfig & );
    std::span<const RGBA> GetColors() const noexcept ;
    
    void RunBench();
private:

    void Advection();
    void ExternalForce();
    void Projection();
    void UpdateVelocity();
    void UpdateDye();

private :
    struct Impl ;
    std::size_t m_shape_x ;
    std::size_t m_shape_y ;
    std::unique_ptr<Impl> m_impl;
};