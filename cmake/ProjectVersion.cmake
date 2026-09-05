function(comptine_set_version)
    set(default_version "0.0")
    set(app_version "${default_version}")
    set(app_version_suffix "")
    set(app_commit_hash "")

    find_package(Git QUIET)
    if(GIT_FOUND)
        execute_process(
            COMMAND ${GIT_EXECUTABLE} describe --tags --abbrev=0
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
            OUTPUT_VARIABLE git_tag
            OUTPUT_STRIP_TRAILING_WHITESPACE
            ERROR_QUIET
            RESULT_VARIABLE git_tag_result
        )

        execute_process(
            COMMAND ${GIT_EXECUTABLE} rev-parse --short HEAD
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
            OUTPUT_VARIABLE app_commit_hash
            OUTPUT_STRIP_TRAILING_WHITESPACE
            ERROR_QUIET
        )

        if(git_tag_result EQUAL 0 AND git_tag)
            set(app_version "${git_tag}")
            execute_process(
                COMMAND ${GIT_EXECUTABLE} rev-list ${git_tag}..HEAD --count
                WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
                OUTPUT_VARIABLE commits_since_tag
                OUTPUT_STRIP_TRAILING_WHITESPACE
                ERROR_QUIET
            )
        else()
            execute_process(
                COMMAND ${GIT_EXECUTABLE} rev-list HEAD --count
                WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
                OUTPUT_VARIABLE commits_since_tag
                OUTPUT_STRIP_TRAILING_WHITESPACE
                ERROR_QUIET
            )
        endif()

        if(NOT git_tag OR commits_since_tag GREATER 0)
            set(app_version_suffix
                "-dev-${commits_since_tag}-${app_commit_hash}"
            )
        endif()
    else()
        set(app_version_suffix "-dev")
    endif()

    set(app_version_full "${app_version}${app_version_suffix}")
    message(STATUS "Comptine version: ${app_version_full}")

    string(REGEX MATCH "^([0-9]+)\\.([0-9]+)" version_match "${app_version}")
    if(version_match)
        set(version_major "${CMAKE_MATCH_1}")
        set(version_minor "${CMAKE_MATCH_2}")
    else()
        set(version_major "0")
        set(version_minor "0")
    endif()

    set(APP_VERSION "${app_version}" PARENT_SCOPE)
    set(APP_VERSION_FULL "${app_version_full}" PARENT_SCOPE)
    set(APP_COMMIT_HASH "${app_commit_hash}" PARENT_SCOPE)
    set(VERSION_MAJOR "${version_major}" PARENT_SCOPE)
    set(VERSION_MINOR "${version_minor}" PARENT_SCOPE)
endfunction()
