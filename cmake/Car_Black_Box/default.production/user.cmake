# =============================================================================
# user.cmake — XC8 Optimisation Override for Car Black Box (PIC16F877A)
# =============================================================================
# Place this file in BOTH of the following directories:
#   Car_Black_Box/cmake/Car_Black_Box/default.production/
#   Car_Black_Box/cmake/Car_Black_Box/default/          (if it exists)
#
# The auto-generated CMakeLists.txt includes this file as an extension point.
# It is safe to edit — it will NOT be overwritten by MPLAB X.
#
# Problem solved:
#   The default XC8 build uses -O0 (no optimisation). On PIC16F877A the
#   program memory (ROM) is only 8 KB words. At -O0 the compiler keeps every
#   stack frame, spills all temporaries to RAM, and duplicates inline helpers —
#   exhausting ROM. -Os ("optimise for size") typically saves 20-35 % of
#   program words on XC8 with no change in observable behaviour.
#
# If -Os causes a miscompilation (unlikely with XC8 ≥ 2.x), switch to -O1
# by replacing "-Os" with "-O1" below.
# =============================================================================

# ── Step 1: Strip every occurrence of -O0 from all CMake flag variables ──────
foreach(_var
        CMAKE_C_FLAGS
        CMAKE_C_FLAGS_DEBUG
        CMAKE_C_FLAGS_RELEASE
        CMAKE_C_FLAGS_MINSIZEREL
        CMAKE_C_FLAGS_RELWITHDEBINFO)

    if(DEFINED ${_var})
        # Replace -O0 — run twice to catch accidental duplicates
        string(REPLACE "-O0" "" ${_var} "${${_var}}")
        string(REPLACE "-O0" "" ${_var} "${${_var}}")
        # Strip any double-spaces left behind
        string(REGEX REPLACE "  +" " " ${_var} "${${_var}}")
        string(STRIP "${${_var}}" ${_var})
    endif()
endforeach()

# ── Step 2: Append -Os to the base C flags ───────────────────────────────────
# -Os = optimise for size. For PIC16 this is almost always the right choice:
#   * Removes dead code aggressively (helps with unused functions)
#   * Folds identical constants into shared ROM entries
#   * Eliminates redundant loads/stores
#   * Does NOT enable loop-unrolling or inlining that would INCREASE code size
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Os")

# ── Step 3: Diagnostic output (visible in MPLAB X Output window) ─────────────
message(STATUS "")
message(STATUS "=== [user.cmake] XC8 optimisation override applied ===")
message(STATUS "  -O0 removed from all flag variables")
message(STATUS "  -Os appended to CMAKE_C_FLAGS")
message(STATUS "  Final CMAKE_C_FLAGS = ${CMAKE_C_FLAGS}")
message(STATUS "=======================================================")
message(STATUS "")

# ── Alternative: MPLAB Configurations XML path ───────────────────────────────
# If your project does NOT use CMake (pure MPLAB X project), set the XC8
# optimisation level via:
#   Project Properties → XC8 Global Options → XC8 Compiler
#   → Optimizations → Optimization Set → "1" or "s" (Size)
# Changing that field writes to nbproject/configurations.xml under:
#   <toolsSet><compileType><XC8-config-global><optimization-level>
# =============================================================================
