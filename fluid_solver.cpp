#include "mats.hpp"
#include "fluid_solver.h"
#include <algorithm>
#include <cmath>


struct FluidSolver::Impl{
    //grids { pressure , velocity , dye , divergence , RGBA buffer }
    Field<float> pressure , pressure_next;
    Field<Vec2f> velocity , velocity_next;
    Field<Vec3f> dye , dye_next;
    Field<float> vel_divergence;
    Field<RGBA> color_buffer; //RGBA

    Vec3f dye_color;  
    float decay;        // dyeing color decay
    float time_stamp ;  // simulation time stamp;
    int jocobian_step;  // for jocobian iteration 
    float f_strength;   // source emittion force strength 
    Vec2f f_gravity ;   // gravity force
    Vec2f emit_source ; // smoke source

    FluidSolver::Impl(std::size_t shape_x, std::size_t shape_y) 
    : pressure(shape_x , shape_y) , pressure_next(shape_x,shape_y) 
    , velocity(shape_x, shape_y) , velocity_next(shape_x,shape_y) 
    , dye(shape_x,  shape_y ) , dye_next(shape_x , shape_y)
    , vel_divergence(shape_x, shape_y)
    , color_buffer(shape_y ,shape_x){}
};

FluidSolver::FluidSolver(std::size_t shape_x, std::size_t shape_y , const FluidConfig & config)
: m_shape_x(shape_x) , m_shape_y(shape_y) 
, m_impl(std::make_unique<Impl>(shape_x , shape_y)){
    Reset();
    SetColor(1,0,0);
    SetConfig(config);
}

FluidSolver::~FluidSolver() {}

void FluidSolver::SolveStep(){
    Advection();
    ExternalForce();
    Projection();
    UpdateVelocity();
    UpdateDye();
}

void FluidSolver::SetConfig(const FluidConfig & config){
    m_impl->decay = std::clamp(config.decay , 0.f , 1.f);
    m_impl->time_stamp = config.time_step;
    m_impl->jocobian_step = config.jacobian_step;
    m_impl->f_strength = 2000;
    m_impl->f_gravity = {config.gravity[0] , config.gravity[1]};
    m_impl->emit_source = {m_shape_x / 2 , 0};
}

void FluidSolver::SetColor(float r , float g , float b){
    m_impl->dye_color = {r , g, b};
}

std::span<const RGBA> FluidSolver::GetColors() const noexcept {
    return m_impl->color_buffer.Span();
}

template<class T>
auto LinearInterpolate(const T& a ,const T& b , float t) {
    return a + t * (b - a);
}

// Notes : For open boundary condition implementation
// sampler should use extropolate method 
// to compute material dirivertive at position where out of field boundary

template<class T>
T BilinearInterpolate(Field<T> & f , Vec2f pos) {
    pos -= 0.5f ;
    // int u = std::floor(pos[0]) , v = std::floor(pos[1]);
    int u = pos[0] , v = pos[1];
    float fu = pos[0] - u , fv = pos[1] - v;
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

enum RK_CLASS:int{ RK_1 , RK_2 , RK_3 };

template<RK_CLASS rk>
auto BackTrace(Field<Vec2f> & vf , Vec2f pos , float dt ) -> Vec2f{
    if constexpr (rk == RK_1) {
        pos -= BilinearInterpolate(vf , pos) * dt;
    }
    else if constexpr(rk == RK_2){
        auto mid = pos - 0.5 * dt * BilinearInterpolate(vf, pos);
        pos -= dt * BilinearInterpolate(vf , mid);
    }
    else {  // RK_3
        auto v1 = BilinearInterpolate(vf , pos);
        Vec2f p1 = pos - 0.5 * dt * v1 ;
        auto v2 = BilinearInterpolate(vf , p1);
        Vec2f p2 = pos - 0.75 * dt * v2;
        auto v3 = BilinearInterpolate(vf , p2);
        pos -= dt * ((2.f / 9) * v1 + (1.f / 3) * v2 + (4.f / 9) * v3);
    }
    return pos;
}

void FluidSolver::Advection(){

    auto do_advect = [&]<class T>(Field<Vec2f> & vf , Field<T> & f , Field<T> & f_next){
        f_next.ForEach([&](T & val , Index2D id){
            //semi-lagurange
            auto pos = BackTrace<RK_2>(vf , Vec2f{id.i , id.j} + 0.5f , m_impl->time_stamp); 
            val = BilinearInterpolate(f , pos);
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
    m_impl->color_buffer.Fill({});
    m_impl->pressure.Fill(0.f);
}

void FluidSolver::ExternalForce(){
    // handle smoke source 
    auto f_strength_dt = m_impl->f_strength * m_impl->time_stamp;
    // auto f_r = m_shape_x / 3.0f;
    // auto inv_f_r = 1.0f / f_r;
    auto f_g_dt = m_impl->f_gravity * m_impl->time_stamp;
    
    m_impl->velocity.ForEach([&](Vec2f & v , const Index2D & index){
        auto d2 = (Vec2f{index.i , index.j} + 0.5 - m_impl->emit_source).square().sum();
        // auto momentum = Vec2f{0 , 1} * f_strength_dt * std::exp(-d2 * inv_f_r) - f_g_dt;
        Vec2f momentum = f_g_dt;
        if(d2 < 400) momentum += Vec2f{0 , 1} * f_strength_dt ;
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
        if(i == 0) vl = 0 ;
        else if(i == m_shape_x - 1) vr = 0 ;
        if(j == 0) vb = 0;
        else if(j == m_shape_y - 1) vt = 0;
        div = (vr - vl + vt - vb) * 0.5 ;   // 1/(2 dx) = 0.5
    });
    //jacobian iteration 
    int times = m_impl->jocobian_step;
    while(times--){
        //jacobian step , solve pressure
        m_impl->pressure_next.ForEach([&](float & p , const Index2D & index){
            // neighborsum fast hack
            auto s = m_impl->pressure.NeighborSum(index);
            auto vdiv = m_impl->vel_divergence[index];
            p = 0.25 * (s - vdiv);
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
    m_impl->dye.ForEach([&](Vec3f & d , const Index2D & index){
        d *= m_impl->decay;
        auto d2 = (Vec2f{index.i , index.j} + 0.5f - m_impl->emit_source).square().sum();
        if(d2 < 400) d = m_impl->dye_color;
        // auto dc = (d + std::exp(-d2 * inv_dye_denom) * m_impl->dye_color )* m_impl->decay;
        // if(d2 < 400) d = dc.cwiseMin(m_impl->dye_color);
        // else d = dc.cwiseMin(1.f);
    });

    //update color buffer
    m_impl->color_buffer.ForEach([&](RGBA & col , Index2D index){
        index = {index.j , static_cast<int>(m_shape_y) - 1 - index.i};
        auto & fcol = m_impl->dye[index];
        constexpr auto tou8 = [](const float & f) constexpr{
            return std::clamp<uint8_t>(std::abs(f) * 255, 0 , 255);
        };
        col = {tou8(fcol[0]) , tou8(fcol[1]) , tou8(fcol[2]) , 255};
    });
}