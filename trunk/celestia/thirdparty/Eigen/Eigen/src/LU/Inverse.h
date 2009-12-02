// This file is part of Eigen, a lightweight C++ template library
// for linear algebra. Eigen itself is part of the KDE project.
//
// Copyright (C) 2008 Benoit Jacob <jacob.benoit.1@gmail.com>
//
// Eigen is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 3 of the License, or (at your option) any later version.
//
// Alternatively, you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 2 of
// the License, or (at your option) any later version.
//
// Eigen is distributed in the hope that it will be useful, but WITHOUT ANY
// WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
// FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License or the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License and a copy of the GNU General Public License along with
// Eigen. If not, see <http://www.gnu.org/licenses/>.

#ifndef EIGEN_INVERSE_H
#define EIGEN_INVERSE_H

/********************************************************************
*** Part 1 : optimized implementations for fixed-size 2,3,4 cases ***
********************************************************************/

template<typename MatrixType>
void ei_compute_inverse_in_size2_case(const MatrixType& matrix, MatrixType* result)
{
  typedef typename MatrixType::Scalar Scalar;
  const Scalar invdet = Scalar(1) / matrix.determinant();
  result->coeffRef(0,0) = matrix.coeff(1,1) * invdet;
  result->coeffRef(1,0) = -matrix.coeff(1,0) * invdet;
  result->coeffRef(0,1) = -matrix.coeff(0,1) * invdet;
  result->coeffRef(1,1) = matrix.coeff(0,0) * invdet;
}

template<typename XprType, typename MatrixType>
bool ei_compute_inverse_in_size2_case_with_check(const XprType& matrix, MatrixType* result)
{
  typedef typename MatrixType::Scalar Scalar;
  const Scalar det = matrix.determinant();
  if(ei_isMuchSmallerThan(det, matrix.cwise().abs().maxCoeff())) return false;
  const Scalar invdet = Scalar(1) / det;
  result->coeffRef(0,0) = matrix.coeff(1,1) * invdet;
  result->coeffRef(1,0) = -matrix.coeff(1,0) * invdet;
  result->coeffRef(0,1) = -matrix.coeff(0,1) * invdet;
  result->coeffRef(1,1) = matrix.coeff(0,0) * invdet;
  return true;
}

template<typename MatrixType>
void ei_compute_inverse_in_size3_case(const MatrixType& matrix, MatrixType* result)
{
  typedef typename MatrixType::Scalar Scalar;
  const Scalar det_minor00 = matrix.minor(0,0).determinant();
  const Scalar det_minor10 = matrix.minor(1,0).determinant();
  const Scalar det_minor20 = matrix.minor(2,0).determinant();
  const Scalar invdet = Scalar(1) / ( det_minor00 * matrix.coeff(0,0)
                                    - det_minor10 * matrix.coeff(1,0)
                                    + det_minor20 * matrix.coeff(2,0) );
  result->coeffRef(0, 0) = det_minor00 * invdet;
  result->coeffRef(0, 1) = -det_minor10 * invdet;
  result->coeffRef(0, 2) = det_minor20 * invdet;
  result->coeffRef(1, 0) = -matrix.minor(0,1).determinant() * invdet;
  result->coeffRef(1, 1) = matrix.minor(1,1).determinant() * invdet;
  result->coeffRef(1, 2) = -matrix.minor(2,1).determinant() * invdet;
  result->coeffRef(2, 0) = matrix.minor(0,2).determinant() * invdet;
  result->coeffRef(2, 1) = -matrix.minor(1,2).determinant() * invdet;
  result->coeffRef(2, 2) = matrix.minor(2,2).determinant() * invdet;
}

template<typename MatrixType>
bool ei_compute_inverse_in_size4_case_helper(const MatrixType& matrix, MatrixType* result)
{
  /* Let's split M into four 2x2 blocks:
    * (P Q)
    * (R S)
    * If P is invertible, with inverse denoted by P_inverse, and if
    * (S - R*P_inverse*Q) is also invertible, then the inverse of M is
    * (P' Q')
    * (R' S')
    * where
    * S' = (S - R*P_inverse*Q)^(-1)
    * P' = P1 + (P1*Q) * S' *(R*P_inverse)
    * Q' = -(P_inverse*Q) * S'
    * R' = -S' * (R*P_inverse)
    */
  typedef Block<MatrixType,2,2> XprBlock22;
  typedef typename MatrixBase<XprBlock22>::PlainMatrixType Block22;
  Block22 P_inverse;
  if(ei_compute_inverse_in_size2_case_with_check(matrix.template block<2,2>(0,0), &P_inverse))
  {
    const Block22 Q = matrix.template block<2,2>(0,2);
    const Block22 P_inverse_times_Q = P_inverse * Q;
    const XprBlock22 R = matrix.template block<2,2>(2,0);
    const Block22 R_times_P_inverse = R * P_inverse;
    const Block22 R_times_P_inverse_times_Q = R_times_P_inverse * Q;
    const XprBlock22 S = matrix.template block<2,2>(2,2);
    const Block22 X = S - R_times_P_inverse_times_Q;
    Block22 Y;
    ei_compute_inverse_in_size2_case(X, &Y);
    result->template block<2,2>(2,2) = Y;
    result->template block<2,2>(2,0) = - Y * R_times_P_inverse;
    const Block22 Z = P_inverse_times_Q * Y;
    result->template block<2,2>(0,2) = - Z;
    result->template block<2,2>(0,0) = P_inverse + Z * R_times_P_inverse;
    return true;
  }
  else
  {
    return false;
  }
}

template<typename MatrixType>
void ei_compute_inverse_in_size4_case(const MatrixType& _matrix, MatrixType* result)
{
  typedef typename MatrixType::Scalar Scalar;
  typedef typename MatrixType::RealScalar RealScalar;

  // we will do row permutations on the matrix. This copy should have negligible cost.
  // if not, consider working in-place on the matrix (const-cast it, but then undo the permutations
  // to nevertheless honor constness)
  typename MatrixType::PlainMatrixType matrix(_matrix);

  // let's extract from the 2 first colums a 2x2 block whose determinant is as big as possible.
  int good_row0, good_row1, good_i;
  Matrix<RealScalar,6,1> absdet;

  // any 2x2 block with determinant above this threshold will be considered good enough.
  // The magic value 1e-1 here comes from experimentation. The bigger it is, the higher the precision,
  // the slower the computation. This value 1e-1 gives precision almost as good as the brutal cofactors
  // algorithm, both in average and in worst-case precision.
  RealScalar d = (matrix.col(0).squaredNorm()+matrix.col(1).squaredNorm()) * RealScalar(1e-1);
  #define ei_inv_size4_helper_macro(i,row0,row1) \
  absdet[i] = ei_abs(matrix.coeff(row0,0)*matrix.coeff(row1,1) \
                                - matrix.coeff(row0,1)*matrix.coeff(row1,0)); \
  if(absdet[i] > d) { good_row0=row0; good_row1=row1; goto good; }
  ei_inv_size4_helper_macro(0,0,1)
  ei_inv_size4_helper_macro(1,0,2)
  ei_inv_size4_helper_macro(2,0,3)
  ei_inv_size4_helper_macro(3,1,2)
  ei_inv_size4_helper_macro(4,1,3)
  ei_inv_size4_helper_macro(5,2,3)

  // no 2x2 block has determinant bigger than the threshold. So just take the one that
  // has the biggest determinant
  absdet.maxCoeff(&good_i);
  good_row0 = good_i <= 2 ? 0 : good_i <= 4 ? 1 : 2;
  good_row1 = good_i <= 2 ? good_i+1 : good_i <= 4 ? good_i-1 : 3;

  // now good_row0 and good_row1 are correctly set
  good:

  // do row permutations to move this 2x2 block to the top
  matrix.row(0).swap(matrix.row(good_row0));
  matrix.row(1).swap(matrix.row(good_row1));
  // now applying our helper function is numerically stable
  ei_compute_inverse_in_size4_case_helper(matrix, result);
  // Since we did row permutations on the original matrix, we need to do column permutations
  // in the reverse order on the inverse
  result->col(1).swap(result->col(good_row1));
  result->col(0).swap(result->col(good_row0));
}

/***********************************************
*** Part 2 : selector and MatrixBase methods ***
***********************************************/

template<typename MatrixType, int Size = MatrixType::RowsAtCompileTime>
struct ei_compute_inverse
{
  static inline void run(const MatrixType& matrix, MatrixType* result)
  {
    LU<MatrixType> lu(matrix);
    lu.computeInverse(result);
  }
};

template<typename MatrixType>
struct ei_compute_inverse<MatrixType, 1>
{
  static inline void run(const MatrixType& matrix, MatrixType* result)
  {
    typedef typename MatrixType::Scalar Scalar;
    result->coeffRef(0,0) = Scalar(1) / matrix.coeff(0,0);
  }
};

template<typename MatrixType>
struct ei_compute_inverse<MatrixType, 2>
{
  static inline void run(const MatrixType& matrix, MatrixType* result)
  {
    ei_compute_inverse_in_size2_case(matrix, result);
  }
};

template<typename MatrixType>
struct ei_compute_inverse<MatrixType, 3>
{
  static inline void run(const MatrixType& matrix, MatrixType* result)
  {
    ei_compute_inverse_in_size3_case(matrix, result);
  }
};

template<typename MatrixType>
struct ei_compute_inverse<MatrixType, 4>
{
  static inline void run(const MatrixType& matrix, MatrixType* result)
  {
    ei_compute_inverse_in_size4_case(matrix, result);
  }
};

/** \lu_module
  *
  * Computes the matrix inverse of this matrix.
  *
  * \note This matrix must be invertible, otherwise the result is undefined.
  *
  * \param result Pointer to the matrix in which to store the result.
  *
  * Example: \include MatrixBase_computeInverse.cpp
  * Output: \verbinclude MatrixBase_computeInverse.out
  *
  * \sa inverse()
  */
template<typename Derived>
inline void MatrixBase<Derived>::computeInverse(PlainMatrixType *result) const
{
  ei_assert(rows() == cols());
  EIGEN_STATIC_ASSERT(NumTraits<Scalar>::HasFloatingPoint,NUMERIC_TYPE_MUST_BE_FLOATING_POINT)
  ei_compute_inverse<PlainMatrixType>::run(eval(), result);
}

/** \lu_module
  *
  * \returns the matrix inverse of this matrix.
  *
  * \note This matrix must be invertible, otherwise the result is undefined.
  *
  * \note This method returns a matrix by value, which can be inefficient. To avoid that overhead,
  * use computeInverse() instead.
  *
  * Example: \include MatrixBase_inverse.cpp
  * Output: \verbinclude MatrixBase_inverse.out
  *
  * \sa computeInverse()
  */
template<typename Derived>
inline const typename MatrixBase<Derived>::PlainMatrixType MatrixBase<Derived>::inverse() const
{
  PlainMatrixType result(rows(), cols());
  computeInverse(&result);
  return result;
}

#endif // EIGEN_INVERSE_H
