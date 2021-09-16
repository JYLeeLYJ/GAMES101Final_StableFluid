#pragma once

#include <cassert>
#include <concepts>
#include <Eigen/Eigen>

using Vec2f = Eigen::Vector2f;
using Vec3f = Eigen::Vector3f;
using Vec3u8= Eigen::Matrix<uint8_t , 3 , 1>;
using Vec4u8= Eigen::Matrix<uint8_t , 4 , 1>;

struct Index2D{
    std::size_t x , y ;
};

template<class V>
requires (std::is_standard_layout_v<V>)
class Field{
public:
    const std::size_t row;
    const std::size_t col;

    explicit Field(std::size_t row , std::size_t col)
    :row(row) , col(col) , m_data(row * col){}

    template<std::invocable<V & , int , int> F>
    void ForEach(F && f) noexcept(noexcept(std::forward<F>(f)(m_data[0] , 0 , 0))) {
        for(std::size_t i = 0; i < row ; ++i) {
            for(std::size_t j = 0 , pos = i * col; j < col ; ++j , ++pos) {
                std::forward<F>(f)(m_data[pos] , i , j);
            }
        }
    }

    V & operator[] (const Index2D & index) noexcept{
        auto pos = index.x * col + index.y ;
        assert(pos < m_data.size());
        return m_data[pos];
    }

    const std::byte * data() const noexcept {
        return reinterpret_cast<std::byte*>(m_data.data());
    }

private:
    std::vector<V> m_data{};
};