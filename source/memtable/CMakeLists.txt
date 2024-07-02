file(GLOB SRC_FILES *.cpp *.cc)

add_library(memtable
            OBJECT
            ${SRC_FILES}
            )

set(ALL_OBJECT_FILES
        ${ALL_OBJECT_FILES} $<TARGET_OBJECTS:memtable>
        PARENT_SCOPE)