//
// Copyright RIME Developers
// Distributed under the BSD License
//
// 2011-10-23 GONG Chen <chen.sst@gmail.com>
//
#ifndef RIME_EDITOR_H_
#define RIME_EDITOR_H_

#include <rime/common.h>
#include <rime/component.h>
#include <rime/key_event.h>
#include <rime/processor.h>
#include <rime/gear/key_binding_processor.h>

namespace rime {

class Context;

class Editor : public Processor, public KeyBindingProcessor<Editor> {
 public:
  typedef ProcessResult CharHandler(Context* ctx, int ch);
  using CharHandlerPtr = ProcessResult (Editor::*)(Context* ctx, int ch);

  using KeyBindingProcessor<Editor>::KeyBindingProcessor;

  Editor(const Ticket& ticket, bool auto_commit);
  ~Editor() override;
  ProcessResult ProcessKeyEvent(const KeyEvent& key_event);

  Handler Confirm;
  Handler ToggleSelection;
  Handler CommitComment;
  Handler CommitScriptText;
  Handler CommitRawInput;
  Handler CommitRawInputAndSendSpace;
  Handler CommitRawInputAndSendEnter;
  Handler ConfirmAndSendEnter;
  Handler ConfirmAndSendSpace;
  Handler CommitComposition;
  Handler RevertLastEdit;
  Handler BackToPreviousInput;
  Handler BackToPreviousSyllable;
  Handler DeleteCandidate;
  Handler DeleteChar;
  Handler CancelComposition;

  CharHandler DirectCommit;
  CharHandler AddToInput;

 protected:
  void LoadConfig();

  CharHandlerPtr char_handler_ = nullptr;

  KeyBindingProcessor<Editor>* release_bindings_ = nullptr;

  struct ModifierState {
    bool pressed = false;
    using TimePoint = std::chrono::steady_clock::time_point;
    TimePoint press_time;
  };
  map<int, ModifierState> modifier_state_;
};

class FluidEditor : public Editor {
 public:
  FluidEditor(const Ticket& ticket);
};

class ExpressEditor : public Editor {
 public:
  ExpressEditor(const Ticket& ticket);
};

}  // namespace rime

#endif  // RIME_EDITOR_H_
