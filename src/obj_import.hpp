#pragma once
#include "defines.hpp"
#include "math.hpp"
#include <vector>

namespace storecast {
struct mesh;

// Flattened list of data in the f face element. For example `f 1/2/3 2/3/4 3/4/5` would be
// represented as {3, true, true, {1,2,3, 2,3,4, 3,4,5}}, while `f 1//2 3//4` would be
// {2, false, true, {1,2, 3,4}}.
struct obj_face_data {
  i32 NumVertices;
  bool HasVt;
  bool HasVn;
  std::vector<i32> Indices;
};
struct obj_file_data {
  std::vector<vec3> v;
  std::vector<vec3> vt;
  std::vector<vec3> vn;
  std::vector<obj_face_data> f;
};

mesh convert_to_mesh(const obj_file_data& Obj);
obj_file_data parse_obj(std::istream& In);

} // namespace storecast
