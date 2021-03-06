// nnet/nnet-randomizer.h

// Copyright 2013  Brno University of Technology (author: Karel Vesely)

// See ../../COPYING for clarification regarding multiple authors
//
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


#ifndef KALDI_NNET_NNET_RANDOMIZER_H_
#define KALDI_NNET_NNET_RANDOMIZER_H_

#include "base/kaldi-math.h"
#include "itf/options-itf.h"
#include "cudamatrix/cu-matrix.h"
#include "cudamatrix/cu-math.h"

namespace kaldi {
namespace nnet1 {

/// Configuration variables that affect how frame-level shuffling is done.
struct NnetDataRandomizerOptions {
  int32 randomizer_size; // Maximum number of samples we want to have in memory at once.
  int32 randomizer_seed;
  int32 minibatch_size;  // Size of a single mini-batch.

  NnetDataRandomizerOptions()
   : randomizer_size(32768), randomizer_seed(777), minibatch_size(256) 
  { }

  void Register(OptionsItf *po) {
    po->Register("randomizer-size", &randomizer_size, "Capacity of randomizer, length of concatenated utterances which are used for frame-level shuffling (in frames, affects memory consumption, max 8000000).");
    po->Register("randomizer-seed", &randomizer_seed, "Seed value for srand, sets fixed order of frame-level shuffling");
    po->Register("minibatch-size", &minibatch_size, "Size of a minibatch.");
  }
};
///


/// Generates index-mask, which is used to randomize order of datapoints (speech frames)
class RandomizerMask {
 public:
  RandomizerMask() { }
  RandomizerMask(const NnetDataRandomizerOptions &conf) { Init(conf); }
  /// Init (only runs srand)
  void Init(const NnetDataRandomizerOptions& conf); 
  /// Generate vector of integers 0..[mask_size -1] with random order.
  const std::vector<int32>& Generate(int32 mask_size);
 private:
  std::vector<int32> mask_;
};


/// Randomizes rows of a matrix according to a mask
class MatrixRandomizer {
 public:
  MatrixRandomizer() : data_begin_(0), data_end_(0) { }
  MatrixRandomizer(const NnetDataRandomizerOptions &conf) : data_begin_(0), data_end_(0) { Init(conf); }
  /// Set the randomizer parameters (size)
  void Init(const NnetDataRandomizerOptions& conf) { conf_ = conf; }

  /// Add data to randomization buffer
  void AddData(const CuMatrix<BaseFloat>& m);
  /// Returns true, when capacity is full
  bool IsFull() { return ((data_begin_ == 0) && (data_end_ > conf_.randomizer_size )); }
  /// Number of frames stored inside the Randomizer
  int32 NumFrames() { return data_end_; }
  /// Randomize matrix row-order using mask
  void Randomize(const std::vector<int32>& mask);

  /// Returns true, if no more data for another mini-batch (after current one)
  bool Done() { return (data_end_ - data_begin_ < conf_.minibatch_size); }
  /// Sets cursor to next mini-batch
  void Next();
  /// Returns matrix-window with next mini-batch
  const CuMatrix<BaseFloat>& Value();

 private:
  CuMatrix<BaseFloat> data_; // can be larger than 'randomizer_size'
  CuMatrix<BaseFloat> data_aux_; // auxiliary buffer for shuffling
  CuMatrix<BaseFloat> minibatch_; // buffer for mini-batch

  /// Cursor to beginning of data (row index, moves as mini-batches are delivered)
  int32 data_begin_;
  /// Cursor past the end of data (row index) 
  int32 data_end_;   

  NnetDataRandomizerOptions conf_;
};


/// Randomizes elements of a vector according to a mask
class VectorRandomizer {
 public:
  VectorRandomizer() : data_begin_(0), data_end_(0) { }
  VectorRandomizer(const NnetDataRandomizerOptions &conf) : data_begin_(0), data_end_(0) { Init(conf); }
  /// Set the randomizer parameters (size)
  void Init(const NnetDataRandomizerOptions& conf) { conf_ = conf; }

  /// Add data to randomization buffer
  void AddData(const Vector<BaseFloat>& v);
  /// Returns true, when capacity is full
  bool IsFull() { return ((data_begin_ == 0) && (data_end_ > conf_.randomizer_size )); }
  /// Number of frames stored inside the Randomizer
  int32 NumFrames() { return data_end_; }
  /// Randomize matrix row-order using mask
  void Randomize(const std::vector<int32>& mask);

  /// Returns true, if no more data for another mini-batch (after current one)
  bool Done() { return (data_end_ - data_begin_ < conf_.minibatch_size); }
  /// Sets cursor to next mini-batch
  void Next();
  /// Returns matrix-window with next mini-batch
  const Vector<BaseFloat>& Value();

 private:
  Vector<BaseFloat> data_; // can be larger than 'randomizer_size'
  Vector<BaseFloat> minibatch_; // buffer for mini-batch

  /// Cursor to beginning of data (row index, moves as mini-batches are delivered)
  int32 data_begin_;
  /// Cursor past the end of data (row index) 
  int32 data_end_;   

  NnetDataRandomizerOptions conf_;
};


/// Randomizes elements of a vector according to a mask
template<typename T>
class StdVectorRandomizer {
 public:
  StdVectorRandomizer() : data_begin_(0), data_end_(0) { }
  StdVectorRandomizer(const NnetDataRandomizerOptions &conf) : data_begin_(0), data_end_(0) { Init(conf); }
  /// Set the randomizer parameters (size)
  void Init(const NnetDataRandomizerOptions& conf) { conf_ = conf; }

  /// Add data to randomization buffer
  void AddData(const std::vector<T>& v);
  /// Returns true, when capacity is full
  bool IsFull() { return ((data_begin_ == 0) && (data_end_ > conf_.randomizer_size )); }
  /// Number of frames stored inside the Randomizer
  int32 NumFrames() { return data_end_; }
  /// Randomize matrix row-order using mask
  void Randomize(const std::vector<int32>& mask);

  /// Returns true, if no more data for another mini-batch (after current one)
  bool Done() { return (data_end_ - data_begin_ < conf_.minibatch_size); }
  /// Sets cursor to next mini-batch
  void Next();
  /// Returns matrix-window with next mini-batch
  const std::vector<T>& Value();

 private:
  std::vector<T> data_; // can be larger than 'randomizer_size'
  std::vector<T> minibatch_; // buffer for mini-batch

  /// Cursor to beginning of data (row index, moves as mini-batches are delivered)
  int32 data_begin_;
  /// Cursor past the end of data (row index) 
  int32 data_end_;   

  NnetDataRandomizerOptions conf_;
};

typedef StdVectorRandomizer<int32> Int32VectorRandomizer;
typedef StdVectorRandomizer<std::vector<std::pair<int32, BaseFloat> > > PosteriorRandomizer;


} // namespace nnet1
} // namespace kaldi

#endif
