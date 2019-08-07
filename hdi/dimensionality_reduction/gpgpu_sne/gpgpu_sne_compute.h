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
#ifndef __APPLE__

#pragma once

#include "hdi/utils/glad/glad.h"
#include "hdi/data/shader.h"
#include "hdi/data/embedding.h"
#include "hdi/data/map_mem_eff.h"
#include "hdi/dimensionality_reduction/tsne_parameters.h"
#include "field_computation.h"

#include <array>
#include <cstdint>

namespace hdi {
  namespace dr {
    struct LinearProbabilityMatrix;

    //! Computation class for texture-based t-SNE using compute shaders
    /*!
    Computation class for texture-based t-SNE using compute shaders
    \author Julian Thijssen
    */
    class GpgpuSneCompute {
    public:
      struct Point2D {
        float x, y;
      };

      struct Bounds2D {
        Point2D min;
        Point2D max;

        Point2D getRange() {
          return Point2D{ max.x - min.x, max.y - min.y };
        }
      };

      typedef hdi::data::Embedding<float> embedding_type;
      typedef std::vector<hdi::data::MapMemEff<uint32_t, float>> sparse_scalar_matrix_type;

    public:
      GpgpuSneCompute();

      void initialize(const embedding_type* embedding, TsneParameters params, const sparse_scalar_matrix_type& P);
      void clean();

      void compute(embedding_type* embedding, float exaggeration, float iteration, float mult);

      void setScalingFactor(float factor) { _resolutionScaling = factor; }

    private:
      void initializeOpenGL(const unsigned int num_points, const LinearProbabilityMatrix& linear_P);

      Bounds2D computeEmbeddingBounds(const embedding_type* embedding, float padding = 0);

      void startTimer();
      void stopTimer();
      double getElapsed();
      void computeEmbeddingBounds1(unsigned int num_points, const float* points, float padding = 0, bool square = false);
      void interpolateFields(float* sum_Q);
      void computeGradients(unsigned int num_points, float sum_Q, double exaggeration);
      void updatePoints(unsigned int num_points, float* points, embedding_type* embedding, float iteration, float mult);
      void updateEmbedding(unsigned int num_points, float exaggeration, float iteration, float mult);

    private:
      const unsigned int FIXED_FIELDS_SIZE = 40;
      const unsigned int MINIMUM_FIELDS_SIZE = 5;
      const float PIXEL_RATIO = 2;

      bool _initialized;
      bool _adaptive_resolution;

      float _resolutionScaling;

      // Shaders
      ShaderProgram _interp_program;
      ShaderProgram _forces_program;
      ShaderProgram _update_program;
      ShaderProgram _bounds_program;
      ShaderProgram _center_and_scale_program;

      // SSBOs
      std::array<GLuint, 10> _compute_buffers;

      GLuint _timerQuery[2];

      ComputeFieldComputation fieldComputation;

      // Embedding bounds
      Bounds2D _bounds;

      // T-SNE parameters
      TsneParameters _params;

      // Probability distribution function support 
      float _function_support;
    };
  }
}

#endif // __APPLE__
