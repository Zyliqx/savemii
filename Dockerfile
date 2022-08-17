FROM wiiuenv/devkitppc:20220806 AS final

COPY --from=wiiuenv/libiosuhax:latest /artifacts $DEVKITPRO

CMD dkp-pacman -Syyu --noconfirm ppc-freetype

RUN git clone --recursive https://github.com/Crementif/libfat && \
  cd libfat && \
  make -j$(nproc) wiiu-install && \
  cd .. && \
  rm -rf libfat

WORKDIR /project
