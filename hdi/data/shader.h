#pragma once

#ifdef __APPLE__
    #include <OpenGL/gl3.h>
#else // __APPLE__
    #include "hdi/utils/glad/glad.h"
#endif // __APPLE__

#include <exception>

#include <string>
#include <vector>
#include <unordered_map>

struct ErrorMessageException : public std::exception
{
  ErrorMessageException(std::string error)
    : _error(error)
  { }

  ErrorMessageException(const std::exception& e)
    : _error(e.what())
  { }

  const char* what() const throw()
  {
    return _error.c_str();
  }

private:
  std::string _error;
};

struct ShaderLoadingException : public ErrorMessageException
{
  using ErrorMessageException::ErrorMessageException;
};

enum ShaderType
{
  VERTEX, GEOMETRY, FRAGMENT, COMPUTE
};

class Shader
{
  friend class ShaderProgram;

public:
  /**
  * Creates a new shader object of the given type
  *
  * @param type The type this shader should be
  */
  Shader(ShaderType type);

  bool isCompiled() const;

  bool loadFromString(const char* source);
  void compile();
  void destroy();

  std::string getInfoLog();
private:
  void create();

  ShaderType _type;

  bool _is_created;
  bool _is_compiled;

  GLuint _handle;
};

class ShaderProgram
{
public:
  ShaderProgram();
  void addShader(ShaderType type, const char* source);
  void build();
  std::string getError();

  bool isCreated();
  bool isLinked();
  bool isValidated();

  void create();
  void bind();
  void release();
  void destroy();

  void uniform1i(const char* name, int i);
  void uniform1ui(const char* name, unsigned int i);
  void uniform1iv(const char* name, int count, int* values);
  void uniform2i(const char* name, int v0, int v1);
  void uniform2ui(const char* name, unsigned int v0, unsigned int v1);
  void uniform1f(const char* name, float value);
  void uniform1fv(const char* name, int count, float* values);
  void uniform2f(const char* name, float v0, float v1);
  void uniform3f(const char* name, float v0, float v1, float v2);
  void uniform3f(const char* name, const float* v);
  void uniform3fv(const char* name, int count, const float* values);
  void uniform4f(const char* name, float v0, float v1, float v2, float v3);
  void uniformMatrix4f(const char* name, const float* m);

private:
  void link();
  void validate();
  void attach(const Shader& shader);
  void detachAll();

  std::string getInfoLog();
  GLint getUniformLocation(const char* name);

private:
  GLuint _handle;

  bool _is_created;
  bool _is_linked;
  bool _is_validated;

  std::vector<std::string> error_log;
  std::vector<Shader> _attached_shaders;
  std::unordered_map<std::string, int> _location_map;
};
