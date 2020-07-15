.global StartFixItemWinPartyDispOrder
.global BranchBackFixItemWinPartyDispOrder
.global StartFixItemWinPartySelectOrder
.global BranchBackFixItemWinPartySelectOrder
.global StartUsePartyRankup
.global BranchBackUsePartyRankup

StartFixItemWinPartyDispOrder:
# Initially:
# %r3 = current partner id
# %r5 = pointer to stack where up to 7 WinPartyData* should be stored
# %r6 = pointer to g_winPartyDt
# %r0, %r4, %r7, %r8 are definitely safe to use
li %r4, 0
mr %r7, %r6
# Check each of the seven party members for the one that matches the current.
lwz %r0, 0x0(%r7)  # Partner id 1
cmpw %r3, %r0
bne+ 0xc
    stwx %r7, %r5, %r4
    addi %r4, %r4, 4
addi %r7, %r7, 0x24
lwz %r0, 0x0(%r7)  # Partner id 2
cmpw %r3, %r0
bne+ 0xc
    stwx %r7, %r5, %r4
    addi %r4, %r4, 4
addi %r7, %r7, 0x24
lwz %r0, 0x0(%r7)  # Partner id 3
cmpw %r3, %r0
bne+ 0xc
    stwx %r7, %r5, %r4
    addi %r4, %r4, 4
addi %r7, %r7, 0x24
lwz %r0, 0x0(%r7)  # Partner id 4
cmpw %r3, %r0
bne+ 0xc
    stwx %r7, %r5, %r4
    addi %r4, %r4, 4
addi %r7, %r7, 0x24
lwz %r0, 0x0(%r7)  # Partner id 5
cmpw %r3, %r0
bne+ 0xc
    stwx %r7, %r5, %r4
    addi %r4, %r4, 4
addi %r7, %r7, 0x24
lwz %r0, 0x0(%r7)  # Partner id 6
cmpw %r3, %r0
bne+ 0xc
    stwx %r7, %r5, %r4
    addi %r4, %r4, 4
addi %r7, %r7, 0x24
lwz %r0, 0x0(%r7)  # Partner id 7
cmpw %r3, %r0
bne+ 0xc
    stwx %r7, %r5, %r4
    addi %r4, %r4, 4
# Add all remaining party members that are currently active.
lis %r7, 0x8041
ori %r7, %r7, 0xeb00
lwz %r7, 0x0(%r7)  # Load the address of the pouch.
lwz %r0, 0x0(%r6)  # Partner id 1
mulli %r8, %r0, 0xe
lhzx %r8, %r8, %r7
rlwinm %r8, %r8, 0, 31, 31
cmpwi %r8, 0       # Check to see if partner is enabled.
beq+ 0x14
    cmpw %r3, %r0
    beq- 0xc
        stwx %r6, %r5, %r4
        addi %r4, %r4, 4
addi %r6, %r6, 0x24
lwz %r0, 0x0(%r6)  # Partner id 2
mulli %r8, %r0, 0xe
lhzx %r8, %r8, %r7
rlwinm %r8, %r8, 0, 31, 31
cmpwi %r8, 0       # Check to see if partner is enabled.
beq+ 0x14
    cmpw %r3, %r0
    beq- 0xc
        stwx %r6, %r5, %r4
        addi %r4, %r4, 4
addi %r6, %r6, 0x24
lwz %r0, 0x0(%r6)  # Partner id 3
mulli %r8, %r0, 0xe
lhzx %r8, %r8, %r7
rlwinm %r8, %r8, 0, 31, 31
cmpwi %r8, 0       # Check to see if partner is enabled.
beq+ 0x14
    cmpw %r3, %r0
    beq- 0xc
        stwx %r6, %r5, %r4
        addi %r4, %r4, 4
addi %r6, %r6, 0x24
lwz %r0, 0x0(%r6)  # Partner id 4
mulli %r8, %r0, 0xe
lhzx %r8, %r8, %r7
rlwinm %r8, %r8, 0, 31, 31
cmpwi %r8, 0       # Check to see if partner is enabled.
beq+ 0x14
    cmpw %r3, %r0
    beq- 0xc
        stwx %r6, %r5, %r4
        addi %r4, %r4, 4
addi %r6, %r6, 0x24
lwz %r0, 0x0(%r6)  # Partner id 5
mulli %r8, %r0, 0xe
lhzx %r8, %r8, %r7
rlwinm %r8, %r8, 0, 31, 31
cmpwi %r8, 0       # Check to see if partner is enabled.
beq+ 0x14
    cmpw %r3, %r0
    beq- 0xc
        stwx %r6, %r5, %r4
        addi %r4, %r4, 4
addi %r6, %r6, 0x24
lwz %r0, 0x0(%r6)  # Partner id 6
mulli %r8, %r0, 0xe
lhzx %r8, %r8, %r7
rlwinm %r8, %r8, 0, 31, 31
cmpwi %r8, 0       # Check to see if partner is enabled.
beq+ 0x14
    cmpw %r3, %r0
    beq- 0xc
        stwx %r6, %r5, %r4
        addi %r4, %r4, 4
addi %r6, %r6, 0x24
lwz %r0, 0x0(%r6)  # Partner id 7
mulli %r8, %r0, 0xe
lhzx %r8, %r8, %r7
rlwinm %r8, %r8, 0, 31, 31
cmpwi %r8, 0       # Check to see if partner is enabled.
beq+ 0x14
    cmpw %r3, %r0
    beq- 0xc
        stwx %r6, %r5, %r4
        addi %r4, %r4, 4
addi %r6, %r6, 0x24
stwx %r6, %r5, %r4  # Store "end of array" value.

BranchBackFixItemWinPartyDispOrder:
b 0

StartFixItemWinPartySelectOrder:
# Initially:
# %r3 = current partner id
# %r5 = pointer to stack where up to 7 WinPartyData* should be stored
# %r6 = pointer to g_winPartyDt
# %r0, %r4, %r7, %r8 are definitely safe to use
li %r4, 0
mr %r7, %r6
# Check each of the seven party members for the one that matches the current.
lwz %r0, 0x0(%r7)  # Partner id 1
cmpw %r3, %r0
bne+ 0xc
    stwx %r7, %r5, %r4
    addi %r4, %r4, 4
addi %r7, %r7, 0x24
lwz %r0, 0x0(%r7)  # Partner id 2
cmpw %r3, %r0
bne+ 0xc
    stwx %r7, %r5, %r4
    addi %r4, %r4, 4
addi %r7, %r7, 0x24
lwz %r0, 0x0(%r7)  # Partner id 3
cmpw %r3, %r0
bne+ 0xc
    stwx %r7, %r5, %r4
    addi %r4, %r4, 4
addi %r7, %r7, 0x24
lwz %r0, 0x0(%r7)  # Partner id 4
cmpw %r3, %r0
bne+ 0xc
    stwx %r7, %r5, %r4
    addi %r4, %r4, 4
addi %r7, %r7, 0x24
lwz %r0, 0x0(%r7)  # Partner id 5
cmpw %r3, %r0
bne+ 0xc
    stwx %r7, %r5, %r4
    addi %r4, %r4, 4
addi %r7, %r7, 0x24
lwz %r0, 0x0(%r7)  # Partner id 6
cmpw %r3, %r0
bne+ 0xc
    stwx %r7, %r5, %r4
    addi %r4, %r4, 4
addi %r7, %r7, 0x24
lwz %r0, 0x0(%r7)  # Partner id 7
cmpw %r3, %r0
bne+ 0xc
    stwx %r7, %r5, %r4
    addi %r4, %r4, 4
# Add all remaining party members that are currently active.
lis %r7, 0x8041
ori %r7, %r7, 0xeb00
lwz %r7, 0x0(%r7)  # Load the address of the pouch.
lwz %r0, 0x0(%r6)  # Partner id 1
mulli %r8, %r0, 0xe
lhzx %r8, %r8, %r7
rlwinm %r8, %r8, 0, 31, 31
cmpwi %r8, 0       # Check to see if partner is enabled.
beq+ 0x14
    cmpw %r3, %r0
    beq- 0xc
        stwx %r6, %r5, %r4
        addi %r4, %r4, 4
addi %r6, %r6, 0x24
lwz %r0, 0x0(%r6)  # Partner id 2
mulli %r8, %r0, 0xe
lhzx %r8, %r8, %r7
rlwinm %r8, %r8, 0, 31, 31
cmpwi %r8, 0       # Check to see if partner is enabled.
beq+ 0x14
    cmpw %r3, %r0
    beq- 0xc
        stwx %r6, %r5, %r4
        addi %r4, %r4, 4
addi %r6, %r6, 0x24
lwz %r0, 0x0(%r6)  # Partner id 3
mulli %r8, %r0, 0xe
lhzx %r8, %r8, %r7
rlwinm %r8, %r8, 0, 31, 31
cmpwi %r8, 0       # Check to see if partner is enabled.
beq+ 0x14
    cmpw %r3, %r0
    beq- 0xc
        stwx %r6, %r5, %r4
        addi %r4, %r4, 4
addi %r6, %r6, 0x24
lwz %r0, 0x0(%r6)  # Partner id 4
mulli %r8, %r0, 0xe
lhzx %r8, %r8, %r7
rlwinm %r8, %r8, 0, 31, 31
cmpwi %r8, 0       # Check to see if partner is enabled.
beq+ 0x14
    cmpw %r3, %r0
    beq- 0xc
        stwx %r6, %r5, %r4
        addi %r4, %r4, 4
addi %r6, %r6, 0x24
lwz %r0, 0x0(%r6)  # Partner id 5
mulli %r8, %r0, 0xe
lhzx %r8, %r8, %r7
rlwinm %r8, %r8, 0, 31, 31
cmpwi %r8, 0       # Check to see if partner is enabled.
beq+ 0x14
    cmpw %r3, %r0
    beq- 0xc
        stwx %r6, %r5, %r4
        addi %r4, %r4, 4
addi %r6, %r6, 0x24
lwz %r0, 0x0(%r6)  # Partner id 6
mulli %r8, %r0, 0xe
lhzx %r8, %r8, %r7
rlwinm %r8, %r8, 0, 31, 31
cmpwi %r8, 0       # Check to see if partner is enabled.
beq+ 0x14
    cmpw %r3, %r0
    beq- 0xc
        stwx %r6, %r5, %r4
        addi %r4, %r4, 4
addi %r6, %r6, 0x24
lwz %r0, 0x0(%r6)  # Partner id 7
mulli %r8, %r0, 0xe
lhzx %r8, %r8, %r7
rlwinm %r8, %r8, 0, 31, 31
cmpwi %r8, 0       # Check to see if partner is enabled.
beq+ 0x14
    cmpw %r3, %r0
    beq- 0xc
        stwx %r6, %r5, %r4
        addi %r4, %r4, 4
addi %r6, %r6, 0x24
stwx %r6, %r5, %r4  # Store "end of array" value.

BranchBackFixItemWinPartySelectOrder:
b 0

StartUsePartyRankup:
# Call C function to check whether the item being used is a Shine Sprite.
bl usePartyRankup
# Restore existing opcode.
lwz %r0, 0x2dc(%r28)

BranchBackUsePartyRankup:
b 0