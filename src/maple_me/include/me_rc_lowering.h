/*
 * Copyright (c) [2019] Huawei Technologies Co.,Ltd.All rights reserved.
 *
 * OpenArkCompiler is licensed under the Mulan PSL v1.
 * You can use this software according to the terms and conditions of the Mulan PSL v1.
 * You may obtain a copy of Mulan PSL v1 at:
 *
 *     http://license.coscl.org.cn/MulanPSL
 *
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR
 * FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v1 for more details.
 */
#ifndef MAPLE_ME_INCLUDE_ME_RC_LOWERING_H
#define MAPLE_ME_INCLUDE_ME_RC_LOWERING_H
#include "class_hierarchy.h"
#include "me_function.h"
#include "me_irmap.h"
#include "me_phase.h"
#include "mir_builder.h"

namespace maple {
class RCLowering {
 public:
  RCLowering(MeFunction &f, KlassHierarchy &kh)
      : func(f),
        mirModule(f.GetMIRModule()),
        irMap(*f.GetIRMap()),
        ssaTab(*f.GetMeSSATab()),
        klassHierarchy(kh) {}

  virtual ~RCLowering() = default;

  void Prepare();
  void PreRCLower();
  void RCLower();
  void PostRCLower();
  void Finish();
  void FastBBLower(BB &bb);

 private:
  MeFunction &func;
  MIRModule &mirModule;
  IRMap &irMap;
  SSATab &ssaTab;
  KlassHierarchy &klassHierarchy;
  std::vector<MeStmt*> rets{};  // std::vector of return statement
  unsigned int tmpCount = 0;
  bool needSpecialHandleException = false;
  std::set<MIRSymbol*> assignedPtrSym;
  std::set<VarMeExpr*> tmpLocalRefVars;
  std::set<MeExpr*> gcMallocObjects{};
  std::map<OStIdx, VarMeExpr*> cleanUpVars{};
  std::map<OStIdx, OriginalSt*> varOStMap{};
  // used to store initialized map, help to optimize dec ref in first assignment
  std::unordered_map<MeExpr*, MapleSet<FieldID>*> initializedFields{};
  std::string PhaseName() const {
    return "rclowering";
  }
  void MarkLocalRefVar();
  void MarkAllRefOpnds();
  void BBLower(BB &bb);
  void CreateCleanupIntrinsics();
  void HandleArguments();
  // create new symbol from name and return its ost
  OriginalSt *RetrieveOSt(const std::string &name, bool isLocalrefvar) const;
  /**
   * create new symbol from temp name and return its VarMeExpr
   * new symbols are stored in a set
   */
  VarMeExpr *CreateNewTmpVarMeExpr(bool isLocalrefvar);
  VarMeExpr *CreateVarMeExprFromSym(MIRSymbol &sym) const;
  // return true if the rhs is simple so we can adjust RC count before assignments
  bool RCFirst(MeExpr &rhs);
  IntrinsiccallMeStmt *GetVarRHSHandleStmt(const MeStmt &stmt);
  IntrinsiccallMeStmt *GetIvarRHSHandleStmt(const MeStmt &stmt);
  MIRIntrinsicID PrepareVolatileCall(const MeStmt &stmt, const MIRIntrinsicID index = INTRN_UNDEFINED);
  IntrinsiccallMeStmt *CreateRCIntrinsic(const MIRIntrinsicID intrnID, const MeStmt &stmt, std::vector<MeExpr*> &opnds,
                                         bool assigned = false);
  void InitializedObjectFields(MeStmt &stmt);
  bool IsInitialized(IvarMeExpr &ivar);
  void PreprocessAssignMeStmt(MeStmt &stmt);
  void HandleAssignMeStmtRHS(MeStmt &stmt);
  void HandleAssignMeStmtRegLHS(MeStmt &stmt);
  void HandleAssignToGlobalVar(MeStmt &stmt);
  void HandleAssignToLocalVar(MeStmt &stmt, MeExpr *pendingDec);
  void HandleAssignMeStmtVarLHS(MeStmt &stmt, MeExpr *pendingDec);
  void HandleAssignMeStmtIvarLHS(MeStmt &stmt);
  void HandleCallAssignedMeStmt(MeStmt &stmt, MeExpr *pendingDec);
  void IntroduceRegRetIntoCallAssigned(MeStmt &stmt);
  void HandleRetOfCallAssignedMeStmt(MeStmt &stmt, MeExpr &pendingDec);
  void HandleReturnVar(RetMeStmt &ret);
  void HandleReturnGlobal(RetMeStmt &ret);
  void HandleReturnRegread(RetMeStmt &ret);
  void HandleReturnFormal(RetMeStmt &ret);
  void HandleReturnIvar(RetMeStmt &ret);
  void HandleReturnReg(RetMeStmt &ret);
  void HandleReturnWithCleanup();
  void HandleReturnNeedBackup();
  void HandleReturnStmt();
  void HandleAssignMeStmt(MeStmt &stmt, MeExpr *pendingDec);
  MIRIntrinsicID SelectWriteBarrier(const MeStmt &stmt);
};

class MeDoRCLowering : public MeFuncPhase {
 public:
  explicit MeDoRCLowering(MePhaseID id) : MeFuncPhase(id) {}

  virtual ~MeDoRCLowering() = default;

  AnalysisResult *Run(MeFunction*, MeFuncResultMgr*, ModuleResultMgr*) override;

  std::string PhaseName() const override {
    return "rclowering";
  }
};
}  // namespace maple
#endif  // MAPLE_ME_INCLUDE_ME_RC_LOWERING_H
