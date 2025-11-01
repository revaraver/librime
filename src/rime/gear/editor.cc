//
// Copyright RIME Developers
// Distributed under the BSD License
//
// 2011-10-23 GONG Chen <chen.sst@gmail.com>
//
#include <rime/common.h>
#include <rime/config.h>
#include <rime/context.h>
#include <rime/engine.h>
#include <rime/schema.h>
#include <rime/key_table.h>
#include <rime/gear/editor.h>
#include <rime/gear/key_binding_processor.h>
#include <rime/gear/translator_commons.h>

namespace rime {

static Editor::ActionDef editor_action_definitions[] = {
    {"confirm", &Editor::Confirm},
    {"toggle_selection", &Editor::ToggleSelection},
    {"commit_comment", &Editor::CommitComment},
    {"commit_raw_input", &Editor::CommitRawInput},
    {"commit_raw_input_and_send_space", &Editor::CommitRawInputAndSendSpace},
    {"commit_raw_input_and_send_enter", &Editor::CommitRawInputAndSendEnter},
    {"confirm_and_send_enter", &Editor::ConfirmAndSendEnter},
    {"confirm_and_send_space", &Editor::ConfirmAndSendSpace},
    {"commit_script_text", &Editor::CommitScriptText},
    {"commit_composition", &Editor::CommitComposition},
    {"revert", &Editor::RevertLastEdit},
    {"back", &Editor::BackToPreviousInput},
    {"back_syllable", &Editor::BackToPreviousSyllable},
    {"delete_candidate", &Editor::DeleteCandidate},
    {"delete", &Editor::DeleteChar},
    {"cancel", &Editor::CancelComposition},
    Editor::kActionNoop};

static struct EditorCharHandlerDef {
  const char* name;
  Editor::CharHandlerPtr action;
} editor_char_handler_definitions[] = {{"direct_commit", &Editor::DirectCommit},
                                       {"add_to_input", &Editor::AddToInput},
                                       {"noop", nullptr}};

Editor::Editor(const Ticket& ticket, bool auto_commit)
    : Processor(ticket), KeyBindingProcessor(editor_action_definitions) {
  engine_->context()->set_option("_auto_commit", auto_commit);
  release_bindings_ = new KeyBindingProcessor<Editor>(editor_action_definitions);
}

Editor::~Editor() { delete release_bindings_; }

ProcessResult Editor::ProcessKeyEvent(const KeyEvent& key_event) {
  Context* ctx = engine_->context();
  int ch = key_event.keycode();

  // --- 通用修饰键状态跟踪 --- 
  bool is_modifier = key_event.modifier() == 0 &&
                     (ch == XK_Shift_L || ch == XK_Shift_R ||
                      ch == XK_Control_L || ch == XK_Control_R ||
                      ch == XK_Alt_L || ch == XK_Alt_R ||
                      ch == XK_Super_L || ch == XK_Super_R);

  if (is_modifier) {
    if (!key_event.release()) { // 修饰键按下
      // 如果已有其他修饰键按下，则将其“消耗”掉，不允许触发“修-放”
      for (auto& p : modifier_state_) {
        p.second.pressed = false;
      }
      modifier_state_[ch].pressed = true;
      modifier_state_[ch].press_time = std::chrono::steady_clock::now();
    } else { // 修饰键抬起
      if (modifier_state_[ch].pressed) {
        // 状态未被“消耗”，且在500ms内抬起，则触发 release 动作
        const auto kMaxTapDuration = std::chrono::milliseconds(500);
        if (std::chrono::steady_clock::now() - modifier_state_[ch].press_time < kMaxTapDuration) {
          if (ctx->IsComposing()) {
            return release_bindings_->ProcessKeyEvent(key_event, ctx, 0, FallbackOptions::All);
          }
        }
        modifier_state_[ch].pressed = false;
      }
    }
    return kNoop; // 修饰键事件本身不向下传递
  } else if (!key_event.release()) {
    // 非修饰键按下，消耗所有已按下的修饰键状态
    for (auto& p : modifier_state_) {
      p.second.pressed = false;
    }
  }
  // --- 状态跟踪结束 ---

  // 对于常规的“按下”事件，以及被判定为“修饰”用途的“抬起”事件，正常处理
  if (key_event.release()) {
    return kRejected; // 默认拒绝所有其他抬起事件
  }

  // key press
  if (ctx->IsComposing()) {
    auto result = KeyBindingProcessor::ProcessKeyEvent(key_event, ctx, 0,
                                                       FallbackOptions::All);
    if (result != kNoop) {
      return result;
    }
  }
  if (char_handler_ && !key_event.ctrl() && !key_event.alt() &&
      !key_event.super() && ch > 0x20 && ch < 0x7f) {
    DLOG(INFO) << "input char: '" << (char)ch << "', " << ch << ", '"
               << key_event.repr() << "'";
    return RIME_THIS_CALL(char_handler_)(ctx, ch);
  }
  // not handled
  return kNoop;
}

void Editor::LoadConfig() {
  if (!engine_) {
    return;
  }
  Config* config = engine_->schema()->config();
  KeyBindingProcessor::LoadConfig(config, "editor");
  if (config->GetMap("editor/on_release")) {
    release_bindings_->LoadConfig(config, "editor/on_release");
  }

  if (auto value = config->GetValue("editor/char_handler")) {
    auto* p = editor_char_handler_definitions;
    while (p->action && p->name != value->str()) {
      ++p;
    }
    if (!p->action && p->name != value->str()) {
      LOG(WARNING) << "invalid char_handler: " << value->str();
    } else {
      char_handler_ = p->action;
    }
  }
}

bool Editor::Confirm(Context* ctx) {
  ctx->ConfirmCurrentSelection() || ctx->Commit();
  return true;
}

bool Editor::ToggleSelection(Context* ctx) {
  ctx->ReopenPreviousSegment() || ctx->ConfirmCurrentSelection();
  return true;
}

bool Editor::CommitComment(Context* ctx) {
  if (auto cand = ctx->GetSelectedCandidate()) {
    if (!cand->comment().empty()) {
      engine_->sink()(cand->comment());
      ctx->Clear();
    }
  }
  return true;
}

bool Editor::CommitScriptText(Context* ctx) {
  engine_->sink()(ctx->GetScriptText());
  ctx->Clear();
  return true;
}

bool Editor::CommitRawInput(Context* ctx) {
  ctx->ClearNonConfirmedComposition();
  ctx->Commit();
  return true;
}

// 提交原始输入并发送一个空格
bool Editor::CommitRawInputAndSendSpace(Context* ctx) {
  ctx->ClearNonConfirmedComposition();
  ctx->Commit();
  engine_->sink()(" ");
  return true;
}

// 发送原始输入并输入回车
bool Editor::CommitRawInputAndSendEnter(Context* ctx) {
  ctx->ClearNonConfirmedComposition();
  ctx->Commit();
  engine_->sink()("\n");
  return true;
}

// 上屏comfirm后发送回车
bool Editor::ConfirmAndSendEnter(Context* ctx) {
  ctx->ConfirmCurrentSelection() || ctx->Commit();
  engine_->sink()("\n");
  return true;
}

// 上屏comfirm后发送空格
bool Editor::ConfirmAndSendSpace(Context* ctx) {
  ctx->ConfirmCurrentSelection() || ctx->Commit();
  engine_->sink()(" ");
  return true;
}

bool Editor::CommitComposition(Context* ctx) {
  if (!ctx->ConfirmCurrentSelection() || !ctx->HasMenu())
    ctx->Commit();
  return true;
}

bool Editor::RevertLastEdit(Context* ctx) {
  // revert last selection or delete last input character
  // depending on recent operation type
  ctx->ReopenPreviousSelection() ||
      (ctx->PopInput() && ctx->ReopenPreviousSegment());
  return true;
}

bool Editor::BackToPreviousInput(Context* ctx) {
  ctx->ReopenPreviousSegment() || ctx->ReopenPreviousSelection() ||
      ctx->PopInput();
  return true;
}

static bool pop_input_by_syllable(Context* ctx) {
  size_t caret_pos = ctx->caret_pos();
  if (caret_pos == 0)
    return false;
  if (auto cand = ctx->GetSelectedCandidate()) {
    if (auto phrase = As<Phrase>(Candidate::GetGenuineCandidate(cand))) {
      size_t stop = phrase->spans().PreviousStop(caret_pos);
      if (stop != caret_pos) {
        ctx->PopInput(caret_pos - stop);
        return true;
      }
    }
  }
  return false;
}

bool Editor::BackToPreviousSyllable(Context* ctx) {
  ctx->ReopenPreviousSelection() ||
      ((pop_input_by_syllable(ctx) || ctx->PopInput()) &&
       ctx->ReopenPreviousSegment());
  return true;
}

bool Editor::DeleteCandidate(Context* ctx) {
  ctx->DeleteCurrentSelection();
  return true;
}

bool Editor::DeleteChar(Context* ctx) {
  ctx->DeleteInput();
  return true;
}

bool Editor::CancelComposition(Context* ctx) {
  if (!ctx->ClearPreviousSegment())
    ctx->Clear();
  return true;
}

ProcessResult Editor::DirectCommit(Context* ctx, int ch) {
  ctx->Commit();
  return kRejected;
}

ProcessResult Editor::AddToInput(Context* ctx, int ch) {
  ctx->PushInput(ch);
  ctx->BeginEditing();
  return kAccepted;
}

FluidEditor::FluidEditor(const Ticket& ticket) : Editor(ticket, false) {
  auto& keymap = get_keymap();
  keymap.Bind({XK_space, 0}, &Editor::Confirm);
  keymap.Bind({XK_BackSpace, 0}, &Editor::BackToPreviousInput);  //
  keymap.Bind({XK_BackSpace, kControlMask}, &Editor::BackToPreviousSyllable);
  keymap.Bind({XK_Return, 0}, &Editor::CommitComposition);          //
  keymap.Bind({XK_Return, kControlMask}, &Editor::CommitRawInput);  //
  keymap.Bind({XK_Return, kShiftMask}, &Editor::CommitScriptText);  //
  keymap.Bind({XK_Return, kControlMask | kShiftMask}, &Editor::CommitComment);
  keymap.Bind({XK_Delete, 0}, &Editor::DeleteChar);
  keymap.Bind({XK_Delete, kControlMask}, &Editor::DeleteCandidate);
  keymap.Bind({XK_Escape, 0}, &Editor::CancelComposition);
  char_handler_ = &Editor::AddToInput;  //
  LoadConfig();
}

ExpressEditor::ExpressEditor(const Ticket& ticket) : Editor(ticket, true) {
  auto& keymap = get_keymap();
  keymap.Bind({XK_space, 0}, &Editor::Confirm);
  keymap.Bind({XK_BackSpace, 0}, &Editor::RevertLastEdit);  //
  keymap.Bind({XK_BackSpace, kControlMask}, &Editor::BackToPreviousSyllable);
  keymap.Bind({XK_Return, 0}, &Editor::CommitRawInput);               //
  keymap.Bind({XK_Return, kControlMask}, &Editor::CommitScriptText);  //
  keymap.Bind({XK_Return, kControlMask | kShiftMask}, &Editor::CommitComment);
  keymap.Bind({XK_Delete, 0}, &Editor::DeleteChar);
  keymap.Bind({XK_Delete, kControlMask}, &Editor::DeleteCandidate);
  keymap.Bind({XK_Escape, 0}, &Editor::CancelComposition);
  char_handler_ = &Editor::DirectCommit;  //
  LoadConfig();
}

}  // namespace rime
