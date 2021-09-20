#pragma once

#include <memory>
#include <span>
#include "global.h"

class FluidSolver{
public:
    explicit FluidSolver(std::size_t, std::size_t ); 
    ~FluidSolver();
    FluidSolver(const FluidSolver & ) = delete;
    FluidSolver & operator=(const FluidSolver & ) = delete;

    void SolveStep();
    void Reset();
    void SetGravity(float  , float);

    std::span<const RGBA> GetColors() const noexcept ;
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