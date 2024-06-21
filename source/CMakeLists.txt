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

set(LSMTREE_LIBS
        util
        block
        lsm
        )

link_directories(crc32c /usr/local/lib)
set(LSMTREE_THIRDPARTY_LIBS
        fmt
        crc32c
)


target_link_libraries(lsm_tree  ${LSMTREE_LIBS}
        ${LSMTREE_THIRDPARTY_LIBS})