# FindANTLR4.cmake - Find ANTLR4 C++ runtime and jar

# Find ANTLR4 C++ runtime library
find_path(ANTLR4_INCLUDE_DIR 
    NAMES antlr4-runtime/antlr4-runtime.h
    PATHS 
        /opt/homebrew/include
        /usr/local/include
        /usr/include
)

find_library(ANTLR4_LIBRARY 
    NAMES antlr4-runtime
    PATHS 
        /opt/homebrew/lib
        /usr/local/lib
        /usr/lib
)

# Find ANTLR4 jar for grammar compilation
find_file(ANTLR4_JAR
    NAMES antlr-4.13.2-complete.jar antlr-4.13.1-complete.jar antlr-4.13.0-complete.jar antlr-complete.jar
    PATHS
        /opt/homebrew/Cellar/antlr/*/
        /usr/local/share/java
        /usr/share/java
    PATH_SUFFIXES antlr
)

# Find Java for running ANTLR
find_program(JAVA_EXECUTABLE
    NAMES java
    PATHS
        /opt/homebrew/Cellar/openjdk/*/bin
        /usr/local/bin
        /usr/bin
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(ANTLR4
    REQUIRED_VARS ANTLR4_INCLUDE_DIR ANTLR4_LIBRARY ANTLR4_JAR JAVA_EXECUTABLE
)

if(ANTLR4_FOUND)
    set(ANTLR4_INCLUDE_DIRS ${ANTLR4_INCLUDE_DIR} ${ANTLR4_INCLUDE_DIR}/antlr4-runtime)
    set(ANTLR4_LIBRARIES ${ANTLR4_LIBRARY})
    
    # Function to compile ANTLR4 grammars
    function(antlr4_generate)
        set(options VISITOR LISTENER)
        set(oneValueArgs OUTPUT_DIR NAMESPACE)
        set(multiValueArgs GRAMMARS DEPENDS)
        cmake_parse_arguments(ANTLR4 "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})
        
        set(ANTLR4_FLAGS -Dlanguage=Cpp)
        if(ANTLR4_VISITOR)
            list(APPEND ANTLR4_FLAGS -visitor)
        endif()
        if(ANTLR4_LISTENER)
            list(APPEND ANTLR4_FLAGS -listener)
        endif()
        if(ANTLR4_NAMESPACE)
            list(APPEND ANTLR4_FLAGS -package ${ANTLR4_NAMESPACE})
        endif()
        
        foreach(GRAMMAR ${ANTLR4_GRAMMARS})
            get_filename_component(GRAMMAR_NAME ${GRAMMAR} NAME_WE)
            get_filename_component(GRAMMAR_DIR ${GRAMMAR} DIRECTORY)
            
            # Output files
            set(GENERATED_FILES
                ${ANTLR4_OUTPUT_DIR}/${GRAMMAR_NAME}.cpp
                ${ANTLR4_OUTPUT_DIR}/${GRAMMAR_NAME}.h
            )
            
            # Add generation command
            add_custom_command(
                OUTPUT ${GENERATED_FILES}
                COMMAND ${CMAKE_COMMAND} -E make_directory ${ANTLR4_OUTPUT_DIR}
                COMMAND ${JAVA_EXECUTABLE} -jar ${ANTLR4_JAR} 
                    ${ANTLR4_FLAGS}
                    -o ${ANTLR4_OUTPUT_DIR}
                    -lib ${GRAMMAR_DIR}
                    ${GRAMMAR}
                DEPENDS ${GRAMMAR} ${ANTLR4_DEPENDS}
                COMMENT "Generating ANTLR4 parser for ${GRAMMAR_NAME}"
                VERBATIM
            )
            
            # Add to parent scope
            set(ANTLR4_${GRAMMAR_NAME}_SOURCES ${GENERATED_FILES} PARENT_SCOPE)
        endforeach()
    endfunction()
    
    mark_as_advanced(ANTLR4_INCLUDE_DIR ANTLR4_LIBRARY ANTLR4_JAR JAVA_EXECUTABLE)
endif()
