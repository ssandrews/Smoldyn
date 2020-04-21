FROM quay.io/pypa/manylinux2010_x86_64
MAINTAINER Dilawar Singh <dilawar.s.rajput@gmail.com>

ENV PATH=/usr/local/bin:$PATH

RUN yum update
RUN yum install -y cmake3 && ln -sf /usr/bin/cmake28 /usr/bin/cmake
WORKDIR .
RUN cd docker && ./build_wheels_linux.sh 
# RUN cd deploy-master && ./test_and_upload.sh $PYPI_PASSWORD
