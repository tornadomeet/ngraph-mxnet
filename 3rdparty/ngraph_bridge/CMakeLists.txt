#*******************************************************************************
# Copyright 2018 Intel Corporation
# 
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
# 
#     http://www.apache.org/licenses/LICENSE-2.0
# 
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#*******************************************************************************

cmake_minimum_required(VERSION 3.0.2)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-comment")
SET(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-g")
SET(CMAKE_CXX_FLAGS_DEBUG  "-O0 -g")
set(NGRAPH_INSTALL_PREFIX ${CMAKE_CURRENT_BINARY_DIR})

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

if(UNIX)
	find_program(LSB_RELEASE_EXEC lsb_release)
	execute_process(COMMAND ${LSB_RELEASE_EXEC} -d
	    OUTPUT_VARIABLE LSB_RELEASE_ID
	    OUTPUT_STRIP_TRAILING_WHITESPACE
	)
	if(LSB_RELEASE_ID MATCHES "Ubuntu 16.04")
		set(NGRAPH_USE_PREBUILT_LLVM 1)
	endif()
endif()

if(USE_NGRAPH_IE)
  add_definitions(-DMXNET_USE_NGRAPH_IE=1)
endif(USE_NGRAPH_IE)

if(USE_NGRAPH_DISTRIBUTED)
  find_package(MPI REQUIRED)
  include_directories(SYSTEM ${MPI_C_INCLUDE_PATH} ${MPI_CXX_INCLUDE_PATH})
  link_directories(${MPI_C_LIBRARIES} ${MPI_CXX_LIBRARIES})
  add_definitions(-DMXNET_USE_NGRAPH_DISTRIBUTED=1)
endif(USE_NGRAPH_DISTRIBUTED)


include(cmake/ngraph.cmake)

include_directories(src src/ops)
include_directories(../../include/)
include_directories(../mshadow/)
include_directories(../dmlc-core/include/)
include_directories(../tvm/nnvm/include/)
include_directories(../tvm/dlpack/include/)
include_directories(build/include/)
include_directories(${CMAKE_CURRENT_BINARY_DIR}/include)

add_subdirectory(src)

add_dependencies(ngraph_bridge ext_ngraph)
