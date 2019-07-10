#define GLSL(version, shader)  "#version " #version "\n" #shader

const char* vsrc = GLSL(330,
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

const char* fsrc = GLSL(330,
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

const char* interp_fields_vert = GLSL(330,
  uniform sampler2D fields;

  uniform vec2 minBounds;
  uniform vec2 invRange;
  
  uniform uvec2 fboSize;
  
  layout(location = 0) in vec2 point;
  
  // Interpolated value of fields for the given point drawn to this pixel
  flat out vec3 interpValue;
  
  void main() {
    // Normalize point to [0-1] range
    vec2 normPoint = (point - minBounds) * invRange;
    // Bilinearly sample the fields at the location of the point
    interpValue = texture(fields, normPoint).xyz;
    
    // Compute sequential position of the point in output array-like texture
    // Calculate size of pixels
    vec2 step = vec2(2, 2) / vec2(fboSize.x, fboSize.y);
      
    // Calculate ID of pixel
    uint id = uint(gl_VertexID);
    uvec2 coords = uvec2(id % fboSize.x, id / fboSize.y);
    
    // Get coordinates of pixel centers in [0, 1] space
    vec2 pointPosition = vec2(-1, -1) + step * coords + (step * 0.5);
    gl_Position = vec4(pointPosition, 0, 1);
  }
);

const char* interp_fields_frag = GLSL(330,
  // Interpolated value of fields for the given point drawn to this pixel
  flat in vec3 interpValue;
  
  layout(location = 0) out vec4 fragColor;
  
  void main() {
    fragColor = vec4(interpValue, 1);
  }
);
