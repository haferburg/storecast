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
  vector<i32> index;
};
struct obj_file_data {
  vector<vec3> v;
  vector<vec3> vt;
  vector<vec3> vn;
  vector<obj_face_data> f;
};
obj_file_data parse_obj(ifstream& In)
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
    } else if (starts_with(Line, "f ")) {
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
}
