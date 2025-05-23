# SQLiteWrapper
An experimental C++ wrapper for SQLite or a template metaprogramming-based home made ORM with some kind of a Code-First approach that allows to generate database table definitions from C++ structures, insert/update/delete/query the data without dealing with column names or column indices and iterating over the recordsets with standard C++ iterators.

## Examples

- [BlobTest.cpp](https://github.com/dmitriano/SQLiteWrapper/blob/main/Tests/BlobTest.cpp) is a very simple example of working with a database table as with `std::set` and working with a column of type BLOB.
- [SetTest.cpp](https://github.com/dmitriano/SQLiteWrapper/blob/main/Tests/SetTest.cpp) is an example of using composite keys and nested structures.

## Cloning the repository

    git clone https://github.com/dmitriano/SQLiteWrapper.git --recursive

or

    git clone https://github.com/dmitriano/SQLiteWrapper.git
    cd SQLiteWrapper
    git submodule init
    git submodule update

## Building on Windows with MSVC2022

Assuming that you cloned the repository into `D:/dev/repos/SQLiteWrapper`, downloaded SQLite sources into `D:\dev\libs\sqlite-amalgamation-3390200` and created `D:\dev\build\sw` directory to build SQLiteWrapper test project in:

    set MY_DRIVE=C:

    %MY_DRIVE%
    cd \dev\build\sw

    set SQLITE_SRC_DIR=%MY_DRIVE%/dev/libs/sqlite-amalgamation-3460000
    set MY_CMAKE_EXE=%MY_DRIVE%\dev\tools\cmake-3.24.2-windows-x86_64\bin\cmake.exe
    set MY_VS_GENERATOR="Visual Studio 17 2022"

    %MY_CMAKE_EXE% ..\..\repos\SQLiteWrapper -G %MY_VS_GENERATOR% -A x64

    %MY_CMAKE_EXE% --build . --target SQLiteWrapperTest --config Debug
    %MY_CMAKE_EXE% --build . --target SQLiteWrapperTest --config RelWithDebInfo

or

    msbuild SQLiteWrapperTest.sln /p:Configuration=Debug /p:Platform=x64
    msbuild SQLiteWrapperTest.sln /p:Configuration=RelWithDebInfo /p:Platform=x64

    Debug\SQLiteWrapperTest.exe
    RelWithDebInfo\SQLiteWrapperTest.exe

## Building on Linux with GCC

    cd ~
    mkdir -p dev/libs
    cd dev/libs
    wget https://www.sqlite.org/2024/sqlite-amalgamation-3460000.zip
    unzip sqlite-amalgamation-3460000.zip
    export SQLITE_SRC_DIR=$(realpath sqlite-amalgamation-3460000)
    cd ~/dev
    mkdir repos
    cd repos
    git clone https://github.com/dmitriano/SQLiteWrapper.git --recursive
    cd ~/dev
    mkdir -p build/sw
    cd build/sw
    cmake ../../repos/SQLiteWrapper -DCMAKE_BUILD_TYPE=Release
    cmake --build . --parallel
    ./SQLiteWrapperTest

## Running the tests on Android device

    adb push SQLiteWrapperTest /data/local/tmp
    adb shell "cd /data/local/tmp && chmod a+x SQLiteWrapperTest && ./SQLiteWrapperTest"
