#pragma once

#include <memory>
#include <concepts>

struct RGBA{ uint8_t r , g , b , a; };

class GUI{
public:
    explicit GUI(std::size_t height , std::size_t width);
    ~GUI() ;

    bool ProcessMessage();
    void UpdateFrameBuffer(RGBA * );
    void Update();
    
    template<std::invocable<> F>
    void Render(F && render_impl){
        RenderBegin();
        render_impl();
        RenderEnd();
    }

private:
    void RenderBegin();
    void RenderEnd();
private:

    struct Impl;
    
    std::unique_ptr<Impl> m_impl;
};