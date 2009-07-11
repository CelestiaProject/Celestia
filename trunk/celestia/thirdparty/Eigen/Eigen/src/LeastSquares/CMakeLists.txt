FILE(GLOB Eigen_LeastSquares_SRCS "*.h")

INSTALL(FILES 
  ${Eigen_LeastSquares_SRCS}
  DESTINATION ${INCLUDE_INSTALL_DIR}/Eigen/src/LeastSquares
  )
