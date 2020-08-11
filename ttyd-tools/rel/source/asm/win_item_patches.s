.global StartFixItemWinPartyDispOrder
.global BranchBackFixItemWinPartyDispOrder
.global StartFixItemWinPartySelectOrder
.global BranchBackFixItemWinPartySelectOrder
.global StartUseSpecialItems
.global BranchBackUseSpecialItems

StartFixItemWinPartyDispOrder:
mr %r3, %r5
bl getPartyMemberMenuOrder

BranchBackFixItemWinPartyDispOrder:
b 0

StartFixItemWinPartySelectOrder:
mr %r3, %r5
bl getPartyMemberMenuOrder

BranchBackFixItemWinPartySelectOrder:
b 0

StartUseSpecialItems:
# Call C function to check whether the item being used is a Shine Sprite or
# Strawberry Cake.
addi %r3, %r1, 0x8
bl useSpecialItems
# Restore existing opcode.
lwz %r0, 0x2dc(%r28)

BranchBackUseSpecialItems:
b 0