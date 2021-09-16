#pragma once

#include <memory>

class FluidSolver{
public:
    explicit FluidSolver(std::size_t, std::size_t ); 

    void Reset();
    void Advection();
    void ExternalForce();
    void Projection();
    void UdpateVelocity();
    void UpdateDye();
    void SolveStep();
private:

private :
    struct Impl ;
    std::unique_ptr<Impl> m_impl;
};