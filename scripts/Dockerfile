FROM quay.io/pypa/manylinux2010_x86_64
MAINTAINER Dilawar Singh <dilawar.s.rajput@gmail.com>

ENV PATH=$PATH:/usr/local/bin

RUN yum -y update
RUN yum -y install freeglut-devel libtiff-devel libXmu-devel libXi-devel && yum clean all
RUN git config --global user.name "Dilawar Singh"
RUN git config --global user.email "dilawar@subcom.tech"
WORKDIR /root
ADD . /root/
RUN ls -ltrh
