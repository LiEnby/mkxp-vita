# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.16

# Delete rule output on recipe failure.
.DELETE_ON_ERROR:


#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:


# Remove some rules from gmake that .SUFFIXES does not remove.
SUFFIXES =

.SUFFIXES: .hpux_make_needs_suffix_list


# Suppress display of executed commands.
$(VERBOSE).SILENT:


# A target that is always out of date.
cmake_force:

.PHONY : cmake_force

#=============================================================================
# Set environment variables for the build.

# The shell in which to execute make rules.
SHELL = /bin/sh

# The CMake executable.
CMAKE_COMMAND = /usr/bin/cmake

# The command to remove a file.
RM = /usr/bin/cmake -E remove -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /home/silica/mkxp-vita/fixgpu/src

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/silica/mkxp-vita/fixgpu/src/build

# Include any dependencies generated for this target.
include CMakeFiles/gpu_fix.dir/depend.make

# Include the progress variables for this target.
include CMakeFiles/gpu_fix.dir/progress.make

# Include the compile flags for this target's objects.
include CMakeFiles/gpu_fix.dir/flags.make

CMakeFiles/gpu_fix.dir/fix_gpu.c.obj: CMakeFiles/gpu_fix.dir/flags.make
CMakeFiles/gpu_fix.dir/fix_gpu.c.obj: ../fix_gpu.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/silica/mkxp-vita/fixgpu/src/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building C object CMakeFiles/gpu_fix.dir/fix_gpu.c.obj"
	/usr/local/vitasdk/bin/arm-vita-eabi-gcc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/gpu_fix.dir/fix_gpu.c.obj   -c /home/silica/mkxp-vita/fixgpu/src/fix_gpu.c

CMakeFiles/gpu_fix.dir/fix_gpu.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/gpu_fix.dir/fix_gpu.c.i"
	/usr/local/vitasdk/bin/arm-vita-eabi-gcc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/silica/mkxp-vita/fixgpu/src/fix_gpu.c > CMakeFiles/gpu_fix.dir/fix_gpu.c.i

CMakeFiles/gpu_fix.dir/fix_gpu.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/gpu_fix.dir/fix_gpu.c.s"
	/usr/local/vitasdk/bin/arm-vita-eabi-gcc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/silica/mkxp-vita/fixgpu/src/fix_gpu.c -o CMakeFiles/gpu_fix.dir/fix_gpu.c.s

# Object files for target gpu_fix
gpu_fix_OBJECTS = \
"CMakeFiles/gpu_fix.dir/fix_gpu.c.obj"

# External object files for target gpu_fix
gpu_fix_EXTERNAL_OBJECTS =

gpu_fix: CMakeFiles/gpu_fix.dir/fix_gpu.c.obj
gpu_fix: CMakeFiles/gpu_fix.dir/build.make
gpu_fix: CMakeFiles/gpu_fix.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/silica/mkxp-vita/fixgpu/src/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Linking C executable gpu_fix"
	$(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/gpu_fix.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
CMakeFiles/gpu_fix.dir/build: gpu_fix

.PHONY : CMakeFiles/gpu_fix.dir/build

CMakeFiles/gpu_fix.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles/gpu_fix.dir/cmake_clean.cmake
.PHONY : CMakeFiles/gpu_fix.dir/clean

CMakeFiles/gpu_fix.dir/depend:
	cd /home/silica/mkxp-vita/fixgpu/src/build && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/silica/mkxp-vita/fixgpu/src /home/silica/mkxp-vita/fixgpu/src /home/silica/mkxp-vita/fixgpu/src/build /home/silica/mkxp-vita/fixgpu/src/build /home/silica/mkxp-vita/fixgpu/src/build/CMakeFiles/gpu_fix.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : CMakeFiles/gpu_fix.dir/depend

