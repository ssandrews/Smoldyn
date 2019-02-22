#include "SimpleMesh.h"
using namespace std;


void SimpleMesh::getCenterCoordinates(int volIndex, double* coords)
{
	std::fill(coords, coords + 3, 1.0);
}

void SimpleMesh::getDeltaXYZ(double* delta)
{
	std::fill(delta, delta + 3, 0.2);
}

void SimpleMesh::getNumXYZ(int* num)
{
	std::fill(num, num + 3, 51);
}
