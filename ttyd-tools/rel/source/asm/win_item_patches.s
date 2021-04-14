.global StartFixItemWinPartyDispOrder
.global BranchBackFixItemWinPartyDispOrder
.global StartFixItemWinPartySelectOrder
.global BranchBackFixItemWinPartySelectOrder
.global StartCheckForUnusableItemInMenu
.global ConditionalBranchCheckForUnusableItemInMenu
.global BranchBackCheckForUnusableItemInMenu
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

StartCheckForUnusableItemInMenu:
# Check to see if the player is trying to use an item on an invalid target.
bl checkForUnusableItemInMenu
cmpwi %r3, 0
# If so, branch past code responsible for processing the item use.
bne- 0xc
lwz %r3, 0x4 (%r28)
BranchBackCheckForUnusableItemInMenu:
b 0
ConditionalBranchCheckForUnusableItemInMenu:
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