// lm/kaldi-lm.h
// Copyright 2009-2011 Gilles Boulianne.

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

/**
  * @file kaldi-lm.h
  * @brief Language model FST definitions.
  *
  * Provides methods to read and write
  * file-based language models, strings and grammars.
  *
  * The entire FST is created at read time.
*/

// Future work (Gilles):
//    - use a StdVectorFst implementation and a Fst interface
//    - have a lm composed of several "factor" lms

#ifndef KALDI_LM_KALDI_LM_H_
#define KALDI_LM_KALDI_LM_H_

#include <string>

#include "fst/fstlib.h"
#include "fst/fst-decl.h"
#include "base/kaldi-common.h"
#include "lm/kaldi-lmtable.h"

namespace kaldi {

/// @defgroup LanguageModel LanguageModel
/// @{
/// @brief Language model and lexicon FST implementations.

/// Controls reading or ARPA, IRSTLM, OpenFST, or text formats
enum GrammarType {
  kArpaLm, kIrstLm, kFst, kTextString
};

/// @brief Finite-state transducer language model.

/// LangModelFst is a standard vector FST that also provides
/// Read() and Write() functions for file-based language models
/// or text files defining strings and grammars.
class LangModelFst : public fst::VectorFst<fst::StdArc> {
 public:

  LangModelFst() {
    pfst_ = new fst::VectorFst<fst::StdArc>;
  }

  LangModelFst(const LangModelFst &lm)
    : pfst_(lm.pfst_ ? new fst::VectorFst<fst::StdArc>(*(lm.pfst_)) : 0) {}

  ~LangModelFst() {
    if (pfst_) delete pfst_;
  }

  /// Reads a language model from an input stream.
  bool Read(std::istream &strm,
            const string &sourcename,
            GrammarType gtype,
            fst::SymbolTable *pst = NULL,
            bool useNaturalLog = true,
            const string startSent = "<s>",
            const string endSent = "</s>") {
    if (pfst_) delete pfst_;
    pfst_ = ReadStream(strm, sourcename,
                       gtype, pst,
                       useNaturalLog,
                       startSent, endSent);
    return(pfst_ ? true : false);
  }

  /// Reads from a named input file. Empty filename reads from standard input.
  bool Read(const string &filename,
            GrammarType gtype,
            fst::SymbolTable *pst = 0,
            bool useNaturalLog = true,
            const string startSent = "<s>",
            const string endSent = "</s>") {
    if (pfst_) { delete pfst_; pfst_ = NULL; }
    if (filename != "") {
      std::ifstream strm(filename.c_str(), std::ifstream::in);
      if (!strm) {
        KALDI_ERR << "LangModelFst: Can't open file: " << filename;
      }
      pfst_ = ReadStream(strm, filename,
                         gtype, pst,
                         useNaturalLog,
                         startSent, endSent);
    } else {
      pfst_ = ReadStream(std::cin, "standard input",
                         gtype, pst,
                         useNaturalLog,
                         startSent, endSent);
    }
    return(pfst_ ? true : false);
  }

  fst::SymbolTable* MutableInputSymbols() {
    return pfst_->MutableInputSymbols();
  }

  const fst::VectorFst<fst::StdArc>* GetFst() const {return pfst_;}
  fst::VectorFst<fst::StdArc>* GetFst() {return pfst_;}

  /// Writes language model FST to named output file, return false on error.
  // TODO : make verify optional rather than mandatory
  bool Write(const string &filename) {
    return (/*fst::Verify(*pfst_) && */ pfst_->Write(filename));
  }

 private:
  fst::VectorFst<fst::StdArc> *pfst_;
  fst::VectorFst<fst::StdArc>* ReadStream(std::istream &strm,
                                          const string &sourcename,
                                          GrammarType gtype,
                                          fst::SymbolTable *pst,
                                          bool useNaturalLog,
                                          const string startSent,
                                          const string endSent);
  void ReadTxtString(std::istream &strm);
  fst::StdArc::StateId ReadTxtLine(const string &inpline);
};
/// @} LanguageModel

}  // end namespace kaldi

#endif  // KALDI_LM_KALDI_LM_H_

