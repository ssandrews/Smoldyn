#
# NOTE: Run this from the top directory.
#
all : ubuntu

ubuntu : Dockerfile.ubuntu
	cd .. && docker build -t smoldyn/ubuntu:latest . -f .ci/$<

ubuntu_test : ubuntu
	cd .. && docker run -it smoldyn/ubuntu:latest bash
