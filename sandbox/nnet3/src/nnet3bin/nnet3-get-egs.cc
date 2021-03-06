// nnet3bin/nnet3-get-egs.cc

// Copyright 2012-2015  Johns Hopkins University (author:  Daniel Povey)
//                2014  Vimal Manohar

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

#include <sstream>

#include "base/kaldi-common.h"
#include "util/common-utils.h"
#include "hmm/transition-model.h"
#include "hmm/posterior.h"
#include "nnet3/nnet-example.h"

namespace kaldi {
namespace nnet3 {


static void ProcessFile(const MatrixBase<BaseFloat> &feats,
                        const MatrixBase<BaseFloat> *ivector_feats,
                        const Posterior &pdf_post,
                        const std::string &utt_id,
                        bool compress,
                        int32 left_context,
                        int32 right_context,
                        int32 num_frames,
                        int64 *num_frames_written,
                        int64 *num_egs_written,
                        NnetExampleWriter *example_writer) {
  KALDI_ASSERT(feats.NumRows() == static_cast<int32>(pdf_post.size()));
  
  for (int32 t = 0; t < feats.NumRows(); t += num_frames) {
    int32 this_num_frames = std::min(num_frames,
                                     feats.NumRows() - t);

    int32 tot_frames = left_context + this_num_frames + right_context;

    Matrix<BaseFloat> input_frames(tot_frames, feats.NumCols());
    
    // Set up "input_frames".
    for (int32 j = -left_context; j < this_num_frames + right_context; j++) {
      int32 t2 = j + t;
      if (t2 < 0) t2 = 0;
      if (t2 >= feats.NumRows()) t2 = feats.NumRows() - 1;
      SubVector<BaseFloat> src(feats, t2),
          dest(input_frames, j + left_context);
      dest.CopyFromVec(src);
    }

    NnetExample eg;
    
    // call the regular input "input".
    eg.input.push_back(Feature("input", t - left_context,
                               input_frames));

    // get the labels.
    Posterior labels(num_frames);
    for (int32 i = 0; i < num_frames; i++)
      labels[i] = pdf_post[t + i];
    eg.supervision.push_back(Supervision("output", t, labels));

    // if applicable, add the iVector feature.
    if (ivector_feats != NULL) {
      // try to get closest frame to middle of window to get
      // a representative iVector.
      int32 closest_frame = t + (num_frames / 2);
      KALDI_ASSERT(ivector_feats->NumRows() > 0);
      if (closest_frame >= ivector_feats->NumRows())
        closest_frame = ivector_feats->NumRows() - 1;
      Matrix<BaseFloat> ivector(1, ivector_feats->NumCols());
      ivector.Row(0).CopyFromVec(ivector_feats->Row(closest_frame));
      eg.input.push_back(Feature("ivector", 0, ivector));
    }
    if (compress)
      eg.Compress();
      
    std::ostringstream os;
    os << utt_id << "-" << t;

    std::string key = os.str(); // key is <utt_id>-<frame_id>

    *num_frames_written += this_num_frames;
    *num_egs_written += 1;

    example_writer->Write(key, eg);
  }
}


} // namespace nnet2
} // namespace kaldi

int main(int argc, char *argv[]) {
  try {
    using namespace kaldi;
    using namespace kaldi::nnet3;
    typedef kaldi::int32 int32;
    typedef kaldi::int64 int64;

    const char *usage =
        "Get frame-by-frame examples of data for neural network training.\n"
        "Essentially this is a format change from features and posteriors\n"
        "into a special frame-by-frame format.  This program handles the\n"
        "common case where you have some input features, possibly some\n"
        "iVectors, and one set of labels.  If people in future want to\n"
        "do different things they may have to extend this program or create\n"
        "different versions of it for different tasks.\n"
        "\n"
        "Usage:  nnet3-get-egs [options] <features-rspecifier> "
        "<pdf-post-rspecifier> <egs-out>\n"
        "\n"
        "An example [where $feats expands to the actual features]:\n"
        "nnet-get-egs --left-context=12 --right-context=9 --num-frames=8 \"$feats\"\\\n"
        "\"ark:gunzip -c exp/nnet/ali.1.gz | ali-to-pdf exp/nnet/1.nnet ark:- ark:- | ali-to-post ark:- ark:- |\" \\\n"
        "   ark:- \n";
        

    bool compress = true;
    int32 left_context = 0, right_context = 0,
        num_frames = 1, length_tolerance = 100;
        
    std::string ivector_rspecifier;
    
    ParseOptions po(usage);
    po.Register("compress", &compress, "If true, write egs in "
                "compressed format.");
    po.Register("left-context", &left_context, "Number of frames of left "
                "context the neural net requires.");
    po.Register("right-context", &right_context, "Number of frames of right "
                "context the neural net requires.");
    po.Register("num-frames", &num_frames, "Number of frames with labels "
                "that each example contains.");
    po.Register("ivectors", &ivector_rspecifier, "Rspecifier of ivector "
                "features, as matrix.");
    po.Register("length-tolerance", &length_tolerance, "Tolerance for "
                "difference in num-frames between feat and ivector matrices");
    
    po.Read(argc, argv);

    if (po.NumArgs() != 3) {
      po.PrintUsage();
      exit(1);
    }

    std::string feature_rspecifier = po.GetArg(1),
        pdf_post_rspecifier = po.GetArg(2),
        examples_wspecifier = po.GetArg(3);

    // Read in all the training files.
    SequentialBaseFloatMatrixReader feat_reader(feature_rspecifier);
    RandomAccessPosteriorReader pdf_post_reader(pdf_post_rspecifier);
    NnetExampleWriter example_writer(examples_wspecifier);
    RandomAccessBaseFloatMatrixReader ivector_reader(ivector_rspecifier);
    
    int32 num_done = 0, num_err = 0;
    int64 num_frames_written = 0, num_egs_written = 0;
    
    for (; !feat_reader.Done(); feat_reader.Next()) {
      std::string key = feat_reader.Key();
      const Matrix<BaseFloat> &feats = feat_reader.Value();
      if (!pdf_post_reader.HasKey(key)) {
        KALDI_WARN << "No pdf-level posterior for key " << key;
        num_err++;
      } else {
        const Posterior &pdf_post = pdf_post_reader.Value(key);
        if (pdf_post.size() != feats.NumRows()) {
          KALDI_WARN << "Posterior has wrong size " << pdf_post.size()
                     << " versus " << feats.NumRows();
          num_err++;
          continue;
        }
        const Matrix<BaseFloat> *ivector_feats = NULL;
        if (!ivector_rspecifier.empty()) {
          if (!ivector_reader.HasKey(key)) {
            KALDI_WARN << "No iVectors for utterance " << key;
            num_err++;
            continue;
          } else {
            // this address will be valid until we call HasKey() or Value()
            // again.
            ivector_feats = &(ivector_reader.Value(key));
          }
        }

        if (ivector_feats != NULL &&
            (abs(feats.NumRows() - ivector_feats->NumRows()) > length_tolerance
             || ivector_feats->NumRows() == 0)) {
          KALDI_WARN << "Length difference between feats " << feats.NumRows()
                     << " and iVectors " << ivector_feats->NumRows()
                     << "exceeds tolerance " << length_tolerance;
          num_err++;
          continue;
        }
          
        ProcessFile(feats, ivector_feats, pdf_post, key, compress,
                    left_context, right_context, num_frames,
                    &num_frames_written, &num_egs_written,
                    &example_writer);
        num_done++;
      }
    }

    KALDI_LOG << "Finished generating examples, "
              << "successfully processed " << num_done
              << " feature files, wrote " << num_egs_written << " examples, "
              << " with " << num_frames_written << " egs in total; "
              << num_err << " files had errors.";
    return (num_egs_written == 0 || num_err > num_done ? 1 : 0);
  } catch(const std::exception &e) {
    std::cerr << e.what() << '\n';
    return -1;
  }
}
