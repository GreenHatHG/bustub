//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// p0_starter.h
//
// Identification: src/include/primer/p0_starter.h
//
// Copyright (c) 2015-2020, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#pragma once

#include <memory>
#include <stdexcept>
#include <vector>

#include "common/exception.h"
#include "common/logger.h"

namespace bustub {

/**
 * The Matrix type defines a common
 * interface for matrix operations.
 */
template <typename T>
class Matrix {
 protected:
  /**
   * TODO(P0): Add implementation
   *
   * Construct a new Matrix instance.
   * @param rows The number of rows
   * @param cols The number of columns
   *
   */
  Matrix(int rows, int cols) : rows_{rows}, cols_{cols}, linear_{new T[rows * cols]} {}

  /** The number of rows in the matrix */
  int rows_;
  /** The number of columns in the matrix */
  int cols_;

  /**
   * TODO(P0): Allocate the array in the constructor.
   * TODO(P0): Deallocate the array in the destructor.
   * A flattened array containing the elements of the matrix.
   */
  T *linear_;

 public:
  /** @return The number of rows in the matrix */
  virtual auto GetRowCount() const -> int = 0;

  /** @return The number of columns in the matrix */
  virtual auto GetColumnCount() const -> int = 0;

  /**
   * Get the (i,j)th matrix element.
   *
   * Throw OUT_OF_RANGE if either index is out of range.
   *
   * @param i The row index
   * @param j The column index
   * @return The (i,j)th matrix element
   * @throws OUT_OF_RANGE if either index is out of range
   */
  virtual auto GetElement(int i, int j) const -> T = 0;

  /**
   * Set the (i,j)th matrix element.
   *
   * Throw OUT_OF_RANGE if either index is out of range.
   *
   * @param i The row index
   * @param j The column index
   * @param val The value to insert
   * @throws OUT_OF_RANGE if either index is out of range
   */
  virtual void SetElement(int i, int j, T val) = 0;

  /**
   * Fill the elements of the matrix from `source`.
   *
   * Throw OUT_OF_RANGE in the event that `source`
   * does not contain the required number of elements.
   *
   * @param source The source container
   * @throws OUT_OF_RANGE if `source` is incorrect size
   */
  virtual void FillFrom(const std::vector<T> &source) = 0;

  /**
   * Destroy a matrix instance.
   * TODO(P0): Add implementation
   */
  virtual ~Matrix() { delete[] this->linear_; }
};

/**
 * The RowMatrix type is a concrete matrix implementation.
 * It implements the interface defined by the Matrix type.
 */
template <typename T>
class RowMatrix : public Matrix<T> {
 public:
  /**
   * TODO(P0): Add implementation
   *
   * Construct a new RowMatrix instance.
   * @param rows The number of rows
   * @param cols The number of columns
   */
  RowMatrix(int rows, int cols) : Matrix<T>(rows, cols) {
    this->data_ = new T *[this->rows_];
    for (int i = 0; i < this->rows_; i++) {
      this->data_[i] = new T[this->cols_];
    }
  }

  /**
   * TODO(P0): Add implementation
   * @return The number of rows in the matrix
   */
  auto GetRowCount() const -> int override { return this->rows_; }

  /**
   * TODO(P0): Add implementation
   * @return The number of columns in the matrix
   */
  auto GetColumnCount() const -> int override { return this->cols_; }

  /**
   * TODO(P0): Add implementation
   *
   * Get the (i,j)th matrix element.
   *
   * Throw OUT_OF_RANGE if either index is out of range.
   *
   * @param i The row index
   * @param j The column index
   * @return The (i,j)th matrix element
   * @throws OUT_OF_RANGE if either index is out of range
   */
  auto GetElement(int i, int j) const -> T override {
    if (i < 0 || j < 0 || i >= this->rows_ || j >= this->cols_) {
      throw Exception(ExceptionType::OUT_OF_RANGE, "index is out of range");
    }
    return this->data_[i][j];
  }

  /**
   * Set the (i,j)th matrix element.
   *
   * Throw OUT_OF_RANGE if either index is out of range.
   *
   * @param i The row index
   * @param j The column index
   * @param val The value to insert
   * @throws OUT_OF_RANGE if either index is out of range
   */
  void SetElement(int i, int j, T val) override {
    if (i < 0 || j < 0 || i >= this->rows_ || j >= this->cols_) {
      throw Exception(ExceptionType::OUT_OF_RANGE, "index is out of range");
    }
    this->data_[i][j] = val;
  }

  /**
   * TODO(P0): Add implementation
   *
   * Fill the elements of the matrix from `source`.
   *
   * Throw OUT_OF_RANGE in the event that `source`
   * does not contain the required number of elements.
   *
   * @param source The source container
   * @throws OUT_OF_RANGE if `source` is incorrect size
   */
  void FillFrom(const std::vector<T> &source) override {
    if (static_cast<std::vector<int>::size_type>(this->rows_ * this->cols_) != source.size()) {
      throw Exception(ExceptionType::OUT_OF_RANGE, "source is incorrect size.");
    }

    int latest_index{0};
    for (int row = 0; row < this->rows_; ++row) {
      for (int col = 0; col < this->cols_; ++col) {
        this->data_[row][col] = source[latest_index++];
      }
    }
  }

  /**
   * TODO(P0): Add implementation
   *
   * Destroy a RowMatrix instance.
   */
  ~RowMatrix() override {
    for (int i = 0; i < this->rows_; i++) {
      delete[] this->data_[i];
    }
    delete[] this->data_;
  };

 private:
  /**
   * A 2D array containing the elements of the matrix in row-major format.
   *
   * TODO(P0):
   * - Allocate the array of row pointers in the constructor.
   * - Use these pointers to point to corresponding elements of the `linear` array.
   * - Don't forget to deallocate the array in the destructor.
   */
  T **data_;
};

/**
 * The RowMatrixOperations class defines operations
 * that may be performed on instances of `RowMatrix`.
 */
template <typename T>
class RowMatrixOperations {
 public:
  /**
   * Compute (`matrixA` + `matrixB`) and return the result.
   * Return `nullptr` if dimensions mismatch for input matrices.
   * @param matrixA Input matrix
   * @param matrixB Input matrix
   * @return The result of matrix addition
   */
  static auto Add(const RowMatrix<T> *matrixA, const RowMatrix<T> *matrixB) -> std::unique_ptr<RowMatrix<T>> {
    // TODO(P0): Add implementation
    auto column_count = matrixA->GetColumnCount();
    auto row_count = matrixA->GetRowCount();
    if (column_count != matrixB->GetColumnCount() || row_count != matrixB->GetRowCount()) {
      return std::unique_ptr<RowMatrix<T>>(nullptr);
    }

    auto *res = new RowMatrix<T>(row_count, column_count);
    for (int i = 0; i < row_count; i++) {
      for (int j = 0; j < column_count; j++) {
        res->SetElement(i, j, matrixA->GetElement(i, j) + matrixB->GetElement(i, j));
      }
    }
    return std::unique_ptr<RowMatrix<T>>(res);
  }

  /**
   * Compute the matrix multiplication (`matrixA` * `matrixB` and return the result.
   * Return `nullptr` if dimensions mismatch for input matrices.
   * @param matrixA Input matrix
   * @param matrixB Input matrix
   * @return The result of matrix multiplication
   */
  static auto Multiply(const RowMatrix<T> *matrixA, const RowMatrix<T> *matrixB) -> std::unique_ptr<RowMatrix<T>> {
    // TODO(P0): Add implementation
    auto r1 = matrixA->GetRowCount();
    auto c1 = matrixA->GetColumnCount();
    auto r2 = matrixB->GetRowCount();
    auto c2 = matrixB->GetColumnCount();
    if (c1 != r2) {
      return std::unique_ptr<RowMatrix<T>>(nullptr);
    }

    auto *res = new RowMatrix<T>(r1, c2);
    for (int i = 0; i < r1; i++) {
      for (int j = 0; j < c2; j++) {
        res->SetElement(i, j, 0);
      }
    }

    for (int i = 0; i < r1; ++i) {
      for (int j = 0; j < c2; ++j) {
        for (int k = 0; k < c1; ++k) {
          auto temp = matrixA->GetElement(i, k) * matrixB->GetElement(k, j);
          res->SetElement(i, j, res->GetElement(i, j) + temp);
        }
      }
    }
    return std::unique_ptr<RowMatrix<T>>(res);
  }

  /**
   * Simplified General Matrix Multiply operation. Compute (`matrixA` * `matrixB` + `matrixC`).
   * Return `nullptr` if dimensions mismatch for input matrices.
   * @param matrixA Input matrix
   * @param matrixB Input matrix
   * @param matrixC Input matrix
   * @return The result of general matrix multiply
   */
  static auto GEMM(const RowMatrix<T> *matrixA, const RowMatrix<T> *matrixB, const RowMatrix<T> *matrixC)
      -> std::unique_ptr<RowMatrix<T>> {
    // TODO(P0): Add implementation
    auto column_count = matrixA->GetColumnCount();
    auto row_count = matrixA->GetRowCount();
    if (column_count != matrixB->GetColumnCount() || row_count != matrixB->GetRowCount() ||
        matrixB->GetRowCount() != matrixC->GetRowCount() || matrixB->GetColumnCount() != matrixC->GetColumnCount() ||
        column_count != matrixC->GetColumnCount() || row_count != matrixC->GetRowCount()) {
      return std::unique_ptr<RowMatrix<T>>(nullptr);
    }

    auto multiply = RowMatrixOperations<T>::Multiply(matrixA, matrixB);
    auto res = RowMatrixOperations<T>::Add(multiply, matrixC);
    return std::unique_ptr<RowMatrix<T>>(res);
  }
};
}  // namespace bustub
