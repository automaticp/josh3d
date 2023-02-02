#pragma once
#include <range/v3/all.hpp>
#include <range/v3/iterator/operations.hpp>
#include <range/v3/range.hpp>
#include <range/v3/range/concepts.hpp>
#include <cassert>
#include <cstddef>
#include <stdexcept>
#include <string>
#include <vector>


template<typename T>
class Matrix2D {
private:
    size_t rows_;
    size_t cols_;
    std::vector<T> data_;

public:
    template<typename It>
    Matrix2D(size_t rows, size_t cols, It beg, It end)
        : rows_{ rows }, cols_{ cols }, data_( beg, end )
    {
        assert((rows * cols) == data_.size());
    }

    Matrix2D(size_t rows, size_t cols)
        : rows_{ rows }, cols_{ cols }, data_( rows_ * cols_ )
    {}

    Matrix2D() : Matrix2D(0, 0) {}


    size_t nrows() const noexcept { return rows_; }
    size_t ncols() const noexcept { return cols_; }

    auto begin() const noexcept { return data_.begin(); }
    auto begin()       noexcept { return data_.begin(); }
    auto end()   const noexcept { return data_.end();   }
    auto end()         noexcept { return data_.end();   }


    T& at(size_t row, size_t col) { return data_.at(index(row, col)); }
    const T& at(size_t row, size_t col) const { return data_.at(index(row, col)); }


    template<typename RandomIt>
    void push_row(RandomIt beg, RandomIt end) {
        size_t cols = (end - beg);
        if (!data_.empty()) {
            // FIXME: throw
            assert(cols == cols_);
        } else {
            cols_ = cols;
        }
        data_.insert(data_.end(), beg, end);
        ++rows_;
    }

    void push_row(ranges::range auto&& r) {
        size_t cols = ranges::distance(r);
        if (!data_.empty()) {
            if (cols != cols_) {
                throw std::runtime_error(
                    "Invalid push of a row with " + std::to_string(cols) +
                    " columns to a Matrix2D with " + std::to_string(cols_) + "columns."
                );
            }
        } else {
            cols_ = cols;
        }
        data_.reserve(data_.size() + cols);
        for (auto&& elem : r) { data_.emplace_back(elem); }
        ++rows_;
    }

private:
    size_t index(size_t row, size_t col) { return row * cols_ + col; }
};
