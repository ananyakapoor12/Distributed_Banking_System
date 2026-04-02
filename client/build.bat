@echo off
REM Build script for Java Banking Client (Windows)

echo ================================================================
echo        Building Distributed Banking System Client
echo ================================================================
echo.

REM Configuration
set SRC_DIR=src
set BUILD_DIR=build
set DIST_DIR=dist
set JAR_NAME=BankingClient.jar
set MAIN_CLASS=client.Main

REM Clean previous build
echo Cleaning previous build...
if exist "%BUILD_DIR%" rmdir /s /q "%BUILD_DIR%"
if exist "%DIST_DIR%" rmdir /s /q "%DIST_DIR%"
mkdir "%BUILD_DIR%"
mkdir "%DIST_DIR%"

REM Compile Java files
echo Compiling Java sources...
javac -d "%BUILD_DIR%" -sourcepath "%SRC_DIR%" "%SRC_DIR%\client\Main.java"

if errorlevel 1 (
    echo Compilation failed!
    exit /b 1
)

echo Compilation successful

REM Create manifest
echo Creating manifest...
(
echo Manifest-Version: 1.0
echo Main-Class: %MAIN_CLASS%
) > "%BUILD_DIR%\MANIFEST.MF"

REM Create JAR
echo Creating JAR file...
cd "%BUILD_DIR%"
jar cfm "..\%DIST_DIR%\%JAR_NAME%" MANIFEST.MF client\*.class client\core\*.class client\protocol\*.class client\ui\*.class client\utils\*.class

if errorlevel 1 (
    echo JAR creation failed!
    cd ..
    exit /b 1
)

cd ..
echo JAR created: %DIST_DIR%\%JAR_NAME%

echo.
echo ================================================================
echo                    BUILD SUCCESSFUL!
echo ================================================================
echo.
echo To run the client:
echo   java -jar %DIST_DIR%\%JAR_NAME% ^<server_host^> ^<server_port^> ^<semantics^>
echo.
echo Example:
echo   java -jar %DIST_DIR%\%JAR_NAME% localhost 2222 at-least-once
echo.
