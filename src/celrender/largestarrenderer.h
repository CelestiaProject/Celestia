#pragma once

#include <memory>
#include <Eigen/Core>

class Color;
class Renderer;
struct Matrices;

namespace celestia::gl
{
class Buffer;
class VertexObject;
} // namespace celestia::gl

class Buffer;
class VertexObject;
namespace celestia::render

{
class LargeStarRenderer
{
public:
    explicit LargeStarRenderer(Renderer &renderer);
    ~LargeStarRenderer();

    void render(const Eigen::Vector3f &position, const Color &color, float size, const Matrices &mvp);

private:
    void initialize();
    bool m_initialized{ false };

    Renderer &m_renderer;

    std::unique_ptr<gl::Buffer> m_bo;
    std::unique_ptr<gl::VertexObject> m_vo;
};
} // namespace celestia::render
