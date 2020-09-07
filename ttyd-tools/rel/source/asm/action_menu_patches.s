.global StartSpendFpOnSwitchPartner
.global BranchBackSpendFpOnSwitchPartner

StartSpendFpOnSwitchPartner:
mr %r3, %r28
bl spendFpOnSwitchPartner
lwz %r0, 0x8 (%r28)  # Original opcode replaced.

BranchBackSpendFpOnSwitchPartner:
b 0