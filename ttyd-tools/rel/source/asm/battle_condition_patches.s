.global StartRuleDisp
.global BranchBackRuleDisp

StartRuleDisp:
addi %r3, %r1, 0x10
bl getBattleConditionString

BranchBackRuleDisp:
b 0