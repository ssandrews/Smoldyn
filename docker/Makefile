VERSION:=$(shell cat ./VERSION)

all : wheels

LABEL:="dilawar/smoldyn_$(USER):$(VERSION)"

wheels : ./Dockerfile ./build_wheels_linux.sh 
	mkdir -p $(HOME)/wheelhouse
	(cd .. && docker build \
	    -t $(LABEL) \
	    -f docker/Dockerfile \
	    --build-arg PYPY_PASSWORD=$(PYPY_PASSWORD) . | tee log ) 
