//
// Created by Kyle Smith on 2025-09-26.
//
#pragma once

#include <string>
#include <iostream>

#include "Vectors.h"

/// A templated matrix type
/// Provides support for templated matrix-vector and matrix-matrix multiplication
/// Data is stored in column-major format, for efficiency when doing said multiplication
template<typename T, int X, int Y>
struct Matrix {
    // column-major
    T data[X * Y];

    vecn<T, Y> *operator[](int column) {
        return (vecn<T, Y> *) &data[column * Y];
    }

    /// Matrix-matrix multiply
    /// Since this is templated, the compiler can generate the optimal machine code for any shape
    template<int X2, int Y2>
    Matrix<T, Y, X2> operator *(Matrix<T, X2, Y2> other) {
        // verify that matrix multiplication is actually possible for the two matrices
        static_assert(X == Y2, "Matrix multiplication not defined for this shape!");
        Matrix<T, Y, X2> result;

        // for each of the columns in the right-hand matrix...
        for (int column = 0; column < X2; column++) {
            // grab that column from the right-hand matrix as a vector...
            vecn<T, Y2> *cur_column = other[column];

            // initialize an accumulator...
            vecn<T, Y> acc = vecn<T, Y>::zero();

            // then for each column in the left-hand vector...
            for (int i = 0; i < X; i++) {
                // multiply that column by the matching row in the right-hand column...
                auto vector = *this->operator[](i);
                auto constant = cur_column->operator[](i);
                auto transformed = (vector * constant);

                // then add to the accumulator
                acc = acc + transformed;
            }

            // finally, copy the transformed column to the output matrix
            for (int i = 0; i < Y; i++) {
                result.data[(column * Y) + i] = acc.data[i];
            }
        }

        return result;
    }

    /// Matrix-vector multiplication
    template<int VL>
    vecn<T, Y> operator *(vecn<T, VL> other) {
        static_assert(VL == X, "Incompatible shapes for matrix-vector multiply");

        // setup output accumulator
        vecn<T, Y> acc = vecn<T, Y>::zero();

        // for each column...
        for (int column = 0; column < X; column++) {
            // multiply it by the corresponding lane in the vector and add to the accumulator
            acc += (*this->operator[](column) * other[column]);
        }

        return acc;
    }

    /// Helper function to print matrices to the console
    friend std::ostream &operator<<(std::ostream &os, const Matrix &m) {
        os << X << "x" << Y << " matrix:" << std::endl;

        // have to iterate like this because data is stored column-major
        // but we want to print row-major
        for (int y = 0; y < Y; y++) {
            for (int x = 0; x < X; x++) {
                os << m.data[x * Y + y] << " ";
            }
            os << "\n";
        }
        return os;
    }

    explicit operator const T*() const {
        return &data[0];
    }

    /// Get an identity matrix
    /// Undefined compile for non-square shapes
    static Matrix identity() {
        static_assert(X == Y, "Matrix identity not defined for non-square matrices");
        Matrix m;
        for (int i = 0; i < X; i++) {
            for (int j = 0; j < Y; j++) {
#pragma warning(suppress: 4244) // no data loss possibility here
                m.data[i * Y + j] = i == j ? 1 : 0;
            }
        }
        return m;
    }
};

/// Typedef the shapes we'll commonly use for convenience
typedef Matrix<float, 4, 4> mat4;
typedef Matrix<float, 3, 3> mat3;