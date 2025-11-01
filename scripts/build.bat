@echo off
REM Balance - Windows Build Script

echo ======================================
echo Balance Encryption Toolkit - Builder
echo ======================================
echo.

REM Configuration
set BUILD_TYPE=Release
if not "%1"=="" set BUILD_TYPE=%1

set BUILD_DIR=build

echo Build type: %BUILD_TYPE%
echo Build directory: %BUILD_DIR%
echo.

REM Create build directory
if exist "%BUILD_DIR%" (
    echo Cleaning existing build directory...
    rmdir /s /q "%BUILD_DIR%"
)

mkdir "%BUILD_DIR%"
cd "%BUILD_DIR%"

REM Configure
echo Configuring with CMake...
cmake .. -G "Visual Studio 17 2022" -A x64

if errorlevel 1 (
    echo.
    echo ERROR: CMake configuration failed!
    exit /b 1
)

REM Build
echo.
echo Building...
cmake --build . --config %BUILD_TYPE% --parallel

if errorlevel 1 (
    echo.
    echo ERROR: Build failed!
    exit /b 1
)

echo.
echo ======================================
echo Build completed successfully!
echo ======================================
echo.
echo Executable: %BUILD_DIR%\%BUILD_TYPE%\balance.exe
echo Tests: %BUILD_DIR%\tests\%BUILD_TYPE%\balance_tests.exe
echo.
echo To run the application:
echo   cd %BUILD_DIR% ^&^& %BUILD_TYPE%\balance.exe
echo.
echo To run tests:
echo   cd %BUILD_DIR% ^&^& ctest -C %BUILD_TYPE% --output-on-failure
echo.
