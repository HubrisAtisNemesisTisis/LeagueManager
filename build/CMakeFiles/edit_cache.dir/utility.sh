set -e

cd /c/Users/shant/Videos/LeagueRepo/build
/usr/bin/ccmake.exe -S$(CMAKE_SOURCE_DIR) -B$(CMAKE_BINARY_DIR)
