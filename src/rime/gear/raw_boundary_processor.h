//
// Copyright RIME Developers
// Distributed under the BSD License
//
#ifndef RIME_RAW_BOUNDARY_PROCESSOR_H_
#define RIME_RAW_BOUNDARY_PROCESSOR_H_

#include <rime/processor.h>

namespace rime {

class RawBoundaryProcessor : public Processor {
 public:
  explicit RawBoundaryProcessor(const Ticket& ticket);
  virtual ProcessResult ProcessKeyEvent(const KeyEvent& key_event);

 protected:
  bool IsBoundarySymbol(int ch) const;

  bool enabled_ = false;
  bool commit_raw_before_symbols_ = false;
  string symbols_;
};

}  // namespace rime

#endif  // RIME_RAW_BOUNDARY_PROCESSOR_H_
