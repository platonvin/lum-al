.ONESHELL:

#setting up include and lib directories for dependencies
I = -Isrc
L = 

OTHER_DIRS := $(filter-out vcpkg_installed/vcpkg, $(wildcard vcpkg_installed/*))
INCLUDE_LIST := $(addsuffix /include, $(OTHER_DIRS))
INCLUDE_LIST += $(addsuffix /include/vma, $(OTHER_DIRS))
INCLUDE_LIST += $(addsuffix /include/volk, $(OTHER_DIRS))
INCLUDE_LIST := $(addprefix -I, $(INCLUDE_LIST))
# OTHER_DIRS := $(filter-out vcpkg_installed/vcpkg, $(wildcard vcpkg_installed/*))
LIB_LIST := $(addsuffix /lib, $(OTHER_DIRS))
LIB_LIST := $(addprefix -L, $(LIB_LIST))

I += $(INCLUDE_LIST)
L += $(LIB_LIST)
#spirv things are installed with vcpkg and not set in envieroment, so i find needed tools myself
GLSLC_DIR := $(firstword $(foreach dir, $(OTHER_DIRS), $(wildcard $(dir)/tools/shaderc)))
GLSLC = $(GLSLC_DIR)/glslc


STATIC_OR_DYNAMIC = 
ifeq ($(OS),Windows_NT)
	REQUIRED_LIBS = -lglfw3 -lgdi32        -lvolk
	STATIC_OR_DYNAMIC += -static
else
	REQUIRED_LIBS = -lglfw3 -lpthread -ldl -lvolk
endif
	
# all of them united
always_enabled_flags = -fno-exceptions -Wuninitialized -std=c++20 -Wno-inconsistent-missing-destructor-override
debug_specific_flags   = -O0 -g
release_specific_flags = -Ofast -DVKNDEBUG -mmmx -msse -msse2 -msse3 -mssse3 -msse4.1 -msse4.2 -mcx16 -mavx -mpclmul -fdata-sections -ffunction-sections -s -mfancy-math-387 -fno-math-errno -Wl,--gc-sections
release_flags = $(release_specific_flags) $(always_enabled_flags)
  debug_flags = $(debug_specific_flags)   $(always_enabled_flags)
#for "just libs"
special_otp_flags = $(always_enabled_flags) -fno-exceptions -Wuninitialized -Ofast -DVKNDEBUG -mmmx -msse -msse2 -msse3 -mssse3 -msse4.1 -msse4.2 -mcx16 -mavx -mpclmul -fdata-sections -ffunction-sections -s  -mfancy-math-387 -fno-math-errno -Wl,--gc-sections
#for crazy builds
crazy_flags = $(always_enabled_flags) -Ofast -flto -fopenmp -floop-parallelize-all -ftree-parallelize-loops=8 -D_GLIBCXX_PARALLEL -DVKNDEBUG -fno-exceptions -funroll-loops -w -mmmx -msse -msse2 -msse3 -mssse3 -msse4.1 -msse4.2 -mcx16 -mavx -mpclmul -fdata-sections -ffunction-sections -s  -fno-math-errno -Wl,--gc-sections

SHADER_FLAGS = --target-env=vulkan1.1 -g -O

srcs := \
	examples/triangle.cpp\
	src/al.cpp\
	src/setup.cpp\

#default target
all: 
	GLSLC examples/triag.frag -o examples/frag.spv
	GLSLC examples/triag.vert -o examples/vert.spv
	c++ $(srcs) -o triag $(I) $(L) $(REQUIRED_LIBS)
	.\triag