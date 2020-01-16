abcd  NB     M_DREG          M_DREG          %1100rrr100000rrr   m_abcd    +
-     NB     M_APREDEC       M_APREDEC       %1100rrr100001rrr   m_abcd

add   NBWL   C_ALL           M_DREG          %1101rrr0sseR1100   m_ea      +
-     NBWL   M_DREG          C_ALTMEM        %1101rrr1sseR0101   m_ea      +
adda  NWL    C_ALL           M_AREG          %1101rrrs11eeeeee   m_adda    +
addi  NWBL   M_IMMED         C_ALTDATA       %00000110sseeeS11   m_ea      +
-     NWBL   M_IMMED         C_ALTDATA       %00000110sseeeS10   m_ea030

addq  NBWL   M_IMMED         C_ALT           %0101ddd0sseeeeee   m_addq

addx  NBWL   M_DREG          M_DREG          %1101rrr1ss000rrS   m_abcd    +
-     NBWL   M_APREDEC       M_APREDEC       %1101rrr1ss001rrS   m_abcd

and   NBWL   C_DATA          M_DREG          %1100rrr0sseR1S00   m_ea      +
-     NBWL   M_DREG          C_ALTMEM        %1100rrr1sseR0S01   m_ea      +
andi  NBWL   M_IMMED         C_ALTDATA       %00000010sseeeS11   m_ea      +
-     NBWL   M_IMMED         C_DATA030       %00000010sseeeS10   m_ea030   +
-     NB     M_IMMED         M_AM_CCR        %0000001000111100   m_imm8    +
-     NW     M_IMMED         M_AM_SR         %0000001001111100   m_imm

asl   NBWL   M_DREG          M_DREG          %1110rrr1ss100rrr   m_shr     +
-     NBWL   M_IMMED         M_DREG          %1110ccc1ss000rrr   m_shi     +
-     NBWL   M_DREG          M_AM_NONE       %11100011ss000rrS   m_reg     +
-     NW     C_ALTMEM        M_AM_NONE       %1110000111eee000   m_ea

asr   NBWL   M_DREG          M_DREG          %1110rrr0ss100rrr   m_shr     +
-     NBWL   M_IMMED         M_DREG          %1110ccc0ss000rrr   m_shi     +
-     NBWL   M_DREG          M_AM_NONE       %11100010ss000rrS   m_reg     +
-     NW     C_ALTMEM        M_AM_NONE       %1110000011eee000   m_ea

bcc   NBWS   C_LABEL         M_AM_NONE       %01100100bbbbbbbb   m_br +
bhs
-     L      C_LABEL         M_AM_NONE       %0110010011111111   m_br30
bcs   NBWS   C_LABEL         M_AM_NONE       %01100101bbbbbbbb   m_br +
blo
-     L      C_LABEL         M_AM_NONE       %0110010111111111   m_br30
beq   NBWS   C_LABEL         M_AM_NONE       %01100111bbbbbbbb   m_br +
bze
bz
-     L      C_LABEL         M_AM_NONE       %0110011111111111   m_br30
bge   NBWS   C_LABEL         M_AM_NONE       %01101100bbbbbbbb   m_br +
-     L      C_LABEL         M_AM_NONE       %0110110011111111   m_br30
bgt   NBWS   C_LABEL         M_AM_NONE       %01101110bbbbbbbb   m_br +
-     L      C_LABEL         M_AM_NONE       %0110111011111111   m_br30
bhi   NBWS   C_LABEL         M_AM_NONE       %01100010bbbbbbbb   m_br +
-     L      C_LABEL         M_AM_NONE       %0110001011111111   m_br30
ble   NBWS   C_LABEL         M_AM_NONE       %01101111bbbbbbbb   m_br +
-     L      C_LABEL         M_AM_NONE       %0110111111111111   m_br30
bls   NBWS   C_LABEL         M_AM_NONE       %01100011bbbbbbbb   m_br +
-     L      C_LABEL         M_AM_NONE       %0110001111111111   m_br30
blt   NBWS   C_LABEL         M_AM_NONE       %01101101bbbbbbbb   m_br +
-     L      C_LABEL         M_AM_NONE       %0110110111111111   m_br30
bmi   NBWS   C_LABEL         M_AM_NONE       %01101011bbbbbbbb   m_br +
-     L      C_LABEL         M_AM_NONE       %0110101111111111   m_br30
bne   NBWS   C_LABEL         M_AM_NONE       %01100110bbbbbbbb   m_br +
bnz
-     L      C_LABEL         M_AM_NONE       %01100110bbbbbbbb   m_br30
bpl   NBWS   C_LABEL         M_AM_NONE       %01101010bbbbbbbb   m_br +
-     L      C_LABEL         M_AM_NONE       %0110101011111111   m_br30
bvc   NBWS   C_LABEL         M_AM_NONE       %01101000bbbbbbbb   m_br +
-     L      C_LABEL         M_AM_NONE       %0110100011111111   m_br30
bvs   NBWS   C_LABEL         M_AM_NONE       %01101001bbbbbbbb   m_br +
-     L      C_LABEL         M_AM_NONE       %0110100111111111   m_br30

bchg  NL     M_DREG          M_DREG          %0000rrr101eeeeee   m_bitop  +
-     NB     M_DREG          C_ALTDATA       %0000rrr101eeeeee   m_bitop  +
-     NL     M_IMMED         M_DREG          %0000100001eeeeee   m_bitop  +
-     NB     M_IMMED         C_ALTDATA       %0000100001eeeeee   m_bitop

bclr  NL     M_DREG          M_DREG          %0000rrr110eeeeee   m_bitop  +
-     NB     M_DREG          C_ALTDATA       %0000rrr110eeeeee   m_bitop  +
-     NL     M_IMMED         M_DREG          %0000100010eeeeee   m_bitop  +
-     NB     M_IMMED         C_ALTDATA       %0000100010eeeeee   m_bitop

bfchg N      C_BF1+M_BITFLD  M_AM_NONE       %1110101011eeeeee   m_bfop
bfclr N      C_BF1+M_BITFLD  M_AM_NONE       %1110110011eeeeee   m_bfop
bfexts N     C_BF2+M_BITFLD  M_DREG          %1110101111eeeeee   m_bfop
bfextu N     C_BF2+M_BITFLD  M_DREG          %1110100111eeeeee   m_bfop
bfffo N      C_BF2+M_BITFLD  M_DREG          %1110110111eeeeee   m_bfop
bfins N      M_DREG          C_BF1+M_BITFLD  %1110111111eeeeee   m_bfop
bfset N      C_BF1+M_BITFLD  M_AM_NONE       %1110111011eeeeee   m_bfop
bftst N      C_BF2+M_BITFLD  M_AM_NONE       %1110100011eeeeee   m_bfop

bkpt  N      M_IMMED         M_AM_NONE       %0100100001001bbb   m_bkpt

bra   NBWS   C_LABEL         M_AM_NONE       %01100000bbbbbbbb   m_br +
-     L      C_LABEL         M_AM_NONE       %0110000011111111   m_br30
bt

bset  NL     M_DREG          M_DREG          %0000rrr111eeeeee   m_bitop  +
-     NB     M_DREG          C_ALTDATA       %0000rrr111eeeeee   m_bitop  +
-     NL     M_IMMED         M_DREG          %0000100011eeeeee   m_bitop  +
-     NB     M_IMMED         C_ALTDATA       %0000100011eeeeee   m_bitop

bsr   NBWS   C_LABEL         M_AM_NONE       %01100001bbbbbbbb   m_br +
-     L      C_LABEL         M_AM_NONE       %0110000111111111   m_br30

btst  NL     M_DREG          M_DREG          %0000rrr100eeeeee   m_bitop  +
-     NB     M_DREG          C_DATA          %0000rrr100eeeeee   m_bitop  +
-     NL     M_IMMED         M_DREG          %0000100000eeeeee   m_bitop  +
-     NB     M_IMMED         C_DATA-M_IMMED  %0000100000eeeeee   m_bitop

callm N      M_IMMED         C_ALT030        %0000011011eeeeee   m_callm

cas   !      M_AM_NONE       M_AM_NONE       %00001ss011eeeeee   m_cas
cas2  !      M_AM_NONE       M_AM_NONE       %00001ss011111100   m_cas2

chk   NW     C_DATA          M_DREG          %0100rrr110eR1000   m_ea +
-     L      C_DATA030       M_DREG          %0100rrr100eeeeee   m_ea030

cinvl N      M_CACHE40       M_AIND          %11110100cc001rrr   m_cinv
cinvp N      M_CACHE40       M_AIND          %11110100cc010rrr   m_cinv
cinva N      M_CACHE40       M_AM_NONE       %11110100cc011rrr   m_cinv

chk2  NBWL   C_DATA          M_DREG+M_AREG   %00000ss011eeeeee   m_chk2

clr   L      M_DREG          M_AM_NONE       %0100001010000eee   m_clrd    +
-     NBWL   C_ALTDATA       M_AM_NONE       %01000010sseeeS00   m_ea      +
-     NWL    M_AREG          M_AM_NONE       %1001rrrs11001rrr   m_clra

cmp   NWL    M_AREG          M_DREG          %1011rrr0sseR1S00   m_ea      +
-     NBWL   C_ALL           M_DREG          %1011rrr0sseR1S00   m_ea      +
cmpa  NWL    C_ALL           M_AREG          %1011rrrs11eeeeee   m_adda    +
cmpi  NBWL   M_IMMED         C_ALTDATA       %00001100sseeeS11   m_ea	   +
-     NBWL   M_IMMED         C_ALT030        %00001100sseeeS10   m_ea030   +
cmpm  NBWL   M_APOSTINC      M_APOSTINC      %1011xxx1ss001yRS   m_reg

cmp2  NBWL   C_ALL030        M_DREG+M_AREG   %00000ss011eeeeee   m_cmp2

cpbeq NBW    C_LABEL         M_AM_NONE       %111100101s000001   m_cpbcc
cpbze
cpbz
cpbge NBW    C_LABEL         M_AM_NONE       %111100101s010011   m_cpbcc
cpbgt NBW    C_LABEL         M_AM_NONE       %111100101s010010   m_cpbcc
cpbgl NBW    C_LABEL         M_AM_NONE       %111100101s010110   m_cpbcc
cpbgle NBW   C_LABEL         M_AM_NONE       %111100101s010111   m_cpbcc
cpble NBW    C_LABEL         M_AM_NONE       %111100101s010101   m_cpbcc
cpblt NBW    C_LABEL         M_AM_NONE       %111100101s010100   m_cpbcc
cpbne NBW    C_LABEL         M_AM_NONE       %111100101s001110   m_cpbcc
cpbngt NBW   C_LABEL         M_AM_NONE       %111100101s011101   m_cpbcc
cpbnge NBW   C_LABEL         M_AM_NONE       %111100101s011100   m_cpbcc
cpbngl NBW   C_LABEL         M_AM_NONE       %111100101s011001   m_cpbcc
cpbnle NBW   C_LABEL         M_AM_NONE       %111100101s011010   m_cpbcc
cpbngle NBW  C_LABEL         M_AM_NONE       %111100101s011000   m_cpbcc
cpbnz
cpbogt NBW   C_LABEL         M_AM_NONE       %111100101s000010   m_cpbcc
cpbule NBW   C_LABEL         M_AM_NONE       %111100101s001101   m_cpbcc
cpboge NBW   C_LABEL         M_AM_NONE       %111100101s000011   m_cpbcc
cpbult NBW   C_LABEL         M_AM_NONE       %111100101s001100   m_cpbcc
cpbolt NBW   C_LABEL         M_AM_NONE       %111100101s000100   m_cpbcc
cpbuge NBW   C_LABEL         M_AM_NONE       %111100101s001011   m_cpbcc
cpbole NBW   C_LABEL         M_AM_NONE       %111100101s000101   m_cpbcc
cpbugt NBW   C_LABEL         M_AM_NONE       %111100101s001010   m_cpbcc
cpbogl NBW   C_LABEL         M_AM_NONE       %111100101s000110   m_cpbcc
cpbueq NBW   C_LABEL         M_AM_NONE       %111100101s001001   m_cpbcc
cpbor NBW    C_LABEL         M_AM_NONE       %111100101s000111   m_cpbcc
cpbun NBW    C_LABEL         M_AM_NONE       %111100101s001000   m_cpbcc
cpbt  NBW    C_LABEL         M_AM_NONE       %111100101s001111   m_cpbcc
cpbra
cpbf  NBW    C_LABEL         M_AM_NONE       %111100101s000000   m_cpbcc
cpbsf NBW    C_LABEL         M_AM_NONE       %111100101s010000   m_cpbcc
cpbst NBW    C_LABEL         M_AM_NONE       %111100101s011111   m_cpbcc
cpbseq NBW   C_LABEL         M_AM_NONE       %111100101s010001   m_cpbcc
cpbsne NBW   C_LABEL         M_AM_NONE       %111100101s011110   m_cpbcc

cpdbeq NBW   M_DREG          C_LABEL         %111100101s000001   m_cpdbr
cpdbze
cpdbz
cpdbge NBW   M_DREG          C_LABEL         %111100101s010011   m_cpdbr
cpdbgt NBW   M_DREG          C_LABEL         %111100101s010010   m_cpdbr
cpdbgl NBW   M_DREG          C_LABEL         %111100101s010110   m_cpdbr
cpdbgle NBW  M_DREG          C_LABEL         %111100101s010111   m_cpdbr
cpdble NBW   M_DREG          C_LABEL         %111100101s010101   m_cpdbr
cpdblt NBW   M_DREG          C_LABEL         %111100101s010100   m_cpdbr
cpdbne NBW   M_DREG          C_LABEL         %111100101s001110   m_cpdbr
cpdbngt NBW  M_DREG          C_LABEL         %111100101s011101   m_cpdbr
cpdbnge NBW  M_DREG          C_LABEL         %111100101s011100   m_cpdbr
cpdbngl NBW  M_DREG          C_LABEL         %111100101s011001   m_cpdbr
cpdbnle NBW  M_DREG          C_LABEL         %111100101s011010   m_cpdbr
cpdbngle NBW M_DREG          C_LABEL         %111100101s011000   m_cpdbr
cpdbnz
cpdbogt NBW  M_DREG          C_LABEL         %111100101s000010   m_cpdbr
cpdbule NBW  M_DREG          C_LABEL         %111100101s001101   m_cpdbr
cpdboge NBW  M_DREG          C_LABEL         %111100101s000011   m_cpdbr
cpdbult NBW  M_DREG          C_LABEL         %111100101s001100   m_cpdbr
cpdbolt NBW  M_DREG          C_LABEL         %111100101s000100   m_cpdbr
cpdbuge NBW  M_DREG          C_LABEL         %111100101s001011   m_cpdbr
cpdbole NBW  M_DREG          C_LABEL         %111100101s000101   m_cpdbr
cpdbugt NBW  M_DREG          C_LABEL         %111100101s001010   m_cpdbr
cpdbogl NBW  M_DREG          C_LABEL         %111100101s000110   m_cpdbr
cpdbueq NBW  M_DREG          C_LABEL         %111100101s001001   m_cpdbr
cpdbor NBW   M_DREG          C_LABEL         %111100101s000111   m_cpdbr
cpdbun NBW   M_DREG          C_LABEL         %111100101s001000   m_cpdbr
cpdbt  NBW   M_DREG          C_LABEL         %111100101s001111   m_cpdbr
cpdbra
cpdbf  NBW   M_DREG          C_LABEL         %111100101s000000   m_cpdbr
cpdbsf NBW   M_DREG          C_LABEL         %111100101s010000   m_cpdbr
cpdbst NBW   M_DREG          C_LABEL         %111100101s011111   m_cpdbr
cpdbseq NBW  M_DREG          C_LABEL         %111100101s010001   m_cpdbr
cpdbsne NBW  M_DREG          C_LABEL         %111100101s011110   m_cpdbr

cprestore N  C_ALL030        M_AM_NONE       %1111001101eeeeee   m_cprest
cpsave N     C_ALL030        M_AM_NONE       %1111001100eeeeee   m_cprest

cpsbs   BN   C_MOVES         M_AM_NONE       %111100100100000    m_fscc
cpsbc   BN   C_MOVES         M_AM_NONE       %111100100100001    m_fscc
cpsls   BN   C_MOVES         M_AM_NONE       %111100100100010    m_fscc
cpslc   BN   C_MOVES         M_AM_NONE       %111100100100011    m_fscc
cpsss   BN   C_MOVES         M_AM_NONE       %111100100100100    m_fscc
cpssc   BN   C_MOVES         M_AM_NONE       %111100100100101    m_fscc
cpsas   BN   C_MOVES         M_AM_NONE       %111100100100110    m_fscc
cpsac   BN   C_MOVES         M_AM_NONE       %111100100100111    m_fscc
cpsws   BN   C_MOVES         M_AM_NONE       %111100100101000    m_fscc
cpswc   BN   C_MOVES         M_AM_NONE       %111100100101001    m_fscc
cpsis   BN   C_MOVES         M_AM_NONE       %111100100101010    m_fscc
cpsic   BN   C_MOVES         M_AM_NONE       %111100100101011    m_fscc
cpsgc   BN   C_MOVES         M_AM_NONE       %111100100101100    m_fscc
cpsgs   BN   C_MOVES         M_AM_NONE       %111100100101101    m_fscc
cpscs   BN   C_MOVES         M_AM_NONE       %111100100101110    m_fscc
cpscc   BN   C_MOVES         M_AM_NONE       %111100100101111    m_fscc


cpushl N     CACHES          M_AIND          %11110100cc001rrr   m_cinv
cpushp N     CACHES          M_AIND          %11110100cc010rrr   m_cinv
cpusha N     CACHES          M_AM_NONE       %11110100cc011rrr   m_cinv

dbcc  NW     M_DREG          C_LABEL         %0101010011001rrr   m_dbra
dbcs  NW     M_DREG          C_LABEL         %0101010111001rrr   m_dbra
dblo
dbeq  NW     M_DREG          C_LABEL         %0101011111001rrr   m_dbra
dbze
dbf   NW     M_DREG          C_LABEL         %0101000111001rrr   m_dbra
dbra
dbge  NW     M_DREG          C_LABEL         %0101110011001rrr   m_dbra
dbgt  NW     M_DREG          C_LABEL         %0101111011001rrr   m_dbra
dbhi  NW     M_DREG          C_LABEL         %0101001011001rrr   m_dbra
dbhs
dble  NW     M_DREG          C_LABEL         %0101111111001rrr   m_dbra
dbls  NW     M_DREG          C_LABEL         %0101001111001rrr   m_dbra
dblt  NW     M_DREG          C_LABEL         %0101110111001rrr   m_dbra
dbmi  NW     M_DREG          C_LABEL         %0101101111001rrr   m_dbra
dbne  NW     M_DREG          C_LABEL         %0101011011001rrr   m_dbra
dbnz
dbpl  NW     M_DREG          C_LABEL         %0101101011001rrr   m_dbra
dbt   NW     M_DREG          C_LABEL         %0101000011001rrr   m_dbra
dbvc  NW     M_DREG          C_LABEL         %0101100011001rrr   m_dbra
dbvs  NW     M_DREG          C_LABEL         %0101100111001rrr   m_dbra

divs  NW     C_DATA          M_DREG          %1000rrr111eR1000   m_ea     +
-     L      C_DATA030       M_DREG          %0100111101eeeeee   m_muls

divsl NL     C_DATA030       M_DREG          %0100111001eeeeee   m_muls

divu  NW     C_DATA          M_DREG          %1000rrr011eR1000   m_ea     +
-     L      C_DATA030       M_DREG          %0100110101eeeeee   m_muls

divul LN     C_DATA030       M_DREG          %0100110001eeeeee   m_muls

eor   NBWL   M_DREG          C_ALTDATA       %1011rrr1sseR0S01   m_ea      +
eori  NBWL   M_IMMED         C_ALTDATA       %00001010sseeeS11   m_ea      +
-     NBWL   M_IMMED         C_ALT030        %00001010sseeeS10   m_ea030   +
-     NB     M_IMMED         M_AM_CCR        %0000101000111100   m_imm8    +
-     NW     M_IMMED         M_AM_SR         %0000101001111100   m_imm

exg   NL     M_DREG|M_AREG   M_DREG|M_AREG   %1100rrr1ooooorrr   m_exg

ext   NW     M_DREG          M_AM_NONE       %0100100010000rrr   m_reg     +
-     L      M_DREG          M_AM_NONE       %0100100011000rrr   m_reg
extb  NL     M_DREG          M_AM_NONE       %0100100111000rrr   m_reg

fabs  NBWLSDXP C_ALL030      M_FREG          %1111001000eeeeee   m_fabs    +
-     NX       M_FREG        M_FREG          %1111001000eeeeee   m_fabs    +
-     NX       M_FREG        M_AM_NONE       %1111001000eeeeee   m_fabs
fsabs NBWLSDXP C_ALL030      M_FREG          %1111001000eeeeee   m_fsabs   +
-     NX       M_FREG        M_FREG          %1111001000eeeeee   m_fsabs   +
-     NX       M_FREG        M_AM_NONE       %1111001000eeeeee   m_fsabs
fdabs NBWLSDXP C_ALL030      M_FREG          %1111001000eeeeee   m_fdabs   +
-     NX       M_FREG        M_FREG          %1111001000eeeeee   m_fdabs   +
-     NX       M_FREG        M_AM_NONE       %1111001000eeeeee   m_fdabs
facos NBWLSDXP C_ALL030      M_FREG          %1111001000eeeeee   m_facos   +
-     NX       M_FREG        M_FREG          %1111001000eeeeee   m_facos   +
-     NX       M_FREG        M_AM_NONE       %1111001000eeeeee   m_facos
fadd  NBWLSDXP C_ALL030      M_FREG          %1111001000eeeeee   m_fadd    +
-     NX       M_FREG        M_FREG          %1111001000eeeeee   m_fadd
fsadd NBWLSDXP C_ALL030      M_FREG          %1111001000eeeeee   m_fsadd   +
-     NX       M_FREG        M_FREG          %1111001000eeeeee   m_fsadd
fdadd NBWLSDXP C_ALL030      M_FREG          %1111001000eeeeee   m_fdadd   +
-     NX       M_FREG        M_FREG          %1111001000eeeeee   m_fdadd
fasin NBWLSDXP C_ALL030      M_FREG          %1111001000eeeeee   m_fasin   +
-     NX       M_FREG        M_FREG          %1111001000eeeeee   m_fasin   +
-     NX       M_FREG        M_AM_NONE       %1111001000eeeeee   m_fasin
fatan NBWLSDXP C_ALL030      M_FREG          %1111001000eeeeee   m_fatan   +
-     NX       M_FREG        M_FREG          %1111001000eeeeee   m_fatan   +
-     NX       M_FREG        M_AM_NONE       %1111001000eeeeee   m_fatan
fatanh NBWLSDXP C_ALL030     M_FREG          %1111001000eeeeee   m_fatanh  +
-      NX       M_FREG        M_FREG          %1111001000eeeeee   m_fatanh +
-      NX       M_FREG        M_AM_NONE       %1111001000eeeeee   m_fatanh

fbeq  NBW      C_LABEL       M_AM_NONE       %111100101s000001   m_fbcc
fbze
fbz
fbge  NWL      C_LABEL       M_AM_NONE       %111100101s010011   m_fbcc
fbgt  NWL      C_LABEL       M_AM_NONE       %111100101s010010   m_fbcc
fbgl  NWL      C_LABEL       M_AM_NONE       %111100101s010110   m_fbcc
fbgle NWL      C_LABEL       M_AM_NONE       %111100101s010111   m_fbcc
fble  NWL      C_LABEL       M_AM_NONE       %111100101s010101   m_fbcc
fblt  NWL      C_LABEL       M_AM_NONE       %111100101s010100   m_fbcc
fbne  NWL      C_LABEL       M_AM_NONE       %111100101s001110   m_fbcc
fbngt NWL      C_LABEL       M_AM_NONE       %111100101s011101   m_fbcc
fbnge NWL      C_LABEL       M_AM_NONE       %111100101s011100   m_fbcc
fbngl NWL      C_LABEL       M_AM_NONE       %111100101s011001   m_fbcc
fbnle NWL      C_LABEL       M_AM_NONE       %111100101s011010   m_fbcc
fbnlt NWL      C_LABEL       M_AM_NONE       %111100101s011011   m_fbcc
fbngle NWL     C_LABEL       M_AM_NONE       %111100101s011000   m_fbcc
fbnz
fbogt NWL      C_LABEL       M_AM_NONE       %111100101s000010   m_fbcc
fbule NWL      C_LABEL       M_AM_NONE       %111100101s001101   m_fbcc
fboge NWL      C_LABEL       M_AM_NONE       %111100101s000011   m_fbcc
fbult NWL      C_LABEL       M_AM_NONE       %111100101s001100   m_fbcc
fbolt NWL      C_LABEL       M_AM_NONE       %111100101s000100   m_fbcc
fbuge NWL      C_LABEL       M_AM_NONE       %111100101s001011   m_fbcc
fbole NWL      C_LABEL       M_AM_NONE       %111100101s000101   m_fbcc
fbugt NWL      C_LABEL       M_AM_NONE       %111100101s001010   m_fbcc
fbogl NWL      C_LABEL       M_AM_NONE       %111100101s000110   m_fbcc
fbueq NWL      C_LABEL       M_AM_NONE       %111100101s001001   m_fbcc
fbor  NWL      C_LABEL       M_AM_NONE       %111100101s000111   m_fbcc
fbun  NWL      C_LABEL       M_AM_NONE       %111100101s001000   m_fbcc
fbt   NWL      C_LABEL       M_AM_NONE       %111100101s001111   m_fbcc
fbra
fbf   NWL      C_LABEL       M_AM_NONE       %111100101s000000   m_fbcc
fbsf  NWL      C_LABEL       M_AM_NONE       %111100101s010000   m_fbcc
fbst  NWL      C_LABEL       M_AM_NONE       %111100101s011111   m_fbcc
fbseq NWL      C_LABEL       M_AM_NONE       %111100101s010001   m_fbcc
fbsne NWL      C_LABEL       M_AM_NONE       %111100101s011110   m_fbcc

fcmp  NBWLSDXP C_ALL030      M_FREG          %1111001000eeeeee   m_fcmp   +
-     NX       M_FREG        M_FREG          %1111001000eeeeee   m_fcmp
fcos  NBWLSDXP C_ALL030      M_FREG          %1111001000eeeeee   m_fcos    +
-     NX       M_FREG        M_FREG          %1111001000eeeeee   m_fcos    +
-     NX       M_FREG        M_AM_NONE       %1111001000eeeeee   m_fcos
fcosh NBWLSDXP C_ALL030      M_FREG          %1111001000eeeeee   m_fcosh   +
-     NX       M_FREG        M_FREG          %1111001000eeeeee   m_fcosh   +
-     NX       M_FREG        M_AM_NONE       %1111001000eeeeee   m_fcosh

fdbeq  N       M_DREG        C_LABEL         %1111001001000001  m_fdbcc
fdbze
fdbz
fdbge  N       M_DREG        C_LABEL         %1111001001010011  m_fdbcc
fdbgt  N       M_DREG        C_LABEL         %1111001001010010  m_fdbcc
fdbgl  N       M_DREG        C_LABEL         %1111001001010110  m_fdbcc
fdbgle N       M_DREG        C_LABEL         %1111001001010111  m_fdbcc
fdble  N       M_DREG        C_LABEL         %1111001001010101  m_fdbcc
fdblt  N       M_DREG        C_LABEL         %1111001001010100  m_fdbcc
fdbne  N       M_DREG        C_LABEL         %1111001001001110  m_fdbcc
fdbngt N       M_DREG        C_LABEL         %1111001001011101  m_fdbcc
fdbnge N       M_DREG        C_LABEL         %1111001001011100  m_fdbcc
fdbngl N       M_DREG        C_LABEL         %1111001001011001  m_fdbcc
fdbnle N       M_DREG        C_LABEL         %1111001001011010  m_fdbcc
fdbnlt N       M_DREG        C_LABEL         %1111001001011011  m_fdbcc
fdbngle N      M_DREG        C_LABEL         %1111001001011000  m_fdbcc
fdbnz
fdbogt N       M_DREG        C_LABEL         %1111001001000010  m_fdbcc
fdbule N       M_DREG        C_LABEL         %1111001001001101  m_fdbcc
fdboge N       M_DREG        C_LABEL         %1111001001000011  m_fdbcc
fdbult N       M_DREG        C_LABEL         %1111001001001100  m_fdbcc
fdbolt N       M_DREG        C_LABEL         %1111001001000100  m_fdbcc
fdbuge N       M_DREG        C_LABEL         %1111001001001011  m_fdbcc
fdbole N       M_DREG        C_LABEL         %1111001001000101  m_fdbcc
fdbugt N       M_DREG        C_LABEL         %1111001001001010  m_fdbcc
fdbogl N       M_DREG        C_LABEL         %1111001001000110  m_fdbcc
fdbueq N       M_DREG        C_LABEL         %1111001001001001  m_fdbcc
fdbor  N       M_DREG        C_LABEL         %1111001001000111  m_fdbcc
fdbun  N       M_DREG        C_LABEL         %1111001001001000  m_fdbcc
fdbt   N       M_DREG        C_LABEL         %1111001001001111  m_fdbcc
fdbra  N       M_DREG        C_LABEL         %1111001001000000  m_fdbcc
fdbf   N       M_DREG        C_LABEL         %1111001001000000  m_fdbcc
fdbsf  N       M_DREG        C_LABEL         %1111001001010000  m_fdbcc
fdbst  N       M_DREG        C_LABEL         %1111001001011111  m_fdbcc
fdbseq N       M_DREG        C_LABEL         %1111001001010001  m_fdbcc
fdbsne N       M_DREG        C_LABEL         %1111001001011110  m_fdbcc

fdiv  NBWLSDXP  C_ALL030     M_FREG          %1111001000eeeeee  m_fdiv      +
-     NX        M_FREG       M_FREG          %1111001000eeeeee  m_fdiv      +
-     NX        M_FREG       M_AM_NONE       %1111001000eeeeee  m_fdiv
fsdiv NBWLSDXP  C_ALL030     M_FREG          %1111001000eeeeee  m_fsdiv     +
-     NX        M_FREG       M_FREG          %1111001000eeeeee  m_fsdiv     +
-     NX        M_FREG       M_AM_NONE       %1111001000eeeeee  m_fsdiv
fddiv NBWLSDXP  C_ALL030     M_FREG          %1111001000eeeeee  m_fddiv     +
-     NX        M_FREG       M_FREG          %1111001000eeeeee  m_fddiv     +
-     NX        M_FREG       M_AM_NONE       %1111001000eeeeee  m_fddiv
fetox NBWLSDXP  C_ALL030     M_FREG          %1111001000eeeeee  m_fetox     +
-     NX        M_FREG       M_FREG          %1111001000eeeeee  m_fetox     +
-     NX        M_FREG       M_AM_NONE       %1111001000eeeeee  m_fetox
fetoxm1 NBWLSDXP C_ALL030    M_FREG          %1111001000eeeeee  m_fetoxm1   +
-       NX       M_FREG      M_FREG          %1111001000eeeeee  m_fetoxm1   +
-       NX       M_FREG      M_AM_NONE       %1111001000eeeeee  m_fetoxm1
fgetexp NBWLSDXP C_ALL030    M_FREG          %1111001000eeeeee  m_fgetexp   +
-       NX       M_FREG      M_FREG          %1111001000eeeeee  m_fgetexp   +
-       NX       M_FREG      M_AM_NONE       %1111001000eeeeee  m_fgetexp
fgetman NBWLSDXP C_ALL030    M_FREG          %1111001000eeeeee  m_fgetman   +
-       NX       M_FREG      M_FREG          %1111001000eeeeee  m_fgetman   +
-       NX       M_FREG      M_AM_NONE       %1111001000eeeeee  m_fgetman
fint  NBWLSDXP  C_ALL030     M_FREG          %1111001000eeeeee  m_fint      +
-     NX        M_FREG       M_FREG          %1111001000eeeeee  m_fint      +
-     NX        M_FREG       M_AM_NONE       %1111001000eeeeee  m_fint
fintrz NBWLSDXP C_ALL030     M_FREG          %1111001000eeeeee  m_fintrz    + 
-      NX       M_FREG       M_FREG          %1111001000eeeeee  m_fintrz    +
-      NX       M_FREG       M_AM_NONE       %1111001000eeeeee  m_fintrz
flog10 NBWLSDXP C_ALL030     M_FREG          %1111001000eeeeee  m_flog10    + 
-      NX       M_FREG       M_FREG          %1111001000eeeeee  m_flog10    +
-      NX       M_FREG       M_AM_NONE       %1111001000eeeeee  m_flog10
flog2  NBWLSDXP C_ALL030     M_FREG          %1111001000eeeeee  m_flog2     +
-      NX       M_FREG       M_FREG          %1111001000eeeeee  m_flog2     +
-      NX       M_FREG       M_AM_NONE       %1111001000eeeeee  m_flog2
flogn  NBWLSDXP C_ALL030     M_FREG          %1111001000eeeeee  m_flogn     +
-      NX       M_FREG       M_FREG          %1111001000eeeeee  m_flogn     +
-      NX       M_FREG       M_AM_NONE       %1111001000eeeeee  m_flogn
flognp1 NBWLSDXP C_ALL030     M_FREG         %1111001000eeeeee  m_flognp1   + 
-       NX       M_FREG       M_FREG         %1111001000eeeeee  m_flognp1   +
-       NX       M_FREG       M_AM_NONE      %1111001000eeeeee  m_flognp1
fmod   NBWLSDXP C_ALL030     M_FREG          %1111001000eeeeee  m_fmod      +
-      NX       M_FREG       M_FREG          %1111001000eeeeee  m_fmod      +
-      NX       M_FREG       M_AM_NONE       %1111001000eeeeee  m_fmod
fmove  NBWLSDXP C_ALL030     M_FREG          %1111001000eeeeee  m_fmove     +
-      NBWLSDXP M_FREG       C_ALT030        %1111001000eeeeee  m_fmove     +
-      NX       M_FREG       M_FREG          %1111001000eeeeee  m_fmove     +
-      NL       M_FPSCR      C_ALT030        %1111001000eeeeee  m_fmovescr  +
-      NL       C_ALL030     M_FPSCR         %1111001000eeeeee  m_fmovescr  +
fsmove NBWLSDXP C_ALL030     M_FREG          %1111001000eeeeee  m_fsmove
fdmove NBWLSDXP C_ALL030     M_FREG          %1111001000eeeeee  m_fdmove
fmovecr NX      M_IMMED      M_FREG          %1111001000000000  m_fmovecr
fmovem !        M_AM_NONE    M_AM_NONE       %1111001000eeeeee  m_fmovem
fmul  NBWLSDXP C_ALL030      M_FREG          %1111001000eeeeee  m_fmul      +
-     NX       M_FREG        M_FREG          %1111001000eeeeee  m_fmul    
fsmul NBWLSDXP C_ALL030      M_FREG          %1111001000eeeeee  m_fsmul     +
-     NX       M_FREG        M_FREG          %1111001000eeeeee  m_fsmul   
fdmul NX       C_ALL030      M_FREG          %1111001000eeeeee  m_fdmul     +
-     NX       M_FREG        M_FREG          %1111001000eeeeee  m_fdmul   
fneg  NBWLSDXP C_ALL030      M_FREG          %1111001000eeeeee  m_fneg      +
-     NX       M_FREG        M_FREG          %1111001000eeeeee  m_fneg      +
-     NX       M_FREG        M_AM_NONE       %1111001000eeeeee  m_fneg
fsneg NBWLSDXP C_ALL030      M_FREG          %1111001000eeeeee  m_fsneg     +
-     NX       M_FREG        M_FREG          %1111001000eeeeee  m_fsneg     +
-     NX       M_FREG        M_AM_NONE       %1111001000eeeeee  m_fsneg
fdneg NBWLSDXP C_ALL030      M_FREG          %1111001000eeeeee  m_fdneg     +
-     NX       M_FREG        M_FREG          %1111001000eeeeee  m_fdneg     +
-     NX       M_FREG        M_AM_NONE       %1111001000eeeeee  m_fdneg
fnop  N        M_AM_NONE     M_AM_NONE       %1111001010000000  m_fnop
frem  NBWLSDXP C_ALL030      M_FREG          %1111001000eeeeee  m_frem      +
-     NX       M_FREG        M_FREG          %1111001000eeeeee  m_frem
fscale NBWLSDXP C_ALL030     M_FREG          %1111001000eeeeee  m_fscale    +
-      NX      M_FREG        M_FREG          %1111001000eeeeee  m_fscale  

fseq   NB       C_ALL030      M_AM_NONE       %1111001001e00001  m_fscc
fsze
fsz
fsne   NB       C_ALL030      M_AM_NONE       %1111001001e01110  m_fscc
fsgt   NB       C_ALL030      M_AM_NONE       %1111001001e10010  m_fscc
fsngt  NB       C_ALL030      M_AM_NONE       %1111001001e11101  m_fscc
fsge   NB       C_ALL030      M_AM_NONE       %1111001001e10011  m_fscc
fsnge  NB       C_ALL030      M_AM_NONE       %1111001001e11100  m_fscc
fslt   NB       C_ALL030      M_AM_NONE       %1111001001e10100  m_fscc
fsnlt  NB       C_ALL030      M_AM_NONE       %1111001001e11011  m_fscc
fsle   NB       C_ALL030      M_AM_NONE       %1111001001e10101  m_fscc
fsnle  NB       C_ALL030      M_AM_NONE       %1111001001e11010  m_fscc
fsgl   NB       C_ALL030      M_AM_NONE       %1111001001e10110  m_fscc
fsngl  NB       C_ALL030      M_AM_NONE       %1111001001e11001  m_fscc
fsgle  NB       C_ALL030      M_AM_NONE       %1111001001e10111  m_fscc
fsngle NB       C_ALL030      M_AM_NONE       %1111001001e11000  m_fscc
fsogt  NB       C_ALL030      M_AM_NONE       %1111001001e00010  m_fscc
fsule  NB       C_ALL030      M_AM_NONE       %1111001001e01101  m_fscc
fsoge  NB       C_ALL030      M_AM_NONE       %1111001001e00011  m_fscc
fsult  NB       C_ALL030      M_AM_NONE       %1111001001e01100  m_fscc
fsolt  NB       C_ALL030      M_AM_NONE       %1111001001e00100  m_fscc
fsuge  NB       C_ALL030      M_AM_NONE       %1111001001e01011  m_fscc
fsole  NB       C_ALL030      M_AM_NONE       %1111001001e00101  m_fscc
fsugt  NB       C_ALL030      M_AM_NONE       %1111001001e01010  m_fscc
fsogl  NB       C_ALL030      M_AM_NONE       %1111001001e00110  m_fscc
fsueq  NB       C_ALL030      M_AM_NONE       %1111001001e01001  m_fscc
fsor   NB       C_ALL030      M_AM_NONE       %1111001001e00111  m_fscc
fsun   NB       C_ALL030      M_AM_NONE       %1111001001e01000  m_fscc
fsf    NB       C_ALL030      M_AM_NONE       %1111001001e00000  m_fscc
fst    NB       C_ALL030      M_AM_NONE       %1111001001e01111  m_fscc
fssf   NB       C_ALL030      M_AM_NONE       %1111001001e10000  m_fscc
fsst   NB       C_ALL030      M_AM_NONE       %1111001001e11111  m_fscc
fsseq  NB       C_ALL030      M_AM_NONE       %1111001001e10001  m_fscc
fssne  NB       C_ALL030      M_AM_NONE       %1111001001e11110  m_fscc

fsgldiv NBWLSDXP C_ALL030    M_FREG          %1111001000eeeeee   m_fsgldiv  + 
-       NX       M_FREG      M_FREG          %1111001000eeeeee   m_fsgldiv 
fsglmul NBWLSDXP C_ALL030    M_FREG          %1111001000eeeeee   m_fsglmul  + 
-       NX       M_FREG      M_FREG          %1111001000eeeeee   m_fsglmul 
fsin   NBWLSDXP C_ALL030     M_FREG          %1111001000eeeeee   m_fsin     +
-      NX       M_FREG       M_FREG          %1111001000eeeeee   m_fsin     +
-      NX       M_FREG       M_AM_NONE       %1111001000eeeeee   m_fsin
fsincos NBWLSDXP C_ALL030    M_FREG          %1111001000eeeeee   m_fsincos  + 
-       NX       M_FREG      M_FREG          %1111001000eeeeee   m_fsincos
fsinh  NBWLSDXP C_ALL030     M_FREG          %1111001000eeeeee   m_fsinh    +
-      NX       M_FREG       M_FREG          %1111001000eeeeee   m_fsinh    +
-      NX       M_FREG       M_AM_NONE       %1111001000eeeeee   m_fsinh
fsqrt  NBWLSDXP C_ALL030     M_FREG          %1111001000eeeeee   m_fsqrt    +
-      NX       M_FREG       M_FREG          %1111001000eeeeee   m_fsqrt    +
-      NX       M_FREG       M_AM_NONE       %1111001000eeeeee   m_fsqrt
fssqrt NBWLSDXP C_ALL030     M_FREG          %1111001000eeeeee   m_fsfsqrt  +
-      NX       M_FREG       M_FREG          %1111001000eeeeee   m_fsfsqrt  +
-      NX       M_FREG       M_AM_NONE       %1111001000eeeeee   m_fsfsqrt
fdsqrt NBWLSDXP C_ALL030     M_FREG          %1111001000eeeeee   m_fdfsqrt  +
-      NX       M_FREG       M_FREG          %1111001000eeeeee   m_fdfsqrt  +
-      NX       M_FREG       M_AM_NONE       %1111001000eeeeee   m_fdfsqrt
fsub  NBWLSDXP C_ALL030      M_FREG          %1111001000eeeeee   m_fsub     +
-     NX       M_FREG        M_FREG          %1111001000eeeeee   m_fsub
fssub NBWLSDXP C_ALL030      M_FREG          %1111001000eeeeee   m_fsub     +
-     NX       M_FREG        M_FREG          %1111001000eeeeee   m_fsub
fdsub NBWLSDXP C_ALL030      M_FREG          %1111001000eeeeee   m_fdsub    +
-     NX       M_FREG        M_FREG          %1111001000eeeeee   m_fdsub
ftan  NBWLSDXP C_ALL030      M_FREG          %1111001000eeeeee   m_ftan     +
-     NX       M_FREG        M_FREG          %1111001000eeeeee   m_ftan     +
-     NX       M_FREG        M_AM_NONE       %1111001000eeeeee   m_ftan
ftanh NBWLSDXP C_ALL030      M_FREG          %1111001000eeeeee   m_ftanh    +
-     NX       M_FREG        M_FREG          %1111001000eeeeee   m_ftanh    +
-     NX       M_FREG        M_AM_NONE       %1111001000eeeeee   m_ftanh
ftentox NBWLSDXP C_ALL030    M_FREG          %1111001000eeeeee   m_ftentox  +
-       NX       M_FREG      M_FREG          %1111001000eeeeee   m_ftentox  +
-       NX       M_FREG      M_AM_NONE       %1111001000eeeeee   m_ftentox
ftst  NBWLSDXP C_ALL030      M_AM_NONE       %1111001000eeeeee   m_ftst     +
-     NX         M_FREG      M_AM_NONE       %1111001000eeeeee   m_ftst
ftwotox NBWLSDXP C_ALL030    M_FREG          %1111001000eeeeee   m_ftwotox  +
-       NX       M_FREG      M_FREG          %1111001000eeeeee   m_ftwotox  +
-       NX       M_FREG      M_AM_NONE       %1111001000eeeeee   m_ftwotox

ftrapeq   WLN  M_IMMED|M_AM_NONE    M_AM_NONE       %1111001000001mmm  m_ftrapcc
ftrapze   WLN  M_IMMED|M_AM_NONE    M_AM_NONE       %1111001000001mmm  m_ftrapcc
ftrapz    WLN  M_IMMED|M_AM_NONE    M_AM_NONE       %1111001000001mmm  m_ftrapcc
ftrapge   WLN  M_IMMED|M_AM_NONE    M_AM_NONE       %1111001001110mmm  m_ftrapcc
ftrapgt   WLN  M_IMMED|M_AM_NONE    M_AM_NONE       %1111001010010mmm  m_ftrapcc
ftrapgl   WLN  M_IMMED|M_AM_NONE    M_AM_NONE       %1111001011101mmm  m_ftrapcc
ftrapgle  WLN  M_IMMED|M_AM_NONE    M_AM_NONE       %1111001010011mmm  m_ftrapcc
ftraple   WLN  M_IMMED|M_AM_NONE    M_AM_NONE       %1111001011100mmm  m_ftrapcc
ftraplt   WLN  M_IMMED|M_AM_NONE    M_AM_NONE       %1111001010100mmm  m_ftrapcc
ftrapne   WLN  M_IMMED|M_AM_NONE    M_AM_NONE       %1111001011011mmm  m_ftrapcc
ftrapngt  WLN  M_IMMED|M_AM_NONE    M_AM_NONE       %1111001010101mmm  m_ftrapcc
ftrapnge  WLN  M_IMMED|M_AM_NONE    M_AM_NONE       %1111001011010mmm  m_ftrapcc
ftrapngl  WLN  M_IMMED|M_AM_NONE    M_AM_NONE       %1111001010110mmm  m_ftrapcc
ftrapnle  WLN  M_IMMED|M_AM_NONE    M_AM_NONE       %1111001011001mmm  m_ftrapcc
ftrapnlt  WLN  M_IMMED|M_AM_NONE    M_AM_NONE       %1111001010111mmm  m_ftrapcc
ftrapngle WLN  M_IMMED|M_AM_NONE    M_AM_NONE       %1111001011000mmm  m_ftrapcc
ftrapogt  WLN  M_IMMED|M_AM_NONE    M_AM_NONE       %1111001000010mmm  m_ftrapcc
ftrapule  WLN  M_IMMED|M_AM_NONE    M_AM_NONE       %1111001001101mmm  m_ftrapcc
ftrapoge  WLN  M_IMMED|M_AM_NONE    M_AM_NONE       %1111001000011mmm  m_ftrapcc
ftrapult  WLN  M_IMMED|M_AM_NONE    M_AM_NONE       %1111001001100mmm  m_ftrapcc
ftrapolt  WLN  M_IMMED|M_AM_NONE    M_AM_NONE       %1111001000100mmm  m_ftrapcc
ftrapuge  WLN  M_IMMED|M_AM_NONE    M_AM_NONE       %1111001001011mmm  m_ftrapcc
ftrapole  WLN  M_IMMED|M_AM_NONE    M_AM_NONE       %1111001000101mmm  m_ftrapcc
ftrapugt  WLN  M_IMMED|M_AM_NONE    M_AM_NONE       %1111001001010mmm  m_ftrapcc
ftrapogl  WLN  M_IMMED|M_AM_NONE    M_AM_NONE       %1111001000110mmm  m_ftrapcc
ftrapueq  WLN  M_IMMED|M_AM_NONE    M_AM_NONE       %1111001001001mmm  m_ftrapcc
ftrapor   WLN  M_IMMED|M_AM_NONE    M_AM_NONE       %1111001000111mmm  m_ftrapcc
ftrapun   WLN  M_IMMED|M_AM_NONE    M_AM_NONE       %1111001001000mmm  m_ftrapcc
ftrapt    WLN  M_IMMED|M_AM_NONE    M_AM_NONE       %1111001000000mmm  m_ftrapcc
ftrapf    WLN  M_IMMED|M_AM_NONE    M_AM_NONE       %1111001001111mmm  m_ftrapcc
ftrapsf   WLN  M_IMMED|M_AM_NONE    M_AM_NONE       %1111001010000mmm  m_ftrapcc
ftrapst   WLN  M_IMMED|M_AM_NONE    M_AM_NONE       %1111001011111mmm  m_ftrapcc
ftrapseq  WLN  M_IMMED|M_AM_NONE    M_AM_NONE       %1111001010001mmm  m_ftrapcc
ftrapsne  WLN  M_IMMED|M_AM_NONE    M_AM_NONE       %1111001011110mmm  m_ftrapcc


frestore N   C_DATA030       M_AM_NONE       %1111001101eeeeee   m_frestore
fsave N      C_DATA030       M_AM_NONE       %1111001100eeeeee   m_frestore

illegal N    M_AM_NONE       M_AM_NONE       %0100101011111100   m_self

jmp   N      C_CTRL          M_AM_NONE       %0100111011eee000   m_ea     +
-     N      C_CTRL030       M_AM_NONE       %0100111011eee000   m_ea030

jsr   N      C_CTRL          M_AM_NONE       %0100111010eee000   m_ea     +
-     N      C_CTRL030       M_AM_NONE       %0100111010eee000   m_ea030

lea   NL     C_CTRL          M_AREG          %0100rrr111eR1000   m_lea

link  NWL    M_AREG          M_IMMED         %0100111001010rrr   m_link

lpstop NW    M_IMMED         M_AM_NONE       %1111100000000000   m_lpstop

lsl   NBWL   M_DREG          M_DREG          %1110rrr1ss101rrr   m_shr     +
-     NBWL   M_IMMED         M_DREG          %1110ccc1ss001rrr   m_shi     +
-     NBWL   C_ALTMEM        M_AM_NONE       %1110001111eee000   m_ea      +
-     NBWL   M_DREG          M_AM_NONE       %11100011ss001rrS   m_reg

lsr   NBWL   M_DREG          M_DREG          %1110rrr0ss101rrr   m_shr     +
-     NBWL   M_IMMED         M_DREG          %1110ccc0ss001rrr   m_shi     +
-     NBWL   C_ALTMEM        M_AM_NONE       %1110001011eee000   m_ea      +
-     NBWL   M_DREG          M_AM_NONE       %11100010ss001rrS   m_reg

move  NBWL   C_ALL           C_ALTDATA       %00ssddddddssssss   m_move    +
-     NWL    C_ALL           M_AREG          %00ssddd001ssssss   m_move    +
-     NW     C_DATA          M_AM_SR         %0100011011sss000   m_ea      +
-     NW     M_AM_SR         C_ALTDATA       %0100000011ddd001   m_ea      +
-     NW     C_DATA          M_AM_CCR        %0100010011sss000   m_ea      +
-     NL     M_AM_USP        M_AREG          %0100111001101rrr   m_usp     +
-     NL     M_AREG          M_AM_USP        %0100111001100rrr   m_usp     +
-     NBWL   C_ALL030        C_ALTDATA       %00ssddddddssssss   m_move30  +
-     NBWL   C_ALL030        C_ALT030        %00ssddddddssssss   m_move30  +
-     NW     M_AM_CCR        C_DATA030       %0100001011sss000   m_ea030

movea NWL    C_ALL           M_AREG          %00ssddd001ssssss   m_move

movec NL     M_DREG+M_AREG   C_CREG          %0100111001111011   m_movec   +
-     NL     C_CREG          M_DREG+M_AREG   %0100111001111010   m_movec

moves NBWL   M_DREG+M_AREG   C_MOVES         %00001110sseeeeee   m_moves   +
-     NBWL   C_MOVES         M_DREG+M_AREG   %00001110sseeeeee   m_moves

move16 N     APOSTINC       APOSTINC         %1111011000100rrr   m_move16a +
-     N      AIND+APOSTINC+ABSL AIND+APOSTINC+ABSL %11110110000ooorrr m_move16b

movem !      M_AM_NONE       M_AM_NONE       %01001d001seeeeee   m_movem

movep NWL    M_DREG          M_AIND|M_ADISP  %0000rrr11s001aaa   m_movep   +
-     NWL    M_AIND|M_ADISP  M_DREG          %0000rrr10s001aaa   m_movep

moveq NL     M_IMMED         M_DREG          %0111rrr0dddddddd   m_moveq

muls  NW     C_DATA          M_DREG          %1100rrr111eR1000   m_ea      +
-     L      C_DATA030       M_DREG          %0100111100eeeeee   m_muls    
mulu  NW     C_DATA          M_DREG          %1100rrr011eR1000   m_ea      +
-     L      C_DATA030       M_DREG          %0100110100eeeeee   m_muls

nbcd  NB     C_ALTDATA       M_AM_NONE       %0100100000eee000   m_ea
neg   NBWL   C_ALTDATA       M_AM_NONE       %01000100sseeeS00   m_ea
negx  NBWL   C_ALTDATA       M_AM_NONE       %01000000sseeeS00   m_ea
nop   N      M_AM_NONE       M_AM_NONE       0x4e71              m_self
not   NBWL   C_ALTDATA       M_AM_NONE       %01000110sseee100   m_ea

or    NBWL   C_DATA          M_DREG          %1000rrr0sseR1S00   m_ea      +
-     NBWL   M_DREG          C_MEM           %1000rrr1sseR0S01   m_ea      +
ori   NBWL   M_IMMED         C_ALTDATA       %00000000sseeeS11   m_ea      +
-     NBWL   M_IMMED         C_ALT030        %00000000sseeeS10   m_ea030   +
-     NB     M_IMMED         M_AM_CCR        %0000000000111100   m_imm8    +
-     NW     M_IMMED         M_AM_SR         %0000000001111100   m_imm

pack  !      M_AM_NONE       M_AM_NONE       %1000rrr10100mrrr   m_pack

pbbs  NWL    C_LABEL         M_AM_NONE       %111100001s000000   m_pbcc
pbbc  NWL    C_LABEL         M_AM_NONE       %111100001s000001   m_pbcc
pbls  NWL    C_LABEL         M_AM_NONE       %111100001s000010   m_pbcc
pblc  NWL    C_LABEL         M_AM_NONE       %111100001s000011   m_pbcc
pbss  NWL    C_LABEL         M_AM_NONE       %111100001s000100   m_pbcc
pbsc  NWL    C_LABEL         M_AM_NONE       %111100001s000101   m_pbcc
pbas  NWL    C_LABEL         M_AM_NONE       %111100001s000110   m_pbcc
pbac  NWL    C_LABEL         M_AM_NONE       %111100001s000111   m_pbcc
pbws  NWL    C_LABEL         M_AM_NONE       %111100001s001000   m_pbcc
pbwc  NWL    C_LABEL         M_AM_NONE       %111100001s001001   m_pbcc
pbis  NWL    C_LABEL         M_AM_NONE       %111100001s001010   m_pbcc
pbic  NWL    C_LABEL         M_AM_NONE       %111100001s001011   m_pbcc
pbgs  NWL    C_LABEL         M_AM_NONE       %111100001s001100   m_pbcc
pbgc  NWL    C_LABEL         M_AM_NONE       %111100001s001101   m_pbcc
pbcs  NWL    C_LABEL         M_AM_NONE       %111100001s001110   m_pbcc
pbcc  NWL    C_LABEL         M_AM_NONE       %111100001s001111   m_pbcc

pdbbs NWL    M_DREG          C_LABEL         %1111000001001ccc   m_cpdbr
pdbls NWL    M_DREG          C_LABEL         %1111000001001ccc   m_cpdbr
pdbss NWL    M_DREG          C_LABEL         %1111000001001ccc   m_cpdbr
pdbas NWL    M_DREG          C_LABEL         %1111000001001ccc   m_cpdbr
pdbws NWL    M_DREG          C_LABEL         %1111000001001ccc   m_cpdbr
pdbis NWL    M_DREG          C_LABEL         %1111000001001ccc   m_cpdbr
pdbgs NWL    M_DREG          C_LABEL         %1111000001001ccc   m_cpdbr
pdbcs NWL    M_DREG          C_LABEL         %1111000001001ccc   m_cpdbr
pdbbc NWL    M_DREG          C_LABEL         %1111000001001ccc   m_cpdbr
pdblc NWL    M_DREG          C_LABEL         %1111000001001ccc   m_cpdbr
pdbsc NWL    M_DREG          C_LABEL         %1111000001001ccc   m_cpdbr
pdbac NWL    M_DREG          C_LABEL         %1111000001001ccc   m_cpdbr
pdbwc NWL    M_DREG          C_LABEL         %1111000001001ccc   m_cpdbr
pdbic NWL    M_DREG          C_LABEL         %1111000001001ccc   m_cpdbr
pdbgc NWL    M_DREG          C_LABEL         %1111000001001ccc   m_cpdbr
pdbcc NWL    M_DREG          C_LABEL         %1111000001001ccc   m_cpdbr

pflush !     M_AM_NONE       M_AM_NONE       %1111000000eeeeee   m_pflush
pflushn !    M_AM_NONE       M_AM_NONE       %1111010100000111   m_pflush
pflushan N   M_AM_NONE       M_AM_NONE       %1111010100010000   m_pflushan
pflusha N    M_AM_NONE       M_AM_NONE       %1111000000000000   m_pflusha
pflushr N    C_ALL030        M_AM_NONE       %1111000000eeeeee   m_pflushr

ploadr N     M_FC            C_PMOVE         %1111000000eeeeee   m_ploadr
ploadw N     M_FC            C_PMOVE         %1111000000eeeeee   m_ploadw     

plpa   N     M_AIND          M_AM_NONE       %1111010110001rrr   m_plpa

pmove NWLD   M_MRN           C_PMOVE         %1111000000eeeeee   m_pmove +
-     NWLD   C_PMOVE         M_MRN           %1111000000eeeeee   m_pmove
pmovefd NWLD C_PMOVE         M_MRN           %1111000000eeeeee   m_pmovefd

prestore N   C_ALL030        M_AM_NONE       %1111000101eeeeee   m_ea
psave  N     C_ALL030        M_AM_NONE       %1111000100eeeeee   m_ea

psbs    BN   C_MOVES         M_AM_NONE       %111100000100000    m_fscc
psbc    BN   C_MOVES         M_AM_NONE       %111100000100001    m_fscc
psls    BN   C_MOVES         M_AM_NONE       %111100000100010    m_fscc
pslc    BN   C_MOVES         M_AM_NONE       %111100000100011    m_fscc
psss    BN   C_MOVES         M_AM_NONE       %111100000100100    m_fscc
pssc    BN   C_MOVES         M_AM_NONE       %111100000100101    m_fscc
psas    BN   C_MOVES         M_AM_NONE       %111100000100110    m_fscc
psac    BN   C_MOVES         M_AM_NONE       %111100000100111    m_fscc
psws    BN   C_MOVES         M_AM_NONE       %111100000101000    m_fscc
pswc    BN   C_MOVES         M_AM_NONE       %111100000101001    m_fscc
psis    BN   C_MOVES         M_AM_NONE       %111100000101010    m_fscc
psic    BN   C_MOVES         M_AM_NONE       %111100000101011    m_fscc
psgc    BN   C_MOVES         M_AM_NONE       %111100000101100    m_fscc
psgs    BN   C_MOVES         M_AM_NONE       %111100000101101    m_fscc
pscs    BN   C_MOVES         M_AM_NONE       %111100000101110    m_fscc
pscc    BN   C_MOVES         M_AM_NONE       %111100000101111    m_fscc

ptestr N     M_FC            C_PMOVE         %1111000000eeeeee   m_ptestr
ptestw N     M_FC            C_PMOVE         %1111000000eeeeee   m_ptestw

ptrapbs WLN  M_IMMED|M_AM_NONE         M_AM_NONE       %1111000001100000   m_ptrapcc
ptrapbc WLN  M_IMMED|M_AM_NONE         M_AM_NONE       %1111000001100001   m_ptrapcc
ptrapls WLN  M_IMMED|M_AM_NONE         M_AM_NONE       %1111000001100010   m_ptrapcc
ptraplc WLN  M_IMMED|M_AM_NONE         M_AM_NONE       %1111000001100011   m_ptrapcc
ptrapss WLN  M_IMMED|M_AM_NONE         M_AM_NONE       %1111000001100100   m_ptrapcc
ptrapsc WLN  M_IMMED|M_AM_NONE         M_AM_NONE       %1111000001100101   m_ptrapcc
ptrapas WLN  M_IMMED|M_AM_NONE         M_AM_NONE       %1111000001100110   m_ptrapcc
ptrapac WLN  M_IMMED|M_AM_NONE         M_AM_NONE       %1111000001100111   m_ptrapcc
ptrapws WLN  M_IMMED|M_AM_NONE         M_AM_NONE       %1111000001101000   m_ptrapcc
ptrapwc WLN  M_IMMED|M_AM_NONE         M_AM_NONE       %1111000001101001   m_ptrapcc
ptrapis WLN  M_IMMED|M_AM_NONE         M_AM_NONE       %1111000001101010   m_ptrapcc
ptrapic WLN  M_IMMED|M_AM_NONE         M_AM_NONE       %1111000001101011   m_ptrapcc
ptrapgc WLN  M_IMMED|M_AM_NONE         M_AM_NONE       %1111000001101100   m_ptrapcc
ptrapgs WLN  M_IMMED|M_AM_NONE         M_AM_NONE       %1111000001101101   m_ptrapcc
ptrapcs WLN  M_IMMED|M_AM_NONE         M_AM_NONE       %1111000001101110   m_ptrapcc
ptrapcc WLN  M_IMMED|M_AM_NONE         M_AM_NONE       %1111000001101111   m_ptrapcc

pea   NL     C_CTRL          M_AM_NONE       %0100100001eee000   m_ea

reset N      M_AM_NONE       M_AM_NONE       0x4e70              m_self

rol   NBWL   M_DREG          M_DREG          %1110rrr1ss111rrr   m_shr     +
-     NBWL   M_IMMED         M_DREG          %1110ccc1ss011rrr   m_shi     +
-     NBWL   C_ALTMEM        M_AM_NONE       %1110011111eee000   m_ea      +
-     NBWL   M_DREG          M_AM_NONE       %11100011ss011rrS   m_reg

ror   NBWL   M_DREG          M_DREG          %1110rrr0ss111rrr   m_shr     +
-     NBWL   M_IMMED         M_DREG          %1110ccc0ss011rrr   m_shi     +
-     NBWL   C_ALTMEM        M_AM_NONE       %1110011011eee000   m_ea      +
-     NBWL   M_DREG          M_AM_NONE       %11100010ss011rrS   m_reg

roxl  NBWL   M_DREG          M_DREG          %1110rrr1ss110rrr   m_shr     +
-     NBWL   M_IMMED         M_DREG          %1110ccc1ss010rrr   m_shi     +
-     NBWL   C_ALTMEM        M_AM_NONE       %1110010111eee000   m_ea      +
-     NBWL   M_DREG          M_AM_NONE       %11100011ss010rrS   m_reg

roxr  NBWL   M_DREG          M_DREG          %1110rrr0ss110rrr   m_shr     +
-     NBWL   M_IMMED         M_DREG          %1110ccc0ss010rrr   m_shi     +
-     NBWL   C_ALTMEM        M_AM_NONE       %1110010011eee000   m_ea      +
-     NBWL   M_DREG          M_AM_NONE       %11100010ss010rrS   m_reg

rtd   N      M_IMMED         M_AM_NONE       %0100111001110100   m_rtd
rte   N      M_AM_NONE       M_AM_NONE       0x4e73              m_self
rtm   N      M_DREG+M_AREG   M_AM_NONE       %000001101100drrr   m_rtm
rtr   N      M_AM_NONE       M_AM_NONE       0x4e77              m_self
rts   N      M_AM_NONE       M_AM_NONE       0x4e75              m_self

sbcd  NB     M_DREG          M_DREG          %1000rrr100000rrr   m_abcd    +
-     NB     M_APREDEC       M_APREDEC       %1000rrr100001rrr   m_abcd

scc   NB     C_ALTDATA       M_AM_NONE       %0101010011eee000   m_ea
shs
scs   NB     C_ALTDATA       M_AM_NONE       %0101010111eee000   m_ea
slo
seq   NB     C_ALTDATA       M_AM_NONE       %0101011111eee000   m_ea
sze
sf    NB     C_ALTDATA       M_AM_NONE       %0101000111eee000   m_ea
sge   NB     C_ALTDATA       M_AM_NONE       %0101110011eee000   m_ea
sgt   NB     C_ALTDATA       M_AM_NONE       %0101111011eee000   m_ea
shi   NB     C_ALTDATA       M_AM_NONE       %0101001011eee000   m_ea
sle   NB     C_ALTDATA       M_AM_NONE       %0101111111eee000   m_ea
sls   NB     C_ALTDATA       M_AM_NONE       %0101001111eee000   m_ea
slt   NB     C_ALTDATA       M_AM_NONE       %0101110111eee000   m_ea
smi   NB     C_ALTDATA       M_AM_NONE       %0101101111eee000   m_ea
sne   NB     C_ALTDATA       M_AM_NONE       %0101011011eee000   m_ea
snz
spl   NB     C_ALTDATA       M_AM_NONE       %0101101011eee000   m_ea
st    NB     C_ALTDATA       M_AM_NONE       %0101000011eee000   m_ea
svc   NB     C_ALTDATA       M_AM_NONE       %0101100011eee000   m_ea
svs   NB     C_ALTDATA       M_AM_NONE       %0101100111eee000   m_ea

stop  N      M_IMMED         M_AM_NONE       %0100111001110010   m_imm

sub   NBWL   C_ALL           M_DREG          %1001rrr0sseR1S00   m_ea      +
-     NBWL   M_DREG          C_ALTMEM        %1001rrr1sseR0S01   m_ea      +
suba  NWL    C_ALL           M_AREG          %1001rrrs11eeeeee   m_adda    +
subi  NBWL   M_IMMED         C_ALTDATA       %00000100sseeeS11   m_ea      +
-     NBWL   M_IMMED         C_ALT030        %00000100sseeeS11   m_ea030

subq  NBWL   M_IMMED         C_ALT           %0101ddd1sseeeeee   m_addq

subx  NBWL   M_DREG          M_DREG          %1001xxx1ss000yyS   m_abcd    +
-     NBWL   M_APREDEC       M_APREDEC       %1001xxx1ss001yyS   m_abcd

swap  NW     M_DREG          M_AM_NONE       %0100100001000rrr   m_reg
tas   NB     C_ALTDATA       M_AM_NONE       %0100101011eee000   m_ea
trap  N      M_IMMED         M_AM_NONE       %010011100100vvvv   m_trap
trapcc NWL   M_IMMED         M_AM_NONE       %0101010011111ooo   m_trapcc +
-      NWL   M_AM_NONE       M_AM_NONE       %0101010011111100   m_self
trapcs NWL   M_IMMED         M_AM_NONE       %0101010111111ooo   m_trapcc +
-      NWL   M_AM_NONE       M_AM_NONE       %0101010111111100   m_self
traplo
trapeq NWL   M_IMMED         M_AM_NONE       %0101011111111ooo   m_trapcc +
-      NWL   M_AM_NONE       M_AM_NONE       %0101011111111100   m_self
trapze
trapf  NWL   M_IMMED         M_AM_NONE       %0101000111111ooo   m_trapcc +
-      NWL   M_AM_NONE       M_AM_NONE       %0101000111111100   m_self
trapge NWL   M_IMMED         M_AM_NONE       %0101110011111ooo   m_trapcc +
-      NWL   M_AM_NONE       M_AM_NONE       %0101110011111100   m_self
trapgt NWL   M_IMMED         M_AM_NONE       %0101111011111ooo   m_trapcc +
-      NWL   M_AM_NONE       M_AM_NONE       %0101111011111100   m_self
traphi NWL   M_IMMED         M_AM_NONE       %0101001011111ooo   m_trapcc +
-      NWL   M_AM_NONE       M_AM_NONE       %0101001011111100   m_self
traple NWL   M_IMMED         M_AM_NONE       %0101111111111ooo   m_trapcc +
-      NWL   M_AM_NONE       M_AM_NONE       %0101111111111100   m_self
trapls NWL   M_IMMED         M_AM_NONE       %0101001111111ooo   m_trapcc +
-      NWL   M_AM_NONE       M_AM_NONE       %0101001111111100   m_self
traplt NWL   M_IMMED         M_AM_NONE       %0101110111111ooo   m_trapcc +
-      NWL   M_AM_NONE       M_AM_NONE       %0101110111111100   m_self
trapmi NWL   M_IMMED         M_AM_NONE       %0101101111111ooo   m_trapcc +
-      NWL   M_AM_NONE       M_AM_NONE       %0101101111111100   m_self
trapne NWL   M_IMMED         M_AM_NONE       %0101011011111ooo   m_trapcc +
-      NWL   M_AM_NONE       M_AM_NONE       %0101011011111100   m_self
trapnz
trappl NWL   M_IMMED         M_AM_NONE       %0101101011111ooo   m_trapcc +
-      NWL   M_AM_NONE       M_AM_NONE       %0101101011111100   m_self
trapt  NWL   M_IMMED         M_AM_NONE       %0101000011111ooo   m_trapcc +
-      NWL   M_AM_NONE       M_AM_NONE       %0101000011111100   m_self
trapvc NWL   M_IMMED         M_AM_NONE       %0101100011111ooo   m_trapcc +
-      NWL   M_AM_NONE       M_AM_NONE       %0101100011111100   m_self
trapvs NWL   M_IMMED         M_AM_NONE       %0101100111111ooo   m_trapcc +
-      NWL   M_AM_NONE       M_AM_NONE       %0101100111111100   m_self
trapv N      M_AM_NONE       M_AM_NONE       0x4e76              m_self
tst   NBWL   C_ALTDATA       M_AM_NONE       %01001010sseeeS00   m_ea      +
-     NBWL   C_ALL030        M_AM_NONE       %01001010sseeeS00   m_ea030
unlk  N      M_AREG          M_AM_NONE       %0100111001011rrr   m_reg
unpk  !      M_AM_NONE       M_AM_NONE       %1000rrr11000mrrr   m_pack
