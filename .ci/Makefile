#
# NOTE: Run this from the top directory.
#
all : ubuntu

ubuntu : Dockerfile.ubuntu
	cd .. && docker build -t smoldyn/ubuntu:latest . -f .ci/$<
