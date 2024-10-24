.ONESHELL:

#setting up include and lib directories for dependencies
I = -Isrc
L = 

CPP_COMPILER = c++

lib_objs := \
	obj/al.o\
	obj/setup.o\

ifeq ($(OS),Windows_NT)
	REQUIRED_LIBS = -lglfw3 -lgdi32        -lvolk
	STATIC_OR_DYNAMIC += -static
else
	REQUIRED_LIBS = -lglfw3 -lpthread -ldl -lvolk
endif
	
always_enabled_flags = -fno-exceptions -Wuninitialized -std=c++20 -Wno-inconsistent-missing-destructor-override -Wno-nullability-completeness
library_opt_flags = -Os

srcs := \
	examples/triangle.cpp\
	src/al.cpp\
	src/setup.cpp\

#default target
profile = -D_PRINTLINE -DVKNDEBUG
profile = 
all: init triangle

obj/%.o: src/%.cpp | init vcpkg_installed_eval
	$(CPP_COMPILER) $(library_opt_flags) $(always_enabled_flags) $(I) $(args) $(profile) -MMD -MP -c $< -o $@
DEPS = $(lib_objs:.o=.d)
-include $(DEPS)

obj/triangle.o: examples/triangle.cpp init vcpkg_installed_eval
	$(CPP_COMPILER) $(always_enabled_flags) $(I) $(args) $(profile) -MMD -MP -c $< -o $@ 
DEPS = $(obj/triangle.d)
-include $(DEPS)


triangle: init vcpkg_installed_eval lib/liblumal.a build_example shaders
ifeq ($(OS),Windows_NT)
	.\triangle
else
	./triangle
endif

shaders: examples/triag.vert examples/triag.frag examples/posteffect.frag init vcpkg_installed_eval
	$(GLSLC) -o examples/vert.spv examples/triag.vert
	$(GLSLC) -o examples/frag.spv examples/triag.frag
	$(GLSLC) -o examples/posteffect.spv examples/posteffect.frag

# $(CPP_COMPILER) $(lib_objs) $(I) $(L) $(REQUIRED_LIBS) $(STATIC_OR_DYNAMIC) -c -o lumal 
lib/liblumal.a: $(lib_objs) | init vcpkg_installed_eval
	ar rvs lib/liblumal.a $(lib_objs)

library: lib/liblumal.a

build_example: lib/liblumal.a obj/triangle.o
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
	del "obj\*.o" "lib\*.a" "examples\*.spv"
else
	rm -rf obj/*.o lib/*.a examples/*.spv
endif

.PHONY: vcpkg_installed_eval 
vcpkg_installed_eval: vcpkg_installed
	$(eval OTHER_DIRS := $(filter-out vcpkg_installed/vcpkg, $(wildcard vcpkg_installed/*)) )
	$(eval INCLUDE_LIST := $(addsuffix /include, $(OTHER_DIRS)) )
	$(eval INCLUDE_LIST += $(addsuffix /include/vma, $(OTHER_DIRS)) )
	$(eval INCLUDE_LIST += $(addsuffix /include/volk, $(OTHER_DIRS)) )
	$(eval LIB_LIST := $(addsuffix /lib, $(OTHER_DIRS)) )
	$(eval I += $(addprefix -I, $(INCLUDE_LIST)) )
	$(eval L += $(addprefix -L, $(LIB_LIST)) )
#spirv things are installed with vcpkg and not set in envieroment, so i find needed tools myself
	$(eval GLSLC_DIR := $(firstword $(foreach dir, $(OTHER_DIRS), $(wildcard $(dir)/tools/shaderc))) )
	$(eval GLSLC := $(strip $(GLSLC_DIR))/glslc )
ifeq ($(OS),Windows_NT)
	$(eval GLSLC := $(subst /,\,$(GLSLC)) )
endif

vcpkg_installed:
	echo installind vcpkg dependencies. Please do not interrupt
	vcpkg install