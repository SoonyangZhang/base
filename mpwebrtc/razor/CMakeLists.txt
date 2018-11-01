add_definitions(-D__linux__)
set(razor_FILES
cf_stream.c
cf_crc32.c
posix.c
sim_proto.c
)
add_library(razor STATIC ${razor_FILES})

