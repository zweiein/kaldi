// idlakfex/fexfunctionscatalog.cc

// Copyright 2013 CereProc Ltd.  (Author: Matthew Aylett)

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
//

// Each function fills a feature extraction buffer up with a feature based on
// an XML node and context.

#include "./fex.h"
#include "./fexfunctions.h"

namespace kaldi {

const char * const FEXFUNCLBL[] = {"p"};

const fexfunction FEXFUNC[] = {&FexFuncCURSTRp};

// Pause behavior
// pauctx defaults to false but can be set specifically in the fex setup for each
// extraction function.
// CUR - return '0' unless function connected to a set in which case return
//       the default value for that set unless pauctx is set to true then run
//       the extraction function.
// PRE - if pause context (pauctx) is true use previous context of initial break
//       in mid utterance break pair or NONE on initial break
// PST - if pause context (pauctx) is true use post context of second break in
//       mid utterance break pair or NONE on final break

const enum FEXPAU_TYPE FEXFUNCPAUTYPE[] = {FEXPAU_TYPE_CUR};

// Return type
// INT - integer either a count or a class
// STR - a string, part of a defined set of possible values listed in
//       fex-default.xml

const enum FEX_TYPE FEXFUNCTYPE[] = {FEX_TYPE_STR};

}  // namespace kaldi
