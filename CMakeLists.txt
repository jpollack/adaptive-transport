cmake_minimum_required(VERSION 3.12)

project (bwest)

set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_CXX_STANDARD 17)  # for c++14

find_package (Threads)
# find_package(Python2 COMPONENTS Development)

# include_directories(${Python2_INCLUDE_DIRS} ${Python2_NumPy_INCLUDE_DIRS})

# remove src/Plot.cpp
set(SOURCE_COMMON src/Address.cpp src/FileDescriptor.cpp src/Poller.cpp src/Socket.cpp src/TSReturn.cpp src/PTT.cpp src/fullio.cpp src/UDPStream.cpp src/Timing.cpp src/CircBuff.cpp src/RTXQueue.cpp)

add_executable(send_data ${SOURCE_COMMON} src/send_data.cpp)
target_link_libraries (send_data Threads::Threads)

add_executable(recv_data ${SOURCE_COMMON} src/recv_data.cpp)
target_link_libraries (recv_data Threads::Threads)

# add_executable(ptt_tester ${SOURCE_COMMON} src/ptt_tester.cpp)
# target_link_libraries (ptt_tester Threads::Threads Python2::Python)

# add_executable(plot_test ${SOURCE_COMMON} src/plot_test.cpp)
# target_link_libraries (plot_test Threads::Threads Python2::Python)


