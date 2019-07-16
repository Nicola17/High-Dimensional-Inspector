#include "field_computation.h"

#include <cmath>
#include <vector>
#include <iostream>
#include <cstdint>

#define GLSL(version, shader)  "#version " #version "\n" #shader

const char* point_vert = GLSL(330,
  layout(location = 0) in vec2 point;

  uniform vec4 bounds;

  void main()
  {
    vec2 min_bounds = bounds.xy;
    vec2 max_bounds = bounds.zw;
    vec2 range = max_bounds - min_bounds;

    gl_Position = vec4(((point - min_bounds) / range) * 2 - 1, 0, 1);
  }
);

const char* point_frag = GLSL(330,
  out float fragColor;

  void main()
  {
    fragColor = 1;
  }
);

const char* raster_vsrc = GLSL(330,
  layout(location = 0) in vec2 vertex;
  layout(location = 1) in vec2 tex_coord;
  layout(location = 2) in vec2 position;

  out vec2 pass_tex_coord;

  uniform mat4 view_matrix;

  uniform vec2 support;

  void main() {
    pass_tex_coord = tex_coord;
    gl_Position = view_matrix * vec4(position, 0, 1) + vec4(support * vertex, 0, 0);
  }
);

const char* raster_fsrc = GLSL(330,
  in vec2 pass_tex_coord;

  uniform sampler2D tex;
  uniform sampler2D pixelTex;

  uniform vec2 invWindowSize;

  out vec4 outColor;

  void main() {
    //float shouldProcess = texture(pixelTex, gl_FragCoord.xy * invWindowSize).r;
    //if (shouldProcess < 0.5)
    //  discard;
    outColor = texture(tex, pass_tex_coord);
  }
);

const char* gpgpu_compute_fields_source = GLSL(430,
  layout(std430, binding = 0) buffer Pos{ vec2 Positions[]; };
  layout(std430, binding = 1) buffer BoundsInterface { vec2 Bounds[]; };
  layout(local_size_x = 256, local_size_y = 1, local_size_z = 1) in;

  layout(rgba32f, binding = 0) writeonly uniform image2D Fields;
  layout(r8, binding = 1) readonly uniform image2D Stencil;

  const uint groupSize = gl_WorkGroupSize.x;
  const uint hgSize = groupSize / 2;
  shared vec3 reduction_array[hgSize];

  uniform uint num_points;
  uniform uvec2 size;
  uniform float support;

  void main() {
    uint x = gl_WorkGroupID.x;
    uint y = gl_WorkGroupID.y;
    
    float mask = imageLoad(Stencil, ivec2(x, y)).x;
    if (mask == 0) return;

    uint lid = gl_LocalInvocationIndex.x;

    vec2 min_bounds = Bounds[0];
    vec2 max_bounds = Bounds[1];
    vec2 range = max_bounds - min_bounds;

    // Position of the pixel in the domain
    vec2 pixel_pos = ((vec2(x, y) + vec2(0.5)) / size) * range + min_bounds;

    vec4 value = vec4(0);

    for (uint i = lid; i < num_points; i += groupSize)
    {
      // Distance between pixel and kernel center in domain units
      vec2 t = pixel_pos - Positions[i];

      //if (abs(t.x) > support || abs(t.y) > support)
      //  continue;

      float eucl_sqrd = dot(t, t);

      float tstud = 1.0 / (1.0 + eucl_sqrd);
      float tstud2 = tstud*tstud;

      value.xyz += vec3(tstud, tstud2*t.x, tstud2*t.y);
    }
    if (lid >= hgSize) {
      reduction_array[lid - hgSize] = value.xyz;
    }
    barrier();
    if (lid < hgSize) {
      reduction_array[lid] += value.xyz;
    }
    for (uint reduceSize = hgSize/2; reduceSize > 1; reduceSize /= 2)
    {
      barrier();
      if (lid < reduceSize) {
        reduction_array[lid] += reduction_array[lid + reduceSize];
      }
    }
    barrier();
    if (lid < 1) {
      imageStore(Fields, ivec2(x, y), vec4(reduction_array[0] + reduction_array[1], 0));
    }
  }
);

void RasterFieldComputation::generateTSNEKernel(float kernel_radius, std::vector<float>& kernel, float function_support) {
  uint32_t kernel_width = kernel_radius * 2 + 1;

  kernel.resize(kernel_width*kernel_width * 4);

  const double pi = std::acos(-1);
  const float mult = 1. / std::sqrt(pi);

  for (int j = 0; j < kernel_width; ++j) {
    for (int i = 0; i < kernel_width; ++i) {
      const double x = (double(i) - kernel_radius) / kernel_radius*function_support;
      const double y = (double(j) - kernel_radius) / kernel_radius*function_support;
      const double eucl_sqrd = x*x + y*y;
      const double tstud = 1. / (1. + eucl_sqrd);

      const uint32_t id = (j*kernel_width + i) * 4;
      //assert(id + 3 < _kernel.size());
      kernel[id + 0] = tstud;
      kernel[id + 1] = tstud*tstud*x;
      kernel[id + 2] = tstud*tstud*y;
      kernel[id + 3] = 0;
    }
  }
}

void RasterFieldComputation::init(GLuint positionBuffer, float function_support)
{
  glEnable(GL_BLEND);
  glBlendFunc(GL_ONE, GL_ONE);

  // Load in shader programs
  try {
    _fields_program.create();
    _fields_program.addShader(VERTEX, raster_vsrc);
    _fields_program.addShader(FRAGMENT, raster_fsrc);
    _fields_program.build();
  }
  catch (const ShaderLoadingException& e) {
    std::cout << e.what() << std::endl;
  }

  float kernel_radius = 32;
  uint32_t kernel_width = kernel_radius * 2 + 1;

  // Generate fields texture
  glGenTextures(1, &_field_texture);
  glBindTexture(GL_TEXTURE_2D, _field_texture);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  // Extend field borders to not break bilinear sampling
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

  // Generate framebuffer
  glGenFramebuffers(1, &_field_fbo);
  glBindFramebuffer(GL_FRAMEBUFFER, _field_fbo);
  GLenum attachment = GL_COLOR_ATTACHMENT0;
  glFramebufferTexture(GL_FRAMEBUFFER, attachment, _field_texture, 0);
  GLenum attachments[1] = { attachment };
  glDrawBuffers((GLsizei)1, attachments);
  glBindFramebuffer(GL_FRAMEBUFFER, 0);

  std::vector<float> kernel(kernel_width * kernel_width * 4);
  generateTSNEKernel(kernel_radius, kernel, function_support);
  glGenTextures(1, &_kernel_texture);
  glBindTexture(GL_TEXTURE_2D, _kernel_texture);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, kernel_width, kernel_width, 0, GL_RGBA, GL_FLOAT, kernel.data());
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

  // Kernel VAO and VBO
  glGenVertexArrays(1, &_quad_vao);
  glBindVertexArray(_quad_vao);
  GLuint quad_buffer;
  glGenBuffers(1, &quad_buffer);

  std::vector<float> quadVertices = {
    -1, -1, 0, 0,
    1, -1, 1, 0,
    -1,  1, 0, 1,
    1,  1, 1, 1
  };

  glBindBuffer(GL_ARRAY_BUFFER, quad_buffer);
  glBufferData(GL_ARRAY_BUFFER, sizeof(float) * quadVertices.size(), quadVertices.data(), GL_STATIC_DRAW);
  glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (GLvoid*)(0 * sizeof(float)));
  glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (GLvoid*)(2 * sizeof(float)));
  glEnableVertexAttribArray(0);
  glEnableVertexAttribArray(1);

  glBindBuffer(GL_ARRAY_BUFFER, positionBuffer);
  glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, 0);
  glEnableVertexAttribArray(2);
  glVertexAttribDivisor(2, 1);
}

void RasterFieldComputation::clean()
{
  //glDeleteTextures(1, &_field_texture);
  glDeleteTextures(1, &_kernel_texture);
}

void RasterFieldComputation::compute(unsigned int width, unsigned int height, float function_support, unsigned int num_points, float x_min, float x_max, float y_min, float y_max)
{
  // Set the fields texture resolution
  glBindTexture(GL_TEXTURE_2D, _field_texture);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width, height, 0, GL_RGBA, GL_FLOAT, nullptr);

  glBindFramebuffer(GL_FRAMEBUFFER, _field_fbo);

  glClear(GL_COLOR_BUFFER_BIT);

  glViewport(0, 0, width, height);

  _fields_program.bind();

  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, _kernel_texture);
  _fields_program.uniform1i("tex", 0);

  //_fields_program.uniform2f("invWindowSize", 1.0f / _width, 1.0f / _height);

  _fields_program.uniform2f("support", 2*function_support / (x_max - x_min), 2*function_support / (y_max - y_min));

  for (int i = 0; i < 16; i++) _view_matrix[i] = 0;
  _view_matrix[0] = 2 / (x_max - x_min);
  _view_matrix[5] = 2 / (y_max - y_min);
  _view_matrix[10] = 1;
  _view_matrix[12] = -(x_max + x_min) / (x_max - x_min);
  _view_matrix[13] = -(y_max + y_min) / (y_max - y_min);
  _view_matrix[15] = 1;
  
  _fields_program.uniformMatrix4f("view_matrix", _view_matrix);

  glBindVertexArray(_quad_vao);

  //draws
  glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, num_points);
}

#ifndef __APPLE__

void ComputeFieldComputation::init(unsigned int num_points)
{
  // Load in shader programs
  try {
    _compute_program.create();
    _compute_program.addShader(COMPUTE, gpgpu_compute_fields_source);
    _compute_program.build();
    _stencil_program.create();
    _stencil_program.addShader(VERTEX, point_vert);
    _stencil_program.addShader(FRAGMENT, point_frag);
    _stencil_program.build();
  }
  catch (const ShaderLoadingException& e) {
    std::cout << e.what() << std::endl;
  }

  _compute_program.bind();
  _compute_program.uniform1ui("num_points", num_points);

  // Generate fields texture
  glGenTextures(1, &_field_texture);
  glBindTexture(GL_TEXTURE_2D, _field_texture);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  // Extend field borders to not break bilinear sampling
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

  // Generate stencil texture
  glGenTextures(1, &_stencil_texture);
  glBindTexture(GL_TEXTURE_2D, _stencil_texture);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  // Extend field borders to not break bilinear sampling
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

  // Generate framebuffer
  glGenFramebuffers(1, &_field_fbo);
  glBindFramebuffer(GL_FRAMEBUFFER, _field_fbo);
  GLenum attachment = GL_COLOR_ATTACHMENT0;
  glFramebufferTexture(GL_FRAMEBUFFER, attachment, _stencil_texture, 0);
  GLenum attachments[1] = { attachment };
  glDrawBuffers((GLsizei)1, attachments);
  glBindFramebuffer(GL_FRAMEBUFFER, 0);

  glGenVertexArrays(1, &_point_vao);
}

void ComputeFieldComputation::clean()
{
  glDeleteTextures(1, &_field_texture);

  glDeleteFramebuffers(1, &_field_fbo);

  _compute_program.destroy();
}

void ComputeFieldComputation::compute(unsigned int width, unsigned int height, float function_support, unsigned int num_points, GLuint position_buffer, GLuint bounds_buffer, float minx, float miny, float maxx, float maxy)
{
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, _stencil_texture);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, width, height, 0, GL_RED, GL_UNSIGNED_BYTE, nullptr);

  glViewport(0, 0, width, height);
  glBindFramebuffer(GL_FRAMEBUFFER, _field_fbo);
  _stencil_program.bind();
  _stencil_program.uniform4f("bounds", minx, miny, maxx, maxy);
  glBindVertexArray(_point_vao);
  glBindBuffer(GL_ARRAY_BUFFER, position_buffer);
  glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);
  glEnableVertexAttribArray(0);

  glPointSize(3);
  glDrawArrays(GL_POINTS, 0, num_points);

  glBindFramebuffer(GL_FRAMEBUFFER, 0);

  // Field computation
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, _field_texture);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width, height, 0, GL_RGBA, GL_FLOAT, nullptr);

  _compute_program.bind();

  _compute_program.uniform2ui("size", width, height);
  _compute_program.uniform1f("support", function_support);

  // Upload the points to the position buffer
  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, position_buffer);
  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, bounds_buffer);

  // Bind the fields texture for writing
  glBindImageTexture(0, _field_texture, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);
  glBindImageTexture(1, _stencil_texture, 0, GL_FALSE, 0, GL_READ_ONLY, GL_R8);

  // Compute the fields texture
  glDispatchCompute(width, height, 1);

  glMemoryBarrier(GL_TEXTURE_FETCH_BARRIER_BIT);
}

#endif
