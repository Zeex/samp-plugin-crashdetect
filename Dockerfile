# Usage:
#
# docker build . -t crashdetect
# docker run --rm --name crashdetect -v $PWD/../../gamemodes:/usr/local/samp03/gamemodes -v $PWD:/usr/local/samp03/plugins/crashdetect -it crashdetect
#
# Build:
#
# mkdir -p build/linux/docker-debug
# cd build/linux/docker-debug
# cmake ../../../ -DCMAKE_BUILD_TYPE=Debug
# make
# make test
#
# Run:
#
# samp-server-cli -o -g grandlarc -d ./crashdetect.so

FROM ubuntu:bionic

RUN apt-get update -q
RUN apt-get install -y wget vim
RUN apt-get install -y gcc g++ gcc-multilib g++-multilib make cmake gdb
RUN apt-get install -y python-pip

RUN wget http://files.sa-mp.com/samp037svr_R2-2-1.tar.gz
RUN tar xvzf samp037svr_R2-2-1.tar.gz -C /usr/local/
RUN chmod +x /usr/local/samp03/samp03svr
RUN mkdir /usr/local/samp03/plugins
RUN rm samp037svr_R2-2-1.tar.gz
ENV SAMP_SERVER_ROOT=/usr/local/samp03

RUN pip install samp-server-cli

RUN wget https://github.com/pawn-lang/compiler/releases/download/v3.10.9/pawnc-3.10.9-linux.tar.gz
RUN tar xzvf pawnc-3.10.9-linux.tar.gz -C /usr/local --strip-components 1
RUN rm pawnc-3.10.9-linux.tar.gz
RUN ldconfig
ENV PAWNCC_FLAGS="-i$SAMP_SERVER_ROOT/include -(+ -;+"

RUN wget https://github.com/Zeex/samp-plugin-runner/releases/download/v1.3/plugin-runner-1.3-linux.tar.gz
RUN mkdir /usr/local/plugin-runner
RUN tar xvf plugin-runner-1.3-linux.tar.gz \
    --strip-components 1 \
    -C /usr/local/plugin-runner \
    plugin-runner-1.3-linux/plugin-runner \
    plugin-runner-1.3-linux/include
RUN rm plugin-runner-1.3-linux.tar.gz
ENV PATH=$PATH:/usr/local/plugin-runner

WORKDIR $SAMP_SERVER_ROOT/plugins/crashdetect
