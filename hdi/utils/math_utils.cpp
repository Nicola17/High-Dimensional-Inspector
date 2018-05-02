/*
 *
 * Copyright (c) 2014, Nicola Pezzotti (Delft University of Technology)
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 1. Redistributions of source code must retain the above copyright
 *  notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *  notice, this list of conditions and the following disclaimer in the
 *  documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *  must display the following acknowledgement:
 *  This product includes software developed by the Delft University of Technology.
 * 4. Neither the name of the Delft University of Technology nor the names of
 *  its contributors may be used to endorse or promote products derived from
 *  this software without specific prior written permission.
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

#include "math_utils_inl.h"
#include <vector>
#include <unordered_map>
#include <map>

namespace hdi{
  namespace utils{

    template float euclideanDistance<float>(const std::vector<float>& a, const std::vector<float>& b);
    template double euclideanDistance<double>(const std::vector<double>& a, const std::vector<double>& b);

    template float euclideanDistanceSquared<float>(const std::vector<float>& a, const std::vector<float>& b);
    template double euclideanDistanceSquared<double>(const std::vector<double>& a, const std::vector<double>& b);

    template float euclideanDistance<float>(std::vector<float>::const_iterator a_begin, std::vector<float>::const_iterator a_end, std::vector<float>::const_iterator b_begin, std::vector<float>::const_iterator b_end);
    template double euclideanDistance<double>(std::vector<double>::const_iterator a_begin, std::vector<double>::const_iterator a_end, std::vector<double>::const_iterator b_begin, std::vector<double>::const_iterator b_end);

    template float euclideanDistanceSquared<float>(std::vector<float>::const_iterator a_begin, std::vector<float>::const_iterator a_end, std::vector<float>::const_iterator b_begin, std::vector<float>::const_iterator b_end);
    template double euclideanDistanceSquared<double>(std::vector<double>::const_iterator a_begin, std::vector<double>::const_iterator a_end, std::vector<double>::const_iterator b_begin, std::vector<double>::const_iterator b_end);

    template float euclideanDistance<float>(const float* a_begin, const float* a_end, const float* b_begin, const float* b_end);
    template double euclideanDistance<double>(const double* a_begin, const double* a_end, const double* b_begin, const double* b_end);

    template float euclideanDistanceSquared<float>(const float* a_begin, const float* a_end, const float* b_begin, const float* b_end);
    template double euclideanDistanceSquared<double>(const double* a_begin, const double* a_end, const double* b_begin, const double* b_end);

    template double computeGaussianDistribution<std::vector<float>>(std::vector<float>::const_iterator distances_begin, std::vector<float>::const_iterator distances_end, std::vector<float>::iterator P_begin, std::vector<float>::iterator P_end, double sigma);
    template double computeGaussianDistribution<std::vector<double>>(std::vector<double>::const_iterator distances_begin, std::vector<double>::const_iterator distances_end, std::vector<double>::iterator P_begin, std::vector<double>::iterator P_end, double sigma);

    template double computeGaussianDistributionWithFixedPerplexity<std::vector<float>>(std::vector<float>::const_iterator distances_begin, std::vector<float>::const_iterator distances_end, std::vector<float>::iterator P_begin, std::vector<float>::iterator P_end, double perplexity, int max_iterations, double tol, int ignore);
    template double computeGaussianDistributionWithFixedPerplexity<std::vector<double>>(std::vector<double>::const_iterator distances_begin, std::vector<double>::const_iterator distances_end, std::vector<double>::iterator P_begin, std::vector<double>::iterator P_end, double perplexity, int max_iterations, double tol, int ignore);

    template double computeGaussianDistributionWithFixedWeight<std::vector<float>>(std::vector<float>::const_iterator distances_begin, std::vector<float>::const_iterator distances_end, std::vector<float>::iterator P_begin, std::vector<float>::iterator P_end, double perplexity, int max_iterations, double tol, int ignore);
    template double computeGaussianDistributionWithFixedWeight<std::vector<double>>(std::vector<double>::const_iterator distances_begin, std::vector<double>::const_iterator distances_end, std::vector<double>::iterator P_begin, std::vector<double>::iterator P_end, double perplexity, int max_iterations, double tol, int ignore);

    template double computeGaussianFunction<std::vector<float>>(std::vector<float>::const_iterator distances_begin, std::vector<float>::const_iterator distances_end, std::vector<float>::iterator P_begin, std::vector<float>::iterator P_end, double sigma, double alpha);
    template double computeGaussianFunction<std::vector<double>>(std::vector<double>::const_iterator distances_begin, std::vector<double>::const_iterator distances_end, std::vector<double>::iterator P_begin, std::vector<double>::iterator P_end, double sigma, double alpha);


    template void computeHeterogeneity<std::vector<float>, std::vector<std::unordered_map<uint32_t,float>>>(const std::vector<std::unordered_map<uint32_t,float>>& matrix, std::vector<float>& res);
    template void computeHeterogeneity<std::vector<double>, std::vector<std::unordered_map<uint32_t,double>>>(const std::vector<std::unordered_map<uint32_t,double>>& matrix, std::vector<double>& res);
    template void computeHeterogeneity<std::vector<float>, std::vector<std::map<uint32_t,float>>>(const std::vector<std::map<uint32_t,float>>& matrix, std::vector<float>& res);
    template void computeHeterogeneity<std::vector<double>, std::vector<std::map<uint32_t,double>>>(const std::vector<std::map<uint32_t,double>>& matrix, std::vector<double>& res);

  }
}
