#include "mesh.hpp"

#include <vector>
#include <string>

namespace storecast
{
using std::vector;

vector<draw_command> get_draw_command_list(const mesh& Mesh)
{
  vector<draw_command> Result;
  Result.reserve(Mesh.TriangleIndices.size() + Mesh.QuadIndices.size());
  for (auto IndexOffset = 0; IndexOffset < Mesh.TriangleIndices.size(); IndexOffset += 3) {
    Result.push_back({draw_command::type::TRIANGLE, IndexOffset, 3});
  }
  for (auto IndexOffset = 0; IndexOffset < Mesh.QuadIndices.size(); IndexOffset += 4) {
    Result.push_back({draw_command::type::QUAD, IndexOffset, 4});
  }
  return Result;
}

std::ostream& operator <<(std::ostream& Out, const draw_command& Command)
{
  if (Command.Type == draw_command::type::TRIANGLE) {
    Out << "TRIANGLE " << Command.StartIndex << " " << Command.NumVertices;
  } else if (Command.Type == draw_command::type::QUAD) {
    Out << "QUAD     " << Command.StartIndex << " " << Command.NumVertices;
  }
  return Out;
}

} // namespace storecast
