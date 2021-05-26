all : wheels

LABEL:="dilawars/smoldyn:latest"

build : ./Dockerfile ./build_wheels_linux.sh 
	mkdir -p $(HOME)/wheelhouse
	# Generate version.
	(cd .. && docker build \
	    --rm \
	    -t $(LABEL) \
	    -f scripts/Dockerfile \
	    --build-arg PYPI_PASSWORD=$(PYPI_PASSWORD) . | tee log ) 

wheels : ./Dockerfile ./build_wheels_linux.sh build
	mkdir -p $(HOME)/wheelhouse
	# Generate version.
	CONTAINER := $(shell docker run -e PYPI_PASSWORD=$(PYPI_PASSWORD) \
		$(LABEL))
	docker cp $(CONTAINER):/root/wheelhouse/*.whl $(HOME)/wheelhouse/
	docker rm -v $(CONTAINER)

test : build
	docker run -it \
	    --env="DISPLAY" \
	    --volume="/tmp/.X11-unix:/tmp/.X11-unix:rw" \
	    --rm \
	    dilawars/smoldyn:latest bash
