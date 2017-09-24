#include "obj_import.hpp"

#include <algorithm>
#include <tuple>
#include <numeric>
#include <string>
#include <sstream>

// #include
#include "mesh.hpp"
#include "defines.hpp"

namespace storecast
{
using std::vector;
using std::string;
using std::istream;
using std::stringstream;

namespace {
i32 parse_int(const string& S) {
  return S.size() > 0 ? std::stoi(S) : 0;
}
bool starts_with(const string& S, const string& Prefix)
{
  return !S.compare(0, Prefix.size(), Prefix);
}
} // anonymous namespace

mesh convert_to_mesh(const obj_file_data& Obj)
{
  mesh Result;
  if (Obj.v.empty()) {
    return Result;
  }

  i32 NumTriangles = (i32)std::count_if(Obj.f.begin(), Obj.f.end(), [](auto f){return f.NumVertices==3;});
  i32 NumQuads = (i32)std::count_if(Obj.f.begin(), Obj.f.end(), [](auto f){return f.NumVertices==4;});

  typedef std::tuple<i32, i32, i32> vert_indices;
  vector<vert_indices> VertexIndices;
  VertexIndices.reserve(3 * NumTriangles + 4 * NumQuads);

  for (auto& f: Obj.f) {
    auto Stride = 1 + (f.HasVt ? 1 : 0) + (f.HasVn ? 1 : 0);
    auto UVOffset = 1;
    auto NormalOffset = 1 + (f.HasVt ? 1 : 0);
    for (auto I = 0; I < f.NumVertices; ++I) {
      auto VertexIndex = f.Indices[Stride * I];
      auto UVIndex = f.HasVt ? f.Indices[Stride * I + UVOffset] : 0;
      auto NormalIndex = f.HasVn ? f.Indices[Stride * I + NormalOffset] : 0;
      VertexIndices.push_back(std::make_tuple(VertexIndex, UVIndex, NormalIndex));
    }
  }

  // Sort indices to find out which indices are unique.
  vector<i32> Sorting(VertexIndices.size());
  vector<i32> Replacement(VertexIndices.size());
  {
    std::iota(begin(Sorting), end(Sorting), static_cast<i32>(0));
    std::sort(begin(Sorting), end(Sorting),
        [&](auto L, auto R) { return VertexIndices[L] < VertexIndices[R]; });
  }
  {
    auto It1 = Sorting.begin();
    auto It2 = std::next(Sorting.begin());
    auto ItEnd = Sorting.end();
    Replacement[0] = 0;
    for (; It2 != ItEnd; ++It1, ++It2) {
      if (VertexIndices[*It1] == VertexIndices[*It2]) {
        Replacement[*It2] = *It1;
      } else {
        Replacement[*It2] = *It2;
      }
    }
  }

  i32 NumRequiredVertexIndices = 0;
  auto& NumReplacedUpTo = Sorting; // "Rename" variable.
  for (i32 I = 0, NumReplacedSoFar = 0; I < Replacement.size(); ++I) {
    if (I == Replacement[I]) {
      ++NumRequiredVertexIndices;
    } else {
      ++NumReplacedSoFar;
    }
    NumReplacedUpTo[I] = NumReplacedSoFar;
  }

  // Ex.: Replacement[I] is
  //   0 1 2 2 1 5 6 7 8 8 7 11 12
  // The indices 3,4,9,10 were replaced, so for the final index we would like to find a tight
  // packing with indices 0..8. NumReplacedUpTo[I] is
  //   0 0 0 1 2 2 2 2 2 3 4  4  4
  // That means NumReplacedUpTo[Replacement[I]] is
  //   0 0 0 0 0 2 2 2 2 2 2  4  4
  // and the diff Replacement[I]-NumReplacedUpTo[Replacement[I]] is:
  //   0 1 2 2 1 3 4 5 6 6 5  7  8

  Result.Vertices.resize(NumRequiredVertexIndices);
  Result.TriangleIndices.reserve(3*NumTriangles);
  Result.QuadIndices.reserve(4*NumQuads);
  i32 Index = 0;
  for (auto& f: Obj.f) {
    auto Stride = 1 + (f.HasVt ? 1 : 0) + (f.HasVn ? 1 : 0);
    auto UVOffset = 1;
    auto NormalOffset = 1 + (f.HasVt ? 1 : 0);
    for (auto I = 0; I < f.NumVertices; ++I) {
      auto VertexIndex = f.Indices[Stride * I];
      auto UVIndex = f.HasVt ? f.Indices[Stride * I + UVOffset] : 0;
      auto NormalIndex = f.HasVn ? f.Indices[Stride * I + NormalOffset] : 0;
      auto FinalIndex = Replacement[Index] - NumReplacedUpTo[Replacement[Index]];
      if (Replacement[Index] == Index) {
        auto& Vertex = Result.Vertices[FinalIndex];
        Vertex.Position = Obj.v[VertexIndex - 1];
        if (f.HasVn) {
          Vertex.Normal = Obj.vn[NormalIndex - 1];
        } else {
          Vertex.Normal = vec3{0.f, 0.f, 0.f};
        }
        if (f.HasVt) {
          Vertex.TextureCoords = Obj.vt[UVIndex - 1];
        } else {
          Vertex.TextureCoords = vec3{0.f, 0.f, 0.f};
        }
      }
      if (f.NumVertices == 3) {
        Result.TriangleIndices.push_back(FinalIndex);
      } else if (f.NumVertices == 4) {
        Result.QuadIndices.push_back(FinalIndex);
      }
      ++Index;
    }
  }

  return Result;
}

obj_file_data parse_obj(istream& In)
{
  obj_file_data Data;
  for (string Line; getline(In, Line); ) {
    if (starts_with(Line, "#") || Line.empty()) {
      continue;
    } else if (starts_with(Line, "v ")) {
      stringstream LineStream(Line);
      vec3 Value;
      string Token;
      getline(LineStream, Token, ' ');
      LineStream >> Value.X;
      LineStream >> Value.Y;
      LineStream >> Value.Z;
      // Ignore any values after the third
      Data.v.push_back(Value);
    } else if (starts_with(Line, "vt ")) {
      stringstream LineStream(Line);
      vec3 Value;
      string Token;
      getline(LineStream, Token, ' ');
      LineStream >> Value.X;
      LineStream >> Value.Y;
      if (!(LineStream >> Value.Z)) {
        Value.Z = 1.0f;
      }
      // Ignore any values after the third
      Data.vt.push_back(Value);
    } else if (starts_with(Line, "vn ")) {
      stringstream LineStream(Line);
      vec3 Value;
      string Token;
      getline(LineStream, Token, ' ');
      LineStream >> Value.X;
      LineStream >> Value.Y;
      LineStream >> Value.Z;
      // Ignore any values after the third
      Data.vn.push_back(Value);
    } else if (starts_with(Line, "f ")) {
      stringstream LineStream(Line);
      obj_face_data Value = {0};
      string VertexToken;
      getline(LineStream, VertexToken, ' ');
      i32 NumIndexTokens = 1;
      while (getline(LineStream, VertexToken, ' ')) {
        stringstream VertexStream(VertexToken);
        string IndexToken;
        if (bool ThisIsTheFirstVertex = Value.NumVertices == 0) {
          // Parse the first vertex in order to determine how many entries we have per vertex,
          // and what they're going to mean.
          // The spec says:
          // > When you are using a series of triplets, you must be consistent in the
          // > way you reference the vertex data. For example, it is illegal to give
          // > vertex normals for some vertices, but not all.
          // >
          // > The following is an example of an illegal statement.
          // >
          // >     f 1/1/1 2/2/2 3//3 4//4

          getline(VertexStream, IndexToken, '/');
          Value.Indices.push_back(parse_int(IndexToken));

          if (getline(VertexStream, IndexToken, '/')) {
            i32 Index = parse_int(IndexToken);
            Value.HasVt = Index > 0;
            if (Value.HasVt) {
              Value.Indices.push_back(Index);
            }

            if (getline(VertexStream, IndexToken, '/')) {
              Index = parse_int(IndexToken);
              Value.HasVn = Index > 0;
              if (Value.HasVn) {
                Value.Indices.push_back(Index);
              }
            }
          }
          NumIndexTokens = 1 + (Value.HasVt || Value.HasVn ? 1 : 0) + (Value.HasVn ? 1 : 0);
        } else {
          for (i32 I = 0; I < NumIndexTokens; ++I) {
            getline(VertexStream, IndexToken, '/');
            if (IndexToken.size()) {
              Value.Indices.push_back(std::stoi(IndexToken));
            }
          }
        }
        ++Value.NumVertices;
      }

      // "For this assignment, we just ask you to ignore all polygons that are not a triangle
      // or a quad."
      if (3 <= Value.NumVertices && Value.NumVertices <= 4) {
        Data.f.push_back(Value);
      }
    }
  }
  return Data;
}

} // namespace storecast
