cmake_minimum_required(VERSION 3.0)#cmake版本要求
project(main)#项目名称（不是可执行文件名称）

#配置编译选项
set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} -g)#2 

#配置头文件搜索路径
# include_directories() #4

#配置库文件搜索路径
# link_directories()#5

#设置需要编译的源文件列表
set(SRC_LIST ./muduo_server.cpp)#3 SRC_LIST为cpp文件列表名称，cpp文件少使用

#设置可执行文件最终存储的路径,把生成的可执行文件放入bin
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)

#把. 指定路径下的所有源文件名字放入变量名SRC_LIST里面
# aux_source_directory(. SRC_LIST)#3 cpp文件多使用

#表示生成可执行文件muduo_server，由SRC_LIST变量所定义的源文件编译而来
add_executable(muduo_server ${SRC_LIST})#1 muduo_server为可执行文件名称
#表示muduo_server这个目标程序需要连接的三个库文件
target_link_libraries(muduo_server muduo_net muduo_base pthread)#6
