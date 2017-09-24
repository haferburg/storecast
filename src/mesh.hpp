#pragma once
#include "math.hpp"
#include <vector>

namespace storecast
{

struct vertex_data {
  vec3 Position;
  vec3 Normal;
  vec3 TextureCoords;
};
struct mesh {
  std::vector<vertex_data> Vertices;
  // I could have modeled the index buffer as vector<vector<i32> Indices>>, where Indices.size() is
  // the number of face vertices. It would have been easier to write code for, but it wastes a
  // lot of space, and has a lot worse memory locality, resulting in more cache misses. Quads
  // and triangles should be the main use case, especially for rendering.
  std::vector<i32> TriangleIndices;
  std::vector<i32> QuadIndices;
};

struct draw_command {
  enum class type {
    TRIANGLE,
    QUAD,
  } Type;
  // Index into mesh::TriangleIndices if Type==TRIANGLE, otherwise into mesh::QuadIndices.
  i32 StartIndex;
  i32 NumVertices;
};

std::vector<draw_command> get_draw_command_list(const mesh& Mesh);

std::ostream& operator <<(std::ostream& Out, const draw_command& Command);

} // namespace storecast
