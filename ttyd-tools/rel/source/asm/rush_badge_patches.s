.global StartGetDangerStrength
.global BranchBackGetDangerStrength
.global StartGetPerilStrength
.global BranchBackGetPerilStrength

# 800fd93c
StartGetDangerStrength:
bl getDangerStrength
BranchBackGetDangerStrength:
b 0

# 800fd91c
StartGetPerilStrength:
bl getPerilStrength
BranchBackGetPerilStrength:
b 0