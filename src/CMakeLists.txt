set( batchSystem_SRC
        batchSystem.cpp
        lsfBatch.cpp
        lsf.cpp
        pbsBatch.cpp
        pbs.cpp
        slurmBatch.cpp
        slurm.cpp
        batchJson.cpp
)

set( batchSystem_HDR
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

add_library( batchsystem_dep
        ${batchSystem_SRC}
        ${batchSystem_HDR}
)

target_include_directories( batchsystem_dep
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_include_directories( batchsystem_dep
        SYSTEM PUBLIC	${LIBXMLPP_INCLUDE_DIRS}
)

target_link_libraries( batchsystem_dep
        ${LIBXMLPP_LIBRARIES}
)