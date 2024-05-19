add_subdirectory(util)

add_library(lsm
            OBJECT
            block.cpp
            keys.cpp
            footer_block.cpp
            memtable.cpp
            wal.cpp
            filter_block.cpp
            )
set(ALL_OBJECT_FILES
        ${ALL_OBJECT_FILES} $<TARGET_OBJECTS:lsm>
        PARENT_SCOPE)
add_library(lsm_tree STATIC ${ALL_OBJECT_FILES})