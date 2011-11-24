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

template<typename XprType, typename MatrixType>
void ei_compute_inverse_in_size2_case(const XprType& matrix, MatrixType* result)
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

template<typename Derived, typename OtherDerived>
void ei_compute_inverse_in_size3_case(const Derived& matrix, OtherDerived* result)
{
  typedef typename Derived::Scalar Scalar;
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

template<typename Derived, typename OtherDerived, typename Scalar = typename Derived::Scalar>
struct ei_compute_inverse_in_size4_case
{
  static void run(const Derived& matrix, OtherDerived& result)
  {
    result.coeffRef(0,0) = matrix.minor(0,0).determinant();
    result.coeffRef(1,0) = -matrix.minor(0,1).determinant();
    result.coeffRef(2,0) = matrix.minor(0,2).determinant();
    result.coeffRef(3,0) = -matrix.minor(0,3).determinant();
    result.coeffRef(0,2) = matrix.minor(2,0).determinant();
    result.coeffRef(1,2) = -matrix.minor(2,1).determinant();
    result.coeffRef(2,2) = matrix.minor(2,2).determinant();
    result.coeffRef(3,2) = -matrix.minor(2,3).determinant();
    result.coeffRef(0,1) = -matrix.minor(1,0).determinant();
    result.coeffRef(1,1) = matrix.minor(1,1).determinant();
    result.coeffRef(2,1) = -matrix.minor(1,2).determinant();
    result.coeffRef(3,1) = matrix.minor(1,3).determinant();
    result.coeffRef(0,3) = -matrix.minor(3,0).determinant();
    result.coeffRef(1,3) = matrix.minor(3,1).determinant();
    result.coeffRef(2,3) = -matrix.minor(3,2).determinant();
    result.coeffRef(3,3) = matrix.minor(3,3).determinant();
    result /= (matrix.col(0).cwise()*result.row(0).transpose()).sum();
  }
};

#ifdef EIGEN_VECTORIZE_SSE
// The SSE code for the 4x4 float matrix inverse in this file comes from the file
//   ftp://download.intel.com/design/PentiumIII/sml/24504301.pdf
// its copyright information is:
//   Copyright (C) 1999 Intel Corporation
// See page ii of that document for legal stuff. Not being lawyers, we just assume
// here that if Intel makes this document publically available, with source code
// and detailed explanations, it's because they want their CPUs to be fed with
// good code, and therefore they presumably don't mind us using it in Eigen.
template<typename Derived, typename OtherDerived>
struct ei_compute_inverse_in_size4_case<Derived, OtherDerived, float>
{
  static void run(const Derived& matrix, OtherDerived& result)
  {
    // Variables (Streaming SIMD Extensions registers) which will contain cofactors and, later, the
    // lines of the inverted matrix.
    __m128 minor0, minor1, minor2, minor3;

    // Variables which will contain the lines of the reference matrix and, later (after the transposition),
    // the columns of the original matrix.
    __m128 row0, row1, row2, row3;

    // Temporary variables and the variable that will contain the matrix determinant.
    __m128 det, tmp1;

    // Matrix transposition
    const float *src = matrix.data();
    tmp1  = _mm_loadh_pi(_mm_castpd_ps(_mm_load_sd((double*)src)), (__m64*)(src+ 4));
    row1  = _mm_loadh_pi(_mm_castpd_ps(_mm_load_sd((double*)(src+8))), (__m64*)(src+12));
    row0  = _mm_shuffle_ps(tmp1, row1, 0x88);
    row1  = _mm_shuffle_ps(row1, tmp1, 0xDD);
    tmp1  = _mm_loadh_pi(_mm_castpd_ps(_mm_load_sd((double*)(src+ 2))), (__m64*)(src+ 6));
    row3  = _mm_loadh_pi(_mm_castpd_ps(_mm_load_sd((double*)(src+10))), (__m64*)(src+14));
    row2  = _mm_shuffle_ps(tmp1, row3, 0x88);
    row3  = _mm_shuffle_ps(row3, tmp1, 0xDD);


    // Cofactors calculation. Because in the process of cofactor computation some pairs in three-
    // element products are repeated, it is not reasonable to load these pairs anew every time. The
    // values in the registers with these pairs are formed using shuffle instruction. Cofactors are
    // calculated row by row (4 elements are placed in 1 SP FP SIMD floating point register).

    tmp1   = _mm_mul_ps(row2, row3);
    tmp1   = _mm_shuffle_ps(tmp1, tmp1, 0xB1);
    minor0  = _mm_mul_ps(row1, tmp1);
    minor1  = _mm_mul_ps(row0, tmp1);
    tmp1   = _mm_shuffle_ps(tmp1, tmp1, 0x4E);
    minor0  = _mm_sub_ps(_mm_mul_ps(row1, tmp1), minor0);
    minor1  = _mm_sub_ps(_mm_mul_ps(row0, tmp1), minor1);
    minor1  = _mm_shuffle_ps(minor1, minor1, 0x4E);
    //    -----------------------------------------------
    tmp1   = _mm_mul_ps(row1, row2);
    tmp1   = _mm_shuffle_ps(tmp1, tmp1, 0xB1);
    minor0  = _mm_add_ps(_mm_mul_ps(row3, tmp1), minor0);
    minor3  = _mm_mul_ps(row0, tmp1);
    tmp1   = _mm_shuffle_ps(tmp1, tmp1, 0x4E);
    minor0  = _mm_sub_ps(minor0, _mm_mul_ps(row3, tmp1));
    minor3  = _mm_sub_ps(_mm_mul_ps(row0, tmp1), minor3);
    minor3  = _mm_shuffle_ps(minor3, minor3, 0x4E);
    //    -----------------------------------------------
    tmp1   = _mm_mul_ps(_mm_shuffle_ps(row1, row1, 0x4E), row3);
    tmp1   = _mm_shuffle_ps(tmp1, tmp1, 0xB1);
    row2   = _mm_shuffle_ps(row2, row2, 0x4E);
    minor0  = _mm_add_ps(_mm_mul_ps(row2, tmp1), minor0);
    minor2  = _mm_mul_ps(row0, tmp1);
    tmp1   = _mm_shuffle_ps(tmp1, tmp1, 0x4E);
    minor0  = _mm_sub_ps(minor0, _mm_mul_ps(row2, tmp1));
    minor2  = _mm_sub_ps(_mm_mul_ps(row0, tmp1), minor2);
    minor2  = _mm_shuffle_ps(minor2, minor2, 0x4E);
    //    -----------------------------------------------
    tmp1   = _mm_mul_ps(row0, row1);
    tmp1   = _mm_shuffle_ps(tmp1, tmp1, 0xB1);
    minor2 = _mm_add_ps(_mm_mul_ps(row3, tmp1), minor2);
    minor3 = _mm_sub_ps(_mm_mul_ps(row2, tmp1), minor3);
    tmp1   = _mm_shuffle_ps(tmp1, tmp1, 0x4E);
    minor2 = _mm_sub_ps(_mm_mul_ps(row3, tmp1), minor2);
    minor3 = _mm_sub_ps(minor3, _mm_mul_ps(row2, tmp1));
    //           -----------------------------------------------
    tmp1   = _mm_mul_ps(row0, row3);
    tmp1   = _mm_shuffle_ps(tmp1, tmp1, 0xB1);
    minor1 = _mm_sub_ps(minor1, _mm_mul_ps(row2, tmp1));
    minor2 = _mm_add_ps(_mm_mul_ps(row1, tmp1), minor2);
    tmp1   = _mm_shuffle_ps(tmp1, tmp1, 0x4E);
    minor1 = _mm_add_ps(_mm_mul_ps(row2, tmp1), minor1);
    minor2 = _mm_sub_ps(minor2, _mm_mul_ps(row1, tmp1));
    //           -----------------------------------------------
    tmp1   = _mm_mul_ps(row0, row2);
    tmp1   = _mm_shuffle_ps(tmp1, tmp1, 0xB1);
    minor1 = _mm_add_ps(_mm_mul_ps(row3, tmp1), minor1);
    minor3 = _mm_sub_ps(minor3, _mm_mul_ps(row1, tmp1));
    tmp1   = _mm_shuffle_ps(tmp1, tmp1, 0x4E);
    minor1 = _mm_sub_ps(minor1, _mm_mul_ps(row3, tmp1));
    minor3 = _mm_add_ps(_mm_mul_ps(row1, tmp1), minor3);

    // Evaluation of determinant and its reciprocal value. In the original Intel document,
    // 1/det was evaluated using a fast rcpps command with subsequent approximation using
    // the Newton-Raphson algorithm. Here, we go for a IEEE-compliant division instead,
    // so as to not compromise precision at all.
    det    = _mm_mul_ps(row0, minor0);
    det    = _mm_add_ps(_mm_shuffle_ps(det, det, 0x4E), det);
    det    = _mm_add_ss(_mm_shuffle_ps(det, det, 0xB1), det);
//     tmp1= _mm_rcp_ss(det);
//     det= _mm_sub_ss(_mm_add_ss(tmp1, tmp1), _mm_mul_ss(det, _mm_mul_ss(tmp1, tmp1)));
    det    = _mm_div_ss(_mm_set_ss(1.0f), det); // <--- yay, one original line not copied from Intel
    det    = _mm_shuffle_ps(det, det, 0x00);
    // warning, Intel's variable naming is very confusing: now 'det' is 1/det !

    // Multiplication of cofactors by 1/det. Storing the inverse matrix to the address in pointer src.
    minor0 = _mm_mul_ps(det, minor0);
    float *dst = result.data();
    _mm_storel_pi((__m64*)(dst), minor0);
    _mm_storeh_pi((__m64*)(dst+2), minor0);
    minor1 = _mm_mul_ps(det, minor1);
    _mm_storel_pi((__m64*)(dst+4), minor1);
    _mm_storeh_pi((__m64*)(dst+6), minor1);
    minor2 = _mm_mul_ps(det, minor2);
    _mm_storel_pi((__m64*)(dst+ 8), minor2);
    _mm_storeh_pi((__m64*)(dst+10), minor2);
    minor3 = _mm_mul_ps(det, minor3);
    _mm_storel_pi((__m64*)(dst+12), minor3);
    _mm_storeh_pi((__m64*)(dst+14), minor3);
  }
};
#endif

/***********************************************
*** Part 2 : selector and MatrixBase methods ***
***********************************************/

template<typename Derived, typename OtherDerived, int Size = Derived::RowsAtCompileTime>
struct ei_compute_inverse
{
  static inline void run(const Derived& matrix, OtherDerived* result)
  {
    LU<Derived> lu(matrix);
    lu.computeInverse(result);
  }
};

template<typename Derived, typename OtherDerived>
struct ei_compute_inverse<Derived, OtherDerived, 1>
{
  static inline void run(const Derived& matrix, OtherDerived* result)
  {
    typedef typename Derived::Scalar Scalar;
    result->coeffRef(0,0) = Scalar(1) / matrix.coeff(0,0);
  }
};

template<typename Derived, typename OtherDerived>
struct ei_compute_inverse<Derived, OtherDerived, 2>
{
  static inline void run(const Derived& matrix, OtherDerived* result)
  {
    ei_compute_inverse_in_size2_case(matrix, result);
  }
};

template<typename Derived, typename OtherDerived>
struct ei_compute_inverse<Derived, OtherDerived, 3>
{
  static inline void run(const Derived& matrix, OtherDerived* result)
  {
    ei_compute_inverse_in_size3_case(matrix, result);
  }
};

template<typename Derived, typename OtherDerived>
struct ei_compute_inverse<Derived, OtherDerived, 4>
{
  static inline void run(const Derived& matrix, OtherDerived* result)
  {
    ei_compute_inverse_in_size4_case<Derived, OtherDerived>::run(matrix, *result);
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
template<typename OtherDerived>
inline void MatrixBase<Derived>::computeInverse(MatrixBase<OtherDerived> *result) const
{
  ei_assert(rows() == cols());
  EIGEN_STATIC_ASSERT(NumTraits<Scalar>::HasFloatingPoint,NUMERIC_TYPE_MUST_BE_FLOATING_POINT)
  ei_compute_inverse<PlainMatrixType, OtherDerived>::run(eval(), static_cast<OtherDerived*>(result));
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
