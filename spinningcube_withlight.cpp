// Copyright (C) 2020 Emilio J. Padrón
// Released as Free Software under the X11 License
// https://spdx.org/licenses/X11.html

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <stdio.h>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

// GLM library to deal with matrix operations
#include <glm/glm.hpp>
#include <glm/mat4x4.hpp>               // glm::mat4
#include <glm/gtc/matrix_transform.hpp> // glm::translate, glm::rotate, glm::perspective
#include <glm/gtc/type_ptr.hpp>

#include "textfile_ALT.h"

int gl_width = 640;
int gl_height = 480;

void glfw_window_size_callback(GLFWwindow *window, int width, int height);
void processInput(GLFWwindow *window);
void render(double);
unsigned int loadTexture(const char *path);

GLuint shader_program = 0;                          // shader program to set render pipeline
GLuint vao = 0;                                     // Vertext Array Object to set input data
GLint model_location, view_location, proj_location; // Uniforms for transformation matrices

// Shader names
const char *vertexFileName = "spinningcube_withlight_vs.glsl";
const char *fragmentFileName = "spinningcube_withlight_fs.glsl";

// Camera
glm::vec3 camera_pos(0.0f, 0.0f, 3.0f);

// Lighting
glm::vec3 light_pos(5.0f, 0.0f, 1.0f);
glm::vec3 light_ambient(0.2f, 0.1f, 0.1f);
glm::vec3 light_diffuse(0.5f, 0.1f, 0.1f);
glm::vec3 light_specular(1.0f, 0.0f, 0.0f);

glm::vec3 light_pos2(-5.0f, 0.0f, 1.0f);
glm::vec3 light_ambient2(0.1f, 0.2f, 0.1f);
glm::vec3 light_diffuse2(0.1f, 0.5f, 0.1f);
glm::vec3 light_specular2(0.0f, 1.0f, 0.0f);

// Material
const GLfloat material_shininess = 85.0f;

// Textures
unsigned int diffuseMap, specularMap;

int main()
{
  // start GL context and O/S window using the GLFW helper library
  if (!glfwInit())
  {
    fprintf(stderr, "ERROR: could not start GLFW3\n");
    return 1;
  }

  // glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  // glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  // glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
  // glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

  GLFWwindow *window = glfwCreateWindow(gl_width, gl_height, "My spinning cube", NULL, NULL);
  if (!window)
  {
    fprintf(stderr, "ERROR: could not open window with GLFW3\n");
    glfwTerminate();
    return 1;
  }
  glfwSetWindowSizeCallback(window, glfw_window_size_callback);
  glfwMakeContextCurrent(window);

  // start GLEW extension handler
  // glewExperimental = GL_TRUE;
  glewInit();

  // get version info
  const GLubyte *vendor = glGetString(GL_VENDOR);                        // get vendor string
  const GLubyte *renderer = glGetString(GL_RENDERER);                    // get renderer string
  const GLubyte *glversion = glGetString(GL_VERSION);                    // version as a string
  const GLubyte *glslversion = glGetString(GL_SHADING_LANGUAGE_VERSION); // version as a string
  printf("Vendor: %s\n", vendor);
  printf("Renderer: %s\n", renderer);
  printf("OpenGL version supported %s\n", glversion);
  printf("GLSL version supported %s\n", glslversion);
  printf("Starting viewport: (width: %d, height: %d)\n", gl_width, gl_height);

  // Enable Depth test: only draw onto a pixel if fragment closer to viewer
  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_LESS); // set a smaller value as "closer"

  // Vertex Shader
  char *vertex_shader = textFileRead(vertexFileName);

  // Fragment Shader
  char *fragment_shader = textFileRead(fragmentFileName);

  // Shaders compilation
  GLuint vs = glCreateShader(GL_VERTEX_SHADER);
  glShaderSource(vs, 1, &vertex_shader, NULL);
  free(vertex_shader);
  glCompileShader(vs);

  int success;
  char infoLog[512];
  glGetShaderiv(vs, GL_COMPILE_STATUS, &success);
  if (!success)
  {
    glGetShaderInfoLog(vs, 512, NULL, infoLog);
    printf("ERROR: Vertex Shader compilation failed!\n%s\n", infoLog);

    return (1);
  }

  GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
  glShaderSource(fs, 1, &fragment_shader, NULL);
  free(fragment_shader);
  glCompileShader(fs);

  glGetShaderiv(fs, GL_COMPILE_STATUS, &success);
  if (!success)
  {
    glGetShaderInfoLog(fs, 512, NULL, infoLog);
    printf("ERROR: Fragment Shader compilation failed!\n%s\n", infoLog);

    return (1);
  }

  // Create program, attach shaders to it and link it
  shader_program = glCreateProgram();
  glAttachShader(shader_program, fs);
  glAttachShader(shader_program, vs);
  glLinkProgram(shader_program);

  glValidateProgram(shader_program);
  glGetProgramiv(shader_program, GL_LINK_STATUS, &success);
  if (!success)
  {
    glGetProgramInfoLog(shader_program, 512, NULL, infoLog);
    printf("ERROR: Shader Program linking failed!\n%s\n", infoLog);

    return (1);
  }

  // Release shader objects
  glDeleteShader(vs);
  glDeleteShader(fs);

  // Vertex Array Object
  glGenVertexArrays(1, &vao);
  glBindVertexArray(vao);

  // Cube to be rendered
  //
  //          0        3
  //       7        4 <-- top-right-near
  // bottom
  // left
  // far ---> 1        2
  //       6        5
  //
  const GLfloat vertex_positions[] = {
      -0.25f, -0.25f, -0.25f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f, // 1
      -0.25f, 0.25f, -0.25f, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f,  // 0
      0.25f, -0.25f, -0.25f, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f,  // 2

      0.25f, 0.25f, -0.25f, 0.0f, 0.0f, -1.0f, 1.0f, 1.0f,  // 3
      0.25f, -0.25f, -0.25f, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f, // 2
      -0.25f, 0.25f, -0.25f, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f, // 0

      0.25f, -0.25f, -0.25f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, // 2
      0.25f, 0.25f, -0.25f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f,  // 3
      0.25f, -0.25f, 0.25f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f,  // 5

      0.25f, 0.25f, 0.25f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f,  // 4
      0.25f, -0.25f, 0.25f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, // 5
      0.25f, 0.25f, -0.25f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, // 3

      0.25f, -0.25f, 0.25f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f,  // 5
      0.25f, 0.25f, 0.25f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f,   // 4
      -0.25f, -0.25f, 0.25f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, // 6

      -0.25f, 0.25f, 0.25f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f,  // 7
      -0.25f, -0.25f, 0.25f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, // 6
      0.25f, 0.25f, 0.25f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f,   // 4

      -0.25f, -0.25f, 0.25f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f,  // 6
      -0.25f, 0.25f, 0.25f, -1.0f, 0.0f, 0.0f, 1.0f, 0.0f,   // 7
      -0.25f, -0.25f, -0.25f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f, // 1

      -0.25f, 0.25f, -0.25f, -1.0f, 0.0f, 0.0f, 1.0f, 1.0f,  // 0
      -0.25f, -0.25f, -0.25f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f, // 1
      -0.25f, 0.25f, 0.25f, -1.0f, 0.0f, 0.0f, 1.0f, 0.0f,   // 7

      0.25f, -0.25f, -0.25f, 0.0f, -1.0f, 0.0f, 0.0f, 0.0f,  // 2
      0.25f, -0.25f, 0.25f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f,   // 5
      -0.25f, -0.25f, -0.25f, 0.0f, -1.0f, 0.0f, 0.0f, 1.0f, // 1

      -0.25f, -0.25f, 0.25f, 0.0f, -1.0f, 0.0f, 1.0f, 1.0f,  // 6
      -0.25f, -0.25f, -0.25f, 0.0f, -1.0f, 0.0f, 0.0f, 1.0f, // 1
      0.25f, -0.25f, 0.25f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f,   // 5

      0.25f, 0.25f, 0.25f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f,  // 4
      0.25f, 0.25f, -0.25f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, // 3
      -0.25f, 0.25f, 0.25f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, // 7

      -0.25f, 0.25f, -0.25f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f, // 0
      -0.25f, 0.25f, 0.25f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f,  // 7
      0.25f, 0.25f, -0.25f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f,  // 3
  };

  // Vertex Buffer Object (for vertex coordinates)
  GLuint vbo = 0;
  glGenBuffers(1, &vbo);
  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertex_positions), vertex_positions, GL_STATIC_DRAW);

  // Vertex attributes
  // 0: vertex position (x, y, z)
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), NULL);
  glEnableVertexAttribArray(0);

  // 1: vertex normals (x, y, z)
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *)(3 * sizeof(float)));
  glEnableVertexAttribArray(1);

  // 2: text coords (s, t)
  glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *)(6 * sizeof(float)));
  glEnableVertexAttribArray(2);

  // Unbind vbo (it was conveniently registered by VertexAttribPointer)
  glBindBuffer(GL_ARRAY_BUFFER, 0);

  // Unbind vao
  glBindVertexArray(0);

  // load textures
  diffuseMap = loadTexture("diffuse.png");
  specularMap = loadTexture("specular.png");

  glUseProgram(shader_program);

  // Uniforms
  // - Model matrix
  // - View matrix
  // - Projection matrix
  // - Normal matrix: normal vectors from local to world coordinates
  // - Camera position
  // - Light data
  // - Material data
  model_location = glGetUniformLocation(shader_program, "model");
  view_location = glGetUniformLocation(shader_program, "view");
  proj_location = glGetUniformLocation(shader_program, "projection");

  GLuint loc;

  // View attribute
  loc = glGetUniformLocation(shader_program, "view_pos");
  glUniform3fv(loc, 1, glm::value_ptr(camera_pos));

  // Material attributes
  loc = glGetUniformLocation(shader_program, "material.diffuse");
  glUniform1i(loc, 0);
  loc = glGetUniformLocation(shader_program, "material.specular");
  glUniform1i(loc, 1);
  loc = glGetUniformLocation(shader_program, "material.shininess");
  glUniform1f(loc, material_shininess);

  // Light attributes
  loc = glGetUniformLocation(shader_program, "light1.position");
  glUniform3fv(loc, 1, glm::value_ptr(light_pos));
  loc = glGetUniformLocation(shader_program, "light1.ambient");
  glUniform3fv(loc, 1, glm::value_ptr(light_ambient));
  loc = glGetUniformLocation(shader_program, "light1.diffuse");
  glUniform3fv(loc, 1, glm::value_ptr(light_diffuse));
  loc = glGetUniformLocation(shader_program, "light1.specular");
  glUniform3fv(loc, 1, glm::value_ptr(light_specular));

  loc = glGetUniformLocation(shader_program, "light2.position");
  glUniform3fv(loc, 1, glm::value_ptr(light_pos2));
  loc = glGetUniformLocation(shader_program, "light2.ambient");
  glUniform3fv(loc, 1, glm::value_ptr(light_ambient2));
  loc = glGetUniformLocation(shader_program, "light2.diffuse");
  glUniform3fv(loc, 1, glm::value_ptr(light_diffuse2));
  loc = glGetUniformLocation(shader_program, "light2.specular");
  glUniform3fv(loc, 1, glm::value_ptr(light_specular2));

  // [...]

  // Render loop
  while (!glfwWindowShouldClose(window))
  {
    processInput(window);

    render(glfwGetTime());

    glfwSwapBuffers(window);

    glfwPollEvents();
  }

  glfwTerminate();

  return 0;
}

void render(double currentTime)
{
  float f = (float)currentTime * 0.3f;

  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  //glClearColor(0.6, 0.6, 0.6, 1);

  glViewport(0, 0, gl_width, gl_height);

  glBindVertexArray(vao);

  glm::mat4 model_matrix, view_matrix, proj_matrix;

  // View, model, projection operations
  view_matrix = glm::lookAt(camera_pos, glm::vec3(0.f, 0.f, -1.f), glm::vec3(0.f, 1.f, 0.f));
  glUniformMatrix4fv(view_location, 1, GL_FALSE, glm::value_ptr(view_matrix));

  model_matrix = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, -1.0f));
  model_matrix = glm::translate(model_matrix,
                                glm::vec3(sinf(2.1f * f) * 0.5f,
                                          cosf(1.7f * f) * 0.5f,
                                          sinf(1.3f * f) * cosf(1.5f * f) * 2.0f));

  model_matrix = glm::rotate(model_matrix,
                             glm::radians((float)currentTime * 45.0f),
                             glm::vec3(0.0f, 1.0f, 0.0f));
  model_matrix = glm::rotate(model_matrix,
                             glm::radians((float)currentTime * 81.0f),
                             glm::vec3(1.0f, 0.0f, 0.0f));

  glUniformMatrix4fv(model_location, 1, GL_FALSE, glm::value_ptr(model_matrix));

  proj_matrix = glm::perspective(glm::radians(50.0f),
                                 (float)gl_width / (float)gl_height,
                                 0.1f, 1000.0f);
  glUniformMatrix4fv(proj_location, 1, GL_FALSE, glm::value_ptr(proj_matrix));

  // Texture binding
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, diffuseMap);
  glActiveTexture(GL_TEXTURE1);
  glBindTexture(GL_TEXTURE_2D, specularMap);

  glDrawArrays(GL_TRIANGLES, 0, 36);

  //Second cube
  model_matrix = glm::scale(glm::mat4(1.0f), glm::vec3(0.5f));
  model_matrix = glm::translate(model_matrix, glm::vec3(-1.5f, -1.5f, -1.0f));
  model_matrix = glm::translate(model_matrix,
                                glm::vec3(sinf(-2.1f * f) * 0.5f,
                                          cosf(-1.7f * f) * 0.5f,
                                          sinf(-1.3f * f) * cosf(1.5f * f) * 2.0f));
  model_matrix = glm::rotate(model_matrix,
                             glm::radians((float)currentTime * -45.0f),
                             glm::vec3(0.0f, 1.0f, 0.0f));
  model_matrix = glm::rotate(model_matrix,
                             glm::radians((float)currentTime * -81.0f),
                             glm::vec3(1.0f, 0.0f, 0.0f));
  glUniformMatrix4fv(model_location, 1, GL_FALSE, glm::value_ptr(model_matrix));

  // Texture binding
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, diffuseMap);
  glActiveTexture(GL_TEXTURE1);
  glBindTexture(GL_TEXTURE_2D, specularMap);

  glDrawArrays(GL_TRIANGLES, 0, 36);
}

void processInput(GLFWwindow *window)
{
  if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
    glfwSetWindowShouldClose(window, 1);
}

// Callback function to track window size and update viewport
void glfw_window_size_callback(GLFWwindow *window, int width, int height)
{
  gl_width = width;
  gl_height = height;
  printf("New viewport: (width: %d, height: %d)\n", width, height);
}

unsigned int loadTexture(char const *path)
{
  unsigned int textureID;
  glGenTextures(1, &textureID);

  int width, height, nrComponents;
  unsigned char *data = stbi_load(path, &width, &height, &nrComponents, 0);
  if (data)
  {
    GLenum format;
    if (nrComponents == 1)
      format = GL_RED;
    else if (nrComponents == 3)
      format = GL_RGB;
    else if (nrComponents == 4)
      format = GL_RGBA;

    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    stbi_image_free(data);
  }
  else
  {
    printf("Texture failed to load");
    stbi_image_free(data);
  }

  return textureID;
}