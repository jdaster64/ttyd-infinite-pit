.global StartGetDangerStrength
.global BranchBackGetDangerStrength
.global StartGetPerilStrength
.global BranchBackGetPerilStrength

StartGetDangerStrength:
stwu %sp, -0x10 (%sp)
mflr %r0
stw %r0, 0x14 (%sp)
stw %r4, 0xc (%sp)
bl getDangerStrength
lwz %r4, 0xc (%sp)
lwz %r0, 0x14 (%sp)
mtlr %r0
addi %sp, %sp, 0x10
BranchBackGetDangerStrength:
b 0

StartGetPerilStrength:
stwu %sp, -0x10 (%sp)
mflr %r0
stw %r0, 0x14 (%sp)
stw %r4, 0xc (%sp)
bl getPerilStrength
lwz %r4, 0xc (%sp)
lwz %r0, 0x14 (%sp)
mtlr %r0
addi %sp, %sp, 0x10
BranchBackGetPerilStrength:
b 0