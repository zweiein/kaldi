// nnet-dp/layer-test.cc

// Copyright 2012  Johns Hopkins University (author:  Daniel Povey)

// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//  http://www.apache.org/licenses/LICENSE-2.0
//
// THIS CODE IS PROVIDED *AS IS* BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, EITHER EXPRESS OR IMPLIED, INCLUDING WITHOUT LIMITATION ANY IMPLIED
// WARRANTIES OR CONDITIONS OF TITLE, FITNESS FOR A PARTICULAR PURPOSE,
// MERCHANTABLITY OR NON-INFRINGEMENT.
// See the Apache 2 License for the specific language governing permissions and
// limitations under the License.

#include "nnet-dp/layer.h"

namespace kaldi {

void UnitTestTanhLayer() {
  // We're testing that the gradients are computed correctly:
  // the input gradients and the model gradients.
  
  int32 input_dim = 10 + rand() % 50, output_dim = 10 + rand() % 50;
  BaseFloat learning_rate = 0.2, parameter_stddev = 0.1; // both arbitrary.
  
  TanhLayer test_layer(input_dim, output_dim, learning_rate, parameter_stddev);
  Vector<BaseFloat> objf_vec(output_dim); // objective function is linear function of output.
  objf_vec.SetRandn(); // set to Gaussian noise.

  int32 num_egs = 10 + rand() % 5;
  Matrix<BaseFloat> input(num_egs, input_dim),
      output(num_egs, output_dim);
  input.SetRandn();

  TanhLayer gradient(test_layer);
  gradient.SetZero();
  gradient.SetLearningRate(1.0);
  
  test_layer.Forward(input, &output);
  { // Test backward derivative and model derivatives are correct.
    Vector<BaseFloat> output_objfs(num_egs);
    output_objfs.AddMatVec(1.0, output, kNoTrans, objf_vec);
    BaseFloat objf = output_objfs.Sum();

    Matrix<BaseFloat> output_deriv(output.NumRows(), output.NumCols());
    for (int32 i = 0; i < output_deriv.NumRows(); i++)
      output_deriv.Row(i).CopyFromVec(objf_vec);

    Matrix<BaseFloat> input_deriv(input.NumRows(), input.NumCols());
    
    test_layer.Backward(input, output, output_deriv, &input_deriv, &gradient);

    Matrix<BaseFloat> perturbed_input(input.NumRows(), input.NumCols());
    perturbed_input.SetRandn();
    perturbed_input.Scale(1.0e-05); // scale by a small amount so it's like a delta.
    BaseFloat predicted_difference = TraceMatMat(perturbed_input,
                                                 input_deriv, kTrans);
    perturbed_input.AddMat(1.0, input);
    { // Compute objf with perturbed input and make sure it matches prediction.
      Matrix<BaseFloat> perturbed_output(output.NumRows(), output.NumCols());
      test_layer.Forward(perturbed_input, &perturbed_output);
      Vector<BaseFloat> perturbed_output_objfs(num_egs);
      perturbed_output_objfs.AddMatVec(1.0, perturbed_output, kNoTrans,
                                       objf_vec);
      BaseFloat perturbed_objf = perturbed_output_objfs.Sum(),
          observed_difference = perturbed_objf - objf;
      KALDI_LOG << "Input gradients: comparing " << predicted_difference
                << " and " << observed_difference;
      KALDI_ASSERT (fabs(predicted_difference - observed_difference) <
                    0.25 * ((predicted_difference + observed_difference)/2));
    }
  }
}


} // namespace kaldi

int main() {
  using namespace kaldi;
  UnitTestTanhLayer();
}

