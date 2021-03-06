#pragma once

#include <cassert>
#include <concepts>
#include <Eigen/Eigen>
#include <span>
#include <algorithm>

using Vec2f = Eigen::Array2f;
using Vec3f = Eigen::Array3f;
using Vec3u8= Eigen::Array<uint8_t , 3 , 1>;
using Vec4u8= Eigen::Array<uint8_t , 4 , 1>;

static_assert(sizeof(Vec2f) == sizeof(float) * 2);

struct Index2D{
    int i , j ;
};

template<class V>
requires (std::is_standard_layout_v<V>)
class Field{
public:
    explicit Field(std::size_t shape_x , std::size_t shape_y)
    :m_shape_x(shape_x) , m_shape_y(shape_y) 
    ,m_maxi(shape_x - 1) , m_maxj(shape_y - 1) 
    ,m_data(shape_x * shape_y){}

    std::size_t XSize() const noexcept {return m_shape_x;}
    std::size_t YSize() const noexcept {return m_shape_y;}

    template<std::invocable<V & , Index2D> F>
    void ForEach(F && f) noexcept(noexcept(std::forward<F>(f)(m_data[0] , {0,0}))) {
        // coord (i, j) should be signed integer
        #pragma omp parallel for schedule (dynamic , 8)
        for(int i = 0; i < m_shape_x ; ++i) { 
            auto index = Index2D{i, 0};
            for(int pos = i * m_shape_y; index.j < m_shape_y ; ++index.j , ++pos) {
                std::forward<F>(f)(m_data[pos] , index);
            }
        }
    }

    V & operator[] (const Index2D & index) noexcept{
        auto pos = index.i * m_shape_y + index.j ;
        assert(pos < m_data.size());
        return m_data[pos];
    }

    std::span<const V> Span() const noexcept {
        return std::span{m_data.data() , m_data.size()};
    }

    // clamp sample , no extrapolate
    V Sample(Index2D index) noexcept {
        index.i = std::clamp<int>(index.i , 0 , m_maxi );
        index.j = std::clamp<int>(index.j , 0 , m_maxj );
        return this->operator[](index);
    }

    //requires V += V{}
    V NeighborSum(const Index2D & index) noexcept requires requires (V & v){ v += std::declval<V>(); }{
        auto pos = index.i * m_shape_y + index.j ;
        assert(pos < m_data.size());
        auto mid = m_data[pos];
        auto ans = V{};
        ans += index.i > 0 ? m_data[pos - m_shape_y] : mid;
        ans += index.i < m_maxi ? m_data[pos + m_shape_y]: mid;
        ans += index.j > 0 ? m_data[pos - 1] : mid;
        ans += index.j < m_maxj ? m_data[pos + 1] : mid;
        return ans;
    }

    void Fill(const V & val) {
        std::fill(m_data.begin() , m_data.end() , val);
    }

    void SwapWith(Field & f) noexcept{
        std::swap(m_shape_x , f.m_shape_x);
        std::swap(m_shape_y , f.m_shape_y);
        std::swap(m_data, f.m_data);
    }
private:
    std::size_t m_shape_x;
    std::size_t m_shape_y;
    int m_maxi;
    int m_maxj;
    std::vector<V> m_data{};
};

