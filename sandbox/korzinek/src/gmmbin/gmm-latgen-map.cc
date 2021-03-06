// gmmbin/gmm-latgen-map.cc

// Copyright 2012  Neha Agrawal, Cisco Systems;
//                 Johns Hopkins University (author: Daniel Povey)

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

#include <string>
#include <vector>

#include "base/kaldi-common.h"
#include "util/common-utils.h"
#include "gmm/am-diag-gmm.h"
#include "gmm/mle-am-diag-gmm.h"
#include "hmm/transition-model.h"
#include "transform/fmllr-diag-gmm.h"
#include "fstext/fstext-lib.h"
#include "decoder/lattice-faster-decoder.h"
#include "gmm/decodable-am-diag-gmm.h"
#include "util/timer.h"
#include "lat/kaldi-lattice.h" // for {Compact}LatticeArc

using fst::SymbolTable;
using fst::VectorFst;
using fst::StdArc;
using kaldi::BaseFloat;
using std::string;
using std::vector;
using kaldi::LatticeWeight;
using kaldi::LatticeArc;

namespace kaldi {
// Takes care of output.  Returns true on success.
bool DecodeUtterance(LatticeFasterDecoder &decoder, // not const but is really an input.
                     DecodableInterface &decodable, // not const but is really an input.
                     const fst::SymbolTable *word_syms,
                     std::string utt,
                     double acoustic_scale,
                     bool determinize,
                     bool allow_partial,
                     Int32VectorWriter *alignment_writer,
                     Int32VectorWriter *words_writer,
                     CompactLatticeWriter *compact_lattice_writer,
                     LatticeWriter *lattice_writer,
                     double *like_ptr) { // puts utterance's like in like_ptr on success.
  using fst::VectorFst;

  if (!decoder.Decode(&decodable)) {
    KALDI_WARN << "Failed to decode file " << utt;
    return false;
  }
  if (!decoder.ReachedFinal()) {
    if (allow_partial) {
      KALDI_WARN << "Outputting partial output for utterance " << utt
                 << " since no final-state reached\n";
    } else {
      KALDI_WARN << "Not producing output for utterance " << utt
                 << " since no final-state reached and "
                 << "--allow-partial=false.\n";
      return false;
    }
  }
  double likelihood;
  LatticeWeight weight;
  int32 num_frames;
  { // First do some stuff with word-level traceback...
    VectorFst<LatticeArc> decoded;
    if (!decoder.GetBestPath(&decoded))
      // Shouldn't really reach this point as already checked success.
      KALDI_ERR << "Failed to get traceback for utterance " << utt;

    std::vector<int32> alignment;
    std::vector<int32> words;
    GetLinearSymbolSequence(decoded, &alignment, &words, &weight);
    num_frames = alignment.size();
    if (words_writer->IsOpen())
      words_writer->Write(utt, words);
    if (alignment_writer->IsOpen())
      alignment_writer->Write(utt, alignment);
    if (word_syms != NULL) {
      std::cerr << utt << ' ';
      for (size_t i = 0; i < words.size(); i++) {
        std::string s = word_syms->Find(words[i]);
        if (s == "")
          KALDI_ERR << "Word-id " << words[i] <<" not in symbol table.";
        std::cerr << s << ' ';
      }
      std::cerr << '\n';
    }
    likelihood = -(weight.Value1() + weight.Value2());
  }

  if (determinize) {
    CompactLattice fst;
    if (!decoder.GetLattice(&fst))
      KALDI_ERR << "Unexpected problem getting lattice for utterance "
                << utt;
    if (acoustic_scale != 0.0) // We'll write the lattice without acoustic scaling
      fst::ScaleLattice(fst::AcousticLatticeScale(1.0 / acoustic_scale), &fst);
    compact_lattice_writer->Write(utt, fst);
  } else {
    Lattice fst;
    if (!decoder.GetRawLattice(&fst))
      KALDI_ERR << "Unexpected problem getting lattice for utterance "
                << utt;
    fst::Connect(&fst); // Will get rid of this later... shouldn't have any
    // disconnected states there, but we seem to.
    if (acoustic_scale != 0.0) // We'll write the lattice without acoustic scaling
      fst::ScaleLattice(fst::AcousticLatticeScale(1.0 / acoustic_scale), &fst);
    lattice_writer->Write(utt, fst);
  }
  KALDI_LOG << "Log-like per frame for utterance " << utt << " is "
            << (likelihood / num_frames) << " over "
            << num_frames << " frames.";
  KALDI_VLOG(2) << "Cost for utterance " << utt << " is "
                << weight.Value1() << " + " << weight.Value2();
  *like_ptr = likelihood;
  return true;
}

}

int main(int argc, char *argv[]) {
  try {
    using namespace kaldi;
    typedef kaldi::int32 int32;

    const char *usage = "Decode features using GMM-based model.  Note: the input\n"
        "<gmms-rspecifier> will typically be piped in from gmm-est-map.\n"
        "Note: <model-in> is only needed for the transition-model, which isn't\n"
        "included in <gmms-rspecifier>.\n"
        "\n"
        "Usage: gmm-latgen-map [options] <model-in> "
        "<gmms-rspecifier> <fsts-rxfilename|fsts-rspecifier> <features-rspecifier> "
        "<lattice-wspecifier> [ <words-wspecifier> [ <alignments-wspecifier> ] ]\n";

    ParseOptions po(usage);
    bool binary = true;
    bool allow_partial = true;
    BaseFloat acoustic_scale = 0.1;
        
    std::string word_syms_filename, utt2spk_rspecifier;
    LatticeFasterDecoderConfig decoder_opts;
    decoder_opts.Register(&po);
    po.Register("utt2spk", &utt2spk_rspecifier, "rspecifier for utterance to "
                "speaker map");
    po.Register("binary", &binary, "Write output in binary mode");
    po.Register("acoustic-scale", &acoustic_scale,
                "Scaling factor for acoustic likelihoods");
    po.Register("word-symbol-table", &word_syms_filename,
                "Symbol table for words [for debug output]");
    po.Register("allow-partial", &allow_partial,
                "Produce output even when final state was not reached");
    po.Read(argc, argv);

    if (po.NumArgs() < 5 || po.NumArgs() > 7) {
      po.PrintUsage();
      exit(1);
    }

    std::string model_in_filename = po.GetArg(1),
        gmms_rspecifier = po.GetArg(2),
        fst_in_filename = po.GetArg(3),
        feature_rspecifier = po.GetArg(4),
        lattice_wspecifier = po.GetArg(5),
        words_wspecifier = po.GetOptArg(6),
        alignment_wspecifier = po.GetOptArg(7);

    TransitionModel trans_model;
    {
      bool binary_read;
      Input is(model_in_filename, &binary_read);
      trans_model.Read(is.Stream(), binary_read);
    }
    RandomAccessMapAmDiagGmmReaderMapped gmms_reader(gmms_rspecifier,
                                                     utt2spk_rspecifier);

    Int32VectorWriter words_writer(words_wspecifier);
    Int32VectorWriter alignment_writer(alignment_wspecifier);


    bool determinize = decoder_opts.determinize_lattice;
    if (!determinize)
      KALDI_WARN << "determinize is set to FASLE ...";
    CompactLatticeWriter compact_lattice_writer;
    LatticeWriter lattice_writer;

    if (lattice_wspecifier != "") {
      if (! (determinize ? compact_lattice_writer.Open(lattice_wspecifier)
             : lattice_writer.Open(lattice_wspecifier)))
        KALDI_ERR << "Could not open table for writing lattices: "
                  << lattice_wspecifier;
    }
        
    fst::SymbolTable *word_syms = NULL;
    if (word_syms_filename != "") {
      word_syms = fst::SymbolTable::ReadText(word_syms_filename);
      if (!word_syms) {
        KALDI_ERR << "Could not read symbol table from file "
                  << word_syms_filename;
      }
    }

    BaseFloat tot_like = 0.0;
    kaldi::int64 frame_count = 0;
    int num_success = 0, num_fail = 0;
    Timer timer;

    if (ClassifyRspecifier(fst_in_filename, NULL, NULL) == kNoRspecifier) {
      // Input FST is just one FST, not a table of FSTs.
      VectorFst<StdArc> *decode_fst = fst::ReadFstKaldi(fst_in_filename);

      SequentialBaseFloatMatrixReader feature_reader(feature_rspecifier);
      for (; !feature_reader.Done(); feature_reader.Next()) {
        string utt = feature_reader.Key();

        if (!gmms_reader.HasKey(utt)) {
          KALDI_WARN << "Utterance " << utt
                     << " has no corresponding MAP model skipping this utterance.";
          num_fail++;
          continue;
        }
        AmDiagGmm am_gmm;
        am_gmm.CopyFromAmDiagGmm(gmms_reader.Value(utt));

        Matrix<BaseFloat> features(feature_reader.Value());
        feature_reader.FreeCurrent();
        if (features.NumRows() == 0) {
          KALDI_WARN << "Zero-length utterance: " << utt;
          num_fail++;
          continue;
        }

        LatticeFasterDecoder decoder(*decode_fst, decoder_opts);
        kaldi::DecodableAmDiagGmmScaled gmm_decodable(am_gmm, trans_model,
                                                      features,
                                                      acoustic_scale);
        double like;
        if (DecodeUtterance(decoder, gmm_decodable, word_syms, utt, acoustic_scale,
                            determinize, allow_partial, &alignment_writer, &words_writer,
                            &compact_lattice_writer, &lattice_writer, &like)) {
          tot_like += like;
          frame_count += features.NumRows();
          num_success++;
        } else num_fail++;
      }  // end looping over all utterances
    }else{
      RandomAccessTableReader<fst::VectorFstHolder> fst_reader(fst_in_filename);
      SequentialBaseFloatMatrixReader feature_reader(feature_rspecifier);
      for (; !feature_reader.Done(); feature_reader.Next()) {
        string utt = feature_reader.Key();

        if (!fst_reader.HasKey(utt)) {
          KALDI_WARN << "Utterance " << utt << " has no corresponding FST"
                     << "skipping this utterance.";
          num_fail++;
          continue;
        }

        if (!gmms_reader.HasKey(utt)) {
          KALDI_WARN << "Utterance " << utt
                     << " has no corresponding MAP model skipping this utterance.";
          num_fail++;
          continue;
        }
        AmDiagGmm am_gmm;
        am_gmm.CopyFromAmDiagGmm(gmms_reader.Value(utt));
        
        Matrix<BaseFloat> features(feature_reader.Value());
        feature_reader.FreeCurrent();
        if (features.NumRows() == 0) {
          KALDI_WARN << "Zero-length utterance: " << utt;
          num_fail++;
          continue;
        }

        LatticeFasterDecoder decoder(fst_reader.Value(utt), decoder_opts);
        kaldi::DecodableAmDiagGmmScaled gmm_decodable(am_gmm, trans_model,
                                                      features,
                                                      acoustic_scale);
        double like;
        if (DecodeUtterance(decoder, gmm_decodable, word_syms, utt, acoustic_scale,
                            determinize, allow_partial, &alignment_writer, &words_writer,
                            &compact_lattice_writer, &lattice_writer, &like)) {
          tot_like += like;
          frame_count += features.NumRows();
          num_success++;
        } else num_fail++;
      }  // end looping over all utterances
    }
    KALDI_LOG << "Average log-likelihood per frame is " 
              << (tot_like / frame_count) << " over " << frame_count << " frames.";

    double elapsed = timer.Elapsed();
    KALDI_LOG << "Time taken [excluding initialization] " << elapsed
              << "s: real-time factor assuming 100 frames/sec is "
              << (elapsed * 100.0 / frame_count);
    KALDI_LOG << "Done " << num_success << " utterances, failed for "
              << num_fail;

    if (word_syms) delete word_syms;
    return (num_success != 0 ? 0 : 1);
  }
  catch(const std::exception& e) {
    std::cerr << e.what();
    return -1;
  }
}
