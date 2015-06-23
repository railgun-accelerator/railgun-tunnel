cmake_minimum_required(VERSION 2.8)
project(railgun_tunnel)

SET (CMAKE_C_COMPILER clang)
SET (CMAKE_CXX_COMPILER clang++)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -fstrict-aliasing -O4 -DCAT_THREADED_ALLOCATOR -pthread")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -fstrict-aliasing -O4 -DCAT_THREADED_ALLOCATOR -pthread")

set(SOURCE_FILES main.cpp)

add_executable(railgun_tunnel ${SOURCE_FILES} tunnel.cpp tunnel.h
    shorthair/libcat/MersenneTwister.cpp
    shorthair/libcat/Clock.cpp
    shorthair/libcat/BitMath.cpp
    shorthair/libcat/MemXOR.cpp
    shorthair/libcat/Mutex.cpp
    shorthair/libcat/MemSwap.cpp
    shorthair/libcat/Enforcer.cpp
    shorthair/libcat/EndianNeutral.cpp
    shorthair/libcat/ReuseAllocator.cpp
    shorthair/libcat/ReuseAllocator.cpp
    shorthair/libcat/SecureEqual.cpp
    shorthair/longhair/src/cauchy_256.cpp
    shorthair/src/Shorthair.cpp
)
include_directories( shorthair/include )
include_directories( shorthair/longhair/include )
include_directories( shorthair/libcat )




