#include <string>
#include <iostream>
#include <fstream>
#include <functional>
#include <algorithm>
#include <vector>
#include <sstream>
#include <numeric>

using std::string;
using std::ifstream;
using std::istream;
using std::cout;
using std::endl;
using std::vector;
using std::stringstream;

typedef int32_t i32;
typedef float f32;

// IRL we might use boost::starts_with
bool starts_with(const string& S, const string& Prefix)
{
  return !S.compare(0, Prefix.size(), Prefix);
}

struct vec3 {
  union {
    struct {
      f32 X;
      f32 Y;
      f32 Z;
    };
    f32 Data[3];
  };
};
struct vertex_data {
  vec3 Position;
  vec3 Normal;
  vec3 TextureCoords;
};
struct mesh {
  vector<vertex_data> Vertex;
  vector<i32> Quad;
  vector<i32> Triangle;
};

struct obj_face_data {
  i32 NumVertices;
  bool HasVt;
  bool HasVn;
  vector<i32> Index;
};
struct obj_file_data {
  vector<vec3> v;
  vector<vec3> vt;
  vector<vec3> vn;
  vector<obj_face_data> f;
};
#define for3(I) for(auto I=0; I<3; ++I)
#define for4(I) for(auto I=0; I<4; ++I)

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
      auto VertexIndex = f.Index[Stride * I];
      auto UVIndex = f.HasVt ? f.Index[Stride * I + UVOffset] : 0;
      auto NormalIndex = f.HasVn ? f.Index[Stride * I + NormalOffset] : 0;
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

  Result.Vertex.resize(NumRequiredVertexIndices);
  Result.Triangle.reserve(3*NumTriangles);
  Result.Quad.reserve(4*NumQuads);
  i32 Index = 0;
  for (auto& f: Obj.f) {
    auto Stride = 1 + (f.HasVt ? 1 : 0) + (f.HasVn ? 1 : 0);
    auto UVOffset = 1;
    auto NormalOffset = 1 + (f.HasVt ? 1 : 0);
    for (auto I = 0; I < f.NumVertices; ++I) {
      auto VertexIndex = f.Index[Stride * I];
      auto UVIndex = f.HasVt ? f.Index[Stride * I + UVOffset] : 0;
      auto NormalIndex = f.HasVn ? f.Index[Stride * I + NormalOffset] : 0;
      auto FinalIndex = Replacement[Index] - NumReplacedUpTo[Replacement[Index]];
      if (Replacement[Index] == Index) {
        auto& Vertex = Result.Vertex[FinalIndex];
        Vertex.Position = Obj.v[VertexIndex - 1];
        Vertex.Normal = Obj.vn[NormalIndex - 1];
        Vertex.TextureCoords = Obj.vt[UVIndex - 1];
      }
      if (f.NumVertices == 3) {
        Result.Triangle.push_back(FinalIndex);
      } else if (f.NumVertices == 4) {
        Result.Quad.push_back(FinalIndex);
      }
      ++Index;
    }
  }

  return Result;
}

namespace {
i32 parse_int(const string& S) {
  return S.size() > 0 ? std::stoi(S) : 0;
}
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
          Value.Index.push_back(parse_int(IndexToken));

          if (getline(VertexStream, IndexToken, '/')) {
            i32 Index = parse_int(IndexToken);
            Value.HasVt = Index > 0;
            if (Value.HasVt) {
              Value.Index.push_back(Index);
            }

            if (getline(VertexStream, IndexToken, '/')) {
              Index = parse_int(IndexToken);
              Value.HasVn = Index > 0;
              if (Value.HasVn) {
                Value.Index.push_back(Index);
              }
            }
          }
          NumIndexTokens = 1 + (Value.HasVt || Value.HasVn ? 1 : 0) + (Value.HasVn ? 1 : 0);
        } else {
          for (i32 I = 0; I < NumIndexTokens; ++I) {
            getline(VertexStream, IndexToken, '/');
            if (IndexToken.size()) {
              Value.Index.push_back(std::stoi(IndexToken));
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

// Compare floats by converting to int. We want to make sure two floats have the same bit
// pattern.
template <class left_type, class right_type> bool equals(left_type Left, right_type Right) {
  static_assert(!std::is_floating_point<left_type>::value,
      "Left must not be a floating point type!");
  static_assert(!std::is_floating_point<right_type>::value,
      "Right must not be a floating point type!");
  return Left == Right;
}
template <> bool equals(f32 Left, f32 Right)
{
  return static_cast<i32>(Left) == static_cast<i32>(Right);
}
#define ASSERT_EQ(Value, Expected) {auto V=(Value); if (!equals(V, Expected)) { cout << #Value " is not " << Expected << " but " << V << endl; return false; }}

i32 read_entire_file(ifstream& In)
{
  i32 NumLines = 0;
  for (string Line; getline(In, Line); ) {
    ++NumLines;
  }
  return NumLines;
}

namespace {
const string CubeFilePath = "../data/cube.obj";
const string DuckyFilePath = "../data/ducky.obj";
const vector<vec3> CubeVertices {
  {-0.500000, -0.500000, 0.500000},
  {0.500000, -0.500000, 0.500000},
  {-0.500000, 0.500000, 0.500000},
  {0.500000, 0.500000, 0.500000},
  {-0.500000, 0.500000, -0.500000},
  {0.500000, 0.500000, -0.500000},
  {-0.500000, -0.500000, -0.500000},
  {0.500000, -0.500000, -0.500000},
};
const vector<vec3> CubeUVs = {
  {0.000000, 0.000000},
  {1.000000, 0.000000},
  {0.000000, 1.000000},
  {1.000000, 1.000000},
};
const vector<vec3> CubeNormals = {
  {0.000000, 0.000000, 1.000000},
  {0.000000, 1.000000, 0.000000},
  {0.000000, 0.000000, -1.000000},
  {0.000000, -1.000000, 0.000000},
  {1.000000, 0.000000, 0.000000},
  {-1.000000, 0.000000, 0.000000},
};

const obj_file_data CubeObj = {
  CubeVertices, CubeUVs, CubeNormals,
  {
    {3, true, true, {1,1,1, 2,2,1, 3,3,1}},
    {3, true, true, {3,3,1, 2,2,1, 4,4,1}},
    {3, true, true, {3,1,2, 4,2,2, 5,3,2}},
    {3, true, true, {5,3,2, 4,2,2, 6,4,2}},
    {3, true, true, {5,4,3, 6,3,3, 7,2,3}},
    {3, true, true, {7,2,3, 6,3,3, 8,1,3}},
    {3, true, true, {7,1,4, 8,2,4, 1,3,4}},
    {3, true, true, {1,3,4, 8,2,4, 2,4,4}},
    {3, true, true, {2,1,5, 8,2,5, 4,3,5}},
    {3, true, true, {4,3,5, 8,2,5, 6,4,5}},
    {3, true, true, {7,1,6, 1,2,6, 5,3,6}},
    {3, true, true, {5,3,6, 1,2,6, 3,4,6}},
  }
};
const obj_file_data QuadCubeObj = {
  CubeVertices, CubeUVs, CubeNormals,
  {
    {4, true, true, {1,1,1, 2,2,1, 3,3,1, 4,4,1}},
    {4, true, true, {3,1,2, 4,2,2, 5,3,2, 6,4,2}},
    {4, true, true, {5,4,3, 6,3,3, 7,2,3, 8,1,3}},
    {4, true, true, {7,1,4, 8,2,4, 1,3,4, 2,4,4}},
    {4, true, true, {2,1,5, 8,2,5, 4,3,5, 6,4,5}},
    {4, true, true, {7,1,6, 1,2,6, 5,3,6, 3,4,6}},
  }
};
}

bool test_convert_to_mesh_wont_crash_on_empty_input()
{
  mesh Mesh = convert_to_mesh(obj_file_data());
  return true;
}

bool test_convert_cube_to_mesh_positions()
{
  auto& Obj = CubeObj;
  mesh Mesh = convert_to_mesh(Obj);
  ASSERT_EQ(Mesh.Triangle.size(), 3*Obj.f.size());
  for (i32 TriIndex = 0; TriIndex < Obj.f.size(); ++TriIndex) {
    for3(I) {
      auto MeshTriangleIndex = 3*TriIndex + I;
      ASSERT_EQ(0<=MeshTriangleIndex && MeshTriangleIndex<Mesh.Triangle.size(), true);
      auto& MeshVertexIndex = Mesh.Triangle[MeshTriangleIndex];
      auto& MeshVertexPosition = Mesh.Vertex[MeshVertexIndex].Position;
      auto& ObjVertexIndex = Obj.f[TriIndex].Index[3*I+0];
      auto& ObjVertex = Obj.v[ObjVertexIndex-1];
      for3(J) {
        ASSERT_EQ(MeshVertexPosition.Data[J], ObjVertex.Data[J]);
      }
    }
  }
  return true;
}

bool test_convert_quad_cube_to_mesh()
{
  auto& Obj = QuadCubeObj;
  mesh Mesh = convert_to_mesh(Obj);
  ASSERT_EQ(Mesh.Triangle.size(), 0);
  ASSERT_EQ(Mesh.Quad.size(), 4*Obj.f.size());
  for (i32 QuadIndex = 0; QuadIndex < Obj.f.size(); ++QuadIndex) {
    for4(I) {
      auto MeshQuadIndex = 4*QuadIndex + I;
      ASSERT_EQ(0<=MeshQuadIndex && MeshQuadIndex<Mesh.Quad.size(), true);
      auto& MeshVertexIndex = Mesh.Quad[MeshQuadIndex];
      auto& MeshVertexPosition = Mesh.Vertex[MeshVertexIndex].Position;
      auto& MeshNormal = Mesh.Vertex[MeshVertexIndex].Normal;
      auto& ObjVertexIndex = Obj.f[QuadIndex].Index[3*I+0];
      auto& ObjVertex = Obj.v[ObjVertexIndex-1];
      auto& ObjNormalIndex = Obj.f[QuadIndex].Index[3*I+2];
      auto& ObjNormal = Obj.vn[ObjNormalIndex-1];
      for3(J) {
        ASSERT_EQ(MeshVertexPosition.Data[J], ObjVertex.Data[J]);
        ASSERT_EQ(MeshNormal.Data[J], ObjNormal.Data[J]);
      }
    }
  }
  return true;
}

bool test_convert_cube_to_mesh_normals()
{
  auto& Obj = CubeObj;
  mesh Mesh = convert_to_mesh(Obj);
  ASSERT_EQ(Mesh.Triangle.size(), 3*Obj.f.size());
  for (i32 TriIndex = 0; TriIndex < Obj.f.size(); ++TriIndex) {
    for3(I) {
      auto MeshTriangleIndex = 3*TriIndex + I;
      ASSERT_EQ(0<=MeshTriangleIndex && MeshTriangleIndex<Mesh.Triangle.size(), true);
      auto& MeshVertexIndex = Mesh.Triangle[MeshTriangleIndex];
      auto& MeshNormal = Mesh.Vertex[MeshVertexIndex].Normal;
      auto& ObjNormalIndex = Obj.f[TriIndex].Index[3*I+2];
      auto& ObjNormal = Obj.vn[ObjNormalIndex-1];
      for3(J) {
        ASSERT_EQ(MeshNormal.Data[J], ObjNormal.Data[J]);
      }
    }
  }
  return true;
}

bool test_convert_cube_to_mesh_uvs()
{
  auto& Obj = CubeObj;
  mesh Mesh = convert_to_mesh(Obj);
  ASSERT_EQ(Mesh.Triangle.size(), 3*Obj.f.size());
  for (i32 TriIndex = 0; TriIndex < Obj.f.size(); ++TriIndex) {
    for3(I) {
      auto MeshTriangleIndex = 3*TriIndex + I;
      ASSERT_EQ(0<=MeshTriangleIndex && MeshTriangleIndex<Mesh.Triangle.size(), true);
      auto& MeshVertexIndex = Mesh.Triangle[MeshTriangleIndex];
      auto& MeshUV = Mesh.Vertex[MeshVertexIndex].TextureCoords;
      auto& ObjUVIndex = Obj.f[TriIndex].Index[3*I+1];
      auto& ObjUV = Obj.vt[ObjUVIndex-1];
      for3(J) {
        ASSERT_EQ(ObjUV.Data[J], MeshUV.Data[J]);
      }
    }
  }
  return true;
}

bool test_parse_cube_vertices()
{
  ifstream File(CubeFilePath);
  obj_file_data Data = parse_obj(File);
  ASSERT_EQ(Data.v.size(), 8);
  ASSERT_EQ(Data.v[3].X, 0.5f);
  ASSERT_EQ(Data.v[7].Z, -0.5f);
  return true;
}

bool test_parse_cube_texture_coords()
{
  ifstream File(CubeFilePath);
  obj_file_data Data = parse_obj(File);
  ASSERT_EQ(Data.vt.size(), 4);
  ASSERT_EQ(Data.vt[0].X, 0.f);
  ASSERT_EQ(Data.vt[0].Y, 0.f);
  ASSERT_EQ(Data.vt[0].Z, 1.f);
  ASSERT_EQ(Data.vt[3].X, 1.f);
  ASSERT_EQ(Data.vt[3].Y, 1.f);
  ASSERT_EQ(Data.vt[3].Z, 1.f);
  return true;
}

bool test_parse_cube_normals()
{
  ifstream File(CubeFilePath);
  obj_file_data Data = parse_obj(File);
  ASSERT_EQ(Data.vn.size(), 6);
  ASSERT_EQ(Data.vn[0].X, 0.f);
  ASSERT_EQ(Data.vn[0].Y, 0.f);
  ASSERT_EQ(Data.vn[0].Z, 1.f);
  ASSERT_EQ(Data.vn[5].X, -1.f);
  ASSERT_EQ(Data.vn[5].Y, 0.f);
  ASSERT_EQ(Data.vn[5].Z, 0.f);
  return true;
}

bool test_parse_cube_faces()
{
  ifstream File(CubeFilePath);
  obj_file_data Data = parse_obj(File);
  ASSERT_EQ(Data.f.size(), 12);
  for(auto& f: Data.f) {
    ASSERT_EQ(f.HasVt, true);
    ASSERT_EQ(f.HasVn, true);
    ASSERT_EQ(f.NumVertices, 3);
    ASSERT_EQ(f.Index.size(), 3*f.NumVertices);
  }

  // f 1/1/1 2/2/1 3/3/1
  ASSERT_EQ(Data.f[0].Index[0], 1);
  ASSERT_EQ(Data.f[0].Index[1], 1);
  ASSERT_EQ(Data.f[0].Index[2], 1);
  ASSERT_EQ(Data.f[0].Index[3], 2);
  ASSERT_EQ(Data.f[0].Index[4], 2);
  ASSERT_EQ(Data.f[0].Index[5], 1);
  ASSERT_EQ(Data.f[0].Index[6], 3);
  ASSERT_EQ(Data.f[0].Index[7], 3);
  ASSERT_EQ(Data.f[0].Index[8], 1);

  // f 2/1/5 8/2/5 4/3/5
  ASSERT_EQ(Data.f[8].Index[0], 2);
  ASSERT_EQ(Data.f[8].Index[1], 1);
  ASSERT_EQ(Data.f[8].Index[2], 5);
  ASSERT_EQ(Data.f[8].Index[3], 8);
  ASSERT_EQ(Data.f[8].Index[4], 2);
  ASSERT_EQ(Data.f[8].Index[5], 5);
  ASSERT_EQ(Data.f[8].Index[6], 4);
  ASSERT_EQ(Data.f[8].Index[7], 3);
  ASSERT_EQ(Data.f[8].Index[8], 5);

  return true;
}

bool test_parse_ducky_faces()
{
  ifstream File(DuckyFilePath);
  obj_file_data Data = parse_obj(File);
  ASSERT_EQ(Data.f.size(), 7064);
  for(auto& f: Data.f) {
    ASSERT_EQ(f.HasVt, true);
    ASSERT_EQ(f.HasVn, false);
    ASSERT_EQ(f.NumVertices, 4);
    ASSERT_EQ(f.Index.size(), 2*f.NumVertices);
  }
  return true;
}

bool test_parse_faces_with_only_vertices()
{
  string Contents =
      "f 1 2 3\n"
      "f 4 5 6\n"
      "f 7 8 9\n"
      "f 7 8 9\n";
  stringstream File(Contents);
  obj_file_data Data = parse_obj(File);
  ASSERT_EQ(Data.f.size(), 4);
  for(auto& f: Data.f) {
    ASSERT_EQ(f.HasVt, false);
    ASSERT_EQ(f.HasVn, false);
    ASSERT_EQ(f.NumVertices, 3);
    ASSERT_EQ(f.Index.size(), f.NumVertices);
  }
  return true;
}

bool test_parse_faces_with_vertices_and_tex_coords()
{
  string Contents =
      "f 1/1 2/2 3/4\n"
      "f 4/1 5/2 6/7\n"
      "f 7/1 8/2 9/7\n"
      "f 7/1 8/2 9/7\n";
  stringstream File(Contents);
  obj_file_data Data = parse_obj(File);
  ASSERT_EQ(Data.f.size(), 4);
  for(auto& f: Data.f) {
    ASSERT_EQ(f.HasVt, true);
    ASSERT_EQ(f.HasVn, false);
    ASSERT_EQ(f.NumVertices, 3);
    ASSERT_EQ(f.Index.size(), 2*f.NumVertices);
  }
  return true;
}

bool test_parse_faces_with_vertices_and_normals()
{
  string Contents =
      "f 1//1 2//2 3//4\n"
      "f 4//1 5//2 6//7\n"
      "f 7//1 8//2 9//7\n"
      "f 7//1 8//2 9//7\n";
  stringstream File(Contents);
  obj_file_data Data = parse_obj(File);
  ASSERT_EQ(Data.f.size(), 4);
  for(auto& f: Data.f) {
    ASSERT_EQ(f.HasVt, false);
    ASSERT_EQ(f.HasVn, true);
    ASSERT_EQ(f.NumVertices, 3);
    ASSERT_EQ(f.Index.size(), 2*f.NumVertices);
  }
  return true;
}

bool test_open_cube_file()
{
  ifstream File(CubeFilePath);
  i32 NumLines = read_entire_file(File);
  ASSERT_EQ(NumLines, 47);
  return true;
}

bool test_open_ducky_file()
{
  ifstream File(DuckyFilePath);
  i32 NumLines = read_entire_file(File);
  ASSERT_EQ(NumLines, 23831);
  return true;
}

void run_test(std::function<bool()> test, string TestName)
{
  bool Result = test();
  if (Result) {
    cout << "   " << TestName << " passed!" << endl;
  } else {
    cout << "!!!" << TestName << " failed!" << endl;
  }
}

#define RUN_TEST(TestName) run_test(TestName, #TestName)
int main(int argc, char *argv[])
{
  RUN_TEST(test_convert_cube_to_mesh_positions);
  RUN_TEST(test_convert_quad_cube_to_mesh);
  RUN_TEST(test_convert_cube_to_mesh_normals);
  RUN_TEST(test_convert_cube_to_mesh_uvs);
  RUN_TEST(test_convert_to_mesh_wont_crash_on_empty_input);

  RUN_TEST(test_open_cube_file);
  RUN_TEST(test_parse_cube_vertices);
  RUN_TEST(test_parse_cube_texture_coords);
  RUN_TEST(test_parse_cube_normals);
  RUN_TEST(test_parse_cube_faces);
  RUN_TEST(test_parse_faces_with_only_vertices);
  RUN_TEST(test_parse_faces_with_vertices_and_tex_coords);
  RUN_TEST(test_parse_faces_with_vertices_and_normals);
  RUN_TEST(test_open_ducky_file);
  RUN_TEST(test_parse_ducky_faces);
}
