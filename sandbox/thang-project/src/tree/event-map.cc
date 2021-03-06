// tree/event-map.cc

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

#include <set>
#include <string>
#include "tree/event-map.h"

namespace kaldi {


void EventMap::Write(std::ostream &os, bool binary, EventMap *emap) {
  if (emap == NULL) {
    WriteMarker(os, binary, "NULL");
  } else {
    emap->Write(os, binary);
  }
}

EventMap *EventMap::Read(std::istream &is, bool binary) {
  char c = Peek(is, binary);
  if (c == 'N') {
    ExpectMarker(is, binary, "NULL");
    return NULL;
  } else if (c == 'C') {
    return ConstantEventMap::Read(is, binary);
  } else if (c == 'T') {
    return TableEventMap::Read(is, binary);
  } else if (c == 'S') {
    return SplitEventMap::Read(is, binary);
  } else {
    KALDI_ERR << "EventMap::read, was not expecting character " << CharToString(c)
              << ", at file position " << is.tellg();
    return NULL;  // suppress warning.
  }
}


void ConstantEventMap::Write(std::ostream &os, bool binary) {
  WriteMarker(os, binary, "CE");
  WriteBasicType(os, binary, answer_);
  if (os.fail()) {
    KALDI_ERR << "ConstantEventMap::Write(), could not write to stream.";
  }
}

// static member function.
ConstantEventMap* ConstantEventMap::Read(std::istream &is, bool binary) {
  ExpectMarker(is, binary, "CE");
  EventAnswerType answer;
  ReadBasicType(is, binary, &answer);
  return new ConstantEventMap(answer);
}


void TableEventMap::Write(std::ostream &os, bool binary) {
  WriteMarker(os, binary, "TE");
  WriteBasicType(os, binary, key_);
  uint32 size = table_.size();
  WriteBasicType(os, binary, size);
  WriteMarker(os, binary, "(");
  for (size_t t = 0; t < size; t++) {
    // This Write function works for NULL pointers.
    EventMap::Write(os, binary, table_[t]);
  }
  WriteMarker(os, binary, ")");
  if (!binary) os << '\n';
  if (os.fail()) {
    KALDI_ERR << "TableEventMap::Write(), could not write to stream.";
  }
}

// static member function.
TableEventMap* TableEventMap::Read(std::istream &is, bool binary) {
  ExpectMarker(is, binary, "TE");
  EventKeyType key;
  ReadBasicType(is, binary, &key);
  uint32 size;
  ReadBasicType(is, binary, &size);
  std::vector<EventMap*> table(size);
  ExpectMarker(is, binary, "(");
  for (size_t t = 0; t < size; t++) {
    // This Read function works for NULL pointers.
    table[t] = EventMap::Read(is, binary);
  }
  ExpectMarker(is, binary, ")");
  return new TableEventMap(key, table);
}

void SplitEventMap::Write(std::ostream &os, bool binary) {
  WriteMarker(os, binary, "SE");
  WriteBasicType(os, binary, key_);
  // WriteIntegerVector(os, binary, yes_set_);
  yes_set_.Write(os, binary);
  assert(yes_ != NULL && no_ != NULL);
  WriteMarker(os, binary, "{");
  yes_->Write(os, binary);
  no_->Write(os, binary);
  WriteMarker(os, binary, "}");
  if (!binary) os << '\n';
  if (os.fail()) {
    KALDI_ERR << "SplitEventMap::Write(), could not write to stream.";
  }
}

// static member function.
SplitEventMap* SplitEventMap::Read(std::istream &is, bool binary) {
  ExpectMarker(is, binary, "SE");
  EventKeyType key;
  ReadBasicType(is, binary, &key);
  // std::vector<EventValueType> yes_set;
  // ReadIntegerVector(is, binary, &yes_set);
  ConstIntegerSet<EventValueType> yes_set;
  yes_set.Read(is, binary);
  ExpectMarker(is, binary, "{");
  EventMap *yes = EventMap::Read(is, binary);
  EventMap *no = EventMap::Read(is, binary);
  ExpectMarker(is, binary, "}");
  // yes and no should be non-NULL because NULL values are not valid for SplitEventMap;
  // the constructor checks this.  Therefore this is an unlikely error.
  if (yes == NULL || no == NULL) KALDI_ERR << "SplitEventMap::Read, NULL pointers.";
  return new SplitEventMap(key, yes_set, yes, no);
}


void WriteEventType(std::ostream &os, bool binary, const EventType &evec) {
  WriteMarker(os, binary, "EV");
  uint32 size = evec.size();
  WriteBasicType(os, binary, size);
  for (size_t i = 0; i < size; i++) {
    WriteBasicType(os, binary, evec[i].first);
    WriteBasicType(os, binary, evec[i].second);
  }
  if (!binary) os << '\n';
}

void ReadEventType(std::istream &is, bool binary, EventType *evec) {
  assert(evec != NULL);
  ExpectMarker(is, binary, "EV");
  uint32 size;
  ReadBasicType(is, binary, &size);
  evec->resize(size);
  for (size_t i = 0; i < size; i++) {
    ReadBasicType(is, binary, &( (*evec)[i].first ));
    ReadBasicType(is, binary, &( (*evec)[i].second ));
  }
}



std::string EventTypeToString(const EventType &evec) {
  std::stringstream ss;
  EventType::const_iterator iter = evec.begin(), end = evec.end();
  std::string sep = "";
  for (; iter != end; ++iter) {
    ss << sep << iter->first <<":"<<iter->second;
    sep = " ";
  }
  return ss.str();
}

size_t EventMapVectorHash::operator ()(const EventType &vec) {
  EventType::const_iterator iter = vec.begin(), end = vec.end();
  size_t ans = 0;
  const size_t kPrime1=47087, kPrime2=1321;
  for (; iter != end; ++iter) {
#ifdef KALDI_PARANOID // Check names are distinct and increasing.
    EventType::const_iterator iter2=iter; iter2++;
    if (iter2 != end) { assert(iter->first < iter2->first); }
#endif
    ans += iter->first + kPrime1*iter->second;
    ans *= kPrime2;
  }
  return ans;
}


// static member of EventMap.
void EventMap::Check(const std::vector<std::pair<EventKeyType, EventValueType> > &event) {
  // will crash if not sorted or has duplicates
  size_t sz = event.size();
  for (size_t i = 0;i+1 < sz;i++)
    assert(event[i].first < event[i+1].first);
}


// static member of EventMap.
bool EventMap::Lookup(const EventType &event,
                      EventKeyType key, EventValueType *ans) {
  // this assumes the the "event" array is sorted (e.g. on the KeyType value;
  // just doing std::sort will do this) and has no duplicate values with the same
  // key.  call Check() to verify this.
#ifdef KALDI_PARANOID
  Check(event);
#endif
  std::vector<std::pair<EventKeyType, EventValueType> >::const_iterator
      begin = event.begin(),
      end = event.end(),
      middle;  // "middle" is used as a temporary variable in the algorithm.
  // begin and sz store the current region where the first instance of
  // "value" might appear.
  // This is like this stl algorithm "lower_bound".
  size_t sz = end-begin, half;
  while (sz > 0) {
    half = sz >> 1;
    middle = begin + half;  // "end" here is now reallly the middle.
    if (middle->first < key) {
      begin = middle;
      ++begin;
      sz = sz - half - 1;
    } else {
      sz = half;
    }
  }
  if (begin != end && begin->first == key) {
    *ans = begin->second;
    return true;
  } else {
    return false;
  }
}

TableEventMap::TableEventMap(EventKeyType key, const std::map<EventValueType, EventMap*> &map_in): key_(key) {
  if (map_in.size() == 0)
    return;  // empty table.
  else {
    EventValueType highest_val = map_in.rbegin()->first;
    table_.resize(highest_val+1, NULL);
    std::map<EventValueType, EventMap*>::const_iterator iter = map_in.begin(), end = map_in.end();
    for (; iter != end; ++iter) {
      assert(iter->first >= 0 && iter->first <= highest_val);
      table_[iter->first] = iter->second;
    }
  }
}

TableEventMap::TableEventMap(EventKeyType key, const std::map<EventValueType, EventAnswerType> &map_in): key_(key) {
  if (map_in.size() == 0)
    return;  // empty table.
  else {
    EventValueType highest_val = map_in.rbegin()->first;
    table_.resize(highest_val+1, NULL);
    std::map<EventValueType, EventAnswerType>::const_iterator iter = map_in.begin(), end = map_in.end();
    for (; iter != end; ++iter) {
      assert(iter->first >= 0 && iter->first <= highest_val);
      table_[iter->first] = new ConstantEventMap(iter->second);
    }
  }
}



} // end namespace kaldi
