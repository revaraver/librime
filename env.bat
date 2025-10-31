rem Customize your build environment and save the modified copy to env.bat

set RIME_ROOT=D:\git\rime\librime\librime

rem REQUIRED: path to Boost source directory
if not defined BOOST_ROOT set BOOST_ROOT=C:\dev\boost_1_89_0

rem architecture, Visual Studio version and platform toolset
set ARCH=x64
set BJAM_TOOLSET=msvc-14.3
set CMAKE_GENERATOR="Visual Studio 17 2022"
set PLATFORM_TOOLSET=v143

rem OPTIONAL: path to additional build tools
rem set DEVTOOLS_PATH=%ProgramFiles%\Git\cmd;%ProgramFiles%\CMake\bin;C:\Python27;
