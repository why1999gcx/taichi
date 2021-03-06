#include "taichi/ir/ir.h"
#include <vector>
#include <unordered_set>

TLANG_NAMESPACE_BEGIN

class IRVerifier : public BasicStmtVisitor {
 private:
  Block *current_block;
  // each scope corresponds to an unordered_set
  std::vector<std::unordered_set<Stmt *>> visible_stmts;

 public:
  using BasicStmtVisitor::visit;

  explicit IRVerifier(IRNode *root) : current_block(nullptr) {
    allow_undefined_visitor = true;
    invoke_default_visitor = true;
    TI_ASSERT(root->is<Block>() && root->as<Block>()->parent == nullptr);
  }

  void basic_verify(Stmt *stmt) {
    TI_ASSERT(stmt->parent == current_block);
    for (auto &op : stmt->get_operands()) {
      if (op == nullptr)
        continue;
      bool found = false;
      for (int depth = (int)visible_stmts.size() - 1; depth >= 0; depth--) {
        if (visible_stmts[depth].find(op) != visible_stmts[depth].end()) {
          found = true;
          break;
        }
      }
      TI_ASSERT(found);
    }
    visible_stmts.back().insert(stmt);
  }

  void preprocess_container_stmt(Stmt *stmt) override {
    basic_verify(stmt);
  }

  void visit(Stmt *stmt) override {
    basic_verify(stmt);
  }

  void visit(Block *stmt_list) override {
    TI_ASSERT(stmt_list->parent == current_block);
    auto backup_block = current_block;
    current_block = stmt_list;
    visible_stmts.emplace_back();
    for (auto &stmt : stmt_list->statements) {
      stmt->accept(this);
    }
    current_block = backup_block;
    visible_stmts.pop_back();
  }

  void visit(LocalLoadStmt *stmt) override {
    basic_verify(stmt);
    for (int i = 0; i < stmt->width(); i++) {
      TI_ASSERT(stmt->ptr[i].var->is<AllocaStmt>());
    }
  }

  void visit(LocalStoreStmt *stmt) override {
    basic_verify(stmt);
    TI_ASSERT(stmt->ptr->is<AllocaStmt>());
  }

  static void run(IRNode *root) {
    IRVerifier verifier(root);
    root->accept(&verifier);
  }
};

namespace irpass {
void verify(IRNode *root) {
  IRVerifier::run(root);
}
}  // namespace irpass

TLANG_NAMESPACE_END
