#pragma once

#include <memory>
#include <concepts>
#include <span>
#include <string>
#include "global.h"

class GUI{
public:
    explicit GUI(std::size_t height , std::size_t width);
    ~GUI() ;
    GUI(const GUI &) = delete;
    GUI& operator= (const GUI &) = delete;

    // ProcessMessage() return true if receive quit message
    bool ProcessMessage(bool);
    void UpdateFrameBuffer(std::span<const RGBA>);
    void Update();
    void SetWindowsTitle(std::string);
    
    template<std::invocable<> F>
    void Render(F && render_impl){
        RenderBegin();
        std::forward<F>(render_impl)();
        RenderEnd();
    }

private:
    void RenderBegin();
    void RenderEnd();
private:

    struct Impl;
    std::unique_ptr<Impl> m_impl;
};