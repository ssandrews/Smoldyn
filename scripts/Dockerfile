FROM quay.io/pypa/manylinux2010_x86_64
MAINTAINER Dilawar Singh <dilawar.s.rajput@gmail.com>

ENV PATH=/usr/local/bin:$PATH

# Read PYPI_PASSWORD  
ARG PYPI_PASSWORD
ENV PYPI_PASSWORD=$PYPI_PASSWORD

RUN yum -y update
RUN yum -y install cmake3 && ln -sf /usr/bin/cmake3 /usr/bin/cmake && yum clean all 
RUN yum -y install freeglut-devel libtiff-devel libXmu-devel libXi-devel && yum clean all
WORKDIR /root
ADD . /root/
RUN ls -ltrh
RUN cd scripts && PYPI_PASSWORD=$PYPI_PASSWORD ./build_wheels_linux.sh 
