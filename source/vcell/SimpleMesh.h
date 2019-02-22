#ifndef SIMPLE_MESH_H
#define SIMPLE_MESH_H

#include <smoldyn.h>

class SimpleMesh : public AbstractMesh {
public:
	void getCenterCoordinates(int volIndex, double* coords);
	void getDeltaXYZ(double* delta);
	void getNumXYZ(int* num);
private:
};

#endif
