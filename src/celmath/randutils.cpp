#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <numeric>

#include "randutils.h"

namespace celmath
{
namespace
{
std::mt19937 createRNG()
{
    std::random_device rd;
    std::uniform_int_distribution<std::uint_least32_t> dist{0, UINT32_C(0xffffffff)};
    constexpr std::size_t seedSize = std::mt19937::state_size * (std::mt19937::word_size / 32);
    std::array<std::uint_least32_t, seedSize> seedData;
    std::generate(seedData.begin(), seedData.end(), [&] { return dist(rd); });
    std::seed_seq rngSeed(seedData.begin(), seedData.end());
    return std::mt19937{ rngSeed };
}

// utility functions for Perlin noise
struct PerlinData
{
    static constexpr int TableSize = (1 << 8);
    static constexpr int TableMask = TableSize - 1;

    std::array<int, TableSize * 2> permutation{};
    std::array<float, TableSize> gradients1D{};
    std::array<Eigen::Vector2f, TableSize> gradients2D{};
    std::array<Eigen::Vector3f, TableSize> gradients3D{};

    PerlinData()
    {
        auto& rng = getRNG();
        auto permutationMid = permutation.begin() + TableSize;
        std::iota(permutation.begin(), permutationMid, 0);
        std::shuffle(permutation.begin(), permutationMid, rng);
        std::copy(permutation.begin(), permutationMid, permutationMid);

        std::generate(gradients1D.begin(), gradients1D.end(),
                      [&]() { return RealDists<float>::SignedUnit(rng); });

        std::generate(gradients2D.begin(), gradients2D.end(),
                      [&]() { return randomOnCircle<float>(rng); });

        std::generate(gradients3D.begin(), gradients3D.end(),
                      [&]() { return randomOnSphere<float>(rng); });
    }

    inline int permuteIndices(int x, int y) const
    {
        return permutation[permutation[x] + y];
    }

    inline int permuteIndices(int x, int y, int z) const
    {
        return permutation[permutation[permutation[x] + y] + z];
    }

    inline float gradientAt(int x) const
    {
        return gradients1D[permutation[x]];
    }

    inline const Eigen::Vector2f& gradientAt(int x, int y) const
    {
        return gradients2D[permutation[permutation[x] + y]];
    }

    inline const Eigen::Vector3f& gradientAt(int x, int y, int z) const
    {
        return gradients3D[permutation[permutation[permutation[x] + y] + z]];
    }
};

const PerlinData& getPerlinData()
{
    static PerlinData perlinData;
    return perlinData;
}

constexpr inline float smooth(float t)
{
    return t * t * (3.0f - 2.0f * t);
}

} // end of anonymous namespace

float noise(float arg)
{
    const auto& perlinData = getPerlinData();
    int x0 = static_cast<int>(std::floor(arg)) & PerlinData::TableMask;
    int x1 = (x0 + 1) & PerlinData::TableMask;
    float dx0 = arg - std::floor(arg);
    float dx1 = dx0 - 1.0f;
    float g0 = perlinData.gradientAt(x0);
    float g1 = perlinData.gradientAt(x1);
    return lerp(smooth(dx0), dx0 * g0, dx1 * g1);
}

float noise(const Eigen::Vector2f& arg)
{
    const auto& perlinData = getPerlinData();
    int x0[2] {static_cast<int>(std::floor(arg.x())) & PerlinData::TableMask,
               static_cast<int>(std::floor(arg.y())) & PerlinData::TableMask};
    int x1[2] {(x0[0] + 1) & PerlinData::TableMask,
               (x0[1] + 1) & PerlinData::TableMask};
    float dx0[2] {arg.x() - std::floor(arg.x()),
                  arg.y() - std::floor(arg.y())};
    float dx1[2] {dx0[0] - 1.0f, dx0[1] - 1.0f};
    const Eigen::Vector2f& g00 = perlinData.gradientAt(x0[0], x0[1]);
    const Eigen::Vector2f& g10 = perlinData.gradientAt(x1[0], x0[1]);
    const Eigen::Vector2f& g01 = perlinData.gradientAt(x0[0], x1[1]);
    const Eigen::Vector2f& g11 = perlinData.gradientAt(x1[0], x1[1]);
    Eigen::Vector2f v00{dx0[0], dx0[1]};
    Eigen::Vector2f v10{dx1[0], dx0[1]};
    Eigen::Vector2f v01{dx0[0], dx1[1]};
    Eigen::Vector2f v11{dx1[0], dx1[1]};
    float t[2] {smooth(dx0[0]), smooth(dx0[1])};
    float nx[2] {lerp(t[0], g00.dot(v00), g10.dot(v10)),
                 lerp(t[0], g01.dot(v01), g11.dot(v11))};
    return lerp(t[1], nx[0], nx[1]);
}

float noise(const Eigen::Vector3f& arg)
{
    const auto& perlinData = getPerlinData();
    int x0[3] {static_cast<int>(std::floor(arg.x())) & PerlinData::TableMask,
               static_cast<int>(std::floor(arg.y())) & PerlinData::TableMask,
               static_cast<int>(std::floor(arg.z())) & PerlinData::TableMask};
    int x1[3] {(x0[0] + 1) & PerlinData::TableMask,
               (x0[1] + 1) & PerlinData::TableMask,
               (x0[2] + 1) & PerlinData::TableMask};
    float dx0[3] {arg.x() - std::floor(arg.x()),
                  arg.y() - std::floor(arg.y()),
                  arg.z() - std::floor(arg.z())};
    float dx1[3] {dx0[0] - 1.0f, dx0[1] - 1.0f, dx0[2] - 1.0f};
    const Eigen::Vector3f& g000 = perlinData.gradientAt(x0[0], x0[1], x0[2]);
    const Eigen::Vector3f& g100 = perlinData.gradientAt(x1[0], x0[1], x0[2]);
    const Eigen::Vector3f& g010 = perlinData.gradientAt(x0[0], x1[1], x0[2]);
    const Eigen::Vector3f& g110 = perlinData.gradientAt(x1[0], x1[1], x0[2]);
    const Eigen::Vector3f& g001 = perlinData.gradientAt(x0[0], x0[1], x1[2]);
    const Eigen::Vector3f& g101 = perlinData.gradientAt(x1[0], x0[1], x1[2]);
    const Eigen::Vector3f& g011 = perlinData.gradientAt(x0[0], x1[1], x1[2]);
    const Eigen::Vector3f& g111 = perlinData.gradientAt(x1[0], x1[1], x1[2]);
    Eigen::Vector3f v000{dx0[0], dx0[1], dx0[2]};
    Eigen::Vector3f v100{dx1[0], dx0[1], dx0[2]};
    Eigen::Vector3f v010{dx0[0], dx1[1], dx0[2]};
    Eigen::Vector3f v110{dx1[0], dx1[1], dx0[2]};
    Eigen::Vector3f v001{dx0[0], dx0[1], dx1[2]};
    Eigen::Vector3f v101{dx1[0], dx0[1], dx1[2]};
    Eigen::Vector3f v011{dx0[0], dx1[1], dx1[2]};
    Eigen::Vector3f v111{dx1[0], dx1[1], dx1[2]};
    float t[3] {smooth(dx0[0]), smooth(dx0[1]), smooth(dx0[2])};
    float nx[4] {lerp(t[0], g000.dot(v000), g100.dot(v100)),
                 lerp(t[0], g010.dot(v010), g110.dot(v110)),
                 lerp(t[0], g001.dot(v001), g101.dot(v101)),
                 lerp(t[0], g011.dot(v011), g111.dot(v111))};
    float ny[2] {lerp(t[1], nx[0], nx[1]),
                 lerp(t[1], nx[2], nx[3])};
    return lerp(t[2], ny[0], ny[1]);
}

float turbulence(const Eigen::Vector2f& p, float freq)
{
    float t = 0.0f;
    while (freq >= 1.0f)
    {
        Eigen::Vector2f vec = freq * p;
        t += std::abs(noise(vec)) / freq;
        freq *= 0.5f;
    }

    return t;
}


float turbulence(const Eigen::Vector3f& p, float freq)
{
    float t = 0.0f;
    while (freq >= 1.0f)
    {
        Eigen::Vector3f vec = freq * p;
        t += std::abs(noise(vec)) / freq;
        freq *= 0.5f;
    }

    return t;
}


float fractalsum(const Eigen::Vector2f& p, float freq)
{
    float t = 0.0f;
    while (freq >= 1.0f)
    {
        Eigen::Vector2f vec = freq * p;
        t += noise(vec) / freq;
        freq *= 0.5f;
    }

    return t;
}


float fractalsum(const Eigen::Vector3f& p, float freq)
{
    float t = 0.0f;

    for (t = 0.0f; freq >= 1.0f; freq *= 0.5f)
    {
        Eigen::Vector3f vec = freq * p;
        t += noise(vec) / freq;
        freq *= 0.5f;
    }

    return t;
}

std::mt19937& getRNG()
{
    static std::mt19937 rng = createRNG();
    return rng;
}
} // end namespace celmath
