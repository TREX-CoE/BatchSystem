set( batchsystem_SRC
        batchsystem/batchsystem.cpp
        batchsystem/lsfBatch.cpp
        batchsystem/lsf.cpp
        batchsystem/pbsBatch.cpp
        batchsystem/pbs.cpp
        batchsystem/slurmBatch.cpp
        batchsystem/slurm.cpp
        batchsystem/factory.cpp
        batchsystem/json.cpp
)

set( batchsystem_HDR
        batchsystem/internal/byteParse.h
        batchsystem/internal/runCommand.h
        batchsystem/internal/splitString.h
        batchsystem/internal/streamCast.h
        batchsystem/internal/joinString.h
        batchsystem/internal/timeToEpoch.h
        batchsystem/batchsystem.h
        batchsystem/lsfBatch.h
        batchsystem/lsf.h
        batchsystem/pbsBatch.h
        batchsystem/pbs.h
        batchsystem/slurmBatch.h
        batchsystem/slurm.h
        batchsystem/json.h
        batchsystem/factory.h
        batchsystem/policyOptional.h
)

# Fixme: STATIC because for dynamic, we would need to handle visibility
# and I don't want to do the annotations right now
add_library(batchsystem STATIC
  ${batchsystem_SRC}
  ${batchsystem_HDR}
)

target_compile_features(batchsystem PUBLIC cxx_std_14)

target_include_directories( batchsystem
        SYSTEM PUBLIC	${LIBXMLPP_INCLUDE_DIRS}
)

target_link_libraries( batchsystem
        ${LIBXMLPP_LIBRARIES}
)

target_include_directories( batchsystem
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)
