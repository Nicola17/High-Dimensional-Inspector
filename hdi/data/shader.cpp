#include "shader.h"

#include <sstream>

struct ShaderCompilationException : public ErrorMessageException
{
  ShaderCompilationException(std::string error)
    : ErrorMessageException(error)
  { }
};

struct ShaderLinkageException : public ErrorMessageException
{
  ShaderLinkageException(std::string error)
    : ErrorMessageException(error)
  { }
};

struct ShaderValidationException : public ErrorMessageException
{
  ShaderValidationException(std::string error)
    : ErrorMessageException(error)
  { }
};

Shader::Shader(ShaderType type) :
  _type(type),
  _is_created(false),
  _is_compiled(false),
  _handle(0)
{

}

bool Shader::isCompiled() const
{
  return _is_compiled;
}

bool Shader::loadFromString(const char* source)
{
  create();
  
  glShaderSource(_handle, 1, &source, nullptr);

  return true;
}

void Shader::create()
{
  GLenum gl_type;
  switch (_type)
  {
  case VERTEX: gl_type = GL_VERTEX_SHADER; break;
  case FRAGMENT: gl_type = GL_FRAGMENT_SHADER; break;
  case GEOMETRY: gl_type = GL_GEOMETRY_SHADER; break;
#ifndef __APPLE__
  case COMPUTE: gl_type = GL_COMPUTE_SHADER; break;
#endif // __APPLE__
  }

  _handle = glCreateShader(gl_type);

  _is_created = true;
  _is_compiled = false;
}

void Shader::compile()
{
  glCompileShader(_handle);

  GLint status;
  glGetShaderiv(_handle, GL_COMPILE_STATUS, &status);

  _is_compiled = status == GL_TRUE;

  if (!_is_compiled)
  {
    std::string errorLog = getInfoLog();
    destroy();
    throw ShaderCompilationException(errorLog);
  }
}

void Shader::destroy()
{
  if (_handle != 0)
    glDeleteShader(_handle);

  _is_created = false;
  _is_compiled = false;
}

std::string Shader::getInfoLog()
{
  std::string prefix;
  switch (_type)
  {
  case VERTEX:   prefix = "Vertex shader info log:\n"; break;
  case GEOMETRY: prefix = "Geometry shader info log:\n"; break;
  case FRAGMENT: prefix = "Fragment shader info log:\n"; break;
  case COMPUTE:  prefix = "Compute shader info log:\n"; break;
  }

  GLint log_length;
  glGetShaderiv(_handle, GL_INFO_LOG_LENGTH, &log_length);

  std::vector<GLchar> log(log_length);
  glGetShaderInfoLog(_handle, log_length, nullptr, log.data());
  return prefix + std::string(log.begin(), log.end());
}




ShaderProgram::ShaderProgram() :
  _is_created(false),
  _is_linked(false),
  _is_validated(false),
  _handle(0)
{

}

void ShaderProgram::addShader(ShaderType type, const char* source)
{
  Shader shader(type);
  shader.loadFromString(source);
  attach(shader);
}

void ShaderProgram::build()
{
  if (!isCreated())
    return;

  try
  {
    for (Shader& shader : _attached_shaders)
      shader.compile();

    link();
    validate();

    for (Shader& shader : _attached_shaders)
      shader.destroy();
  }
  catch (ErrorMessageException& e)
  {
    destroy();

    // Visual studio 2013 and under do not support the full C++11 standard
    #if (_MSC_VER <= 1800)
        throw;
    #else
        throw ShaderLoadingException(e);
    #endif
  }
}

bool ShaderProgram::isCreated()
{
  return _is_created;
}

bool ShaderProgram::isLinked()
{
  return _is_linked;
}

bool ShaderProgram::isValidated()
{
  return _is_validated;
}

std::string ShaderProgram::getError()
{
  std::ostringstream ss;
  for (Shader& shader : _attached_shaders)
  {
    ss << shader.getInfoLog() << '\n';
  }
  ss << "Shader program info log:\n";
  ss << getInfoLog();

  return ss.str();
}

void ShaderProgram::create()
{
  if (_is_created)
    destroy();

  _handle = glCreateProgram();

  _is_created = true;
  _is_linked = false;
  _is_validated = false;
  _attached_shaders.clear();
}

void ShaderProgram::bind()
{
  glUseProgram(_handle);
}

void ShaderProgram::release()
{
  glUseProgram(0);
}

void ShaderProgram::destroy()
{
  for (Shader& shader : _attached_shaders)
    shader.destroy();

  // Crashes for reasons unknown, probably because the shader is
  // being kept alive by the exception throwing
  //_attached_shaders.clear();

  if (_handle != 0)
  {
    glDeleteProgram(_handle);
    _handle = 0;
  }

  _is_created = false;
  _is_linked = false;
  _is_validated = false;
}

void ShaderProgram::link()
{
  glLinkProgram(_handle);

  GLint status = 0;
  glGetProgramiv(_handle, GL_LINK_STATUS, &status);

  _is_linked = status == GL_TRUE;

  if (!_is_linked) throw ShaderLinkageException(getInfoLog());
}

void ShaderProgram::validate()
{
  glValidateProgram(_handle);

  GLint status = 0;
  glGetProgramiv(_handle, GL_VALIDATE_STATUS, &status);

  _is_validated = status == GL_TRUE;
}

void ShaderProgram::attach(const Shader& shader)
{
  glAttachShader(_handle, shader._handle);
  _attached_shaders.push_back(shader);
}

void ShaderProgram::detachAll()
{
  for (Shader& shader : _attached_shaders)
  {
    glDetachShader(_handle, shader._handle);
  }
}

std::string ShaderProgram::getInfoLog()
{
  GLint logLength;
  glGetProgramiv(_handle, GL_INFO_LOG_LENGTH, &logLength);

  std::vector<GLchar> log(logLength);
  glGetProgramInfoLog(_handle, logLength, nullptr, log.data());
  return std::string(log.begin(), log.end());
}

GLint ShaderProgram::getUniformLocation(const char* name)
{
  std::unordered_map<std::string, int>::const_iterator it = _location_map.find(std::string(name));
  if (it != _location_map.end()) {
    return it->second;
  }
  else {
    int location = glGetUniformLocation(_handle, name);
    _location_map[std::string(name)] = location;
    return location;
  }
}

void ShaderProgram::uniform1i(const char* name, int i)
{
  glUniform1i(getUniformLocation(name), i);
}

void ShaderProgram::uniform1ui(const char* name, unsigned int i)
{
  glUniform1ui(getUniformLocation(name), i);
}

void ShaderProgram::uniform1iv(const char* name, int count, int* values)
{
  glUniform1iv(getUniformLocation(name), count, (GLint*)values);
}

void ShaderProgram::uniform2i(const char* name, int v0, int v1)
{
  glUniform2i(getUniformLocation(name), v0, v1);
}

void ShaderProgram::uniform2ui(const char* name, unsigned int v0, unsigned int v1)
{
  glUniform2ui(getUniformLocation(name), v0, v1);
}

void ShaderProgram::uniform1f(const char* name, float value)
{
  glUniform1f(getUniformLocation(name), value);
}

void ShaderProgram::uniform1fv(const char* name, int count, float* values)
{
  glUniform1fv(getUniformLocation(name), count, (GLfloat*)values);
}

void ShaderProgram::uniform2f(const char* name, float v0, float v1)
{
  glUniform2f(getUniformLocation(name), v0, v1);
}

void ShaderProgram::uniform3f(const char* name, float v0, float v1, float v2)
{
  glUniform3f(getUniformLocation(name), v0, v1, v2);
}

void ShaderProgram::uniform3f(const char* name, const float* v)
{
  glUniform3f(getUniformLocation(name), v[0], v[1], v[2]);
}

void ShaderProgram::uniform3fv(const char* name, int count, const float* values)
{
  glUniform3fv(getUniformLocation(name), count, (GLfloat*)values);
}

void ShaderProgram::uniform4f(const char* name, float v0, float v1, float v2, float v3)
{
  glUniform4f(getUniformLocation(name), v0, v1, v2, v3);
}

void ShaderProgram::uniformMatrix4f(const char* name, const float* m)
{
  glUniformMatrix4fv(getUniformLocation(name), 1, false, m);
}
