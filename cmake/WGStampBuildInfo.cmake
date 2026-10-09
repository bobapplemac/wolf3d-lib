# Runs during staging so commits reflect the sources that were just built.
foreach(entry "${ROOT}" "${ENGINE}" "${ROOT}/third_party/SDL3" "${ENGINE}/third_party/Nuked-OPL3" "${ENGINE}/third_party/DBOPL")
    if(EXISTS "${entry}")
        execute_process(COMMAND git -C "${entry}" rev-parse HEAD
            OUTPUT_VARIABLE commit RESULT_VARIABLE result
            OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET)
        if(NOT result EQUAL 0)
            set(commit unknown)
        endif()
        execute_process(COMMAND git -C "${entry}" status --porcelain --untracked-files=no
            OUTPUT_VARIABLE changes ERROR_QUIET)
        if(changes)
            set(commit "${commit} (modified tracked sources)")
        endif()
        file(APPEND "${INFO}" "Source ${entry}: ${commit}\n")
    endif()
endforeach()

# Preserve the complete configured cache, including new options added later.
# Do not dump the process environment: it may contain unrelated credentials.
foreach(snapshot "${CACHE}" "${SETTINGS}")
    if(EXISTS "${snapshot}")
        file(READ "${snapshot}" contents)
        file(APPEND "${INFO}" "\n===== ${snapshot} =====\n${contents}\n")
    endif()
endforeach()
