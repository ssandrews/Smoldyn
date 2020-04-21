VERSION:=$(shell cat ./VERSION)

all : wheels

DOCKERFILE:="dilawar/smoldyn_$(USER):$(VERSION)"

wheels : ./Dockerfile ./build_wheels_linux.sh 
	mkdir -p $(HOME)/wheelhouse
	(cd .. && docker build  --no-cache \
	    -t $(DOCKERFILE) \
	    --build-arg PYPY_PASSWORD=$(PYPY_PASSWORD) docker | tee log )
