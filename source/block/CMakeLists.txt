file(GLOB SRC_FILES *.cpp *.cc)

add_library(block
            OBJECT
            ${SRC_FILES}
            )

set(ALL_OBJECT_FILES
        ${ALL_OBJECT_FILES} $<TARGET_OBJECTS:block>
        PARENT_SCOPE)