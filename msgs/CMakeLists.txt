find_package(Protobuf REQUIRED)

set(PROTOBUF_IMPORT_DIRS ${GAZEBO_PROTO_PATH})
set (msgs
  collision_map_request.proto
)
PROTOBUF_GENERATE_CPP(PROTO_SRCS PROTO_HDRS ${msgs})
add_library(collision_map_creator_msgs SHARED ${PROTO_SRCS})
target_link_libraries(collision_map_creator_msgs ${PROTOBUF_LIBRARY})
