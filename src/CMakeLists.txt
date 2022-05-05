set( batchsystem_SRC
        batchSystem.cpp
        lsfBatch.cpp
        lsf.cpp
        pbsBatch.cpp
        pbs.cpp
        slurmBatch.cpp
        slurm.cpp
        batchJson.cpp
)

set( batchsystem_HDR
        batchSystem.h
        lsfBatch.h
        lsf.h
        pbsBatch.h
        pbs.h
        slurmBatch.h
        slurm.h
        batchJson.h
        batchDetail.h
        policyOptional.h
)

# Fixme: STATIC because for dynamic, we would need to handle visibility
# and I don't want to do the annotations right now
add_library(batchsystem STATIC
  ${batchsystem_SRC}
  ${batchsystem_HDR}
)


# depend on bunch of C++11 and C++14 features to have C++14 enabled by default
target_compile_features(Catch2
  PUBLIC
    cxx_alignas
    cxx_alignof
    cxx_attributes
    cxx_auto_type
    cxx_constexpr
    cxx_defaulted_functions
    cxx_deleted_functions
    cxx_final
    cxx_lambdas
    cxx_noexcept
    cxx_override
    cxx_range_for
    cxx_rvalue_references
    cxx_static_assert
    cxx_strong_enums
    cxx_trailing_return_types
    cxx_unicode_literals
    cxx_user_literals
    cxx_variable_templates
    cxx_variadic_macros
)

target_include_directories( batchsystem
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_include_directories( batchsystem
        SYSTEM PUBLIC	${LIBXMLPP_INCLUDE_DIRS}
)

target_link_libraries( batchsystem_dep
        ${LIBXMLPP_LIBRARIES}
)