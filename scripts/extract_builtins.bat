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
        set RETURN_TYPE=void
        set PARAMS=
        set FUNC_FIRST=1
        for /f "usebackq delims=" %%l in (%%f) do (
            set LINE=%%l
            echo !LINE! | findstr /C:"@desc" > nul
            if !errorlevel!==0 (
                for /f "tokens=2*" %%d in ("!LINE!") do set DESC=%%e
            )
            echo !LINE! | findstr /C:"@return" > nul
            if !errorlevel!==0 (
                for /f "tokens=2*" %%d in ("!LINE!") do set RETURN_TYPE=%%e
            )
            echo !LINE! | findstr /C:"@params" > nul
            if !errorlevel!==0 (
                for /f "tokens=2*" %%d in ("!LINE!") do set PARAMS=%%e
            )
            echo !LINE! | findstr /R /C:"getName.*return" > nul
            if !errorlevel!==0 (
                for /f "tokens=2 delims=^"" %%n in ("!LINE!") do (
                    if !FUNC_FIRST!==0 echo , >> %OUTPUT_FILE%
                    set FUNC_FIRST=0
                    set SIG=%%n(!PARAMS!)
                    if not "!RETURN_TYPE!"=="void" set SIG=!SIG! -^> !RETURN_TYPE!
                    echo     {"name": "%%n", "signature": "!SIG!", "description": "!DESC!", "returnType": "!RETURN_TYPE!"} >> %OUTPUT_FILE%
                    set DESC=
                    set RETURN_TYPE=void
                    set PARAMS=
                )
            )
        )
        
        echo   ] >> %OUTPUT_FILE%
    )
)

echo } >> %OUTPUT_FILE%

echo Extracted builtins to %OUTPUT_FILE%
