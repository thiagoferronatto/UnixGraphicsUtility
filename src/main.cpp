#include <algorithm>

#include "external/glm/ext.hpp"
#include "external/glm/glm.hpp"
#include "external/libaffa/aa.h"
#include "gl_util.hpp"
#include "window.hpp"

// int and float typedefs
#if 1
using i8 = signed char;
using u8 = unsigned char;
using i16 = signed short;
using u16 = unsigned short;
using i32 = signed int;
using u32 = unsigned int;
using i64 = signed long long;
using u64 = unsigned long long;
using f32 = float;
using f64 = double;
using f128 = long double;
#endif

#define USE_AA_INSTEAD_OF_IA

#ifdef USE_AA_INSTEAD_OF_IA
using Type = AAF;
#else
using Type = interval;
#endif

struct TriangleMesh {
  std::vector<glm::vec3> vertices;
  std::vector<glm::vec3> normals;
  std::vector<glm::uvec3> triangles;

  // OpenGL stuff
  GLuint vbos[2]{};
  GLuint ebo{};
  size_t elementCount{};
};

struct AxisAlignedBox {
  glm::vec3 min;
  glm::vec3 max;
};

using AAB = AxisAlignedBox;

auto getBoxFromBezierPatch(auto &&ctrlPts, const Type &u, const Type &v) {
  Type B[4];
  B[0] = B[3] = 1;
  B[1] = B[2] = 3;

  auto u2 = u * u, u3 = u2 * u;
  auto uc = Type{1} - u, uc2 = uc * uc, uc3 = uc2 * uc;
  auto v2 = v * v, v3 = v2 * v;
  auto vc = Type{1} - v, vc2 = vc * vc, vc3 = vc2 * vc;
  Type U[4]{1, u, u2, u3};
  Type UC[4]{1, uc, uc2, uc3};
  Type V[4]{1, v, v2, v3};
  Type VC[4]{1, vc, vc2, vc3};

  Type xhat;
  Type yhat;
  Type zhat;
  for (i32 i{}; i < 4; ++i) {
    auto BU = B[i] * U[i] * UC[3 - i];
    for (i32 j{}; j < 4; ++j) {
      auto BV = B[j] * V[j] * VC[3 - j];
      auto p = ctrlPts[4 * i + j];
      auto BUBV = BU * BV;
      xhat = xhat + BUBV * p.x;
      yhat = yhat + BUBV * p.y;
      zhat = zhat + BUBV * p.z;
    }
  }

#ifdef USE_AA_INSTEAD_OF_IA

#if 0 // trying to get the zonotope
  xhat.aafprint();
  yhat.aafprint();
  zhat.aafprint();

  f64 xsum{};
  for (u32 i{}, len{xhat.get_length()}; i < len; ++i)
    xsum += xhat.get_coeff(i);

  f64 xtrunc[5]{
      xhat.get_center(), // x
      xhat.get_coeff(0), // eps_0
      xhat.get_coeff(1), // eps_1
      xhat.get_coeff(2), // eps_2
      xsum               // eps_3 + eps_4 + ... + eps_n
  };
#endif

  auto xi{xhat.convert()};
  auto yi{yhat.convert()};
  auto zi{zhat.convert()};
#else
  auto xi{xhat};
  auto yi{yhat};
  auto zi{zhat};
#endif

  return AAB{{xi.left(), yi.left(), zi.left()},
             {xi.right(), yi.right(), zi.right()}};
}

auto tessellateBezierPatch(auto &&controlPoints, u32 level) {
  TriangleMesh mesh;
  auto stepSize{1.0f / level};
  f32 u{};
  f32 v{};

  for (u32 i{}; i < level; ++i, u += stepSize) {
    for (u32 j{}; j < level; ++j, v += stepSize) {
      f32 B[4];
      B[0] = B[3] = 1;
      B[1] = B[2] = 3;

      auto u2{u * u}, u3{u2 * u};
      auto uc{1 - u}, uc2{uc * uc}, uc3{uc2 * uc};
      auto v2{v * v}, v3{v2 * v};
      auto vc{1 - v}, vc2{vc * vc}, vc3{vc2 * vc};
      f32 U[4]{1, u, u2, u3};
      f32 DU[4]{0, 1, 2 * u, 3 * u2};
      f32 UC[4]{1, uc, uc2, uc3};
      f32 DUC[4]{0, -1, 2 * u - 2, -3 * uc2};
      f32 V[4]{1, v, v2, v3};
      f32 DV[4]{0, 1, 2 * v, 3 * v2};
      f32 VC[4]{1, vc, vc2, vc3};
      f32 DVC[4]{0, -1, 2 * v - 2, -3 * vc2};

      glm::vec3 s{};
      glm::vec3 du{};
      glm::vec3 dv{};
      for (i32 i{}; i < 4; ++i) {
        auto BU{B[i] * U[i] * UC[3 - i]};
        auto DBU{B[i] * (DU[i] * UC[3 - i] + U[i] * DUC[3 - i])};
        for (i32 j{}; j < 4; ++j) {
          auto BV{B[j] * V[j] * VC[3 - j]};
          auto DBV{B[j] * (DV[j] * VC[3 - j] + V[j] * DVC[3 - j])};
          auto p{controlPoints[4 * i + j]};
          s += BU * BV * p;
          du += DBU * BV * p;
          dv += BU * DBV * p;
        }
      }
      mesh.vertices.push_back(std::move(s));
      auto vertexNormal{glm::normalize(glm::cross(dv, du))};
      mesh.normals.push_back(std::move(vertexNormal));
    }
    v = 0;
  }
  for (u32 i{level}, end{level * level}; i < end; ++i) {
    if (i % level == level - 1)
      continue;
    mesh.triangles.push_back({i - level, i, i + 1});
    mesh.triangles.push_back({i + 1, i - level + 1, i - level});
  }
  return mesh;
}

constexpr auto vsSrc{R"(
    #version 460

    layout (location = 0) in vec3 position;
    layout (location = 1) in vec3 normal;

    uniform mat4 view;
    uniform mat4 proj;

    out vec3 fragPosition;
    out vec3 fragNormal;

    void main(void) {
      fragPosition = position;
      fragNormal = normal;
      gl_Position = proj * view * vec4(position, 1);
    }
  )"};

constexpr auto fsSrc{R"(
    #version 460

    uniform mat4 view;
    uniform vec3 camPos;
    uniform vec3 diffuse;
    uniform int shade;

    in vec3 fragNormal;
    in vec3 fragPosition;

    out vec4 fragColor;

    void main(void) {
      if (shade == 0) {
        fragColor = vec4(diffuse, 0.1);
        return;
      }

      vec3 normal = normalize(fragNormal);

      const float ambient = 0.1;

      vec3 lights[6];
      lights[0] = vec3( 1, 3,  -4);

      vec3 v = camPos - fragPosition;
      float camDist = length(v);
      v /= camDist;

      vec3 color = ambient * diffuse;

      for (int i = 0; i < 1; ++i) {
        vec3 lPos = lights[i];

        vec3 l = lPos - fragPosition;
        float lightDist = length(l);
        l /= lightDist;
        float invd = 1.0 /  lightDist;
        float d = dot(normal, l);
        d = 0.5 * d + 0.5;
        d *= d;

        vec3 h = normalize(v + l);
        const float alpha = 100;
        float s = pow(max(dot(normal, h), 0.0), alpha);

        const float intensity = 5;

        color += min(invd * invd, 1.0) * intensity * (d * diffuse + s);
      }

      fragColor = vec4(color, 1);
    }
  )"};

auto setupProgram() {
  auto vs = glCreateShader(GL_VERTEX_SHADER);
  glShaderSource(vs, 1, &vsSrc, nullptr);
  glCompileShader(vs);
  glCheckShaderCompilation(vs);

  auto fs = glCreateShader(GL_FRAGMENT_SHADER);
  glShaderSource(fs, 1, &fsSrc, nullptr);
  glCompileShader(fs);
  glCheckShaderCompilation(fs);

  auto program = glCreateProgram();
  glAttachShader(program, vs);
  glAttachShader(program, fs);
  glLinkProgram(program);
  glCheckProgramLinkage(program);

  glUseProgram(program);

  glEnable(GL_DEPTH_TEST);

  glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

  glClearColor(0.1, 0.1, 0.1, 1);

  return program;
}

GLuint vao{};
void drawMesh(TriangleMesh &mesh) {

  if (!mesh.vbos[0] || !mesh.vbos[1] || !mesh.ebo) {
    glDeleteBuffers(2, mesh.vbos);
    glDeleteBuffers(1, &mesh.ebo);
    glGenBuffers(2, mesh.vbos);
    glBindBuffer(GL_ARRAY_BUFFER, mesh.vbos[0]);
    auto vertexByteCount{mesh.vertices.size() * sizeof(glm::vec3)};
    glBufferData(GL_ARRAY_BUFFER, vertexByteCount, mesh.vertices.data(),
                 GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, mesh.vbos[1]);
    glBufferData(GL_ARRAY_BUFFER, vertexByteCount, mesh.normals.data(),
                 GL_STATIC_DRAW);
    glGenBuffers(1, &mesh.ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.ebo);
    auto triangleCount{mesh.triangles.size()};
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 triangleCount * sizeof(glm::uvec3),
                 mesh.triangles.data(), GL_STATIC_DRAW);
    mesh.elementCount = 3 * triangleCount;
  }

  if (!vao) {
    glCreateVertexArrays(1, &vao);
    glBindVertexArray(vao);
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
  }

  glBindBuffer(GL_ARRAY_BUFFER, mesh.vbos[0]);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
  glBindBuffer(GL_ARRAY_BUFFER, mesh.vbos[1]);
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.ebo);

  glDrawElements(GL_TRIANGLES, mesh.elementCount, GL_UNSIGNED_INT, nullptr);
}

TriangleMesh getMeshFromBox(const glm::vec3 &min, const glm::vec3 &max) {
  TriangleMesh mesh;

  // Define the 8 corners of the AABB
  glm::vec3 v0 = {min.x, min.y, min.z};
  glm::vec3 v1 = {min.x, min.y, max.z};
  glm::vec3 v2 = {max.x, min.y, max.z};
  glm::vec3 v3 = {max.x, min.y, min.z};
  glm::vec3 v4 = {min.x, max.y, min.z};
  glm::vec3 v5 = {min.x, max.y, max.z};
  glm::vec3 v6 = {max.x, max.y, max.z};
  glm::vec3 v7 = {max.x, max.y, min.z};

  // Add vertices for each face (flat shading requires duplication)
  mesh.vertices = {
      v0, v1, v2, v3, // Bottom face
      v4, v5, v6, v7, // Top face
      v1, v2, v6, v5, // Front face
      v0, v4, v7, v3, // Back face
      v0, v1, v5, v4, // Left face
      v2, v3, v7, v6  // Right face
  };

  // Add normals for each face (flat shading requires one normal per vertex)
  glm::vec3 nBottom = {0, -1, 0};
  glm::vec3 nTop = {0, 1, 0};
  glm::vec3 nFront = {0, 0, 1};
  glm::vec3 nBack = {0, 0, -1};
  glm::vec3 nLeft = {-1, 0, 0};
  glm::vec3 nRight = {1, 0, 0};

  mesh.normals = {
      nBottom, nBottom, nBottom, nBottom, // Bottom face
      nTop,    nTop,    nTop,    nTop,    // Top face
      nFront,  nFront,  nFront,  nFront,  // Front face
      nBack,   nBack,   nBack,   nBack,   // Back face
      nLeft,   nLeft,   nLeft,   nLeft,   // Left face
      nRight,  nRight,  nRight,  nRight   // Right face
  };

  // Add triangles (indices)
  mesh.triangles = {
      {0, 1, 2},    {2, 3, 0},    // Bottom face
      {4, 5, 6},    {6, 7, 4},    // Top face
      {8, 9, 10},   {8, 10, 11},  // Front face
      {14, 13, 12}, {15, 14, 12}, // Back face
      {16, 17, 18}, {16, 18, 19}, // Left face
      {20, 21, 22}, {20, 22, 23}  // Right face
  };

  return mesh;
}

TriangleMesh getMeshFromBezierPatchControlPoints(auto &&ctrlPts) {
  TriangleMesh mesh;
  for (auto &&ctrlPt : ctrlPts)
    mesh.vertices.push_back(ctrlPt);

  auto &p{mesh.vertices};
  auto &n{mesh.normals};

  n.push_back(glm::cross(p[1] - p[0], p[4] - p[0]));
  n.push_back(3.0f * p[1] - p[0] - p[2] - p[5]);
  n.push_back(3.0f * p[2] - p[1] - p[3] - p[6]);
  n.push_back(glm::cross(p[7] - p[3], p[2] - p[3]));

  n.push_back(3.0f * p[4] - p[0] - p[5] - p[8]);
  n.push_back(4.0f * p[5] - p[1] - p[4] - p[6] - p[9]);
  n.push_back(4.0f * p[6] - p[2] - p[5] - p[7] - p[10]);
  n.push_back(3.0f * p[7] - p[3] - p[6] - p[11]);

  n.push_back(3.0f * p[8] - p[4] - p[9] - p[12]);
  n.push_back(4.0f * p[9] - p[5] - p[8] - p[10] - p[13]);
  n.push_back(4.0f * p[10] - p[6] - p[9] - p[11] - p[14]);
  n.push_back(3.0f * p[11] - p[7] - p[10] - p[15]);

  n.push_back(glm::cross(p[8] - p[12], p[13] - p[12]));
  n.push_back(3.0f * p[13] - p[9] - p[12] - p[14]);
  n.push_back(3.0f * p[14] - p[10] - p[13] - p[15]);
  n.push_back(glm::cross(p[14] - p[15], p[11] - p[15]));

  mesh.triangles.push_back({0, 1, 5});
  mesh.triangles.push_back({5, 4, 0});
  mesh.triangles.push_back({1, 2, 6});
  mesh.triangles.push_back({6, 5, 1});
  mesh.triangles.push_back({2, 3, 7});
  mesh.triangles.push_back({7, 6, 2});

  mesh.triangles.push_back({4, 5, 9});
  mesh.triangles.push_back({9, 8, 4});
  mesh.triangles.push_back({5, 6, 10});
  mesh.triangles.push_back({10, 9, 5});
  mesh.triangles.push_back({6, 7, 11});
  mesh.triangles.push_back({11, 10, 6});

  mesh.triangles.push_back({8, 9, 13});
  mesh.triangles.push_back({13, 12, 8});
  mesh.triangles.push_back({9, 10, 14});
  mesh.triangles.push_back({14, 13, 9});
  mesh.triangles.push_back({10, 11, 15});
  mesh.triangles.push_back({15, 14, 10});

  return mesh;
}

AAB getBoxFromPatchControlPoints(auto &&ctrlPts) {
  AAB box;
  for (auto &&ctrlPt : ctrlPts) {
    box.min = glm::min(box.min, ctrlPt);
    box.max = glm::max(box.min, ctrlPt);
  }
  return box;
}

int main() {
#ifdef NDEBUG
  puts("Release");
#else
  puts("Debug");
#endif
  constexpr u64 w{1600}, h{900};
  Window window{w, h, "Surface tinkering"};

  TriangleMesh patchMesh;

  std::vector<glm::vec3> ctrlPts;
  /* Control points */ {
    ctrlPts.push_back({0, 1, 0});
    ctrlPts.push_back({1, -1, 0});
    ctrlPts.push_back({2, -1, 0});
    ctrlPts.push_back({3, 1, 0});

    ctrlPts.push_back({0, -1, -1});
    ctrlPts.push_back({1, 2, -1});
    ctrlPts.push_back({2, 2, -1});
    ctrlPts.push_back({3, -1, -1});

    ctrlPts.push_back({0, -1, -2});
    ctrlPts.push_back({1, 2, -2});
    ctrlPts.push_back({2, 2, -2});
    ctrlPts.push_back({3, -1, -2});

    ctrlPts.push_back({0, 1, -3});
    ctrlPts.push_back({1, -1, -3});
    ctrlPts.push_back({2, -1, -3});
    ctrlPts.push_back({3, 1, -3});

    for (auto &ctrlPt : ctrlPts) {
      static constexpr auto m4id{glm::identity<glm::mat4>()};
      static constexpr auto pi{glm::pi<float>()};
      static constexpr glm::vec3 up{0, 1, 0};
      auto matrix{glm::rotate(m4id, 0.25f * pi, up)};
      ctrlPt = matrix * glm::vec4{ctrlPt, 1};
      ctrlPt += glm::vec3{-1.5, 0, -1.5};
    }
  }

  patchMesh = tessellateBezierPatch(ctrlPts, 64);

  std::vector<TriangleMesh> boxMeshes;

  AAF asdf;

  // AABB generation
  auto generateAabbs{[](auto &&ctrlPts, auto &&meshVector, i32 subdCount) {
    meshVector.clear();
    float u{};
    float v{};
    auto incr{1.0f / subdCount};
    for (i32 i{}; i < subdCount; ++i, u += incr, v = 0) {
      for (i32 j{}; j < subdCount; ++j, v += incr) {
        auto box{getBoxFromBezierPatch(ctrlPts, interval{u, u + incr},
                                       interval{v, v + incr})};
        meshVector.push_back(getMeshFromBox(box.min, box.max));
      }
    }
  }};

  i32 subdCount{32};
  generateAabbs(ctrlPts, boxMeshes, subdCount);

  auto program{setupProgram()};

  auto viewLoc{glGetUniformLocation(program, "view")};
  auto projLoc{glGetUniformLocation(program, "proj")};
  auto camPosLoc{glGetUniformLocation(program, "camPos")};
  auto diffuseLoc{glGetUniformLocation(program, "diffuse")};
  auto shadeLoc{glGetUniformLocation(program, "shade")};

  auto camTrs{
      glm::inverse(glm::lookAt(glm::vec3{0, 3, 2}, {0, 2, 0}, {0, 1, 0}))};

  {
    auto view{glm::inverse(camTrs)};
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, &view[0].x);
    glUniform3fv(camPosLoc, 1, &camTrs[3].x);
  }
  auto proj{glm::perspective(glm::radians(74.0f), 16.0f / 9.0f, 0.01f, 100.0f)};
  glUniformMatrix4fv(projLoc, 1, GL_FALSE, &proj[0].x);

  bool wtcUpdated{};
  bool shouldShowAabbs{};
  bool pWasPressedLastFrame{};
  bool upWasPressedLastFrame{};
  bool downWasPressedLastFrame{};
  float dt{};

  glm::vec3 diffuse;
  glUniform1i(shadeLoc, 1);

  window.show();

  while (!window.shouldClose()) {
    auto start{std::chrono::steady_clock::now()};

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    /* User input */ {
      constexpr float factor{5};

      glm::vec3 globalY{glm::inverse(camTrs) * glm::vec4{0, 1, 0, 0}};

      if (window.keyIsPressed('W')) {
        camTrs = glm::translate(camTrs, {0, 0, -factor * dt});
        wtcUpdated = true;
      }

      if (window.keyIsPressed('A')) {
        camTrs = glm::translate(camTrs, {-factor * dt, 0, 0});
        wtcUpdated = true;
      }

      if (window.keyIsPressed('S')) {
        camTrs = glm::translate(camTrs, {0, 0, factor * dt});
        wtcUpdated = true;
      }

      if (window.keyIsPressed('D')) {
        camTrs = glm::translate(camTrs, {factor * dt, 0, 0});
        wtcUpdated = true;
      }

      if (window.keyIsPressed(GLFW_KEY_SPACE)) {
        camTrs = glm::translate(camTrs, factor * dt * globalY);
        wtcUpdated = true;
      }

      if (window.keyIsPressed(GLFW_KEY_LEFT_CONTROL)) {
        camTrs = glm::translate(camTrs, -factor * dt * globalY);
        wtcUpdated = true;
      }

      if (window.keyIsPressed('Q')) {
        camTrs = glm::rotate(camTrs, factor * dt, globalY);
        wtcUpdated = true;
      }

      if (window.keyIsPressed('E')) {
        camTrs = glm::rotate(camTrs, -factor * dt, globalY);
        wtcUpdated = true;
      }

      if (window.keyIsPressed('R')) {
        camTrs = glm::rotate(camTrs, factor * dt, {1, 0, 0});
        wtcUpdated = true;
      }

      if (window.keyIsPressed('F')) {
        camTrs = glm::rotate(camTrs, -factor * dt, {1, 0, 0});
        wtcUpdated = true;
      }

      if (window.keyIsPressed('P')) {
        if (!pWasPressedLastFrame)
          shouldShowAabbs = !shouldShowAabbs;
        pWasPressedLastFrame = true;
      } else {
        pWasPressedLastFrame = false;
      }

      if (window.keyIsPressed(GLFW_KEY_UP)) {
        if (!upWasPressedLastFrame) {
          subdCount += 5;
          if (subdCount > 128)
            subdCount = 128;
          generateAabbs(ctrlPts, boxMeshes, subdCount);
        }
        upWasPressedLastFrame = true;
      } else {
        upWasPressedLastFrame = false;
      }

      if (window.keyIsPressed(GLFW_KEY_DOWN)) {
        if (!downWasPressedLastFrame) {
          subdCount -= 5;
          if (subdCount < 1)
            subdCount = 2;
          generateAabbs(ctrlPts, boxMeshes, subdCount);
        }
        downWasPressedLastFrame = true;
      } else {
        downWasPressedLastFrame = false;
      }

      if (window.keyIsPressed(GLFW_KEY_ESCAPE))
        break;

      if (wtcUpdated) {
        auto view{glm::inverse(camTrs)};
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, &view[0].x);
        glUniform3fv(camPosLoc, 1, &camTrs[3].x);
        wtcUpdated = false;
      }
    }

    // Patch visualization
    diffuse = {1, 0, 0};
    glUniform3fv(diffuseLoc, 1, &diffuse.x);
    drawMesh(patchMesh);

    // AABB visualization
    if (shouldShowAabbs) {
      diffuse = {0, 1, 1};
      glUniform3fv(diffuseLoc, 1, &diffuse.x);

      // This is heavily unoptimized and should not be used for purposes other
      // than visualization. Currently, each box has its own separate mesh,
      // when they could all reference the same mesh with different
      // transformation matrices. Also, batch drawing would improve
      // performance, maybe by using something like glMultiDrawElements.
      for (auto &boxMesh : boxMeshes)
        drawMesh(boxMesh);
    }

    window.swapBuffers();
    window.pollEvents();

    // Frame time calculation
    auto end{std::chrono::steady_clock::now()};
    dt = 1e-9f * (end - start).count();
  }

  return 0;
}
