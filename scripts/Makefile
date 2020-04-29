all : wheels

LABEL:="dilawars/smoldyn:latest"

wheels : ./Dockerfile ./build_wheels_linux.sh 
	mkdir -p $(HOME)/wheelhouse
	# Generate version.
	(cd .. && docker build \
	    -t $(LABEL) \
	    -f scripts/Dockerfile \
	    --build-arg PYPI_PASSWORD=$(PYPI_PASSWORD) . | tee log ) 
