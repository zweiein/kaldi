// featbin/compute-plp-feats.cc

// Copyright 2009-2011  Microsoft Corporation

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
#include "feat/feature-plp.h"
#include "feat/wave-reader.h"


int main(int argc, char *argv[]) {
  try {
    using namespace kaldi;
    const char *usage =
        "Create PLP feature files.\n"
        "Usage:  compute-plp-feats [options...] <wav-rspecifier> <feats-wspecifier>\n";

    // construct all the global objects
    ParseOptions po(usage);
    PlpOptions plp_opts;
    bool subtract_mean = false;
    BaseFloat vtln_warp = 1.0;
    std::string vtln_map_rspecifier;
    std::string utt2spk_rspecifier;
    int32 channel = -1;
    // Define defaults for gobal options
    std::string output_format = "kaldi";

    // Register the options
    po.Register("output-format", &output_format, "Format of the output files [kaldi, htk]");
    po.Register("subtract-mean", &subtract_mean, "Subtract mean of each feature file [CMS]. ");
    po.Register("vtln-warp", &vtln_warp, "Vtln warp factor (only applicable if vtln-map not specified)");    po.Register("vtln-warp", &vtln_warp, "Vtln warp factor");
    po.Register("vtln-map", &vtln_map_rspecifier, "Map from utterance or speaker-id to vtln warp factor (rspecifier)");
    po.Register("utt2spk", &utt2spk_rspecifier, "Utterance to speaker-id map (if doing VTLN and you have warps per speaker)");
    po.Register("channel", &channel, "Channel to extract (-1 -> expect mono, 0 -> left, 1 -> right)");
    // Register the option struct
    plp_opts.Register(&po);

    // OPTION PARSING ..........................................................
    //

    // parse options (+filling the registered variables)
    po.Read(argc, argv);

    if (po.NumArgs() != 2) {
      po.PrintUsage();
      exit(1);
    }

    std::string wav_rspecifier = po.GetArg(1);

    std::string output_wspecifier = po.GetArg(2);

    Plp plp(plp_opts);

    SequentialTableReader<WaveHolder> reader(wav_rspecifier);
    BaseFloatMatrixWriter kaldi_writer;  // typedef to TableWriter<something>.
    TableWriter<HtkMatrixHolder> htk_writer;

    RandomAccessTokenReader utt2spk_reader;  // relates to VTLN.
    if (utt2spk_rspecifier != "") {
      KALDI_ASSERT(vtln_map_rspecifier != "" && "the utt2spk option is only "
                   "needed if the vtln-map option is used.");
      if (!utt2spk_reader.Open(utt2spk_rspecifier))
        KALDI_ERR << "Error opening utt2spk object.";
    }
    RandomAccessBaseFloatReader vtln_map_reader;  // relates to VTLN.
    if (vtln_map_rspecifier != "") {
      if (!vtln_map_reader.Open(vtln_map_rspecifier))
        KALDI_ERR << "Error opening vtln-map object.";
    }

    if (output_format == "kaldi") {
      if (!kaldi_writer.Open(output_wspecifier))
        KALDI_ERR << "Could not initialize output with wspecifier "
                  << output_wspecifier;
    } else if (output_format == "htk") {
      if (!htk_writer.Open(output_wspecifier))
        KALDI_ERR << "Could not initialize output with wspecifier "
                  << output_wspecifier;
    } else {
      KALDI_ERR << "Invalid output_format string " << output_format;
    }

    int32 num_utts = 0, num_success = 0;
    for (; !reader.Done(); reader.Next()) {
      num_utts++;
      std::string utt = reader.Key();
      const WaveData &wave_data = reader.Value();
      int32 num_chan = wave_data.Data().NumRows(), this_chan = channel;
      {  // This block works out the channel (0=left, 1=right...)
        KALDI_ASSERT(num_chan > 0);  // should have been caught in
        // reading code if no channels.
        if (channel == -1) {
          this_chan = 0;
          if (num_chan != 1)
            KALDI_WARN << "Channel not specified but you have data with "
                       << num_chan  << " channels; defaulting to zero";
        } else {
          if (this_chan >= num_chan) {
            KALDI_WARN << "File with id " << utt << " has "
                       << num_chan << " channels but you specified channel "
                       << channel << ", producing no output.";
            continue;
          }
        }
      }
      BaseFloat vtln_warp_local;  // Work out VTLN warp factor.
      if (vtln_map_rspecifier != "") {
        std::string utt_or_spk;
        if (utt2spk_rspecifier != "") {
          if (!utt2spk_reader.HasKey(utt)) {
            KALDI_WARN << "No utt2spk entry for utterance-id " << utt;
            continue;
          }
          utt_or_spk = utt2spk_reader.Value(utt);
        } else utt_or_spk = utt;
        if (!vtln_map_reader.HasKey(utt_or_spk)) {
          KALDI_WARN << "No vtln-map entry for utterance-id (or speaker-id) "
                     << utt_or_spk;
          continue;
        }
        vtln_warp_local = vtln_map_reader.Value(utt_or_spk);
      } else {
        vtln_warp_local = vtln_warp;
      }
      if (plp_opts.frame_opts.samp_freq != wave_data.SampFreq())
        KALDI_ERR << "Sample frequency mismatch: you specified "
                  << plp_opts.frame_opts.samp_freq << " but data has "
                  << wave_data.SampFreq() << " (use --sample-frequency option)";

      SubVector<BaseFloat> waveform(wave_data.Data(), this_chan);
      Matrix<BaseFloat> features;
      try {
        plp.Compute(waveform, vtln_warp_local, &features, NULL);
      } catch (...) {
        KALDI_WARN << "Failed to compute features for utterance "
                   << utt;
        continue;
      }
      if (subtract_mean) {
        Vector<BaseFloat> mean(features.NumCols());
        mean.AddRowSumMat(features);
        mean.Scale(1.0 / features.NumRows());
        for (size_t i = 0; i < features.NumRows(); i++)
          features.Row(i).AddVec(-1.0, mean);
      }
      if (output_format == "kaldi") {
        if (!kaldi_writer.Write(utt, features))
          KALDI_ERR << "Write error writing Kaldi features.";
      } else {
        std::pair<Matrix<BaseFloat>, HtkHeader> p;
        p.first.Resize(features.NumRows(), features.NumCols());
        p.first.CopyFromMat(features);
        HtkHeader header = {
          features.NumRows(),
          100000,  //10ms shift
          sizeof(float)*features.NumCols(),
          013 | // PLP
          020000 // C0 [no option currently to use energy in PLP.
        };
        p.second = header;
        if (!htk_writer.Write(utt, p))
          KALDI_ERR << "Write error writing HTK features.";
      }
      if(num_utts % 10 == 0)
        KALDI_LOG << "Processed " << num_utts << " utterances";
      KALDI_VLOG(2) << "Processed features for key " << utt;
      num_success++;
    }
    KALDI_LOG << " Done " << num_success << " out of " << num_utts
              << " utterances.";
    return 0;
  } catch(const std::exception& e) {
    std::cerr << e.what();
    return -1;
  }
}

