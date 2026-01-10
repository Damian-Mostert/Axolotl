@echo off
setlocal enabledelayedexpansion

set OUTPUT_FILE=%1
shift

set CPP_FILES=
:loop
if "%1"=="" goto :done
set CPP_FILES=!CPP_FILES! %1
shift
goto :loop

:done

echo { > %OUTPUT_FILE%

set FIRST=1
for %%f in (%CPP_FILES%) do (
    if exist %%f (
        if !FIRST!==0 echo , >> %OUTPUT_FILE%
        set FIRST=0
        
        for %%n in (%%~nf) do set CATEGORY=%%n
        set CATEGORY=!CATEGORY:_functions=!
        
        echo   "!CATEGORY!": [ >> %OUTPUT_FILE%
        
        set DESC=
        set PARENT=
        set IN_CLASS=0
        set FUNC_FIRST=1
        
        for /f "usebackq delims=" %%l in (%%f) do (
            set LINE=%%l
            
            echo !LINE! | findstr /C:"class" | findstr /C:"BuiltinFunction" > nul
            if !errorlevel!==0 (
                set IN_CLASS=1
                set DESC=
                set PARENT=
            )
            
            if !IN_CLASS!==1 (
                echo !LINE! | findstr /C:"//@desc" > nul
                if !errorlevel!==0 (
                    for /f "tokens=2*" %%d in ("!LINE!") do (
                        if "!DESC!"=="" (
                            set DESC=%%e
                        ) else (
                            set DESC=!DESC! %%e
                        )
                    )
                )
                
                echo !LINE! | findstr /C:"//@parent" > nul
                if !errorlevel!==0 (
                    for /f "tokens=2*" %%d in ("!LINE!") do set PARENT=%%e
                )
                
                echo !LINE! | findstr /R /C:"getName.*return" > nul
                if !errorlevel!==0 (
                    for /f "tokens=2 delims=^"" %%n in ("!LINE!") do (
                        if !FUNC_FIRST!==0 echo , >> %OUTPUT_FILE%
                        set FUNC_FIRST=0
                        set NAME=%%n
                        set SIG=!NAME!()
                        if not "!PARENT!"=="" set SIG=!PARENT!.!NAME!()
                        echo     {"name": "!NAME!", "signature": "!SIG!", "description": "!DESC!", "parent": "!PARENT!"} >> %OUTPUT_FILE%
                        set DESC=
                        set PARENT=
                        set IN_CLASS=0
                    )
                )
            )
        )
        
        echo   ] >> %OUTPUT_FILE%
    )
)

echo } >> %OUTPUT_FILE%

echo Extracted builtins to %OUTPUT_FILE%
