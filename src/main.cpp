#include <string>
#include <iostream>
#include <fstream>
#include <functional>
#include <vector>
#include <sstream>

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
    f32 Coord[3];
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
      i32 NumEntries = 1;
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
          i32 Index = IndexToken.size() > 0 ? std::stoi(IndexToken) : 0;
          Value.Index.push_back(Index);

          if (getline(VertexStream, IndexToken, '/')) {
            Index = IndexToken.size() > 0 ? std::stoi(IndexToken) : 0;
            Value.HasVt = Index > 0;
            if (Value.HasVt) {
              Value.Index.push_back(Index);
            }

            if (getline(VertexStream, IndexToken, '/')) {
              Index = IndexToken.size() > 0 ? std::stoi(IndexToken) : 0;
              Value.HasVn = Index > 0;
              if (Value.HasVn) {
                Value.Index.push_back(Index);
              }
            }
          }
          NumEntries = 1 + (Value.HasVt ? 1 : 0) + (Value.HasVn ? 1 : 0);
        } else {
          for (i32 I = 0; I < NumEntries; ++I) {
            getline(VertexStream, IndexToken, '/');
            if (IndexToken.size()) {
              Value.Index.push_back(std::stoi(IndexToken));
            }
          }
        }
        ++Value.NumVertices;
      }
      Data.f.push_back(Value);
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
#define ASSERT_EQ(Value, Expected) if (!equals(Value, Expected)) { cout << #Value " is not " << Expected << " but " << Value << endl; return false; }

i32 read_entire_file(ifstream& In)
{
  i32 NumLines = 0;
  for (string Line; getline(In, Line); ) {
    ++NumLines;
  }
  return NumLines;
}

string CubeFilePath = "../data/cube.obj";
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
  string FileName = "../data/ducky.obj";
  ifstream File(FileName);
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
  RUN_TEST(test_open_cube_file);
  RUN_TEST(test_open_ducky_file);
  RUN_TEST(test_parse_cube_vertices);
  RUN_TEST(test_parse_cube_texture_coords);
  RUN_TEST(test_parse_cube_normals);
  RUN_TEST(test_parse_cube_faces);
  RUN_TEST(test_parse_faces_with_only_vertices);
  RUN_TEST(test_parse_faces_with_vertices_and_tex_coords);
  RUN_TEST(test_parse_faces_with_vertices_and_normals);
}
