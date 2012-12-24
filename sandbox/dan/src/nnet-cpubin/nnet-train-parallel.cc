// nnet-cpubin/nnet-train-parallel.cc

// Copyright 2012  Johns Hopkins University (author: Daniel Povey)

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

#include "base/kaldi-common.h"
#include "util/common-utils.h"
#include "hmm/transition-model.h"
#include "nnet-cpu/nnet-randomize.h"
#include "nnet-cpu/train-nnet.h"
#include "nnet-cpu/am-nnet.h"


int main(int argc, char *argv[]) {
  try {
    using namespace kaldi;
    typedef kaldi::int32 int32;
    typedef kaldi::int64 int64;

    const char *usage =
        "Train the neural network parameters with backprop and stochastic\n"
        "gradient descent using minibatches.  The training frames and labels\n"
        "are read via a pipe from nnet-randomize-frames.  This is like nnet-train-simple,\n"
        "but uses multiple threads in a Hogwild type of update.\n"
        "\n"
        "Usage:  nnet-train-parallel [options] <model-in> <training-examples-in> <model-out>\n"
        "\n"
        "e.g.:\n"
        "nnet-randomize-frames [args] | nnet-train-simple 1.nnet ark:- 2.nnet\n";
    
    bool binary_write = true;
    bool zero_occupancy = true;
    int32 minibatch_size = 1024;
    
    ParseOptions po(usage);
    po.Register("binary", &binary_write, "Write output in binary mode");
    po.Register("zero-occupancy", &zero_occupancy, "If true, zero occupation "
                "counts stored with the neural net (only affects mixing up).");
    po.Register("num-threads", &g_num_threads, "Number of training threads to use "
                "in the parallel update. [Note: if you use a parallel "
                "implementation of BLAS, the actual number of threads may be larger.]");
    po.Register("minibatch-size", &minibatch_size, "Number of examples to use for "
                "each minibatch during training.");
    
    po.Read(argc, argv);
    
    if (po.NumArgs() != 3) {
      po.PrintUsage();
      exit(1);
    }
    
    std::string nnet_rxfilename = po.GetArg(1),
        examples_rspecifier = po.GetArg(2),
        nnet_wxfilename = po.GetArg(3);

    TransitionModel trans_model;
    AmNnet am_nnet;
    {
      bool binary_read;
      Input ki(nnet_rxfilename, &binary_read);
      trans_model.Read(ki.Stream(), binary_read);
      am_nnet.Read(ki.Stream(), binary_read);
    }

    KALDI_ASSERT(minibatch_size > 0);

    ExamplesRepository repository;
    
    if (zero_occupancy) am_nnet.GetNnet().ZeroOccupancy();


    int64 num_examples = 0;
    SequentialNnetTrainingExampleReader example_reader(examples_rspecifier);
    BaseFloat tot_loglike = DoBackpropParallel(am_nnet.Nnet(),
                                               minibatch_size,
                                               &example_reader,
                                               &num_examples,
                                               &(am_nnet.Nnet()));
    // This function will have produced logging output, so we have no
    // need for that here.
    
    {
      Output ko(nnet_wxfilename, binary_write);
      trans_model.Write(ko.Stream(), binary_write);
      am_nnet.Write(ko.Stream(), binary_write);
    }
    
    KALDI_LOG << "Finished training, processed " << num_examples
              << " training examples.  Wrote model to "
              << nnet_wxfilename;
    return (num_examples == 0 ? 1 : 0);
  } catch(const std::exception &e) {
    std::cerr << e.what() << '\n';
    return -1;
  }
}


