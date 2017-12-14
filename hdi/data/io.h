/*
 *
 * Copyright (c) 2014, Nicola Pezzotti (Delft University of Technology)
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *    This product includes software developed by the Delft University of Technology.
 * 4. Neither the name of the Delft University of Technology nor the names of
 *    its contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY NICOLA PEZZOTTI ''AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
 * EVENT SHALL NICOLA PEZZOTTI BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
 * OF SUCH DAMAGE.
 *
 */

#ifndef IO_H
#define IO_H


namespace hdi{
    namespace data{
        namespace IO{


            template <typename sparse_scalar_matrix_type, class output_stream_type>
            void saveSparseMatrix(const sparse_scalar_matrix_type& matrix, output_stream_type& stream, utils::AbstractLog* log = nullptr){
                typedef float io_scalar_type;
                typedef uint32_t io_unsigned_int_type;

                //number of rows first
                io_unsigned_int_type num_rows = static_cast<io_unsigned_int_type>(matrix.size());
                stream.write(reinterpret_cast<char*>(&num_rows),sizeof(io_unsigned_int_type));
                for(int j = 0; j < num_rows; ++j){
                    //number of elements in the current row
                    io_unsigned_int_type num_elems = static_cast<io_unsigned_int_type>(matrix[j].size());

                    stream.write(reinterpret_cast<char*>(&num_elems),sizeof(io_unsigned_int_type));
                    for(auto& elem: matrix[j]){
                        io_unsigned_int_type id = static_cast<io_unsigned_int_type>(elem.first);
                        io_scalar_type v = static_cast<io_scalar_type>(elem.second);
                        stream.write(reinterpret_cast<char*>(&id),sizeof(io_unsigned_int_type));
                        stream.write(reinterpret_cast<char*>(&v),sizeof(io_scalar_type));
                    }
                }
            }
            template <typename scalar_vector, class output_stream_type>
            void saveScalarVector(const scalar_vector& vector, output_stream_type& stream, utils::AbstractLog* log = nullptr){
                typedef float io_scalar_type;
                typedef uint32_t io_unsigned_int_type;

                io_unsigned_int_type num_elems = static_cast<io_unsigned_int_type>(vector.size());
                stream.write(reinterpret_cast<char*>(&num_elems),sizeof(io_unsigned_int_type));
                for(auto& elem: vector){
                    io_scalar_type v = static_cast<io_scalar_type>(elem);
                    stream.write(reinterpret_cast<char*>(&v),sizeof(io_scalar_type));
                }
            }
            template <typename uint_vector, class output_stream_type>
            void saveUIntVector(const uint_vector& vector, output_stream_type& stream, utils::AbstractLog* log = nullptr){
                typedef float io_scalar_type;
                typedef uint32_t io_unsigned_int_type;

                io_unsigned_int_type num_elems = static_cast<io_unsigned_int_type>(vector.size());
                stream.write(reinterpret_cast<char*>(&num_elems),sizeof(io_unsigned_int_type));
                for(auto& elem: vector){
                    io_unsigned_int_type v = static_cast<io_unsigned_int_type>(elem);
                    stream.write(reinterpret_cast<char*>(&v),sizeof(io_unsigned_int_type));
                }
            }
            template <typename uint_vector, class output_stream_type>
            void saveIntVector(const uint_vector& vector, output_stream_type& stream, utils::AbstractLog* log = nullptr){
                typedef float io_scalar_type;
                typedef uint32_t io_unsigned_int_type;
                typedef int32_t io_int_type;

                io_unsigned_int_type num_elems = static_cast<io_unsigned_int_type>(vector.size());
                stream.write(reinterpret_cast<char*>(&num_elems),sizeof(io_unsigned_int_type));
                for(auto& elem: vector){
                    io_int_type v = static_cast<io_int_type>(elem);
                    stream.write(reinterpret_cast<char*>(&v),sizeof(io_int_type));
                }
            }

        ///////////////////////////////////////////////////////////////////////

            template <typename sparse_scalar_matrix_type, class output_stream_type>
            void loadSparseMatrix(sparse_scalar_matrix_type& matrix, output_stream_type& stream, utils::AbstractLog* log = nullptr){
                typedef float io_scalar_type;
                typedef uint32_t io_unsigned_int_type;

                //number of rows first
                io_unsigned_int_type num_rows;
                stream.read(reinterpret_cast<char*>(&num_rows),sizeof(io_unsigned_int_type));
                matrix.clear();
                matrix.resize(num_rows);
                for(int j = 0; j < num_rows; ++j){
                    //number of elements in the current row
                    io_unsigned_int_type num_elems;
                    stream.read(reinterpret_cast<char*>(&num_elems),sizeof(io_unsigned_int_type));
                    for(int i = 0; i < num_elems; ++i){
                        io_unsigned_int_type id;
                        io_scalar_type v;
                        stream.read(reinterpret_cast<char*>(&id),sizeof(io_unsigned_int_type));
                        stream.read(reinterpret_cast<char*>(&v),sizeof(io_scalar_type));
                        matrix[j][id] = v;
                    }
                }
            }

            template <typename scalar_vector, class output_stream_type>
            void loadScalarVector(scalar_vector& vector, output_stream_type& stream, utils::AbstractLog* log = nullptr){
                typedef float io_scalar_type;
                typedef uint32_t io_unsigned_int_type;

                io_unsigned_int_type num_elems;
                stream.read(reinterpret_cast<char*>(&num_elems),sizeof(io_unsigned_int_type));
                vector.reserve(num_elems);
                for(int i  = 0; i < num_elems; ++i){
                    io_scalar_type v;
                    stream.read(reinterpret_cast<char*>(&v),sizeof(io_scalar_type));
                    vector.push_back(v);
                }
            }
            template <typename uint_vector, class output_stream_type>
            void loadUIntVector(uint_vector& vector, output_stream_type& stream, utils::AbstractLog* log = nullptr){
                typedef float io_scalar_type;
                typedef uint32_t io_unsigned_int_type;

                io_unsigned_int_type num_elems;
                stream.read(reinterpret_cast<char*>(&num_elems),sizeof(io_unsigned_int_type));
                vector.reserve(num_elems);
                for(int i  = 0; i < num_elems; ++i){
                    io_unsigned_int_type v;
                    stream.read(reinterpret_cast<char*>(&v),sizeof(io_unsigned_int_type));
                    vector.push_back(v);
                }
            }
            template <typename uint_vector, class output_stream_type>
            void loadIntVector(uint_vector& vector, output_stream_type& stream, utils::AbstractLog* log = nullptr){
                typedef float io_scalar_type;
                typedef uint32_t io_unsigned_int_type;
                typedef int32_t io_int_type;

                io_unsigned_int_type num_elems;
                stream.read(reinterpret_cast<char*>(&num_elems),sizeof(io_unsigned_int_type));
                vector.reserve(num_elems);
                for(int i  = 0; i < num_elems; ++i){
                    io_int_type v;
                    stream.read(reinterpret_cast<char*>(&v),sizeof(io_int_type));
                    vector.push_back(v);
                }
            }


        }
    }
}

#endif
