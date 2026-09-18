# Simple build using the HDF5 C++ wrapper compiler (h5c++), which is
# installed alongside libhdf5-dev and automatically sets the right
# include/library flags. This avoids needing CMake or pkg-config.
#
# Usage:
#   make
#   ./fd_convergence
#
# If h5c++ is not on your PATH but HDF5 is installed, replace CXX below
# with g++ and add -I/path/to/hdf5/include -L/path/to/hdf5/lib -lhdf5_cpp -lhdf5

CXX      := h5c++
CXXFLAGS := -O2 -std=c++17 -Wall -Wextra
TARGET   := fd_convergence
SRC      := finite_difference.cpp

.PHONY: all run clean

all: $(TARGET)

$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) -o $@ $<

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET) fd_results.h5
