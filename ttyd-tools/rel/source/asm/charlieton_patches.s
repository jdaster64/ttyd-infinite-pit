.global CharlietonPitPriceListPatchStart
.global CharlietonPitPriceListPatchEnd
.global CharlietonPitPriceItemPatchStart
.global CharlietonPitPriceItemPatchEnd

CharlietonPitPriceListPatchStart:
# Add 31 to 0-index floor number. (reward floors become 40, 50, ... 120)
addi %r5, %r3, 31
li %r0, 1
mullw %r4, %r4, %r5
# Multiply by 1/40 of 2**32 and keep high 32 bits. (prices = 1x, 1.25x ... 3x)
lis %r3, 0x666
addi %r3, %r3, 0x6667
mulhw %r23, %r3, %r4
cmpwi %r23, 900
ble+ 0x8
li %r23, 900

CharlietonPitPriceListPatchEnd:
nop

CharlietonPitPriceItemPatchStart:
lis %r5, 0x8031
ori %r0, %r5, 0x08a8
addi %r5, %r3, 31
add %r3, %r0, %r27
lhz %r0, 0x0014 (%r3)
mullw %r0, %r0, %r5
lis %r4, 0x666
addi %r4, %r4, 0x6667
mulhw %r0, %r4, %r0
cmpwi %r0, 900
ble+ 0x8
li %r0, 900
stw %r0, 0x00a8 (%r31)
lbz %r0, 0x001c (%r3)
extsb %r0, %r0
stw %r0, 0x00ac (%r31)

CharlietonPitPriceItemPatchEnd:
nop