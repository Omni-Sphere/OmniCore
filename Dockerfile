# ==================================================
# Dockerfile: OmniSphere SDK Base Builder (Arch Linux)
# ==================================================
FROM archlinux:latest

# 1. Instalación de Dependencias Mínimas del Sistema
RUN pacman -Syu --noconfirm && \
    pacman -S --noconfirm \
        gcc \
        make \
        pkgconf \
        cmake \
        boost \
        boost-libs \
        libsodium \
        unixodbc \
        postgresql-libs \
        libpqxx \
        mariadb-libs \
        openssl && \
    pacman -Scc --noconfirm

# 2. Copiar contexto local de fuentes ($TARGET_DIR que contiene todos los repos de OmniSphere)
COPY . /tmp/src/

# 3. Compilación e Instalación Autónoma de cppgraphqlgen y la suite OmniSDK
RUN set -e && \
    # A. Compilar e instalar cppgraphqlgen (schemagen) desde fuentes locales
    rm -rf /tmp/src/cppgraphqlgen/build /tmp/cppgraphqlgen-build 2>/dev/null || true && \
    cmake -B /tmp/cppgraphqlgen-build -S /tmp/src/cppgraphqlgen \
        -Wno-dev \
        -Wno-unused-cli \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_INSTALL_PREFIX=/usr \
        -DCMAKE_POSITION_INDEPENDENT_CODE=ON \
        -DCMAKE_CXX_FLAGS="-Wno-maybe-uninitialized" \
        -DGRAPHQL_BUILD_TESTS=OFF \
        -DGRAPHQL_UPDATE_SAMPLES=OFF \
        -DGRAPHQL_USE_RAPIDJSON=OFF \
        -DGRAPHQL_BUILD_SCHEMAGEN=ON && \
    cmake --build /tmp/cppgraphqlgen-build -j$(nproc) && \
    cmake --install /tmp/cppgraphqlgen-build && \
    cp /tmp/cppgraphqlgen-build/src/schemagen /usr/bin/ 2>/dev/null || true && \
    cp /usr/bin/cppgraphqlgen/schemagen /usr/bin/ 2>/dev/null || true && \
    rm -rf /tmp/src/cppgraphqlgen/build /tmp/cppgraphqlgen-build 2>/dev/null || true && \
    \
    # B. Compilar e instalar las 6 librerías de OmniSphere en secuencia
    for LIB in OmniUtils OmniData OmniCore OmniERP OmniRoute OmniGraph; do \
        rm -rf /tmp/src/${LIB}/build /tmp/${LIB}-build 2>/dev/null || true && \
        EXTRA_FLAGS="" && \
        if [ "$LIB" = "OmniGraph" ]; then rm -rf /tmp/src/OmniGraph/GraphQL/Generated/* 2>/dev/null || true && EXTRA_FLAGS="-DOMNIGRAPH_ENABLE_ERP=ON -DOMNIGRAPH_ENABLE_ROUTE=ON"; fi && \
        cmake -B /tmp/${LIB}-build -S /tmp/src/${LIB} \
            -Wno-dev \
            -Wno-unused-cli \
            -DCMAKE_BUILD_TYPE=Release \
            -DCMAKE_INSTALL_PREFIX=/usr \
            -DCMAKE_CXX_FLAGS="-fstack-protector-strong -D_FORTIFY_SOURCE=2 -fPIE" \
            -DCMAKE_EXE_LINKER_FLAGS="-pie -Wl,-z,relro,-z,now" \
            -DFETCHCONTENT_SOURCE_DIR_OMNIUTILS=/tmp/src/OmniUtils \
            -DFETCHCONTENT_SOURCE_DIR_OMNIDATA=/tmp/src/OmniData \
            -DFETCHCONTENT_SOURCE_DIR_OMNICORE=/tmp/src/OmniCore \
            -DFETCHCONTENT_SOURCE_DIR_OMNIERP=/tmp/src/OmniERP \
            -DFETCHCONTENT_SOURCE_DIR_OMNIROUTE=/tmp/src/OmniRoute \
            -DFETCHCONTENT_SOURCE_DIR_OMNIGRAPH=/tmp/src/OmniGraph \
            $EXTRA_FLAGS && \
        cmake --build /tmp/${LIB}-build -j$(nproc) && \
        cmake --install /tmp/${LIB}-build && \
        rm -rf /tmp/src/${LIB}/build /tmp/${LIB}-build 2>/dev/null ; \
    done && \
    \
    # C. Limpieza de carpetas temporales
    rm -rf /tmp/src && \
    pacman -Scc --noconfirm && \
    rm -rf /tmp/* /var/tmp/* && \
    useradd -u 10001 -m -s /bin/bash appuser && \
    mkdir -p /app && chown -R 10001:10001 /app

# Configuración de Entorno
ENV LD_LIBRARY_PATH=/usr/lib/OmniSphere:/usr/lib:$LD_LIBRARY_PATH
ENV CMAKE_PREFIX_PATH=/usr/lib/OmniSphere/cmake:$CMAKE_PREFIX_PATH

USER 10001
WORKDIR /app
CMD ["/bin/bash"]
