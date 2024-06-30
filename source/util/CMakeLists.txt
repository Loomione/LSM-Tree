file(GLOB SRC_FILES *.cpp *.cc)

add_library(util
            OBJECT
            ${SRC_FILES}
            )

set(ALL_OBJECT_FILES
        ${ALL_OBJECT_FILES} $<TARGET_OBJECTS:util>
        PARENT_SCOPE)