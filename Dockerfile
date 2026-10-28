# Toolchain image: devkitPro's devkitA64 (GCC for the Switch's Cortex-A57), libnx and Mesa for the
# Switch, pinned by digest so builds stay reproducible. Adds what Godot's build needs.
FROM devkitpro/devkita64@sha256:1fc388c3a0d34bd2045a6dadcb1020e069d5f876a187fd705de14b4440c00282
RUN apt-get update \
    && apt-get install -y --no-install-recommends scons pkg-config python3 imagemagick \
    && rm -rf /var/lib/apt/lists/*
ENV DEVKITPRO=/opt/devkitpro
