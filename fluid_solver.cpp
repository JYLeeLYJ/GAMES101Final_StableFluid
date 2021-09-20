#include "mats.hpp"
#include "fluid_solver.h"
#include <algorithm>
#include <cmath>

struct FluidSolver::Impl{
    //grids { pressure , velocity , dye , divergence , RGBA buffer }
    Field<float> pressure , pressure_next;
    Field<float> vel_divergence;
    Field<Vec2f> velocity , velocity_next;
    Field<Vec3f> dye , dye_next;
    Field<RGBA> color_buffer; //RGBA
    
    Vec3f dye_color;  // dye color
    float decay;    // color decay
    float time_stamp ; // time_stamp;

    FluidSolver::Impl(std::size_t shape_x, std::size_t shape_y) 
    : pressure(shape_x , shape_y) , pressure_next(shape_x,shape_y) 
    , vel_divergence(shape_x, shape_y)
    , velocity(shape_x, shape_y) , velocity_next(shape_x,shape_y) 
    , dye(shape_x,  shape_y ) , dye_next(shape_x , shape_y)
    , color_buffer(shape_y ,shape_x){}
};

FluidSolver::FluidSolver(std::size_t shape_x, std::size_t shape_y)
: m_shape_x(shape_x) , m_shape_y(shape_y) 
, m_impl(std::make_unique<Impl>(shape_x , shape_y)){
    Reset();
    m_impl->dye_color = {0.9f,0,0};
    m_impl->decay = 0.99;
    m_impl->time_stamp = 0.03;
}

FluidSolver::~FluidSolver() {}

void FluidSolver::SolveStep(){
    Advection();
    ExternalForce();
    Projection();
    UpdateVelocity();
    UpdateDye();
}

std::span<const RGBA> FluidSolver::GetColors() const noexcept {
    return m_impl->color_buffer.Span();
}

template<class T>
T LinearInterpolate(const T& a ,const T& b , float t) {
    return a + t * (b - a);
}

template<class T>
T BilinearInterpolate(Field<T> & f , Vec2f pos) {
    Vec2f p = pos - 0.5 ;
    int u = std::floor(p[0]) , v = std::floor(p[1]);
    float fu = p[0] - u , fv = p[1] - v;
    auto a = f.Sample({u , v });
    auto b = f.Sample({u + 1, v});
    auto c = f.Sample({u , v + 1});
    auto d = f.Sample({u + 1 , v + 1});
    return LinearInterpolate(
        LinearInterpolate(a, b , fu) ,
        LinearInterpolate(c, d , fu) , 
        fv
    );
}

void FluidSolver::Advection(){

    auto back_trace = [](Field<Vec2f> & vf , Vec2f pos , float dt) -> Vec2f{
        return pos -= BilinearInterpolate(vf , pos) * dt;
    };

    auto do_advect = [&]<class T>(Field<Vec2f> & vf , Field<T> & f , Field<T> & f_next){
        f.ForEach([&](T & val , Index2D id){
            //semi-lagurange
            auto pos = back_trace(vf , Vec2f{id.i ,id.j} + 0.5f , m_impl->time_stamp);
            f_next[id] = BilinearInterpolate(f , pos);
        });
    };

    do_advect(m_impl->velocity , m_impl->velocity , m_impl->velocity_next);
    do_advect(m_impl->velocity , m_impl->dye , m_impl->dye_next);
    
    m_impl->velocity.SwapWith(m_impl->velocity_next);
    m_impl->dye.SwapWith(m_impl->dye_next);
}

void FluidSolver::Reset(){
    // fill init values into fields
    m_impl->velocity.Fill({0,0});
    m_impl->dye.Fill({0,0,0});
    m_impl->color_buffer.Fill({0,0,0,255});
    m_impl->pressure.Fill(0.f);
}

void FluidSolver::ExternalForce(){
    auto f_strength_dt = 2000.f * m_impl->time_stamp;
    auto f_r = m_shape_x / 3.0f;
    auto inv_f_r = 1.0f / f_r;
    auto source = Vec2f{m_shape_x / 2.f , 0.f};
    auto f_g_dt = 9.8 * m_impl->time_stamp;
    
    m_impl->velocity.ForEach([&](Vec2f & v , const Index2D & index){
        auto d2 = (Vec2f{index.i , index.j} + 0.5 - source).square().sum();
        auto momentum = Vec2f{0 , 1} * f_strength_dt * std::exp(-d2 * inv_f_r) - f_g_dt;
        v += momentum;
    });
}

void FluidSolver::Projection(){
    //velocity divergence 
    m_impl->vel_divergence.ForEach([&](float & div , Index2D index){
        auto & [i , j] = index;
        auto vl = m_impl->velocity.Sample({i - 1 , j})[0];
        auto vr = m_impl->velocity.Sample({i + 1 , j})[0];
        auto vb = m_impl->velocity.Sample({i , j - 1})[1];
        auto vt = m_impl->velocity.Sample({i , j + 1})[1];
        auto vc = m_impl->velocity[index];
        // if(i == 0) vl = 0 ;
        // else if(i == m_shape_x - 1) vr = 0 ;
        // if(j == 0) vb = 0;
        // else if(j == m_shape_y - 1) vt = 0;
        if(i == 0) vl = -vc[0];
        else if(i == m_shape_x - 1) vr = -vc[0];
        if(j == 0) vb = -vc[1];
        else if(j == m_shape_y - 1) vt = -vc[1];
        div = (vr - vl + vt - vb) * 0.5 ;   // 1/(2 dx) = 0.5
    });

    //jacobian iteration 
    int times = 20;
    while(times--){
        //jacobian step , solve pressure
        m_impl->pressure.ForEach([&](float & _ , const Index2D & index){
            auto &[i , j] = index;
            auto pl = m_impl->pressure.Sample({i - 1 , j});
            auto pr = m_impl->pressure.Sample({i + 1 , j});
            auto pt = m_impl->pressure.Sample({i , j + 1});
            auto pb = m_impl->pressure.Sample({i , j - 1});
            auto vdiv = m_impl->vel_divergence[index];
            m_impl->pressure_next[index] = 0.25 * (pl + pr + pt + pb - vdiv); // dx = 1
        });
        m_impl->pressure_next.SwapWith(m_impl->pressure);
    }
}

void FluidSolver::UpdateVelocity(){
    m_impl->velocity.ForEach([&](Vec2f & v , const Index2D & index){
        auto & [i, j] = index;
        auto pl = m_impl->pressure.Sample({i - 1 , j});
        auto pr = m_impl->pressure.Sample({i + 1 , j});
        auto pb = m_impl->pressure.Sample({i , j - 1});
        auto pt = m_impl->pressure.Sample({i , j + 1});
        v -= 0.5 * Vec2f{pr - pl , pt - pb};
    });
}

void FluidSolver::UpdateDye(){
    float inv_dye_denom = 4.0f / std::pow(m_shape_x / 20.f , 2) ;
    auto source = Vec2f{m_shape_x / 2.f , 0};
    m_impl->dye.ForEach([&](Vec3f & d , const Index2D & index){
        auto d2 = (Vec2f{index.i , index.j} + 0.5f - source).square().sum();
        auto dc = (d + std::exp(-d2 * inv_dye_denom) * m_impl->dye_color) * m_impl->decay;
        d = dc.cwiseMin(m_impl->dye_color);
    });

    //update color buffer
    m_impl->color_buffer.ForEach([&](RGBA & col , Index2D index){
        index = {index.j , static_cast<int>(m_shape_y) - 1 - index.i};
        auto & fcol = m_impl->dye[index];
        constexpr auto tou8 = [](const float & f){
            return std::clamp<uint8_t>(std::abs(f) * 255, 0 , 255);
        };
        col = {tou8(fcol[0]) , tou8(fcol[1]) , tou8(fcol[2]) , 255};
    });
}