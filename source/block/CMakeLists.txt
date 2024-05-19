add_library(block
            OBJECT
            block.cpp
            filter_block.cpp
            footer_block.cpp
            )

set(ALL_OBJECT_FILES
        ${ALL_OBJECT_FILES} $<TARGET_OBJECTS:block>
        PARENT_SCOPE)