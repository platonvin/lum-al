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

objs := \
	obj/al.o\
	obj/setup.o\
	obj/triangle.o\

ifeq ($(OS),Windows_NT)
	REQUIRED_LIBS = -lglfw3 -lgdi32        -lvolk
	STATIC_OR_DYNAMIC += -static
else
	REQUIRED_LIBS = -lglfw3 -lpthread -ldl -lvolk
endif
	
# all of them united
always_enabled_flags = -fno-exceptions -Wuninitialized -std=c++20 -Wno-inconsistent-missing-destructor-override

srcs := \
	examples/triangle.cpp\
	src/al.cpp\
	src/setup.cpp\

#default target
profile = -D_PRINTLINE -DVKNDEBUG
profile = 
all: init release

obj/triangle.o: examples/triangle.cpp
	c++ $(special_otp_flags) $(always_enabled_flags) $(I) $(args) $(profile) -MMD -MP -c $< -o $@ 
DEPS = $(objs:.o=.d)
-include $(DEPS)

obj/%.o: src/%.cpp
	c++ $(special_otp_flags) $(always_enabled_flags) $(I) $(args) $(profile) -MMD -MP -c $< -o $@
DEPS = $(objs:.o=.d)
-include $(DEPS)


release: init $(objs) build
ifeq ($(OS),Windows_NT)
	.\triangle
else
	./triangle
endif

build: $(objs)
	c++ -o triangle $(objs) $(flags) $(I) $(L) $(REQUIRED_LIBS) $(STATIC_OR_DYNAMIC)

init: obj
obj:
ifeq ($(OS),Windows_NT)
	mkdir "obj"
else
	mkdir -p obj
endif

clean:
ifeq ($(OS),Windows_NT)
	del "obj\*.o"
else
	rm -R obj/*.o
endif