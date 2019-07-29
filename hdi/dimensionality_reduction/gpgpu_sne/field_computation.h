#pragma once

#ifdef __APPLE__
    #include <OpenGL/gl3.h>
#else // __APPLE__
    #include "hdi/utils/glad/glad.h"
#endif // __APPLE__

#include "hdi/data/shader.h"

#include <iostream>

class RasterFieldComputation
{
public:
  void generateTSNEKernel(float kernel_radius, std::vector<float>& kernel, float function_support);

  void init(GLuint positionBuffer, float function_support);

  void clean();

  void compute(unsigned int width, unsigned int height, float function_support, unsigned int num_points, float x_min, float x_max, float y_min, float y_max);

  GLuint getFieldTexture()
  {
    return _field_texture;
  }

  void storeField(unsigned int width, unsigned int height, float* pixels)
  {
    glBindFramebuffer(GL_FRAMEBUFFER, _field_fbo);
    glReadPixels(0, 0, width, height, GL_RGBA, GL_FLOAT, pixels);
  }

private:
  ShaderProgram _fields_program;

  GLuint _field_texture;
  GLuint _kernel_texture;

  GLuint _field_fbo;

  GLuint _quad_vao;

  float _view_matrix[16];
};


#ifndef __APPLE__

class ComputeFieldComputation
{
public:
  void init(unsigned int num_points);

  void clean();

  void compute(unsigned int width, unsigned int height, float function_support, unsigned int num_points, GLuint position_buffer, GLuint bounds_buffer, float minx, float miny, float maxx, float maxy);

  GLuint getFieldTexture()
  {
    return _field_texture;
  }

private:
  ShaderProgram _compute_program;
  ShaderProgram _stencil_program;

  GLuint _point_vao;

  GLuint _field_texture;
  GLuint _stencil_texture;

  GLuint _field_fbo;
};

#endif
