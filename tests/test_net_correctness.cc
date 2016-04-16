#include <iostream>
#include <fstream>
#include <string>

#include "caffe.pb.h"
#include "halide_image_io.h"

#include "io_utils.h"
#include "CycleTimer.h"

#include "layers.h" 
#include "vision_layers.h"
#include "activation_layers.h"
#include "common_layers.h"
#include "loss_layers.h"

using namespace std;
using namespace caffe;
using namespace Halide;
using namespace Halide::Tools;
using namespace Latte;

bool
test_net(string image_path, NetParameter *net_model) 
{
  /* Loads the image */
  Image<float> input = load_image(image_path);

  Layer *head = NULL;
  Layer *prev_layer = NULL;
  Layer *curr_ptr = NULL;

  cout << "name \taddr" << endl;
  /* Let's build the net */
  /* TODO we are cheating here. We know the first conv layer starts at 3 */
  for (int i = 3; i < net_model->layer_size(); i++) {
    LayerParameter layer = net_model->layer(i);
    string name = layer.name();
    string type = layer.type();
    bool hit = false;
    /* TODO we are ignoring a couple of types here */
    /* This is a going to be a big switch statement */
    if (type == CONVOLUTION) {
      cout << "hit convolution" << endl;
      ConvolutionParameter conv_param = layer.convolution_param();
      BlobProto weight_blob = layer.blobs(0);
      Convolution *conv_layer;

      if (conv_param.has_bias_term()) {
        BlobProto bias_blob = layer.blobs(1);
        conv_layer = new Convolution(name, &conv_param, &weight_blob, &bias_blob);
      } else {
        conv_layer = new Convolution(name, &conv_param, &weight_blob, NULL);
      }
      curr_ptr = conv_layer;
      hit = true;
      cout << "finish processing convolution" << endl;
    } 

    #if 0
    else if (type == DECONVOLUTION) {
      cout << "hit deconv" << endl;
      ConvolutionParameter deconv_param = layer.convolution_param();
      BlobProto weight_blob = layer.blobs(0);
      Deconvolution *deconv_layer;
      /* Question: why can't load the second blob? */
      if (deconv_param.has_bias_term()) {
        cout << "has bias term" << endl;
        BlobProto bias_blob = layer.blobs(1);
	       cout << "before constructor" << endl;
        deconv_layer = new Deconvolution(name, &deconv_param, &weight_blob, &bias_blob);
      } else {
        deconv_layer = new Deconvolution(name, &deconv_param, &weight_blob, NULL);
      }

      deconv_layer = new Deconvolution(name, &deconv_param, &weight_blob, NULL);
      curr_ptr = deconv_layer;
      hit = true;
      cout << "finish processing deconv" << endl;
    } 
    #endif

    else if (type == RELU) {
      ReLUParameter relu_param = layer.relu_param();
      ReLU *relu_layer = new ReLU(name, &relu_param);
      curr_ptr = relu_layer;
      hit = true;
    } else if (type == POOLING) {
      PoolingParameter pool_param = layer.pooling_param();
      Pooling *pool_layer = new Pooling(name, &pool_param);
      curr_ptr = pool_layer;
      hit = true;
    } else if (type == DROPOUT) {
      DropoutParameter drop_param = layer.dropout_param();
      Dropout *dropout_layer = new Dropout(name, &drop_param);
      curr_ptr = dropout_layer;
      hit = true;
    }

    if (hit) {
      cout << name << "\t" << curr_ptr << endl;
      if (!head)
        head = curr_ptr;

      /* Update prev layer's next */
      if (prev_layer)
        prev_layer->set_next(curr_ptr);
      prev_layer = curr_ptr;
    }
  }
  cout << "layers building done!" << endl;

  /* Display layers information */
  cout << endl;
  cout << "layers are:" << endl;
  cout << "name" << "\t\t\t" << "type" << endl;
  for (Layer *ptr = head; ptr != NULL; ptr = ptr->get_next()) {
    cout << ptr->get_name() << "\t\t\t" << ptr->get_type() << endl;
  }

  /* Run layers */
  double startTime, endTime, allStartTime, allEndTime;
  cout << endl << endl;
  cout << "run layers" << endl;
  Image<float> prev_output = input;
  Image<float> curr_output;
  allStartTime = CycleTimer::currentSeconds();
  for (Layer *ptr = head; ptr != NULL; ptr = ptr->get_next()) {
    cout << "passing volume into [" << ptr->get_name() << "," << ptr->get_type() << "]  " << endl;
    startTime = CycleTimer::currentSeconds();
    curr_output = ptr->run(prev_output);
    endTime = CycleTimer::currentSeconds();
    cout << "time elapsed: " << (endTime - startTime) * 1000 << " ms  " << endl;
    prev_output = curr_output;
  }
  allEndTime = CycleTimer::currentSeconds();
  cout << "total time elapsed: " << (allEndTime - allStartTime) * 1000 << " ms  " << endl;

  /* Save all channels */
  int num_output_layers = curr_output.channels();
  for (int i = 0; i < num_output_layers; i++) {
    Func get_slice;
    Var x, y, z;
    get_slice(x, y, z) = curr_output(x, y, z + i);
    Image<float> slice = get_slice.realize(curr_output.width(), curr_output.height(), 1);
    save_image(slice, "./outputs/channel_" + to_string(i) + ".png");
  }

  return true;
}
