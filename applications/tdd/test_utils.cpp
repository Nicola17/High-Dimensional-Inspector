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


#include <catch.hpp>
#include "hdi/utils/math_utils.h"
#include <map>

template <typename scalar_type>
void euclideanDistances(){
  std::vector<scalar_type> a, b;
  SECTION("same vector"){
  a.push_back(0);
  a.push_back(0);
  a.push_back(0);
  b.push_back(0);
  b.push_back(0);
  b.push_back(0);
  REQUIRE(hdi::utils::euclideanDistance(a,b) == 0.);
  REQUIRE(hdi::utils::euclideanDistanceSquared(a,b) == 0.);

  REQUIRE(hdi::utils::euclideanDistance<scalar_type>(a.begin(),a.end(),b.begin(),b.end()) == 0.);
  REQUIRE(hdi::utils::euclideanDistanceSquared<scalar_type>(a.begin(),a.end(),b.begin(),b.end()) == 0.);

  REQUIRE(hdi::utils::euclideanDistance<scalar_type>(a.data(),a.data()+a.size(),b.data(),b.data()+b.size()) == 0.);
  REQUIRE(hdi::utils::euclideanDistanceSquared<scalar_type>(a.data(),a.data()+a.size(),b.data(),b.data()+b.size()) == 0.);
  }

  SECTION("t0"){
  a.push_back(0);
  a.push_back(0);
  a.push_back(0);
  b.push_back(1);
  b.push_back(1);
  b.push_back(1);
  REQUIRE(hdi::utils::euclideanDistance(a,b) == std::sqrt(scalar_type(3.)));
  REQUIRE(hdi::utils::euclideanDistanceSquared(a,b) == scalar_type(3.));

  REQUIRE(hdi::utils::euclideanDistance<scalar_type>(a.begin(),a.end(),b.begin(),b.end()) == std::sqrt(scalar_type(3.)));
  REQUIRE(hdi::utils::euclideanDistanceSquared<scalar_type>(a.begin(),a.end(),b.begin(),b.end()) == scalar_type(3.));

  REQUIRE(hdi::utils::euclideanDistance<scalar_type>(a.data(),a.data()+a.size(),b.data(),b.data()+b.size()) == std::sqrt(scalar_type(3.)));
  REQUIRE(hdi::utils::euclideanDistanceSquared<scalar_type>(a.data(),a.data()+a.size(),b.data(),b.data()+b.size()) == scalar_type(3.));
  }

  SECTION("t1"){
  a.push_back(0);
  a.push_back(0);
  a.push_back(0);
  b.push_back(2);
  b.push_back(2);
  b.push_back(2);
  REQUIRE(hdi::utils::euclideanDistance(a,b) == std::sqrt(scalar_type(12.)));
  REQUIRE(hdi::utils::euclideanDistanceSquared(a,b) == scalar_type(12.));

  REQUIRE(hdi::utils::euclideanDistance<scalar_type>(a.begin(),a.end(),b.begin(),b.end()) == std::sqrt(scalar_type(12.)));
  REQUIRE(hdi::utils::euclideanDistanceSquared<scalar_type>(a.begin(),a.end(),b.begin(),b.end()) == scalar_type(12.));

  REQUIRE(hdi::utils::euclideanDistance<scalar_type>(a.data(),a.data()+a.size(),b.data(),b.data()+b.size()) == std::sqrt(scalar_type(12.)));
  REQUIRE(hdi::utils::euclideanDistanceSquared<scalar_type>(a.data(),a.data()+a.size(),b.data(),b.data()+b.size()) == scalar_type(12.));
  }
}

template <typename scalar_type>
void gaussianDistributions(){
  {
  std::vector<scalar_type> distances, distribution;
  REQUIRE_THROWS(hdi::utils::computeGaussianDistribution<std::vector<scalar_type>>(distances.begin(), distances.end(), distribution.begin(), distribution.end(), 0.5));

  distances.resize(5);
  distribution.resize(10);
  REQUIRE_THROWS(hdi::utils::computeGaussianDistribution<std::vector<scalar_type>>(distances.begin(), distances.end(), distribution.begin(), distribution.end(), 0.5));


  distances.resize(100);
  distribution.resize(100);
  for(int i = 0; i < distances.size(); ++i){
    double v = std::sin(3.1415 / distances.size() * i);
    distances[i] = static_cast<scalar_type>(v*v);
  }

  double norm(0);
  REQUIRE_NOTHROW( norm = hdi::utils::computeGaussianDistribution<std::vector<scalar_type>>(distances.begin(), distances.end(), distribution.begin(), distribution.end(), 0.5));


  double sum = 0;
  for(auto v : distribution){
    sum += v;
  }
  REQUIRE((sum > 0.9999 && sum < 1.00001));
  }
  {
  std::vector<float> distances, distribution;
  REQUIRE_THROWS(hdi::utils::computeGaussianDistributionWithFixedPerplexity<std::vector<float>>(distances.begin(), distances.end(), distribution.begin(), distribution.end(), 10));

  distances.resize(5);
  distribution.resize(10);
  REQUIRE_THROWS(hdi::utils::computeGaussianDistributionWithFixedPerplexity<std::vector<float>>(distances.begin(), distances.end(), distribution.begin(), distribution.end(), 10));


  distances.resize(100);
  distribution.resize(100);
  for(int i = 0; i < distances.size(); ++i){
    double v = std::cos(3.1415 / distances.size() * i);
    distances[i] = static_cast<float>(v*v);
  }
  REQUIRE_NOTHROW(hdi::utils::computeGaussianDistributionWithFixedPerplexity<std::vector<float>>(distances.begin(), distances.end(), distribution.begin(), distribution.end(), 10));


  double sum = 0;
  for(auto v : distribution){
    sum += v;
  }
  REQUIRE((sum > 0.9999 && sum < 1.00001));
  }
}


template <typename scalar_type>
void gaussianFunctions(){
  std::vector<scalar_type> distances, distribution;
  REQUIRE_THROWS(hdi::utils::computeGaussianFunction<std::vector<scalar_type>>(distances.begin(), distances.end(), distribution.begin(), distribution.end(), 0.5));

  distances.resize(5);
  distribution.resize(10);
  REQUIRE_THROWS(hdi::utils::computeGaussianFunction<std::vector<scalar_type>>(distances.begin(), distances.end(), distribution.begin(), distribution.end(), 0.5));


  distances.resize(100);
  distribution.resize(100);
  for(int i = 0; i < distances.size(); ++i){
  double v = std::sin(3.1415 / distances.size() * i);
  distances[i] = static_cast<scalar_type>(v*v);
  }

  double norm(0);
  REQUIRE_NOTHROW( norm = hdi::utils::computeGaussianFunction<std::vector<scalar_type>>(distances.begin(), distances.end(), distribution.begin(), distribution.end(), 0.5));


  double sum = 0;
  for(auto v : distribution){
  sum += v;
  }
  REQUIRE(sum == norm);
}

template <typename scalar_type>
void perplexityComputation(){
  {
    std::vector<scalar_type> distr(10,0.1);
    REQUIRE(std::abs(hdi::utils::computePerplexity(distr.begin(),distr.end())-10) < 10e-5);
  }
  {
    std::vector<scalar_type> distr(10,0);
    distr[0] = 1;
    REQUIRE(std::abs(hdi::utils::computePerplexity(distr.begin(),distr.end())-1) < 10e-5);
  }
  {
    std::map<int,scalar_type> map;
    map[0] = 0.2;
    map[1] = 0.2;
    map[2] = 0.2;
    map[3] = 0.2;
    map[4] = 0.2;
    REQUIRE(std::abs(hdi::utils::computePerplexity(map)-5) < 10e-5);
  }
  {
    std::map<int,scalar_type> map;
    map[0] = 1;
    map[1] = 0;
    map[2] = 0;
    map[3] = 0;
    map[4] = 0;
    REQUIRE(std::abs(hdi::utils::computePerplexity(map)-1) < 10e-5);
  }
}
template <typename scalar_type>
void vectorMatrixMultiplication(){
  {
    std::vector<scalar_type> a(2,0.5);
    std::vector<std::map<unsigned int, scalar_type>> b(2);
    std::vector<scalar_type> c;

    b[0][0] = 0.3; b[0][1] = 0.7;
    b[1][0] = 0.8; b[1][1] = 0.2;

    hdi::utils::multiply(a,b,c);

    REQUIRE(std::abs(c[0]-0.55) < 10e-5);
    REQUIRE(std::abs(c[1]-0.45) < 10e-5);
  }

  {
    std::vector<scalar_type> a(3,0);
    a[0] = 1;
    std::vector<std::map<unsigned int, scalar_type>> b(3);
    std::vector<scalar_type> c;

    b[0][0] = 0.5; b[0][1] = 0.25; b[0][2] = 0.25;
    b[1][1] = 1;
    b[2][2] = 1;

    hdi::utils::multiply(a,b,c);
    REQUIRE(std::abs(c[0]-0.5) < 10e-5);
    REQUIRE(std::abs(c[1]-0.25) < 10e-5);
    REQUIRE(std::abs(c[2]-0.25) < 10e-5);

    a = c;
    hdi::utils::multiply(a,b,c);

    REQUIRE(std::abs(c[0]-0.25) < 10e-5);
    REQUIRE(std::abs(c[1]-0.375) < 10e-5);
    REQUIRE(std::abs(c[2]-0.375) < 10e-5);

    a = c;
    hdi::utils::multiply(a,b,c);

    REQUIRE(std::abs(c[0]-0.125) < 10e-5);
    REQUIRE(std::abs(c[1]-0.4375) < 10e-5);
    REQUIRE(std::abs(c[2]-0.4375) < 10e-5);

    for(int i = 0; i < 10000; ++i){
      a = c;
      hdi::utils::multiply(a,b,c);
    }
    REQUIRE(std::abs(c[0]-0.) < 10e-5);
    REQUIRE(std::abs(c[1]-0.5) < 10e-5);
    REQUIRE(std::abs(c[2]-0.5) < 10e-5);
  }
}

template <typename scalar_type>
void stationaryDistribution(){
  {
    std::vector<scalar_type> initial_distribution(3,1./3.);
    std::vector<std::map<unsigned int, scalar_type>> transition_matrix(3);
    std::vector<scalar_type> stationary_distribution;

    transition_matrix[0][0] = 0.5; transition_matrix[0][1] = 0.25; transition_matrix[0][2] = 0.25;
    transition_matrix[1][1] = 1;
    transition_matrix[2][2] = 1;

    hdi::utils::stationaryDistributionFMC(initial_distribution,transition_matrix,stationary_distribution);

    REQUIRE(std::abs(stationary_distribution[0]-0.) < 10e-5);
    REQUIRE(std::abs(stationary_distribution[1]-0.5) < 10e-5);
    REQUIRE(std::abs(stationary_distribution[2]-0.5) < 10e-5);
  }
  {
    std::vector<scalar_type> initial_distribution(10,1./10.);
    std::vector<std::map<unsigned int, scalar_type>> transition_matrix(10);
    std::vector<scalar_type> stationary_distribution;

    transition_matrix[0][0] = 0.5; transition_matrix[0][1] = 0.3; transition_matrix[0][2] = 0.2;
    transition_matrix[1][1] = 0.5; transition_matrix[1][2] = 0.3; transition_matrix[1][3] = 0.2;
    transition_matrix[2][2] = 0.5; transition_matrix[2][3] = 0.3; transition_matrix[2][4] = 0.2;
    transition_matrix[3][3] = 0.5; transition_matrix[3][4] = 0.3; transition_matrix[3][5] = 0.2;
    transition_matrix[4][4] = 0.5; transition_matrix[4][5] = 0.3; transition_matrix[4][6] = 0.2;
    transition_matrix[5][5] = 0.5; transition_matrix[5][6] = 0.3; transition_matrix[5][7] = 0.2;
    transition_matrix[6][6] = 0.5; transition_matrix[6][7] = 0.3; transition_matrix[6][8] = 0.2;
    transition_matrix[7][7] = 0.5; transition_matrix[7][8] = 0.3; transition_matrix[7][9] = 0.2;
    transition_matrix[8][8] = 0.5; transition_matrix[8][9] = 0.5;
    transition_matrix[9][9] = 1;

    hdi::utils::stationaryDistributionFMC(initial_distribution,transition_matrix,stationary_distribution);

    REQUIRE(std::abs(stationary_distribution[0]) < 10e-5);
    REQUIRE(std::abs(stationary_distribution[1]) < 10e-5);
    REQUIRE(std::abs(stationary_distribution[2]) < 10e-5);
    REQUIRE(std::abs(stationary_distribution[3]) < 10e-5);
    REQUIRE(std::abs(stationary_distribution[4]) < 10e-5);
    REQUIRE(std::abs(stationary_distribution[5]) < 10e-5);
    REQUIRE(std::abs(stationary_distribution[6]) < 10e-5);
    REQUIRE(std::abs(stationary_distribution[7]) < 10e-5);
    REQUIRE(std::abs(stationary_distribution[8]) < 10e-5);
    REQUIRE(std::abs(stationary_distribution[9]-1) < 10e-5);
  }
}

TEST_CASE( "Euclidean distances are computed correctly - float", "[math]" ) {
  euclideanDistances<float>();
}
TEST_CASE( "Euclidean distances are computed correctly - double", "[math]" ) {
  euclideanDistances<double>();
}

TEST_CASE( "Gaussian distributions are computed correctly - float", "[math]" ) {
  gaussianDistributions<float>();
}
TEST_CASE( "Gaussian distributions are computed correctly - double", "[math]" ) {
  gaussianDistributions<double>();
}

TEST_CASE( "Gaussian functions are computed correctly - float", "[math]" ) {
  gaussianFunctions<float>();
}
TEST_CASE( "Gaussian functions are computed correctly - double", "[math]" ) {
  gaussianFunctions<double>();
}
TEST_CASE( "Perplexity is computed correctly - double", "[math]" ) {
  perplexityComputation<double>();
}
TEST_CASE( "Perplexity is computed correctly - float", "[math]" ) {
  perplexityComputation<float>();
}

TEST_CASE( "Stationary distribution for a Finite Markov Chain - double", "[math]" ) {
  stationaryDistribution<double>();
}
TEST_CASE( "Stationary distribution for a Finite Markov Chain - float", "[math]" ) {
  stationaryDistribution<float>();
}
