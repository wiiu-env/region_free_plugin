FROM wiiuenv/devkitppc:20221228

COPY --from=wiiuenv/wiiupluginsystem:20220924 /artifacts $DEVKITPRO
COPY --from=wiiuenv/libmappedmemory:20220904 /artifacts $DEVKITPRO

WORKDIR project
