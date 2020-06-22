all : run

LABEL:="dilawars/smoldyn:latest"

wheels : ./Dockerfile ./build_wheels_linux.sh 
	mkdir -p $(HOME)/wheelhouse
	# Generate version.
	(cd .. && docker build \
	    -t $(LABEL) \
	    -f scripts/Dockerfile \
	    --build-arg PYPI_PASSWORD=$(PYPI_PASSWORD) . | tee log ) 

run : wheels
	docker run -it dilawars/smoldyn:latest

test : wheels
	docker run -it dilawars/smoldyn:latest bash
