FROM quay.io/pypa/manylinux_1_2_x86_64
ENV PATH=$PATH:/usr/local/bin
RUN yum -y update
RUN yum -y install freeglut-devel libtiff-devel libXmu-devel libffi-devel libXi-devel && yum clean all
WORKDIR /root
ADD . /root/
RUN ls -ltrh
