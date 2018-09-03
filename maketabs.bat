rem @echo off
REM Check for file dates and build .h files if needed

SET FILE1=68ktab
SET FILE2=68ktab.h
if not exist %FILE2% GOTO BUILD
for /F %%i IN ('dir /b /OD %FILE1% %FILE2% ^| more +1') DO SET NEWEST=%%i
if %NEWEST%==%FILE1% GOTO BUILD

SET FILE1=mntab
SET FILE2=mntab.h
if not exist %FILE2% GOTO BUILD
for /F %%i IN ('dir /b /OD %FILE1% %FILE2% ^| more +1') DO SET NEWEST=%%i
if %NEWEST%==%FILE1% GOTO BUILD

SET FILE1=kwtab
SET FILE2=kwtab.h
if not exist %FILE2% GOTO BUILD
for /F %%i IN ('dir /b /OD %FILE1% %FILE2% ^| more +1') DO SET NEWEST=%%i
if %NEWEST%==%FILE1% GOTO BUILD

SET FILE1=risctab
SET FILE2=risckw.h
if not exist %FILE2% GOTO BUILD
for /F %%i IN ('dir /b /OD %FILE1% %FILE2% ^| more +1') DO SET NEWEST=%%i
if %NEWEST%==%FILE1% GOTO BUILD

SET FILE1=6502.tbl
SET FILE2=6502kw.h
if not exist %FILE2% GOTO BUILD
for /F %%i IN ('dir /b /OD %FILE1% %FILE2% ^| more +1') DO SET NEWEST=%%i
if %NEWEST%==%FILE1% GOTO BUILD

SET FILE1=op.tab
SET FILE2=opkw.h
if not exist %FILE2% GOTO BUILD
for /F %%i IN ('dir /b /OD %FILE1% %FILE2% ^| more +1') DO SET NEWEST=%%i
if %NEWEST%==%FILE1% GOTO BUILD

GOTO END

:BUILD

echo Generating files...

68kgen 68kmn <68ktab >68ktab.h
type mntab 68kmn | kwgen mn >mntab.h
kwgen kw <kwtab >kwtab.h
kwgen mr <risctab >risckw.h
kwgen mp <6502.tbl >6502kw.h
kwgen mo <op.tab >opkw.h

rem touch files that include these header files so they'll recompile
echo Generating tables...
copy /b amode.c +,, >NUL
copy /b direct.c +,, >NUL
copy /b expr.c +,, >NUL
copy /b mach.c +,, >NUL
copy /b procln.c +,, >NUL
copy /b riscasm.c +,, >NUL
copy /b token.c +,, >NUL

:END

REM If eagen0.c is newer than eagen.c then "touch" eagen.c so that visual studio will recompile both.
REM Same for amode.c / parmode.h and mach.c / 68ktab.h

SET FILE1=eagen0.c
SET FILE2=eagen.c
for /F %%i IN ('dir /b /OD %FILE1% %FILE2% ^| more +1') DO SET NEWEST=%%i
if %NEWEST%==%FILE2% GOTO CHECK2
Echo touching eagen0.c...
copy /b eagen.c +,, >NUL

:CHECK2
SET FILE1=parmode.h
SET FILE2=amode.c
for /F %%i IN ('dir /b /OD %FILE1% %FILE2% ^| more +1') DO SET NEWEST=%%i
if %NEWEST%==%FILE2% GOTO CHECK3
Echo touching amode.c...
copy /b amode.c +,, >NUL

:CHECK3
SET FILE1=68ktab.h
SET FILE2=mach.c
for /F %%i IN ('dir /b /OD %FILE1% %FILE2% ^| more +1') DO SET NEWEST=%%i
if %NEWEST%==%FILE2% GOTO END
Echo touching mach.c...
copy /b mach.c +,, >NUL

:END

