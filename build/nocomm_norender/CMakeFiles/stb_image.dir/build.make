# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.9

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
CMAKE_SOURCE_DIR = /home/dalton/code/learn-comms/enginee

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/dalton/code/learn-comms/build/nocomm_norender

# Include any dependencies generated for this target.
include CMakeFiles/stb_image.dir/depend.make

# Include the progress variables for this target.
include CMakeFiles/stb_image.dir/progress.make

# Include the compile flags for this target's objects.
include CMakeFiles/stb_image.dir/flags.make

CMakeFiles/stb_image.dir/lib/stb_image.cpp.o: CMakeFiles/stb_image.dir/flags.make
CMakeFiles/stb_image.dir/lib/stb_image.cpp.o: /home/dalton/code/learn-comms/enginee/lib/stb_image.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/dalton/code/learn-comms/build/nocomm_norender/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object CMakeFiles/stb_image.dir/lib/stb_image.cpp.o"
	/usr/bin/c++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/stb_image.dir/lib/stb_image.cpp.o -c /home/dalton/code/learn-comms/enginee/lib/stb_image.cpp

CMakeFiles/stb_image.dir/lib/stb_image.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/stb_image.dir/lib/stb_image.cpp.i"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/dalton/code/learn-comms/enginee/lib/stb_image.cpp > CMakeFiles/stb_image.dir/lib/stb_image.cpp.i

CMakeFiles/stb_image.dir/lib/stb_image.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/stb_image.dir/lib/stb_image.cpp.s"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/dalton/code/learn-comms/enginee/lib/stb_image.cpp -o CMakeFiles/stb_image.dir/lib/stb_image.cpp.s

CMakeFiles/stb_image.dir/lib/stb_image.cpp.o.requires:

.PHONY : CMakeFiles/stb_image.dir/lib/stb_image.cpp.o.requires

CMakeFiles/stb_image.dir/lib/stb_image.cpp.o.provides: CMakeFiles/stb_image.dir/lib/stb_image.cpp.o.requires
	$(MAKE) -f CMakeFiles/stb_image.dir/build.make CMakeFiles/stb_image.dir/lib/stb_image.cpp.o.provides.build
.PHONY : CMakeFiles/stb_image.dir/lib/stb_image.cpp.o.provides

CMakeFiles/stb_image.dir/lib/stb_image.cpp.o.provides.build: CMakeFiles/stb_image.dir/lib/stb_image.cpp.o


# Object files for target stb_image
stb_image_OBJECTS = \
"CMakeFiles/stb_image.dir/lib/stb_image.cpp.o"

# External object files for target stb_image
stb_image_EXTERNAL_OBJECTS =

libstb_image.a: CMakeFiles/stb_image.dir/lib/stb_image.cpp.o
libstb_image.a: CMakeFiles/stb_image.dir/build.make
libstb_image.a: CMakeFiles/stb_image.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/dalton/code/learn-comms/build/nocomm_norender/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Linking CXX static library libstb_image.a"
	$(CMAKE_COMMAND) -P CMakeFiles/stb_image.dir/cmake_clean_target.cmake
	$(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/stb_image.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
CMakeFiles/stb_image.dir/build: libstb_image.a

.PHONY : CMakeFiles/stb_image.dir/build

CMakeFiles/stb_image.dir/requires: CMakeFiles/stb_image.dir/lib/stb_image.cpp.o.requires

.PHONY : CMakeFiles/stb_image.dir/requires

CMakeFiles/stb_image.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles/stb_image.dir/cmake_clean.cmake
.PHONY : CMakeFiles/stb_image.dir/clean

CMakeFiles/stb_image.dir/depend:
	cd /home/dalton/code/learn-comms/build/nocomm_norender && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/dalton/code/learn-comms/enginee /home/dalton/code/learn-comms/enginee /home/dalton/code/learn-comms/build/nocomm_norender /home/dalton/code/learn-comms/build/nocomm_norender /home/dalton/code/learn-comms/build/nocomm_norender/CMakeFiles/stb_image.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : CMakeFiles/stb_image.dir/depend

