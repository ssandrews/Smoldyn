FROM quay.io/pypa/manylinux2010_x86_64
MAINTAINER Dilawar Singh <dilawar.s.rajput@gmail.com>

ENV PATH=/usr/local/bin:$PATH

# Read PYPI_PASSWORD  
ARG PYPI_PASSWORD
ENV PYPI_PASSWORD=$PYPI_PASSWORD

RUN yum -y update
RUN yum -y install freeglut-devel libtiff-devel libXmu-devel libXi-devel && yum clean all
RUN git config --global user.name "Dilawar Singh"
RUN git config --global user.email "dilawar@subcom.tech"
WORKDIR /root
ADD . /root/
RUN /opt/python/cp38-cp38/bin/python -m pip install cmake 
RUN yum -y remove cmake cmake3 
RUN ln -s /opt/python/cp38-cp38/bin/cmake /usr/local/bin/cmake
RUN ls -ltrh
CMD cd scripts && PYPI_PASSWORD=$PYPI_PASSWORD ./build_wheels_linux.sh 
