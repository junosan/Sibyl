/*
   Copyright 2017 Hosang Yoon

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/

#ifndef EIGEN_HELPER_H_
#define EIGEN_HELPER_H_

#include <fstream>
#include <cmath>

namespace Eigen
{

template<class Matrix>
bool write_binary(const std::string &filename, const Matrix &matrix)
{
    std::ofstream out(filename, std::ios::out | std::ios::binary | std::ios::trunc);
    typename Matrix::Index rows = matrix.rows(), cols = matrix.cols();
    out.write((char*) (&rows), sizeof(typename Matrix::Index));
    out.write((char*) (&cols), sizeof(typename Matrix::Index));
    out.write((char*) matrix.data(), rows * cols * sizeof(typename Matrix::Scalar));
    return out.is_open();
}

template<class Matrix>
bool read_binary(const std::string &filename, Matrix &matrix)
{
    std::ifstream in(filename, std::ios::in | std::ios::binary);
    typename Matrix::Index rows = 0, cols = 0;
    in.read((char*) (&rows), sizeof(typename Matrix::Index));
    in.read((char*) (&cols), sizeof(typename Matrix::Index));
    matrix.resize(rows, cols);
    in.read((char *) matrix.data(), rows * cols * sizeof(typename Matrix::Scalar));
    return in.is_open();
}

// write_binary(...) and read_binary(...) uses modified
// code from http://stackoverflow.com/questions/25389480

// Custom defined to circumvent problems with allFinite or hasNaN functions
// under floating point optimization compiler options
template<class Matrix> 
bool is_all_finite(const Matrix &matrix)
{
    bool allFinite = true;
    typedef typename Matrix::Scalar EScalar;
    const EScalar *beg  = matrix.data();
    const EScalar *end  = matrix.data() + matrix.size();
    for (const EScalar *it = beg; it != end; it++)
    {
        const auto &i = *it;
        if (isinff(i) == true || isnanf(i) == true || std::isinf(i) == true || std::isnan(i) == true)
        {
            allFinite = false;
            break;
        } 
    }
    return allFinite;
}

}

#endif /* EIGEN_HELPER_H_ */