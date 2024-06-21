add_subdirectory(util)
add_subdirectory(block)
add_library(lsm
            OBJECT
            keys.cpp
            memtable.cpp
            wal.cpp
            worker.cpp
            )
set(ALL_OBJECT_FILES
        ${ALL_OBJECT_FILES} $<TARGET_OBJECTS:lsm>
        PARENT_SCOPE)
add_library(lsm_tree STATIC ${ALL_OBJECT_FILES})