// latbin/lattice-combine.cc

// Copyright 2012  Arnab Ghoshal

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

// This program is for system combination using MBR decoding as described in:
// "Minimum Bayes Risk decoding and system combination based on a recursion for
// edit distance", Haihua Xu, Daniel Povey, Lidia Mangu and Jie Zhu, Computer
// Speech and Language, 2011. However, instead of averaging the posteriors, as
// described in the paper, this removes the total backward probability from the
// individual lattices being combined and outputs the union of them. The output
// should be used with lattice-mbr-decode (without any acoustic or LM scaling)
// or with lattice-to-ctm-conf with --decode-mbr=true (also without any scaling)

// IMPORTANT CAVEAT: the total backward probability (which is a float) is
// removed from value1_ of arc weight. So graph scores are no longer correct
// but instead only the combined acoustic and graph scores are valid. So no
// acoustic or LM scaling should be done with the output of this program.

#include <string>
using std::string;
#include <vector>
using std::vector;

#include "util/common-utils.h"
#include "lat/kaldi-lattice.h"
#include "lat/kws-functions.h"
#include "lat/sausages.h"

namespace kaldi {

// This removes the total weight from a CompactLattice. Since the total backward
// score is in log likelihood domain, and the lattice weights are in negative
// log likelihood domain, the total weight is *added* to the weight of the final
// states. This is equivalent to dividing the probability of each path by the
// total probability over all paths. There is an additional weight to control
// the relative contribution of individual lattices, which can be used as an
// exponent (i.e. multiply the log scores) or as a multiplicative weight (i.e.
// log of the weight is subtracted from the total backward score).
bool CompactLatticeNormalize(CompactLattice *clat, BaseFloat weight,
                             bool exp_weights) {
  if (weight <= 0.0) {
    KALDI_WARN << "Weights must be positive; found: " << weight;
    return false;
  }

  if (clat->Properties(fst::kTopSorted, false) == 0) {
    if (fst::TopSort(clat) == false) {
      KALDI_WARN << "Cycles detected in lattice: cannot normalize.";
      return false;
    }
  }

  // If exp_weights = true, multiply the log AM & LM scores.
  if (exp_weights)
    fst::ScaleLattice(fst::LatticeScale(weight, weight), clat);
  
  vector<double> beta;
  if (!ComputeCompactLatticeBetas(*clat, &beta)) {
    KALDI_WARN << "Failed to compute backward probabilities on lattice.";
    return false;
  }

  typedef CompactLattice::Arc::StateId StateId;
  StateId start = clat->Start();  // Should be 0
  BaseFloat total_backward_cost = beta[start];

  // If exp_weights = false, add to the log AM & LM scores.
  if (!exp_weights)
    total_backward_cost -= std::log(weight);
  
  for (fst::StateIterator<CompactLattice> sit(*clat); !sit.Done(); sit.Next()) {
    CompactLatticeWeight f = clat->Final(sit.Value());
    LatticeWeight w = f.Weight();
    w.SetValue1(w.Value1() + total_backward_cost);
    f.SetWeight(w);
    clat->SetFinal(sit.Value(), f);
  }
  return true;
}

// This is a wrapper for SplitStringToFloats, with added checks to make sure
// the weights are valid probabilities.
void SplitStringToWeights(const string &full, const char *delim,
                          vector<BaseFloat> *out) {
  vector<BaseFloat> tmp;
  SplitStringToFloats(full, delim, true /*omit empty strings*/, &tmp);
  if (tmp.size() != out->size()) {
    KALDI_WARN << "Expecting " << out->size() << " weights, found " << tmp.size()
               << ": using uniform weights.";
    return;
  }
  BaseFloat sum = 0;
  for (vector<BaseFloat>::const_iterator itr = tmp.begin();
       itr != tmp.end(); ++itr) {
    if (*itr < 0.0) {
      KALDI_WARN << "Cannot use negative weight: " << *itr << "; input string: "
                 << full << "\n\tUsing uniform weights.";
      return;
    }
    sum += (*itr);
  }
  if (sum != 1.0) {
    KALDI_WARN << "Weights sum to " << sum << " instead of 1: renormalizing";
    for (vector<BaseFloat>::iterator itr = tmp.begin();
         itr != tmp.end(); ++itr)
      (*itr) /= sum;
  }
  out->swap(tmp);
}

}  // end namespace kaldi


int main(int argc, char *argv[]) {
  try {
    using namespace kaldi;
    typedef kaldi::int32 int32;

    const char *usage =
        "Combine lattices generated by different systems by removing the total\n"
        "cost of all paths (backward cost) from individual lattices and doing\n"
        "a union of the reweighted lattices.  Note: the acoustic and LM scales\n"
        "that this program applies are not removed before outputting the lattices.\n"
        "Intended for use in system combination prior to MBR decoding, see comments\n"
        "in code.\n"
        "Usage: lattice-combine [options] lattice-rspecifier1 lattice-rspecifier2"
        " [lattice-rspecifier3 ... ] lattice-wspecifier\n";

    ParseOptions po(usage);
    BaseFloat acoustic_scale = 1.0, inv_acoustic_scale = 1.0, lm_scale = 1.0;
    string weight_str;
    bool exp_weights = false;
    po.Register("acoustic-scale", &acoustic_scale, "Scaling factor for "
                "acoustic likelihoods");
    po.Register("inv-acoustic-scale", &inv_acoustic_scale, "An alternative way "
                "of setting the acoustic scale: you can set its inverse.");
    po.Register("lm-scale", &lm_scale, "Scaling factor for language model "
                "probabilities");
    po.Register("lat-weights", &weight_str, "Colon-separated list of weights "
                "for each lattice that sum to 1");
    // The lattice weights can be used either as exponents or as multiplicative
    // weights, determined by the exp_weights flag.
    po.Register("exp-weights", &exp_weights, "If true, lattice weights are "
                "exponents, else they are multiplicative");
    po.Read(argc, argv);

    KALDI_ASSERT(acoustic_scale == 1.0 || inv_acoustic_scale == 1.0);
    if (inv_acoustic_scale != 1.0)
      acoustic_scale = 1.0 / inv_acoustic_scale;
    
    
    int32 num_args = po.NumArgs();
    if (num_args < 3) {
      po.PrintUsage();
      exit(1);
    }

    string lats_rspecifier1 = po.GetArg(1),
        lats_wspecifier = po.GetArg(num_args);
    
    // Output lattice
    CompactLatticeWriter clat_writer(lats_wspecifier);

    // Input lattices
    SequentialCompactLatticeReader clat_reader1(lats_rspecifier1);
    vector<RandomAccessCompactLatticeReader*> clat_reader_vec(
        num_args-2, static_cast<RandomAccessCompactLatticeReader*>(NULL));
    vector<string> clat_rspec_vec(num_args-2);
    for (int32 i = 2; i < num_args; ++i) {
      clat_reader_vec[i-2] = new RandomAccessCompactLatticeReader(po.GetArg(i));
      clat_rspec_vec[i-2] = po.GetArg(i);
    }

    vector<BaseFloat> lat_weights(num_args-1, 1.0/(num_args-1));
    if (!weight_str.empty())
      SplitStringToWeights(weight_str, ":", &lat_weights);

    int32 n_utts = 0, n_total_lats = 0, n_success = 0, n_missing = 0,
        n_other_errors = 0;
    vector< vector<double> > lat_scale = fst::LatticeScale(lm_scale,
                                                           acoustic_scale);

    for (; !clat_reader1.Done(); clat_reader1.Next()) {
      std::string key = clat_reader1.Key();
      CompactLattice clat1 = clat_reader1.Value();
      clat_reader1.FreeCurrent();
      n_utts++;
      n_total_lats++;
      fst::ScaleLattice(lat_scale, &clat1);
      bool success = CompactLatticeNormalize(&clat1, lat_weights[0],
                                             exp_weights);
      if (!success) {
        KALDI_WARN << "Could not normalize lattice for system 1, utterance: "
                   << key;
        n_other_errors++;
        continue;
      }

      for (int32 i = 0; i < num_args-2; ++i) {
        if (clat_reader_vec[i]->HasKey(key)) {
          CompactLattice clat2 = clat_reader_vec[i]->Value(key);
          n_total_lats++;
          fst::ScaleLattice(lat_scale, &clat2);
          success = CompactLatticeNormalize(&clat2, lat_weights[i+1], exp_weights);
          if (!success) {
            KALDI_WARN << "Could not normalize lattice for system "<< (i + 2)
                       << ", utterance: " << key;
            n_other_errors++;
            continue;
          }
          fst::Union(&clat1, clat2);
        } else {
          KALDI_WARN << "No lattice found for utterance " << key << " for "
                     << "system " << (i + 2) << ", rspecifier: "
                     << clat_rspec_vec[i];
          n_missing++;
        }
      }

      clat_writer.Write(key, clat1);
      n_success++;
    }

    KALDI_LOG << "Processed " << n_utts << " utterances: with a total of "
              << n_total_lats << " lattices across " << (num_args-1)
              << " different systems";
    KALDI_LOG << "Produced output for " << n_success << " utterances; "
              << n_missing << " total missing lattices and " << n_other_errors
              << " total lattices had errors in processing.";
    DeletePointers(&clat_reader_vec);
    // The success code we choose is that at least one lattice was output,
    // and more lattices were "all there" than had at least one system missing.
    return (n_success != 0 && n_missing < (n_success - n_missing) ? 0 : 1);
  } catch(const std::exception &e) {
    std::cerr << e.what();
    return -1;
  }
}
