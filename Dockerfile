FROM ubuntu:14.04

ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update
RUN apt-get install -y \
    git \
    gcc-4.4 \
    g++-4.4 \
    automake \
    libtool \
    libgnome2-dev \
    libgnomeui-dev \
    libtheora-dev \
    mesa-common-dev \
    libgl1-mesa-dev \
    libglu1-mesa-dev \
    libgtkglext1-dev \
    libjpeg-dev \
    liblua5.1-0-dev

COPY . /Celestia
WORKDIR /Celestia
#RUN git checkout 6bbf7c0

RUN autoreconf -iv
ENV CXX=/usr/bin/g++-4.4
ENV CC=/usr/bin/gcc-4.4
RUN ./configure --with-gnome --enable-cairo --enable-theora --with-lua --disable-warnings
RUN make -j -l2
