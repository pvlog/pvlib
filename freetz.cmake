set(FREETZ_PATH $ENV{HOME}/freetz) 

set(CMAKE_SYSTEM_NAME Linux)


# specify the cross compiler
set(CMAKE_C_COMPILER   ${FREETZ_PATH}/toolchain/target/bin/mipsel-linux-gcc) 
set(CMAKE_CXX_COMPILER ${FREETZ_PATH}/toolchain/target/bin/mipsel-linux-g++)
#set(CMAKE_C_COMPILER   ${FREETZ_PATH}/toolchain/build/gcc-4.2.1-uClibc-0.9.29/mipsel-linux-uclibc/bin/mipsel-linux-gcc) 
#set(CMAKE_CXX_COMPILER ${FREETZ_PATH}/toolchain/build/gcc-4.2.1-uClibc-0.9.29/mipsel-linux-uclibc/bin/mipsel-linux-g++)

set(CMAKE_C_FLAGS "-Os -static -pipe -march=4kc -Wa,--trap")
set(CMAKE_CXX_FLAGS "-Os -static -pipe -march=4kc -Wa,--trap")

# where is the target environment 
set(CMAKE_FIND_ROOT_PATH ${FREETZ_PATH}/toolchain/target)
#set(CMAKE_FIND_ROOT_PATH ${FREETZ_PATH}/toolchain/build/gcc-4.2.1-uClibc-0.9.29/mipsel-linux-uclibc)

# search for programs in the build host directories
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
# for libraries and headers in the target directories
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

