rem @echo off
rem Check for file dates and build .h files if needed

call :CHECK_OUT_OF_DATE 68k.mch 68ktab.h
call :CHECK_OUT_OF_DATE direct.tab mntab.h
call :CHECK_OUT_OF_DATE kw.tab kwtab.h
call :CHECK_OUT_OF_DATE risc.tab risckw.h
call :CHECK_OUT_OF_DATE dsp56k.mch dsp56ktab.h
call :CHECK_OUT_OF_DATE 6502.tab 6502kw.h
call :CHECK_OUT_OF_DATE op.tab opkw.h
call :CHECK_OUT_OF_DATE 68kregs.tab 68kregs.h
call :CHECK_OUT_OF_DATE 56kregs.tab 56kregs.h
call :CHECK_OUT_OF_DATE 6502regs.tab 6502regs.h
call :CHECK_OUT_OF_DATE riscregs.tab riscregs.h
call :CHECK_OUT_OF_DATE unary.tab unarytab.h

GOTO NOGEN

:BUILD

echo Generating files...

68kgen 68k.tab <68k.mch >68ktab.h
dsp56kgen dsp56k.tab <dsp56k.mch >dsp56ktab.h
type direct.tab 68k.tab | kwgen mn >mntab.h
kwgen kw <kw.tab >kwtab.h
kwgen mr <risc.tab >risckw.h
kwgen dsp <dsp56k.tab >dsp56kkw.h
kwgen mp <6502.tab >6502kw.h
kwgen mp <6502.tab >6502kw.h
kwgen mo <op.tab >opkw.h
kwgen reg68 <68kregs.tab >68kregs.h
kwgen reg56 <56kregs.tab >56kregs.h
kwgen reg65 <6502regs.tab >6502regs.h
kwgen regrisc <riscregs.tab >riscregs.h
kwgen unary <unary.tab >unarytab.h

rem Touch timestamps of files that include these header files so they'll recompile
echo Generating tables...
copy /b amode.c +,, >NUL
copy /b direct.c +,, >NUL
copy /b expr.c +,, >NUL
copy /b mach.c +,, >NUL
copy /b procln.c +,, >NUL
copy /b riscasm.c +,, >NUL
copy /b token.c +,, >NUL
copy /b dsp56k_mach.c +,, >NUL
copy /b eagen.c +,, >NUL
goto :END

:NOGEN

REM If eagen0.c is newer than eagen.c then "touch" eagen.c so that visual studio will recompile oth.
REM Same for amode.c / parmode.h and mach.c / 68ktab.h
call :CHECK_AND_TOUCH eagen0.c eagen.c
call :CHECK_AND_TOUCH parmode.h amode.c
call :CHECK_AND_TOUCH 68ktab.h mach.c

:END
exit /b

rem Check if second passed file is older compared to the first, or doesn't exist, or is zero size. In which case go generate everything just to be sure
:CHECK_OUT_OF_DATE
SET FILE1=%1
SET FILE2=%2
if not exist %FILE2% GOTO BUILD
FOR /F "usebackq" %%A IN ('%FILE2%') DO set size=%%~zA
if "%size%"=="0" GOTO BUILD
for /F %%i IN ('dir /b /OD %FILE1% %FILE2% ^| more +1') DO SET NEWEST=%%i
if %NEWEST%==%FILE1% GOTO BUILD

exit /b

rem Check if second passed file is older compared to the first, and "touch" the file stamp of hat file if yes
:CHECK_AND_TOUCH
SET FILE1=%1
SET FILE2=%2
for /F %%i IN ('dir /b /OD %FILE1% %FILE2% ^| more +1') DO SET NEWEST=%%i
if %NEWEST%==%FILE2% GOTO :CHECK_AND_TOUCH_END
Echo touching %2...
copy /b %2 +,, >NUL

:CHECK_AND_TOUCH_END
exit /b
