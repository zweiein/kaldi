// fb/simple-forward.h

#ifndef KALDI_FB_TEST_UTILS_H_
#define KALDI_FB_TEST_UTILS_H_
#include "itf/decodable-itf.h"

#include <vector>

namespace kaldi {
namespace unittest {

class DummyDecodable : public DecodableInterface {
 private:
  int32 num_states_;
  int32 num_frames_;
  std::vector<BaseFloat> observations_;

 public:
  DummyDecodable() : DecodableInterface(), num_states_(0), num_frames_(-1) { }

  void Init(int32 num_states, int32 num_frames,
            const std::vector<BaseFloat>& observations) {
    KALDI_ASSERT(observations.size() == num_states * num_frames);
    num_states_ = num_states;
    num_frames_ = num_frames;
    observations_ = observations;
  }

  virtual BaseFloat LogLikelihood(int32 frame, int32 state_index) {
    KALDI_ASSERT(frame >= 0 && frame < NumFramesReady());
    KALDI_ASSERT(state_index > 0 && state_index <= NumIndices());
    return observations_[frame * num_states_ + state_index - 1];
  }

  virtual int32 NumFramesReady() const { return num_frames_; }

  virtual int32 NumIndices() const { return num_states_; }

  virtual bool IsLastFrame(int32 frame) const {
    KALDI_ASSERT(frame < NumFramesReady());
    return (frame == NumFramesReady() - 1);
  }
};

void CreateWFST_DummyState(fst::VectorFst<fst::StdArc>* fst, bool final) {
  fst->DeleteStates();
  fst->AddState();
  fst->SetStart(0);
  if (final) {
    fst->SetFinal(0, -log(1.0));
  }
}

// An arbitrary WFST with two input/output symbols + epsilon transitions.
// The WFST contains non-epsilon loops and it is non-stochastic.
void CreateWFST_Arbitrary(fst::VectorFst<fst::StdArc>* fst) {
  fst->DeleteStates();

  fst->AddState();  // State 0
  fst->AddState();  // State 1
  fst->AddState();  // State 2
  fst->AddState();  // State 3

  fst->AddArc(0, fst::StdArc(1, 1, -log(1.0), 0));
  fst->AddArc(0, fst::StdArc(2, 2, -log(1.0), 0));
  fst->AddArc(0, fst::StdArc(0, 0, -log(1.0), 1));
  fst->AddArc(0, fst::StdArc(1, 1, -log(1.0), 1));
  fst->AddArc(0, fst::StdArc(2, 2, -log(1.0), 1));
  fst->AddArc(0, fst::StdArc(0, 0, -log(0.5), 2));

  fst->AddArc(1, fst::StdArc(2, 2, -log(0.5), 1));
  fst->AddArc(1, fst::StdArc(0, 0, -log(0.5), 2));
  fst->AddArc(1, fst::StdArc(1, 1, -log(1.0), 2));
  fst->AddArc(1, fst::StdArc(0, 0, -log(0.5), 3));

  fst->AddArc(2, fst::StdArc(0, 0, -log(1.0), 3));
  fst->AddArc(2, fst::StdArc(1, 1, -log(1.0), 3));
  fst->AddArc(2, fst::StdArc(2, 2, -log(1.0), 3));

  fst->AddArc(3, fst::StdArc(1, 1, -log(0.5), 1));
  fst->AddArc(3, fst::StdArc(2, 2, -log(0.5), 1));

  fst->SetStart(0);
  fst->SetFinal(1, -log(1.0));
  fst->SetFinal(3, -log(0.5));
}

void CreateObservation_Empty(DummyDecodable* decodable) {
  decodable->Init(2, 0, std::vector<BaseFloat>());
}

void CreateObservation_Arbitrary(DummyDecodable* decodable) {
  std::vector<BaseFloat> observation(2 * 4);
  // t = 1
  observation[0] = log(0.5);
  observation[1] = log(0.5);
  // t = 2
  observation[2] = log(0.0);
  observation[3] = log(1.0);
  // t = 3
  observation[4] = log(3.0);
  observation[5] = log(0.0);
  // t = 4
  observation[6] = log(1.3);
  observation[7] = log(0.7);
  decodable->Init(2, 4, observation);
}

}  // namespace unittest
}  // namespace kaldi

#endif  // KALDI_FB_TEST_UTILS_H_
