#include "Halide.h"

#include "image_utils.h"

using namespace Halide;

Image<float>
im2col(Image<float> input, int kernel_size, int pad, int stride)
{
  int input_width = input.width();
  int input_height = input.height();
  int input_channels = input.channels();

  /* x_steps = number of steps the kernel takes before reaching the last column 
   * y_steps = number of steps the kernel takes before reaching the last row */
  int x_steps = (input_width  + 2*pad - kernel_size)/stride + 1;
  int y_steps = (input_height + 2*pad - kernel_size)/stride + 1;

  /* Figure out the output dimension */
  int output_width = x_steps * y_steps;
  int output_height = kernel_size * kernel_size * input_channels;

  Func output;
  Var x, y;

  /* Each column in the final matrix is a concatenation of all the rows under 
   * the kernel footprint for each channel. Think of x, y as the cell position 
   * in the final matrix */

  /* Let's figure out the top left corner's coordinates */
  Expr top_x = x % x_steps * stride - pad;
  Expr top_y = x / x_steps * stride - pad; 

  /* Then let's figure out where in the box are we */
  Expr del_x = y % kernel_size;
  Expr del_y = y / kernel_size % kernel_size;
  Expr del_z = y / (kernel_size*kernel_size);

  /* Then add the delta to the start */
  Expr input_x = top_x + del_x;
  Expr input_y = top_y + del_y;
  Expr input_z = 0     + del_z;

  /* Zero pad */
  Func clamped = BoundaryConditions::constant_exterior(input, 0.f);
  output(x, y) = clamped(input_x, input_y, input_z);

  /* TODO schedule */

  return output.realize(output_width, output_height);
}



Image<float>
col2im(Image<float> output, int kernel_size, int pad, int stride, 
  int width, int height, int channels)
{
  int input_width   = (width + 2 * pad - kernel_size) / stride + 1;
  int input_height  = (height + 2 * pad - kernel_size) / stride + 1;
  int input_channels = channels;

#if 0
  int x_steps = (input_width  + 2*pad - kernel_size)/stride + 1;
  int y_steps = (input_height + 2*pad - kernel_size)/stride + 1;

  int output_width = output.width();
  int output_height = output.height();
#endif

  /* Initialize output */
  Image<float> input(input_width, input_height, input_channels);
  for (int z = 0; z < input_channels; z++) {
    for (int j = -pad; j < input_height+pad; j += stride) {
      for (int i = -pad; i < input_width+pad; i += stride) {
        for (int j_k = 0; j_k < kernel_size; j_k++) {

          if (j+j_k >= 0 && j+j_k < input_height) {
            for (int i_k = 0; i_k < kernel_size; i_k++) {

              if (i+i_k >=0 && i+i_k < input_width) {
                int out_x = (i+i_k) + (j+j_k)*input_width;
                int out_y = i_k + j_k*kernel_size + z*kernel_size*kernel_size;
                input(i+i_k, j+j_k, z) += output(out_x, out_y);
              }
            }
          }
        }
      }
    }
  }

  return input;
}
