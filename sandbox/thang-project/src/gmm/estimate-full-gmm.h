// gmm/estimate-full-gmm.h

// Copyright 2009-2011  Jan Silovsky;  Saarland University;
//                      Microsoft Corporation

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

#ifndef KALDI_GMM_ESTIMATE_FULL_GMM_H_
#define KALDI_GMM_ESTIMATE_FULL_GMM_H_

#include <vector>

#include "gmm/model-common.h"
#include "gmm/full-gmm.h"

namespace kaldi {

/** \struct MleFullGmmOptions
 *  Configuration variables like variance floor, minimum occupancy, etc.
 *  needed in the estimation process.
 */
struct MleFullGmmOptions {
  /// Minimum weight below which a Gaussian is removed
  BaseFloat min_gaussian_weight;
  /// Minimum occupancy count below which a Gaussian is removed
  BaseFloat min_gaussian_occupancy;
  /// Floor on eigenvalues of covariance matrices
  BaseFloat variance_floor;
  /// Maximum condition number of covariance matrices (apply
  /// floor to eigenvalues if they pass this).
  BaseFloat max_condition;
  bool remove_low_count_gaussians;
  MleFullGmmOptions() {
    min_gaussian_weight    = 1.0e-05;
    min_gaussian_occupancy     = 100.0;
    variance_floor         = 0.001;
    max_condition          = 1.0e+04;
    remove_low_count_gaussians = true;
  }
  void Register(ParseOptions *po) {
    std::string module = "MleFullGmmOptions: ";
    po->Register("min-gaussian-weight", &min_gaussian_weight,
                 module+"Min Gaussian weight before we remove it.");
    po->Register("min-gaussian-occupancy", &min_gaussian_occupancy,
                 module+"Minimum count before we remove a Gaussian.");
    po->Register("variance-floor", &variance_floor,
                 module+"Minimum eigenvalue of covariance matrix.");
    po->Register("max-condition", &max_condition,
                 module+"Maximum condition number of covariance matrix (use it to floor).");
    po->Register("remove-low-count-gaussians", &remove_low_count_gaussians,
                 module+"If true, remove Gaussians that fall below the floors.");
  }
};

/** Class for computing the maximum-likelihood estimates of the parameters of
 *  a Gaussian mixture model.
 */
class MlEstimateFullGmm {
 public:
  MlEstimateFullGmm(): dim_(0), num_comp_(0), flags_(0) { }
  explicit MlEstimateFullGmm(const FullGmm &gmm, GmmFlagsType flags) {
    ResizeAccumulators(gmm, flags);
  }
  // provide copy constructor.
  explicit MlEstimateFullGmm(const MlEstimateFullGmm &other);

  /// Allocates memory for accumulators
  void ResizeAccumulators(int32 num_components, int32 dim,
                          GmmFlagsType flags);
  /// Calls ResizeAccumulators with arguments based on gmm_ptr_
  void ResizeAccumulators(const FullGmm &gmm, GmmFlagsType flags);
  /// Returns the number of mixture components
  int32 NumGauss() const { return num_comp_; }
  /// Returns the dimensionality of the feature vectors
  int32 Dim() const { return dim_; }

  void ZeroAccumulators(GmmFlagsType flags);

  void ScaleAccumulators(BaseFloat f, GmmFlagsType flags);  // scale stats.

  /// Accumulate for a single component, given the posterior
  void AccumulateForComponent(const VectorBase<BaseFloat>& data,
                              int32 comp_index, BaseFloat weight);

  /// Accumulate for all components, given the posteriors.
  void AccumulateFromPosteriors(const VectorBase<BaseFloat>& data,
                                const VectorBase<BaseFloat>& gauss_posteriors);

  /// Accumulate for all components given a full-covariance GMM.
  /// Computes posteriors and returns log-likelihood
  BaseFloat AccumulateFromFull(const FullGmm &gmm,
                               const VectorBase<BaseFloat>& data,
                               BaseFloat frame_posterior);

  /// Accumulate for all components given a diagonal-covariance GMM.
  /// Computes posteriors and returns log-likelihood
  BaseFloat AccumulateFromDiag(const DiagGmm &gmm,
                               const VectorBase<BaseFloat>& data,
                               BaseFloat frame_posterior);

  /// Const version of the update which preserves the accumulators
  void Update(const MleFullGmmOptions &config, GmmFlagsType f, FullGmm *gmm,
              BaseFloat *obj_change_out, BaseFloat *count_out) const;

  BaseFloat MlObjective(const FullGmm& gmm) const;

  void Read(std::istream &in_stream, bool binary, bool add);
  void Write(std::ostream &out_stream, bool binary) const;
  const GmmFlagsType Flags() const { return flags_; }

 private:
  /// Non-const version of Update that is used internally. This destroys
  /// the accumulators at the end.
  void UpdateInternal(const MleFullGmmOptions &config, GmmFlagsType flags,
                      FullGmm *gmm);

  int32 dim_;
  int32 num_comp_;
  GmmFlagsType flags_;

  Vector<double> occupancy_;
  Matrix<double> mean_accumulator_;
  std::vector<SpMatrix<double> > covariance_accumulator_;

  /// Resizes arrays to this dim. Does not initialize data.
  void ResizeVarAccumulator(int32 nMix, int32 dim);

  /// Returns "augmented" version of flags: e.g. if just updating means, need
  /// weights too.
  static GmmFlagsType AugmentFlags(GmmFlagsType f);

  /// Removes Gaussian component, returns total components after removal
  int32 RemoveComponent(int32 comp);

  // Disallow assignment: make this private.
  MlEstimateFullGmm &operator= (const MlEstimateFullGmm &other);
};

inline void MlEstimateFullGmm::ResizeAccumulators(const FullGmm &gmm,
                                                  GmmFlagsType flags) {
  ResizeAccumulators(gmm.NumGauss(), gmm.Dim(), flags);
}


}  // End namespace kaldi


#endif  // KALDI_GMM_ESTIMATE_FULL_GMM_H_
