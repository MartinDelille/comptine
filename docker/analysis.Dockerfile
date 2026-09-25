FROM ubuntu:24.04

ARG QT_VERSION

ENV DEBIAN_FRONTEND=noninteractive
ENV QT_ROOT=/opt/Qt/${QT_VERSION}/gcc_64
ENV PATH=/opt/Qt/${QT_VERSION}/gcc_64/bin:/opt/clazy/bin:/usr/lib/llvm-22/bin:${PATH}

RUN apt-get update \
    && apt-get install -y --no-install-recommends \
        ca-certificates \
        build-essential \
        cmake \
        git \
        ninja-build \
        python3 \
        python3-pip \
        python3-venv \
        wget \
    && rm -rf /var/lib/apt/lists/*

RUN wget -qO- https://apt.llvm.org/llvm.sh | bash -s -- 22 \
    && apt-get update \
    && apt-get install -y --no-install-recommends \
        clang-22 \
        clang-tidy-22 \
        clang-tools-22 \
        libclang-cpp22-dev \
        libclang-22-dev \
        llvm-22 \
        llvm-22-dev \
    && rm -rf /var/lib/apt/lists/*

RUN python3 -m pip install --break-system-packages --no-cache-dir \
        aqtinstall \
        conan

RUN test -n "${QT_VERSION}" \
    && aqt install-qt linux desktop "${QT_VERSION}" gcc_64 \
        --outputdir /opt/Qt \
        --archives qtbase qtdeclarative qttools

RUN git clone --depth 1 --branch v1.17.1 \
        https://github.com/KDE/clazy.git /tmp/clazy-src \
    && cmake -S /tmp/clazy-src -B /tmp/clazy-build \
        -G Ninja \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_INSTALL_PREFIX=/opt/clazy \
        -DCMAKE_PREFIX_PATH=/usr/lib/llvm-22 \
        -DCLAZY_LINK_CLANG_DYLIB=ON \
        -DLLVM_ROOT=/usr/lib/llvm-22 \
    && cmake --build /tmp/clazy-build \
    && cmake --install /tmp/clazy-build \
    && rm -rf /tmp/clazy-src /tmp/clazy-build

WORKDIR /src
