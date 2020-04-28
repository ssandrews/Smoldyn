FROM quay.io/pypa/manylinux2010_x86_64
MAINTAINER Dilawar Singh <dilawar.s.rajput@gmail.com>

ENV PATH=/usr/local/bin:$PATH

RUN yum -y update
RUN yum -y install cmake3 && ln -sf /usr/bin/cmake3 /usr/bin/cmake && yum clean all 
RUN yum -y install freeglut-devel libtiff-devel libXmu-devel libXi-devel && yum clean all
WORKDIR /root
ADD . /root/
RUN ls -ltrh
RUN cd docker && ./build_wheels_linux.sh 
# RUN cd deploy-master && ./test_and_upload.sh $PYPI_PASSWORD
