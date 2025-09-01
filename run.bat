@echo off
setlocal

echo Building and running project...

cd build
cmake --build . --config Debug -j%NUMBER_OF_PROCESSORS% && cmake --build . --target run --config Debug
if %ERRORLEVEL% neq 0 (
    echo Build or run failed!
    exit /b 1
)

echo Program completed.