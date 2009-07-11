FILE(GLOB Eigen_Array_SRCS "*.h")

INSTALL(FILES
  ${Eigen_Array_SRCS}
  DESTINATION ${INCLUDE_INSTALL_DIR}/Eigen/src/Array
  )
