.global StartGivePlayerInvuln
.global BranchBackGivePlayerInvuln
.global StartBtlSeqEndJudgeRule
.global BranchBackBtlSeqEndJudgeRule

StartGivePlayerInvuln:
li %r3, 3000
bl marioSetMutekiTime
bl marioGetPtr
mr %r27, %r3
li %r3, 30000
bl sysMsec2Frame
sth %r3, 0x2d8 (%r27)

BranchBackGivePlayerInvuln:
b 0

StartBtlSeqEndJudgeRule:
bl BtlActRec_JudgeRuleKeep
mr %r3, %r26

BranchBackBtlSeqEndJudgeRule:
b 0