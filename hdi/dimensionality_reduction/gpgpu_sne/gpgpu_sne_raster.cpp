#include "gpgpu_sne_raster.h"
#include "raster_shaders.glsl"

#include <vector>
#include <iostream>

namespace hdi {
  namespace dr {
    typedef GpgpuSneRaster::Bounds2D Bounds2D;
    typedef GpgpuSneRaster::Point2D Point2D;

    enum BufferType
    {
      POSITION
    };

    // Linearized sparse neighbourhood matrix
    struct LinearProbabilityMatrix
    {
      std::vector<uint32_t> neighbours;
      std::vector<float> probabilities;
      std::vector<int> indices;
    };

    GpgpuSneRaster::GpgpuSneRaster() :
      _initialized(false),
      _adaptive_resolution(true),
      _resolutionScaling(PIXEL_RATIO)
    {

    }

    Bounds2D GpgpuSneRaster::computeEmbeddingBounds(const embedding_type* embedding, float padding) {
      const float* points = embedding->getContainer().data();

      Bounds2D bounds;
      bounds.min.x = std::numeric_limits<float>::max();
      bounds.max.x = -std::numeric_limits<float>::max();
      bounds.min.y = std::numeric_limits<float>::max();
      bounds.max.y = -std::numeric_limits<float>::max();

      for (int i = 0; i < embedding->numDataPoints(); ++i) {
        float x = embedding->getContainer().data()[i * 2 + 0];
        float y = embedding->getContainer().data()[i * 2 + 1];

        bounds.min.x = std::min<float>(x, bounds.min.x);
        bounds.max.x = std::max<float>(x, bounds.max.x);
        bounds.min.y = std::min<float>(y, bounds.min.y);
        bounds.max.y = std::max<float>(y, bounds.max.y);
      }

      // Add any extra padding if requested
      if (padding != 0) {
        float half_padding = padding / 2;

        float x_padding = (bounds.max.x - bounds.min.x) * half_padding;
        float y_padding = (bounds.max.y - bounds.min.y) * half_padding;

        bounds.min.x -= x_padding;
        bounds.max.x += x_padding;
        bounds.min.y -= y_padding;
        bounds.max.y += y_padding;
      }

      return bounds;
    }

    void GpgpuSneRaster::initialize(const embedding_type* embedding, TsneParameters params, const sparse_scalar_matrix_type& P) {
      _params = params;
      _P = P;

      unsigned int num_points = embedding->numDataPoints();

      _gradient.resize(num_points * params._embedding_dimensionality, 0);
      _previous_gradient.resize(num_points * params._embedding_dimensionality, 0);
      _gain.resize(num_points*params._embedding_dimensionality, 1);
      _positive_forces.resize(num_points*_params._embedding_dimensionality);
      _negative_forces.resize(num_points*_params._embedding_dimensionality);

      _interpolated_fields.resize(4 * P.size());

      // Linearize sparse probability matrix
      LinearProbabilityMatrix linear_P;
      unsigned int num_pnts = embedding->numDataPoints();
      for (int i = 0; i < num_pnts; ++i) {
        linear_P.indices.push_back(linear_P.neighbours.size());
        int size = 0;
        for (const auto& pij : P[i]) {
          linear_P.neighbours.push_back(pij.first);
          linear_P.probabilities.push_back(pij.second);
          size++;
        }
        linear_P.indices.push_back(size);
      }

      // Compute initial data bounds
      _bounds = computeEmbeddingBounds(embedding);

      _function_support = 6.5f;

      // Initialize all OpenGL resources
      initializeOpenGL(num_points, linear_P);

      _initialized = true;
    }

    void GpgpuSneRaster::clean()
    {
      glDeleteBuffers(1, &_position_buffer);

      fieldComputation.clean();
    }

    void GpgpuSneRaster::initializeOpenGL(const unsigned int num_points, const LinearProbabilityMatrix& linear_P) {
      glClearColor(0, 0, 0, 0);

      // Create dummy vao and already bind the position buffer to it for the interpolation step
      glGenVertexArrays(1, &_dummy_vao);
      glBindVertexArray(_dummy_vao);

      glGenBuffers(1, &_position_buffer);
      glBindBuffer(GL_ARRAY_BUFFER, _position_buffer);
      glBufferData(GL_ARRAY_BUFFER, num_points * sizeof(float) * 2, 0, GL_STREAM_DRAW);

      glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);
      glEnableVertexAttribArray(0);
      glBindVertexArray(0);

      // Initialize the computation of the fields by splatting
      fieldComputation.init(_position_buffer, _function_support);

      // Load in shader programs
      try {
        _interp_program.create();

        _interp_program.addShader(VERTEX, interp_fields_vert);
        _interp_program.addShader(FRAGMENT, interp_fields_frag);
        
        _interp_program.build();
      }
      catch (const ShaderLoadingException& e) {
        std::cout << e.what() << std::endl;
      }

      _interp_program.bind();
      _interp_program.uniform1i("fields", 0);
      // Set up dummy framebuffer and texture for GPGPU computation
      glGenFramebuffers(1, &_dummy_fbo);
      glBindFramebuffer(GL_FRAMEBUFFER, _dummy_fbo);
      glGenTextures(1, &_dummy_tex);
      glBindTexture(GL_TEXTURE_2D, _dummy_tex);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
      glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, _dummy_tex, 0);
      GLenum DrawBuffers[1] = { GL_COLOR_ATTACHMENT0 };
      glDrawBuffers(1, DrawBuffers);
    }

    void GpgpuSneRaster::compute(embedding_type* embedding, float exaggeration, float iteration, float mult) {
      float* points = embedding->getContainer().data();

      glBindBuffer(GL_ARRAY_BUFFER, _position_buffer);
      glBufferSubData(GL_ARRAY_BUFFER, 0, embedding->numDataPoints() * sizeof(Point2D), points);
      // Compute the bounds of the given embedding and add a 10% border around it
      _bounds = computeEmbeddingBounds(embedding, 0.1f);
      Point2D range = _bounds.getRange();

      float aspect = range.x / range.y;

      uint32_t width = _adaptive_resolution ? std::max((unsigned int)(range.x * _resolutionScaling), MINIMUM_FIELDS_SIZE) : (int)(FIXED_FIELDS_SIZE * aspect);
      uint32_t height = _adaptive_resolution ? std::max((unsigned int)(range.y * _resolutionScaling), MINIMUM_FIELDS_SIZE) : FIXED_FIELDS_SIZE;

      // Compute the fields texture
      fieldComputation.compute(width, height, _function_support, embedding->numDataPoints(), _bounds.min.x, _bounds.max.x, _bounds.min.y, _bounds.max.y);

      interpolateFields(embedding->numDataPoints(), width, height);
      // Compute the gradients of the KL-function
      computeGradients(embedding, embedding->numDataPoints(), exaggeration);

      // Update the point positions
      updateEmbedding(embedding, exaggeration, iteration, mult);

      //std::cout << "Bounds: " << boundsTime << " Fields: " << fieldsTime << " Gradients: " << gradientTime << " Update: " << updateTime << std::endl;
    }

    void GpgpuSneRaster::interpolateFields(unsigned int num_points, unsigned int width, unsigned int height)
    {
      // Bind dummy fbo with empty texture to store interpolated field values
      int fbo_size = ceil(sqrt(num_points));

      glBindTexture(GL_TEXTURE_2D, _dummy_tex);
      glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, fbo_size, fbo_size, 0, GL_RGBA, GL_FLOAT, 0);

      glBindFramebuffer(GL_FRAMEBUFFER, _dummy_fbo);
      glClear(GL_COLOR_BUFFER_BIT);
      glViewport(0, 0, fbo_size, fbo_size);

      // Bind fields texture for bilinear sampling
      glActiveTexture(GL_TEXTURE0);
      glBindTexture(GL_TEXTURE_2D, fieldComputation.getFieldTexture());

      // Set shader and uniforms
      _interp_program.bind();
      _interp_program.uniform2f("minBounds", _bounds.min.x, _bounds.min.y);
      _interp_program.uniform2f("invRange", 1 / (_bounds.max.x - _bounds.min.x), 1 / (_bounds.max.y - _bounds.min.y));
      _interp_program.uniform2ui("fboSize", fbo_size, fbo_size);

      // Interpolate field values at points
      glBindVertexArray(_dummy_vao);
      glDrawArrays(GL_POINTS, 0, num_points);
      glBindVertexArray(0);

      // TEMP Read back interpolated values from framebuffer
      std::vector<float> interp_values(4 * fbo_size * fbo_size);
      glBindTexture(GL_TEXTURE_2D, _dummy_tex);
      glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_FLOAT, interp_values.data());

      std::memcpy(_interpolated_fields.data(), interp_values.data(), num_points * 4 * sizeof(float));

      glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void GpgpuSneRaster::computeGradients(embedding_type* embedding, unsigned int num_points, double exaggeration)
    {
      const uint32_t n_pnts = num_points;
      float* embedding_ptr = embedding->getContainer().data();
      float* interpolated_fields_ptr = _interpolated_fields.data();

      // Compute sumQ value
      float sum_Q = 0;
      for (int i = 0; i < n_pnts; i++)
      {
        sum_Q += std::max<float>(interpolated_fields_ptr[i * 4] - 1, 0.);
      }
      std::cout << sum_Q << std::endl;

      if (sum_Q == 0) {
        return;
      }

      for (int i = 0; i < n_pnts; ++i) {
        const float xi = embedding_ptr[i * 2];
        const float yi = embedding_ptr[i * 2 + 1];

        //computing positive forces
        double sum_positive_x = 0;
        double sum_positive_y = 0;

        for (const auto& pij : _P[i]) {
          const float xj = embedding_ptr[pij.first * 2];
          const float yj = embedding_ptr[pij.first * 2 + 1];
          const double dist_x = (xi - xj);
          const double dist_y = (yi - yj);
          const double qij = 1 / (1 + dist_x*dist_x + dist_y*dist_y);

          sum_positive_x += pij.second * qij * dist_x / (n_pnts);
          sum_positive_y += pij.second * qij * dist_y / (n_pnts);
        }

        //computing negative forces
        const double sum_negative_x = interpolated_fields_ptr[i * 4 + 1] / sum_Q;
        const double sum_negative_y = interpolated_fields_ptr[i * 4 + 2] / sum_Q;

        _gradient[i * 2 + 0] = 4 * (exaggeration*sum_positive_x - sum_negative_x);
        _gradient[i * 2 + 1] = 4 * (exaggeration*sum_positive_y - sum_negative_y);
      }
    }

    template <typename T>
    T sign(T x) { return (x == .0 ? .0 : (x < .0 ? -1.0 : 1.0)); }

    void GpgpuSneRaster::updateEmbedding(embedding_type* embedding, float exaggeration, float iteration, float mult) {
      for (int i = 0; i < _gradient.size(); ++i) {
        _gain[i] = static_cast<float>((sign(_gradient[i]) != sign(_previous_gradient[i])) ? (_gain[i] + .2) : (_gain[i] * .8));
        if (_gain[i] < _params._minimum_gain) {
          _gain[i] = static_cast<float>(_params._minimum_gain);
        }
        _gradient[i] = static_cast<float>((_gradient[i]>0 ? 1 : -1)*std::abs(_gradient[i] * _params._eta* _gain[i]) / (_params._eta*_gain[i]));

        _previous_gradient[i] = static_cast<float>(((iteration < _params._mom_switching_iter) ? _params._momentum : _params._final_momentum) * _previous_gradient[i] - _params._eta * _gain[i] * _gradient[i]);
        embedding->getContainer()[i] += static_cast<float>(_previous_gradient[i] * mult);
      }

      //MAGIC NUMBER
      if (exaggeration > 1.2) {
        embedding->scaleIfSmallerThan(0.1);
      }
      else {
        embedding->zeroCentered();
      }
    }
  }
}
