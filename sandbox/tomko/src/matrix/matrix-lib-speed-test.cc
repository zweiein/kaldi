// matrix/matrix-lib-test.cc

// Copyright 2009-2014   Microsoft Corporation;  Mohit Agarwal;  Lukas Burget;
//                       Ondrej Glembek;  Saarland University (Author: Arnab Ghoshal);
//                       Go Vivace Inc.;  Yanmin Qian;  Jan Silovsky;
//                       Johns Hopkins University (Author: Daniel Povey);
//                       Haihua Xu; Wei Shi; Karel Vesely

// See ../../COPYING for clarification regarding multiple authors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at

//  http://www.apache.org/licenses/LICENSE-2.0

// THIS CODE IS PROVIDED *AS IS* BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, EITHER EXPRESS OR IMPLIED, INCLUDING WITHOUT LIMITATION ANY IMPLIED
// WARRANTIES OR CONDITIONS OF TITLE, FITNESS FOR A PARTICULAR PURPOSE,
// MERCHANTABLITY OR NON-INFRINGEMENT.
// See the Apache 2 License for the specific language governing permissions and
// limitations under the License.

#include "matrix/matrix-lib.h"
#include "base/timer.h"
#include <numeric>

namespace kaldi {

template<typename Real> static void UnitTestRealFftSpeed() {
  // First, test RealFftInefficient.
  Timer t;
  MatrixIndexT sz = 512;  // fairly typical size.
  for (MatrixIndexT i = 0; i < 3000; i++) {
    if (i % 1000 == 0) KALDI_LOG << "done 1000 [ == ten seconds of speech]";
    Vector<Real> v(sz);
    RealFft(&v, true);
  }
  KALDI_LOG << __func__ << " finished in " << t.Elapsed() << " seconds.";
}

template<typename Real> static void UnitTestSplitRadixRealFftSpeed() {
  Timer t;
  MatrixIndexT sz = 512;  // fairly typical size.
  SplitRadixRealFft<Real> srfft(sz);
  for (MatrixIndexT i = 0; i < 6000; i++) {
    if (i % 1000 == 0)
      KALDI_LOG << "done 1000 [ == ten seconds of speech, split-radix]";
    Vector<Real> v(sz);
    srfft.Compute(v.Data(), true);
  }
  KALDI_LOG << __func__ << " finished in " << t.Elapsed() << " seconds.";
}

template<typename Real>
static void UnitTestSvdSpeed() {
  Timer t;
  std::vector<MatrixIndexT> sizes;
  sizes.push_back(100);
  sizes.push_back(150);
  sizes.push_back(200);
  sizes.push_back(300);
  // sizes.push_back(500);
  // sizes.push_back(750);
  for (size_t i = 0; i < sizes.size(); i++) {
    MatrixIndexT size = sizes[i];
    {
      Timer t1;
      SpMatrix<Real> S(size);
      Vector<Real> l(size);
      S.Eig(&l);
      KALDI_LOG << "For size " << size << ", Eig without eigenvectors took " << t1.Elapsed()
                << " seconds.";
    }
    {
      Timer t1;
      SpMatrix<Real> S(size);
      S.SetRandn();
      Vector<Real> l(size);
      Matrix<Real> P(size, size);
      S.Eig(&l, &P);
      KALDI_LOG << "For size " << size << ", Eig with eigenvectors took " << t1.Elapsed()
                << " seconds.";
    }
    {
      Timer t1;
      Matrix<Real> M(size, size);
      M.SetRandn();
      Vector<Real> l(size);
      M.Svd(&l, NULL, NULL);
      KALDI_LOG << "For size " << size << ", SVD without eigenvectors took " << t1.Elapsed()
                << " seconds.";
    }
    {
      Timer t1;
      Matrix<Real> M(size, size), U(size, size), V(size, size);
      M.SetRandn();
      Vector<Real> l(size);
      M.Svd(&l, &U, &V);
      KALDI_LOG << "For size " << size << ", SVD with eigenvectors took " << t1.Elapsed()
                << " seconds.";
    }
  }
  KALDI_LOG << __func__ << " finished in " << t.Elapsed() << " seconds.";
}

template<typename Real>
static void UnitTestAddMatMatSpeed() {
  Timer t;
  std::vector<MatrixIndexT> sizes;
  sizes.push_back(512);
  sizes.push_back(1024);
  for (size_t i = 0; i < sizes.size(); i++) {
    MatrixIndexT size = sizes[i];
    {
      Timer t1;
      for (int32 j=0; j<2; j++) {
        Matrix<Real> A(size,size), B(size,size), C(size,size);
        A.SetRandn(); B.SetRandn();
        C.AddMatMat(1.0, A, kNoTrans, B, kNoTrans, 0.0); 
        C.AddMatMat(1.0, A, kNoTrans, B, kTrans, 0.0); 
        C.AddMatMat(1.0, A, kTrans, B, kNoTrans, 0.0); 
        C.AddMatMat(1.0, A, kTrans, B, kTrans, 0.0); 
      }
      KALDI_LOG << "For size " << size << ", AddMatMat (2x) took " << t1.Elapsed()
                << " seconds.";
    }
  }
  KALDI_LOG << __func__ << " finished in " << t.Elapsed() << " seconds.";
}

template<typename Real> static void MatrixUnitSpeedTest() {
  UnitTestRealFftSpeed<Real>();
  UnitTestSplitRadixRealFftSpeed<Real>();
  UnitTestSvdSpeed<Real>();
  UnitTestAddMatMatSpeed<Real>();
}

} // namespace kaldi

int main() {
  using namespace kaldi;
  Timer t;
  KALDI_LOG << "Starting, Single precision";
  kaldi::MatrixUnitSpeedTest<float>();
  KALDI_LOG << "Starting, Double precision";
  kaldi::MatrixUnitSpeedTest<double>();
  KALDI_LOG << "Tests succeeded, total duration " << t.Elapsed() << " seconds.";
}

