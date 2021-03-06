// tied/am-tied-diag-gmm.h

// Copyright 2011 Univ. Erlangen-Nuremberg, Korbinian Riedhammer

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

#ifndef KALDI_TIED_AM_TIED_DIAG_GMM_H_
#define KALDI_TIED_AM_TIED_DIAG_GMM_H_ 1

#include <vector>

#include "base/kaldi-common.h"
#include "gmm/diag-gmm.h"
#include "tied/tied-gmm.h"

namespace kaldi {
/// @defgroup TiedGmm TiedGmm
/// @{
/// kaldi Tied Diagonal Gaussian Mixture Models

class AmTiedDiagGmm {
 public:
  AmTiedDiagGmm() : dim_(0) { }
  ~AmTiedDiagGmm();

  /// Initializes with a single GMM as codebook and initializes num_tied_pdfs
  /// (uniform) tied pdfs
  void Init(const DiagGmm &proto);

  /// Adds a DiagGmm as codebook to the model
  void AddCodebook(const DiagGmm &gmm);

  /// Adds a tied PDF to the model
  void AddTiedPdf(const TiedGmm &tied);

  /// Set the codebook of the designated tied pdf
  void ReplaceCodebook(int32 tied_pdf_index, int32 new_codebook_index);

  /// Replace the designated codebook
  void ReplaceCodebook(int32 codebook_index, const DiagGmm &gmm);

  /// Copies the parameters from another model. Allocates necessary memory.
  void CopyFromAmTiedDiagGmm(const AmTiedDiagGmm &other);

  /// Sets the gconsts for all the PDFs. Returns the total number of Gaussians
  /// over all PDFs that are "invalid" e.g. due to zero weights or variances.
  int32 ComputeGconsts();

  void SetupPerFrameVars(TiedGmmPerFrameVars *per_frame_vars) const;

  /// Needs to be called for each frame prior to any likelihood computation
  void ComputePerFrameVars(const VectorBase<BaseFloat> &data,
                           TiedGmmPerFrameVars *per_frame_vars) const;

  /// Evaluate the target codebook and save the scores to svq
  BaseFloat ComputePerFrameVars(const VectorBase<BaseFloat> &data,
                                int32 codebook_index,
                                Vector<BaseFloat> *svq) const;

  BaseFloat LogLikelihood(int32 tied_pdf_index,
                          TiedGmmPerFrameVars *per_frame_vars) const;

  void Read(std::istream &in_stream, bool binary);
  void Write(std::ostream &out_stream, bool binary) const;

  int32 Dim() const { return dim_; }
  int32 NumCodebooks() const { return densities_.size(); }
  int32 NumTiedPdfs() const { return tied_densities_.size(); }

  /// Accessors
  DiagGmm& GetCodebook(int32 codebook_index);
  const DiagGmm& GetCodebook(int32 codebook_index) const;

  TiedGmm& GetTiedPdf(int32 tied_pdf_index);
  const TiedGmm& GetTiedPdf(int32 tied_pdf_index) const;

  int32 GetCodebookIndexOfTiedPdf(int32 tied_pdf_index) const;

 private:
  std::vector<DiagGmm*> densities_;
  std::vector<TiedGmm*> tied_densities_;
  int32 dim_;

  KALDI_DISALLOW_COPY_AND_ASSIGN(AmTiedDiagGmm);
};

inline BaseFloat AmTiedDiagGmm::LogLikelihood(
                   int32 tied_pdf_index,
                   TiedGmmPerFrameVars *per_frame_vars) const {
  TiedGmm *tied = tied_densities_[tied_pdf_index];

  int32 i = tied->codebook_index();

  // get the svq vector
  Vector<BaseFloat> *svq = &(per_frame_vars->svq[i]);

  // refresh the svq values
  if (!per_frame_vars->current[i]) {
    per_frame_vars->c(i) = ComputePerFrameVars(per_frame_vars->x, i, svq);
    per_frame_vars->current[i] = true;
  }

  return tied->LogLikelihood(per_frame_vars->c(i), *svq);
}

inline DiagGmm& AmTiedDiagGmm::GetCodebook(int32 codebook_index) {
  KALDI_ASSERT((static_cast<size_t>(codebook_index) < densities_.size())
               && (densities_[codebook_index] != NULL));
  return *(densities_[codebook_index]);
}

inline const DiagGmm& AmTiedDiagGmm::GetCodebook(int32 codebook_index) const {
  KALDI_ASSERT((static_cast<size_t>(codebook_index) < densities_.size())
               && (densities_[codebook_index] != NULL));
  return *(densities_[codebook_index]);
}

inline TiedGmm& AmTiedDiagGmm::GetTiedPdf(int32 tied_pdf_index) {
  KALDI_ASSERT((static_cast<size_t>(tied_pdf_index) < tied_densities_.size())
               && (tied_densities_[tied_pdf_index] != NULL));
  return *(tied_densities_[tied_pdf_index]);
}

inline const TiedGmm& AmTiedDiagGmm::GetTiedPdf(int32 tied_pdf_index) const {
  KALDI_ASSERT((static_cast<size_t>(tied_pdf_index) < tied_densities_.size())
               && (tied_densities_[tied_pdf_index] != NULL));
  return *(tied_densities_[tied_pdf_index]);
}

inline int32 AmTiedDiagGmm::GetCodebookIndexOfTiedPdf(int32 tied_pdf_index)
  const {
  KALDI_ASSERT((static_cast<size_t>(tied_pdf_index) < tied_densities_.size())
               && (tied_densities_[tied_pdf_index] != NULL));
  return tied_densities_[tied_pdf_index]->codebook_index();
}

}  // namespace kaldi

/// @} TiedGmm
#endif  // KALDI_TIED_AM_TIED_DIAG_GMM_H_
