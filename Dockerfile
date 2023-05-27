FROM ubuntu:22.04
RUN echo 'APT::Install-Suggests "0";' >> /etc/apt/apt.conf.d/00-docker
RUN echo 'APT::Install-Recommends "0";' >> /etc/apt/apt.conf.d/00-docker
RUN DEBIAN_FRONTEND=noninteractive \
  apt-get update \
  && apt-get install -y \
  python3 \
  python3-pip \
  clang-15 \
  cmake \
  build-essential \
  wget \
  git \
  software-properties-common
RUN bash -c "$(wget -O - https://apt.llvm.org/llvm.sh)"
RUN DEBIAN_FRONTEND=noninteractive \
  apt-get install -y \ 
  libc++-15-dev \ 
  libc++abi-15-dev \
  && rm -rf /var/lib/apt/lists/*
RUN pip3 install conan

WORKDIR /app/
RUN git clone https://github.com/rekrutik/mai-arch.git

WORKDIR /app/mai-arch

# COPY ./conan_profile /root/.conan2/profiles/default
# COPY ./conanfile.txt .
RUN mkdir -p /root/.conan2/profiles && cp ./conan_profile /root/.conan2/profiles/default
RUN update-alternatives --install /usr/bin/cc cc /usr/bin/clang-15 60
RUN update-alternatives --install /usr/bin/c++ c++ /usr/bin/clang++-15 60
RUN conan install . --output-folder=build --build=missing
# COPY ./ ./
WORKDIR /app/mai-arch/build
RUN cmake .. -DCMAKE_TOOLCHAIN_FILE=conan_toolchain.cmake -DCMAKE_BUILD_TYPE=Release
RUN cmake --build .

# CMD ["./auth/auth_service"]
CMD [ "sh" ]
