#
# NOTE: Run this from the top directory.
#
all : ubuntu

build_ubuntu_image: Dockerfile.ubuntu
	cd .. && docker build -t smoldyn/ubuntu:latest . -f .ci/$<

ubuntu : build_ubuntu_image
	docker run -t smoldyn/ubuntu:latest 

ubuntu_test : build_ubuntu_image
	cd .. && docker run -it smoldyn/ubuntu:latest bash
