NVCC=nvcc

CUDA_INCLUDEPATH=/usr/local/cuda-5.0/include

NVCC_OPTS=-O3 -arch=sm_20 -Xcompiler -Wall -Xcompiler -Wextra -m64

GCC_OPTS= -Wall -Wextra -m64 -g

Main: Main.o Vec2.o Vec3.o Triangle.o Face.o OBJFile.o Intersect.o BoundingBox.o SparseVoxelOctree.o Node.o Voxels.o Makefile
	g++ -o main Main.o Vec2.o Vec3.o Triangle.o Face.o OBJFile.o Intersect.o BoundingBox.o SparseVoxelOctree.o Node.o Voxels.o $(GCC_OPTS)

Vec2.o: Vec2.cpp Vec2.hpp
	g++ -c Vec2.cpp $(GCC_OPTS) -I $(CUDA_INCLUDEPATH)

Vec3.o: Vec3.cpp Vec3.hpp
	g++ -c Vec3.cpp $(GCC_OPTS) -I $(CUDA_INCLUDEPATH)

Triangle.o: Triangle.cpp Triangle.hpp Vec3.hpp
	g++ -c Triangle.cpp $(GCC_OPTS) -I $(CUDA_INCLUDEPATH)

Face.o: Face.cpp Face.hpp
	g++ -c Face.cpp $(GCC_OPTS) -I $(CUDA_INCLUDEPATH)

OBJFile.o: OBJFile.cpp OBJFile.hpp Vec3.hpp Face.hpp Triangle.hpp BoundingBox.hpp
	g++ -c OBJFile.cpp $(GCC_OPTS) -I $(CUDA_INCLUDEPATH)

Intersect.o: Intersect.cpp Intersect.hpp Vec3.hpp Triangle.hpp Vec2.hpp
	g++ -c Intersect.cpp $(GCC_OPTS) -I $(CUDA_INCLUDEPATH)

BoundingBox.o: BoundingBox.cpp BoundingBox.hpp Vec3.hpp
	g++ -c BoundingBox.cpp $(GCC_OPTS) -I $(CUDA_INCLUDEPATH)

SparseVoxelOctree.o: SparseVoxelOctree.cpp Intersect.hpp Vec3.hpp Triangle.hpp Vec2.hpp Voxels.hpp
	g++ -c SparseVoxelOctree.cpp $(GCC_OPTS) -I $(CUDA_INCLUDEPATH)

Node.o: Node.cpp Node.hpp
	g++ -c Node.cpp $(GCC_OPTS) -I $(CUDA_INCLUDEPATH)

Voxels.o: Voxels.cpp Voxels.hpp 
	g++ -c Voxels.cpp $(GCC_OPTS) -I $(CUDA_INCLUDEPATH)

Main.o: Main.cpp Intersect.hpp
	g++ -c Main.cpp $(GCC_OPTS)

clean:
	rm -f *.o main
