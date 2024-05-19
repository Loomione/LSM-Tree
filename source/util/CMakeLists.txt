add_library(util
            OBJECT
            encode.cpp
            file_util.cpp
            hash_util.cpp
            monitor_logger.cpp
            murmur3_hash.cpp
            )

set(ALL_OBJECT_FILES
        ${ALL_OBJECT_FILES} $<TARGET_OBJECTS:util>
        PARENT_SCOPE)