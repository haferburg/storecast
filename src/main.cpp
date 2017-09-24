#include "tests.hpp"

#include <string>
#include <fstream>
#include <iostream>

#include "obj_import.hpp"
#include "mesh.hpp"

namespace storecast
{
void print_command_list_for_file(std::string Filename)
{
  std::ifstream File(Filename);
  auto Obj = parse_obj(File);
  auto Mesh = convert_to_mesh(Obj);
  auto CommandList = get_draw_command_list(Mesh);
  for (auto Command: CommandList) {
    std::cout << Command << std::endl;
  }
}
} // namespace storecast

int main(int argc, char *argv[])
{
  bool RunTests = true;
  if (argc == 2) {
    storecast::print_command_list_for_file(argv[1]);
    RunTests = false;
  }

  if (RunTests) {
    storecast::run_all_tests();
  }
}

