.global StartSweetTreatSetupTargets
.global BranchBackSweetTreatSetupTargets
.global StartSweetTreatBlinkNumbers
.global BranchBackSweetTreatBlinkNumbers

StartSweetTreatSetupTargets:
# Replace existing logic to set up how many of each target type appears.
bl sweetTreatSetupTargets

BranchBackSweetTreatSetupTargets:
b 0

StartSweetTreatBlinkNumbers:
# Replace existing logic to support Sweet Feast's values being divided by 5.
bl sweetTreatBlinkNumbers

BranchBackSweetTreatBlinkNumbers:
b 0

# StartAudienceItem:
# stwu %sp,-0x18(%sp)
# stmw %r29,0x8(%sp)
# mr %r31,%r3
# mr %r30,%r4
# mr %r29,%r6

# mr %r3,%r0
# bl getAudienceItem
# mr %r0,%r3

# mr %r3,%r31
# mr %r4,%r30
# mr %r6,%r29
# lmw %r29,0x8(%sp)
# addi %sp,%sp,0x18

# # Restore overwritten assembly
# stw %r0,0x10(%r6)

# BranchBackAudienceItem:
# b 0