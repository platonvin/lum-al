.ONESHELL:

#setting up include and lib directories for dependencies
I = -Isrc
L = 

OTHER_DIRS := $(filter-out vcpkg_installed/vcpkg, $(wildcard vcpkg_installed/*))
INCLUDE_LIST := $(addsuffix /include, $(OTHER_DIRS))
INCLUDE_LIST += $(addsuffix /include/vma, $(OTHER_DIRS))
INCLUDE_LIST += $(addsuffix /include/volk, $(OTHER_DIRS))
INCLUDE_LIST := $(addprefix -I, $(INCLUDE_LIST))
LIB_LIST := $(addsuffix /lib, $(OTHER_DIRS))
LIB_LIST := $(addprefix -L, $(LIB_LIST))

I += $(INCLUDE_LIST)
L += $(LIB_LIST)
#spirv things are installed with vcpkg and not set in envieroment, so i find needed tools myself
GLSLC_DIR := $(firstword $(foreach dir, $(OTHER_DIRS), $(wildcard $(dir)/tools/shaderc)))
GLSLC = $(GLSLC_DIR)/glslc

# IT MIGHT RANDOMLY WORK SOMETIMES
ifeq ($(OS),Windows_NT)
	GLSLC := $(subst /,\\,$(GLSLC))
endif

lib_objs := \
	obj/al.o\
	obj/setup.o\

ifeq ($(OS),Windows_NT)
	REQUIRED_LIBS = -lglfw3 -lgdi32        -lvolk
	STATIC_OR_DYNAMIC += -static
else
	REQUIRED_LIBS = -lglfw3 -lpthread -ldl -lvolk
endif
	
always_enabled_flags = -fno-exceptions -Wuninitialized -std=c++20 -Wno-inconsistent-missing-destructor-override

srcs := \
	examples/triangle.cpp\
	src/al.cpp\
	src/setup.cpp\

#default target
profile = -D_PRINTLINE -DVKNDEBUG
profile = 
all: init example

obj/triangle.o: examples/triangle.cpp
	c++ $(special_otp_flags) $(always_enabled_flags) $(I) $(args) $(profile) -MMD -MP -c $< -o $@ 
DEPS = $(obj/triangle.d)
-include $(DEPS)

obj/%.o: src/%.cpp
	c++ $(special_otp_flags) $(always_enabled_flags) $(I) $(args) $(profile) -MMD -MP -c $< -o $@
DEPS = $(lib_objs:.o=.d)
-include $(DEPS)


example: init library build_example shaders
ifeq ($(OS),Windows_NT)
	.\triangle
else
	./triangle
endif

shaders: examples/triag.vert examples/triag.frag examples/posteffect.frag
	$(GLSLC) -o examples/vert.spv examples/triag.vert 
	$(GLSLC) -o examples/frag.spv examples/triag.frag 
	$(GLSLC) -o examples/posteffect.spv examples/posteffect.frag

# c++ $(lib_objs) $(I) $(L) $(REQUIRED_LIBS) $(STATIC_OR_DYNAMIC) -c -o lumal 
library: init $(lib_objs)
	ar rvs lib/liblumal.a $(lib_objs)

build_example: library obj/triangle.o
	c++ -o triangle obj/triangle.o -llumal -Llib $(flags) $(I) $(L) $(REQUIRED_LIBS) $(STATIC_OR_DYNAMIC)

init: obj lib
obj:
ifeq ($(OS),Windows_NT)
	mkdir "obj"
else
	mkdir -p obj
endif

lib:
ifeq ($(OS),Windows_NT)
	mkdir "lib"
else
	mkdir -p lib
endif

clean:
ifeq ($(OS),Windows_NT)
	del "obj\*.o"
else
	rm -R obj/*.o
endif