set(Eigen_HEADERS Core LU Cholesky QR Geometry
                  Sparse Array SVD LeastSquares
                  QtAlignedMalloc StdVector NewStdVector)

if(EIGEN_BUILD_LIB)
    set(Eigen_SRCS
      src/Core/CoreInstantiations.cpp
      src/Cholesky/CholeskyInstantiations.cpp
      src/QR/QrInstantiations.cpp
    )

    add_library(Eigen2 SHARED ${Eigen_SRCS})

    install(TARGETS Eigen2
            RUNTIME DESTINATION bin
            LIBRARY DESTINATION lib
            ARCHIVE DESTINATION lib)
endif(EIGEN_BUILD_LIB)

if(CMAKE_COMPILER_IS_GNUCXX)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g1 -O2")
    set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} -g1 -O2")
endif(CMAKE_COMPILER_IS_GNUCXX)

install(FILES
  ${Eigen_HEADERS}
  DESTINATION ${INCLUDE_INSTALL_DIR}/Eigen
  )

add_subdirectory(src)
