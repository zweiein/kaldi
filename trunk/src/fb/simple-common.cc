// fb/simple-common.cc

#include "fb/simple-common.h"

namespace kaldi {


void Token::UpdateEmitting(
    const double prev_cost, const double edge_cost,
    const double acoustic_cost) {
  cost = -kaldi::LogAdd(-cost, -(prev_cost + edge_cost + acoustic_cost));
}


bool Token::UpdateNonEmitting(
    const double prev_cost, const double edge_cost,
    const double threshold) {
  const double old_cost = cost;
  cost = -kaldi::LogAdd(-cost, - (prev_cost + edge_cost));
  last_cost = -kaldi::LogAdd(-last_cost, -(prev_cost + edge_cost));
  return !kaldi::ApproxEqual(cost, old_cost, threshold);
}


void PruneToks(double beam, TokenMap *toks) {
  if (toks->empty()) {
    KALDI_VLOG(2) <<  "No tokens to prune.\n";
    return;
  }
  TokenMap::const_iterator tok = toks->begin();
  // Get best cost
  double best_cost = tok->second.cost;
  for (++tok; tok != toks->end(); ++tok) {
    best_cost = std::min(best_cost, tok->second.cost);
  }
  // Mark all tokens with cost greater than the cutoff
  std::vector<TokenMap::const_iterator> remove_toks;
  const double cutoff = best_cost + beam;
  for (tok = toks->begin(); tok != toks->end(); ++tok) {
    if (tok->second.cost > cutoff) {
      remove_toks.push_back(tok);
    }
  }
  // Prune tokens
  for (size_t i = 0; i < remove_toks.size(); ++i) {
    toks->erase(remove_toks[i]);
  }
  KALDI_VLOG(2) <<  "Pruned " << remove_toks.size() << " to "
                << toks->size() << " toks.\n";
}


double RescaleToks(TokenMap* toks) {
  // Compute scale constant
  double scale = -kaldi::kLogZeroDouble;
  for (TokenMap::iterator t = toks->begin(); t != toks->end(); ++t) {
    scale = -kaldi::LogAdd(-scale, -t->second.cost);
  }
  // Rescale tokens and labels
  for (TokenMap::iterator t = toks->begin(); t != toks->end(); ++t) {
    t->second.cost -= scale;
  }
  return scale;
}


// Compute Likelihood of the observed sequence.
// WARNING: This is not backward[0][Start], since epsilon transitions may exist!
double ComputeLikelihood(const TokenMap& fwd0, const TokenMap& bkw0) {
  double lkh_obs = kaldi::kLogZeroDouble;
  for (TokenMap::const_iterator ftok = fwd0.begin(); ftok != fwd0.end();
       ++ftok) {
    TokenMap::const_iterator btok = bkw0.find(ftok->first);
    const double& bc = btok != bkw0.end() ?
        -btok->second.cost : kaldi::kLogZeroDouble;
    const double& fc = -ftok->second.cost;
    lkh_obs = kaldi::LogAdd(lkh_obs, fc + bc);
  }
  return lkh_obs;
}


void ComputeLabelsPosterior(
    const fst::Fst<fst::StdArc>& fst,
    const std::vector<TokenMap>& fwd,
    const std::vector<TokenMap>& bkw,
    DecodableInterface* decodable,
    std::vector<LabelMap>* pst) {
  typedef fst::StdArc StdArc;
  typedef fst::Fst<StdArc> Fst;
  typedef fst::ArcIterator<Fst> ArcIterator;

  KALDI_ASSERT(fwd.size() == bkw.size());
  KALDI_ASSERT(fwd.size() > 0);
  pst->clear();
  pst->resize(fwd.size() - 1);

  for (size_t t = 0; t < pst->size(); ++t) {
    double sum = kaldi::kLogZeroDouble;
    // Traverse all active states i, at time t
    for (TokenMap::const_iterator ftok = fwd[t].begin(); ftok != fwd[t].end();
         ++ftok) {
      // Forward cost to state i, in time t
      const double& fc = ftok->second.cost;
      if (fc == -kaldi::kLogZeroDouble) continue;
      // Traverse outgoing edges from state i, which emit some label
      // WARNING: I am ignoring epsilon edges, since they do not correspond to
      // any learnable transition-id.
      for (ArcIterator aiter(fst, ftok->first); !aiter.Done(); aiter.Next()) {
        const StdArc arc = aiter.Value();
        const StateId j = arc.nextstate;
        if (arc.ilabel == 0 || arc.weight == StdArc::Weight::Zero()) continue;
        // Backward cost to state j, in time t + 1
        TokenMap::const_iterator btok = bkw[t + 1].find(j);
        if (btok == bkw[t + 1].end() ||
            btok->second.cost == -kaldi::kLogZeroDouble) continue;
        const double& bc = btok->second.cost;
        // Acoustic cost of emiting current label at time t
        const double acoustic_cost = -decodable->LogLikelihood(t, arc.ilabel);
        if (acoustic_cost == -kaldi::kLogZeroDouble) continue;
        // Update label likelihood, and total likelihood
        double& logp = (*pst)[t].insert(make_pair(
            arc.ilabel, kaldi::kLogZeroDouble)).first->second;
        const double inc_p = -(fc + bc + arc.weight.Value() + acoustic_cost);
        logp = kaldi::LogAdd(logp, inc_p);
        sum = kaldi::LogAdd(sum, inc_p);
      }
    }
    // Normalize label log-likelihood to get posteriors.
    for (LabelMap::iterator it = (*pst)[t].begin(); it != (*pst)[t].end();
         ++it) {
      it->second -= sum;
    }
  }
}


void PrintTokenMap(const TokenMap& toks, const string& name, int32 t) {
  if (t < 0) {
    for (TokenMap::const_iterator tk = toks.begin(); tk != toks.end(); ++tk) {
      KALDI_LOG << name << "[" << tk->first << "] = " << tk->second.cost;
    }
  } else {
    for (TokenMap::const_iterator tk = toks.begin(); tk != toks.end(); ++tk) {
      KALDI_LOG << name << "[" << t << "," << tk->first << "] = "
                << tk->second.cost;
    }
  }
}


void PrintTokenTable(const vector<TokenMap>& table, const string& name) {
  for (size_t t = 0; t < table.size(); ++t) {
    PrintTokenMap(table[t], name, t);
  }
}


}  // namespace kaldi
