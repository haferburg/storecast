#include "tests.hpp"

#include <string>
#include <iostream>
#include <istream>
#include <fstream>
#include <functional>
#include <sstream>
#include <vector>

#include "defines.hpp"
#include "math.hpp"
#include "obj_import.hpp"
#include "mesh.hpp"

namespace storecast
{

using std::string;
using std::stringstream;
using std::ifstream;
using std::cout;
using std::vector;
using std::endl;

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

bool test_convert_to_mesh_without_uvs()
{
  obj_file_data Obj = { CubeVertices, {}, CubeNormals,
    {
      {4, false, true, {1,1, 2,1, 3,1, 4,1}},
      {4, false, true, {3,2, 4,2, 5,2, 6,2}},
      {4, false, true, {5,3, 6,3, 7,3, 8,3}},
    }
  };
  mesh Mesh = convert_to_mesh(Obj);
  return true;
}

bool test_convert_cube_to_mesh_positions()
{
  auto& Obj = CubeObj;
  mesh Mesh = convert_to_mesh(Obj);
  ASSERT_EQ(Mesh.TriangleIndices.size(), 3*Obj.f.size());
  for (i32 TriIndex = 0; TriIndex < Obj.f.size(); ++TriIndex) {
    for3(I) {
      auto MeshTriangleIndex = 3*TriIndex + I;
      ASSERT_EQ(0<=MeshTriangleIndex && MeshTriangleIndex<Mesh.TriangleIndices.size(), true);
      auto& MeshVertexIndex = Mesh.TriangleIndices[MeshTriangleIndex];
      auto& MeshVertexPosition = Mesh.Vertices[MeshVertexIndex].Position;
      auto& ObjVertexIndex = Obj.f[TriIndex].Indices[3*I+0];
      auto& ObjVertex = Obj.v[ObjVertexIndex-1];
      for3(Dim) {
        ASSERT_EQ(MeshVertexPosition.Data[Dim], ObjVertex.Data[Dim]);
      }
    }
  }
  return true;
}

bool test_convert_quad_cube_to_mesh()
{
  auto& Obj = QuadCubeObj;
  mesh Mesh = convert_to_mesh(Obj);
  ASSERT_EQ(Mesh.TriangleIndices.size(), 0);
  ASSERT_EQ(Mesh.QuadIndices.size(), 4*Obj.f.size());
  for (i32 QuadIndex = 0; QuadIndex < Obj.f.size(); ++QuadIndex) {
    for4(I) {
      auto MeshQuadIndex = 4*QuadIndex + I;
      ASSERT_EQ(0<=MeshQuadIndex && MeshQuadIndex<Mesh.QuadIndices.size(), true);
      auto& MeshVertexIndex = Mesh.QuadIndices[MeshQuadIndex];
      auto& MeshVertexPosition = Mesh.Vertices[MeshVertexIndex].Position;
      auto& MeshNormal = Mesh.Vertices[MeshVertexIndex].Normal;
      auto& MeshUV = Mesh.Vertices[MeshVertexIndex].TextureCoords;
      auto& ObjVertexIndex = Obj.f[QuadIndex].Indices[3*I+0];
      auto& ObjVertex = Obj.v[ObjVertexIndex-1];
      auto& ObjUVIndex = Obj.f[QuadIndex].Indices[3*I+1];
      auto& ObjUV = Obj.vt[ObjUVIndex-1];
      auto& ObjNormalIndex = Obj.f[QuadIndex].Indices[3*I+2];
      auto& ObjNormal = Obj.vn[ObjNormalIndex-1];
      for3(Dim) {
        ASSERT_EQ(MeshVertexPosition.Data[Dim], ObjVertex.Data[Dim]);
        ASSERT_EQ(MeshNormal.Data[Dim], ObjNormal.Data[Dim]);
        ASSERT_EQ(ObjUV.Data[Dim], MeshUV.Data[Dim]);
      }
    }
  }
  return true;
}

bool test_convert_cube_to_mesh_normals()
{
  auto& Obj = CubeObj;
  mesh Mesh = convert_to_mesh(Obj);
  ASSERT_EQ(Mesh.TriangleIndices.size(), 3*Obj.f.size());
  for (i32 TriIndex = 0; TriIndex < Obj.f.size(); ++TriIndex) {
    for3(I) {
      auto MeshTriangleIndex = 3*TriIndex + I;
      ASSERT_EQ(0<=MeshTriangleIndex && MeshTriangleIndex<Mesh.TriangleIndices.size(), true);
      auto& MeshVertexIndex = Mesh.TriangleIndices[MeshTriangleIndex];
      auto& MeshNormal = Mesh.Vertices[MeshVertexIndex].Normal;
      auto& ObjNormalIndex = Obj.f[TriIndex].Indices[3*I+2];
      auto& ObjNormal = Obj.vn[ObjNormalIndex-1];
      for3(Dim) {
        ASSERT_EQ(MeshNormal.Data[Dim], ObjNormal.Data[Dim]);
      }
    }
  }
  return true;
}

bool test_convert_cube_to_mesh_uvs()
{
  auto& Obj = CubeObj;
  mesh Mesh = convert_to_mesh(Obj);
  ASSERT_EQ(Mesh.TriangleIndices.size(), 3*Obj.f.size());
  for (i32 TriIndex = 0; TriIndex < Obj.f.size(); ++TriIndex) {
    for3(I) {
      auto MeshTriangleIndex = 3*TriIndex + I;
      ASSERT_EQ(0<=MeshTriangleIndex && MeshTriangleIndex<Mesh.TriangleIndices.size(), true);
      auto& MeshVertexIndex = Mesh.TriangleIndices[MeshTriangleIndex];
      auto& MeshUV = Mesh.Vertices[MeshVertexIndex].TextureCoords;
      auto& ObjUVIndex = Obj.f[TriIndex].Indices[3*I+1];
      auto& ObjUV = Obj.vt[ObjUVIndex-1];
      for3(Dim) {
        ASSERT_EQ(ObjUV.Data[Dim], MeshUV.Data[Dim]);
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
    ASSERT_EQ(f.Indices.size(), 3*f.NumVertices);
  }

  // f 1/1/1 2/2/1 3/3/1
  ASSERT_EQ(Data.f[0].Indices[0], 1);
  ASSERT_EQ(Data.f[0].Indices[1], 1);
  ASSERT_EQ(Data.f[0].Indices[2], 1);
  ASSERT_EQ(Data.f[0].Indices[3], 2);
  ASSERT_EQ(Data.f[0].Indices[4], 2);
  ASSERT_EQ(Data.f[0].Indices[5], 1);
  ASSERT_EQ(Data.f[0].Indices[6], 3);
  ASSERT_EQ(Data.f[0].Indices[7], 3);
  ASSERT_EQ(Data.f[0].Indices[8], 1);

  // f 2/1/5 8/2/5 4/3/5
  ASSERT_EQ(Data.f[8].Indices[0], 2);
  ASSERT_EQ(Data.f[8].Indices[1], 1);
  ASSERT_EQ(Data.f[8].Indices[2], 5);
  ASSERT_EQ(Data.f[8].Indices[3], 8);
  ASSERT_EQ(Data.f[8].Indices[4], 2);
  ASSERT_EQ(Data.f[8].Indices[5], 5);
  ASSERT_EQ(Data.f[8].Indices[6], 4);
  ASSERT_EQ(Data.f[8].Indices[7], 3);
  ASSERT_EQ(Data.f[8].Indices[8], 5);

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
    ASSERT_EQ(f.Indices.size(), 2*f.NumVertices);
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
    ASSERT_EQ(f.Indices.size(), f.NumVertices);
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
    ASSERT_EQ(f.Indices.size(), 2*f.NumVertices);
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
    ASSERT_EQ(f.Indices.size(), 2*f.NumVertices);
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
void run_test(std::function<bool()> test, std::string TestName);
void run_all_tests()
{
  RUN_TEST(test_convert_to_mesh_without_uvs);
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

} // namespace storecast
