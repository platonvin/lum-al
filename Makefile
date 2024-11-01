.ONESHELL:

#setting up include and lib directories for dependencies
INCLUDE_DIRS = -Isrc
LINK_DIRS = 

lib_objs := \
	obj/al.o\
	obj/setup.o\

EXTERNAL_LIBS := -lglfw3 -lvolk
ifeq ($(OS),Windows_NT)
	EXTERNAL_LIBS += -lgdi32       
	STATIC_OR_DYNAMIC += -static
	WHICH_WHERE = where
	RUN_POSTFIX = .exe
	RUN_PREFIX = .\\
	SLASH = \\

else
	EXTERNAL_LIBS += -lpthread -ldl
	RUN_PREFIX = ./
	WHICH_WHERE = which
	RUN_POSTFIX = 
	SLASH = /
endif

REQUIRED_LIBS = $(EXTERNAL_LIBS)
always_enabled_flags = -fno-exceptions -Wuninitialized -std=c++20 -Wno-inconsistent-missing-destructor-override -Wno-nullability-completeness
library_opt_flags = -Os
# default version. If native found, it overrides this one
VCPKG_EXEC := $(RUN_PREFIX)lum_vcpkg$(SLASH)vcpkg$(RUN_POSTFIX) --vcpkg-root=lum_vcpkg
SHADER_FLAGS = -V --target-env vulkan1.1 -g

srcs := \
	examples/triangle.cpp\
	src/al.cpp\
	src/setup.cpp\

#default target
profile = -D_PRINTLINE -DVKNDEBUG
profile = 
all: init triangle

obj/%.o: src/%.cpp | init vcpkg_installed_eval
	c++ $(library_opt_flags) $(always_enabled_flags) $(INCLUDE_DIRS) $(args) $(profile) -MMD -MP -c $< -o $@
DEPS = $(lib_objs:.o=.d)
-include $(DEPS)

obj/triangle.o: examples/triangle.cpp | init vcpkg_installed_eval
	c++ $(always_enabled_flags) $(INCLUDE_DIRS) $(args) $(profile) -MMD -MP -c $< -o $@ 
DEPS = $(obj/triangle.d)
-include $(DEPS)


triangle: init vcpkg_installed_eval lib/liblumal.a build_example shaders
ifeq ($(OS),Windows_NT)
	.\triangle
else
	./triangle
endif

shaders: examples/triag.vert examples/triag.frag examples/posteffect.frag init vcpkg_installed_eval
	$(GLSLC) $(SHADER_FLAGS) -o examples/vert.spv examples/triag.vert
	$(GLSLC) $(SHADER_FLAGS) -o examples/frag.spv examples/triag.frag
	$(GLSLC) $(SHADER_FLAGS) -o examples/posteffect.spv examples/posteffect.frag

# c++ $(lib_objs) $(INCLUDE_DIRS) $(LINK_DIRS) $(REQUIRED_LIBS) $(STATIC_OR_DYNAMIC) -c -o lumal 
lib/liblumal.a: $(lib_objs) | init vcpkg_installed_eval
	ar rvs lib/liblumal.a $(lib_objs)

library: lib/liblumal.a

build_example: lib/liblumal.a obj/triangle.o
	c++ -o triangle obj/triangle.o -llumal -Llib $(args) $(always_enabled_flags) $(INCLUDE_DIRS) $(LINK_DIRS) $(REQUIRED_LIBS) $(STATIC_OR_DYNAMIC)

init: obj/ lib/
%/: #pattern rule to make all folders
ifeq ($(OS),Windows_NT)
	mkdir "$@"
else
	mkdir -p $@
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
	$(eval INCLUDE_DIRS += $(addprefix -I, $(INCLUDE_LIST)) )
	$(eval LINK_DIRS += $(addprefix -L, $(LIB_LIST)) )
	$(eval GLSLC_DIR := $(firstword $(foreach dir, $(OTHER_DIRS), $(wildcard $(dir)/tools/glslang))) )
	$(eval GLSLC := $(strip $(GLSLC_DIR))/glslang )
	$(eval GLSLC := $(subst /,\\,$(GLSLC)) )

# try to find native vcpkg 
VCPKG_FOUND := $(shell $(WHICH_WHERE) vcpkg)

#sorry microsoft no telemetry today
lum_vcpkg:
	@echo No vcpkg in PATH, installing vcpkg
	git clone https://github.com/microsoft/vcpkg.git lum_vcpkg
	cd lum_vcpkg
ifeq ($(OS),Windows_NT)
	-$(RUN_PREFIX)bootstrap-vcpkg.bat -disableMetrics 
else
	-$(RUN_PREFIX)bootstrap-vcpkg.sh -disableMetrics 
endif
	vcpkg$(RUN_POSTFIX) integrate install
	@echo bootstrapped

# if no vcpkg found, install it directly in Lum 
ifdef VCPKG_FOUND
check_vcpkg_itself:
	$(info NATIVE VCPKG IS FOUND AT $(VCPKG_FOUND))
	$(eval VCPKG_EXEC := vcpkg)
else
check_vcpkg_itself: | lum_vcpkg
	$(info NATIVE VCPKG NOT FOUND, INSTALLED POCKET VERSION)
endif

vcpkg_installed: | check_vcpkg_itself
	@echo installing vcpkg dependencies. Please do not interrupt
	$(VCPKG_EXEC) install --overlay-triplets=vcpkg_triplets --triplet=x64-lum-rel

update: init clean | check_vcpkg_itself
	@echo updating vcpkg dependencies. Please do not interrupt
	$(info VCPKG IS FOUND AS $(VCPKG_EXEC))
	$(VCPKG_EXEC) install --overlay-triplets=vcpkg_triplets --triplet=x64-lum-rel