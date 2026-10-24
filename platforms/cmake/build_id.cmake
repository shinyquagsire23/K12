# Writes build_id.h (K2P_BUILD_ID: the commit, "-dirty" with uncommitted src/ or platforms/ edits),
# rewriting it only when the id changes. Run every build: cmake -DOUT=<header> -DREPO=<repo> -P this
execute_process(COMMAND git rev-parse --short HEAD WORKING_DIRECTORY "${REPO}"
                OUTPUT_VARIABLE id OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET)
execute_process(COMMAND git diff --quiet HEAD -- src platforms WORKING_DIRECTORY "${REPO}"
                RESULT_VARIABLE dirty ERROR_QUIET)
if(NOT id)
  set(id "unknown")
elseif(dirty)
  set(id "${id}-dirty")
endif()
file(WRITE "${OUT}.tmp" "#define K2P_BUILD_ID \"${id}\"\n")
file(COPY_FILE "${OUT}.tmp" "${OUT}" ONLY_IF_DIFFERENT)
file(REMOVE "${OUT}.tmp")
