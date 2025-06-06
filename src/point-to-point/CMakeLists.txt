set(mpi_sources)
set(mpi_headers)
set(mpi_libraries)

if(${ENABLE_MPI})
  set(mpi_sources
      model/point-to-point-remote-channel.cc
  )
  set(mpi_headers
      model/point-to-point-remote-channel.h
  )
  set(mpi_libraries
      ${libmpi}
      MPI::MPI_CXX
  )
endif()

build_lib(
  LIBNAME point-to-point
  SOURCE_FILES
    ${mpi_sources}
    helper/point-to-point-helper.cc
    helper/qbb-helper.cc
    model/point-to-point-channel.cc
    model/point-to-point-net-device.cc
    model/ppp-header.cc
    model/qbb-net-device.cc
    model/qbb-channel.cc
    model/rdma-queue-pair.cc
    model/pause-header.cc
    model/qbb-header.cc
    model/cn-header.cc
    model/pint.cc
    model/rdma-driver.cc
    model/rdma-hw.cc
    model/switch-mmu.cc
    model/switch-node.cc
    model/cncp-control-header.cc
  HEADER_FILES
    ${mpi_headers}
    helper/point-to-point-helper.h
    helper/qbb-helper.h
    helper/sim-setting.h
    model/point-to-point-channel.h
    model/point-to-point-net-device.h
    model/ppp-header.h
    model/qbb-net-device.h
    model/qbb-channel.h
    model/rdma-queue-pair.h
    model/pause-header.h
    model/qbb-header.h
    model/cn-header.h
    model/pint.h
    model/rdma-driver.h
    model/rdma-hw.h
    model/switch-mmu.h
    model/switch-node.h
    model/cncp-control-header.h
  LIBRARIES_TO_LINK ${libnetwork}
                    ${libinternet}
                    ${mpi_libraries}
  TEST_SOURCES test/point-to-point-test.cc
)
