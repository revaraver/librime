//
// Copyright RIME Developers
// Distributed under the BSD License
//
#include <rime/common.h>
#include <rime/config.h>
#include <rime/context.h>
#include <rime/engine.h>
#include <rime/key_event.h>
#include <rime/schema.h>
#include <rime/gear/raw_boundary_processor.h>

namespace rime {

namespace {

const char kDefaultBoundarySymbols[] =
    "`~!@#$%^&*()-_=+[{]}\\|;:'\",<.>/?";

}  // namespace

RawBoundaryProcessor::RawBoundaryProcessor(const Ticket& ticket)
    : Processor(ticket), symbols_(kDefaultBoundarySymbols) {
  Config* config = engine_->schema()->config();
  if (!config) {
    return;
  }
  config->GetBool("raw_boundary/enabled", &enabled_);
  config->GetBool("raw_boundary/commit_raw_before_symbols",
                  &commit_raw_before_symbols_);
  string configured_symbols;
  if (config->GetString("raw_boundary/symbols", &configured_symbols)) {
    symbols_ = configured_symbols;
  }
}

ProcessResult RawBoundaryProcessor::ProcessKeyEvent(
    const KeyEvent& key_event) {
  if (!enabled_ || !commit_raw_before_symbols_ || key_event.release() ||
      key_event.ctrl() || key_event.alt() || key_event.super()) {
    return kNoop;
  }

  int ch = key_event.keycode();
  if (!IsBoundarySymbol(ch)) {
    return kNoop;
  }

  Context* ctx = engine_->context();
  if (!ctx || !ctx->IsComposing()) {
    return kNoop;
  }

  DLOG(INFO) << "raw boundary before symbol: '" << static_cast<char>(ch)
             << "'";
  ctx->ClearNonConfirmedComposition();
  ctx->Commit();
  return kNoop;
}

bool RawBoundaryProcessor::IsBoundarySymbol(int ch) const {
  if (ch <= 0x20 || ch >= 0x7f) {
    return false;
  }
  return symbols_.find(static_cast<char>(ch)) != string::npos;
}

}  // namespace rime
