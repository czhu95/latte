#include <string>
#include <iostream>

#include "Halide.h"
#include "halide_image_io.h"

#include "image_utils.h"

using namespace std;
using namespace Halide;
using namespace Halide::Tools;

bool
test_im2col(string image_path)
{
  Image<float> input = load_image(image_path);
  cout << "input dimension" << endl;
  cout << input.width() << endl;
  cout << input.height() << endl;

  int kernel_size = 3;
  int stride = 1;
  Image<float> output = im2col(input, kernel_size, stride);
  cout << "output dimension" << endl;
  cout << output.width() << endl;
  cout << output.height() << endl;
  save_image(output, "im2col.png");

  return true;
}
