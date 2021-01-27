/*
* This is a personal academic project. Dear PVS-Studio, please check it.
* PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com
*/
#include <pch.h>
#include "gui.h"

#include <gl/shaders/diffuseshader.h>
#include <gl/shaders/normalshader.h>
#include <gl/shaders/brdfshader.h>

#include "gl/camera.h"
#include "gl/object.h"
#include "gl/light.h"

#include "gl/vertexbufferobject.h"
#include "gl/linevertexbufferobject.h"
#include "gl/icospherevertexbufferobject.h"

#include "gl/framebufferobject.h"
#include "gl/openglscene.h"

#include "gl/vbos/cube.h"
#include "gl/vbos/plane.h"
#include "gl/vbos/icosahedron.h"
#include "gl/vbos/halficosahedron.h"
#include "gl/vbos/gizmo.h"

#include "gl/openglrenderer.h"
#include "embree/embreerenderer.h"

static bool show_demo_window = false;
static ImVec4 clear_color = ImVec4(1.0f, 1.0f, 1.0f, 1.00f);
static ImVec2 winSize;

static float yaw = M_PI / 4.0f;
static float pitch = M_PI / 4.0f;
//static float pitch = 0.3f;
static float dist = 7.0f;
static float thetha = M_PI / 4.0f; // <0, PI/2>
static float phi = M_PI - (M_PI / 4.0f); // <0, 2*PI>
static bool mouseInput = false;
static bool geometry = false;
static bool shallInvalidateRTC = false;

//static std::shared_ptr<LineVertexBufferObject> incidentVectorVBO = nullptr;
//static std::shared_ptr<LineVertexBufferObject> reflectedVectorVBO = nullptr;
//static std::shared_ptr<BRDFShader> brdfShader = nullptr;

static bool shallSave = false;

void Gui::init() {
  glfwSetErrorCallback(glfw_error_callback);
  if (!glfwInit())
    throw "Unable to init GLFW";
  
  // Decide GL+GLSL versions
#if __APPLE__
  // GL 3.2 + GLSL 150
    const char* glsl_version = "#version 150";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // Required on Mac
#else
  glsl_version = "#version 460";
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
//  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
//  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // 3.0+ only
#endif
  
  // Create window with graphics context
  window = glfwCreateWindow(1280, 720, winName_, NULL, NULL);
  if (window == NULL)
    throw "Unable to create window";
  glfwMakeContextCurrent(window);
  glfwSwapInterval(1); // Enable vsync
  
  // Initialize OpenGL loader
#if defined(IMGUI_IMPL_OPENGL_LOADER_GL3W)
  bool err = gl3wInit() != 0;
#elif defined(IMGUI_IMPL_OPENGL_LOADER_GLEW)
  bool err = glewInit() != GLEW_OK;
#elif defined(IMGUI_IMPL_OPENGL_LOADER_GLAD)
  bool err = gladLoadGL() == 0;
#else
  bool err = false; // If you use IMGUI_IMPL_OPENGL_LOADER_CUSTOM, your loader is likely to requires some form of initialization.
#endif
  if (err) {
    fprintf(stderr, "Failed to initialize OpenGL loader!\n");
    throw "Failed to initialize OpenGL loader";
  }

//  VertexBufferObject::setupStaticObjects();
  std::shared_ptr<VertexBufferObject> cube = nullptr;
  std::shared_ptr<VertexBufferObject> plane = nullptr;
  std::shared_ptr<VertexBufferObject> disk = nullptr;
  std::shared_ptr<VertexBufferObject> icosahedron = nullptr;
  std::shared_ptr<VertexBufferObject> halficosahedron = nullptr;
  std::shared_ptr<LineVertexBufferObject> gizmo = nullptr;
  
  cube = std::make_shared<VertexBufferObject>(
      std::vector<Vertex>(std::begin(vbo::cube::vertices), std::end(vbo::cube::vertices)),
      std::vector<unsigned int>(std::begin(vbo::cube::indices), std::end(vbo::cube::indices)));
  
  plane = std::make_shared<VertexBufferObject>(
      std::vector<Vertex>(std::begin(vbo::plane::vertices), std::end(vbo::plane::vertices)),
      std::vector<unsigned int>(std::begin(vbo::plane::indices), std::end(vbo::plane::indices)));
  
  icosahedron = std::make_shared<VertexBufferObject>(
      std::vector<Vertex>(std::begin(vbo::icosahedron::vertices), std::end(vbo::icosahedron::vertices)),
      std::vector<unsigned int>(std::begin(vbo::icosahedron::indices), std::end(vbo::icosahedron::indices)));
  
  halficosahedron = std::make_shared<VertexBufferObject>(
      std::vector<Vertex>(std::begin(vbo::halficosahedron::vertices), std::end(vbo::halficosahedron::vertices)),
      std::vector<unsigned int>(std::begin(vbo::halficosahedron::indices), std::end(vbo::halficosahedron::indices)));
  
  //generate disk
  {
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    const unsigned int resolution = 20;
    double radiansAngle = 0;
    const double radiansFullAngle = 2. * M_PI;
    const double radius = 1.;
    const double radiansStep = radiansFullAngle / static_cast<double>(resolution);
    vertices.emplace_back(glm::vec3(0, 0, 0), glm::vec3(0, 1, 0)); // center
    for (int i = 0; i < resolution; i++) {
      vertices.emplace_back(glm::vec3(radius * std::cos(radiansAngle), radius * std::sin(radiansAngle), 0), glm::vec3(0, 1, 0));
//      vertices.emplace_back(glm::vec3(radius * std::cos(radiansAngle), 0, radius * std::sin(radiansAngle)), glm::vec3(std::cos(radiansAngle), 0, std::sin(radiansAngle)));
      radiansAngle += radiansStep;

//      int vidx1 = i;
//      int vidx2 = (i + 1) % resolution;
      indices.emplace_back(0);
      indices.emplace_back(((i + 1) % resolution) + 1);
      indices.emplace_back(i + 1);
    }
    
    disk = std::make_shared<VertexBufferObject>(vertices, indices);
  }
  gizmo = std::make_shared<LineVertexBufferObject>(
      std::vector<Vertex>(std::begin(vbo::gizmo::vertices), std::end(vbo::gizmo::vertices)),
      std::vector<unsigned int>(std::begin(vbo::gizmo::indices), std::end(vbo::gizmo::indices)),
      5);

//  LineVertexBufferObject::setupStaticObjects();
  renderer_ = std::make_unique<OpenGLRenderer>();
  embreeRenderer_ = std::make_unique<EmbreeRenderer>(400, 400);
  
  fbo_ = std::make_unique<FrameBufferObject>(800, 800);
  
  std::shared_ptr<OpenGLScene> scene = std::make_shared<OpenGLScene>();
  
  std::shared_ptr<Camera> cam = std::make_shared<Camera>();
  std::shared_ptr<CameraTransformation> camTransformation = cam->transformation;
  camTransformation->overrideTarget.first = glm::vec3(10.f, 10.f, 0.f);
  camTransformation->overrideTarget.second = true;
  camTransformation->setPosition(glm::vec3(5, 5, 5));
  
  scene->addCamera(cam);
  
  std::shared_ptr<Shader> defShader = std::make_shared<DiffuseShader>("./Shaders/Default/VertexShader.glsl", "./Shaders/Default/FragmentShader.glsl");
  std::shared_ptr<Shader> normalShader = std::make_shared<NormalShader>("./Shaders/Normal/VertexShader.glsl", "./Shaders/Normal/FragmentShader.glsl");
  auto brdfShader = std::make_shared<BRDFShader>("./Shaders/brdf/VertexShader.glsl", "./Shaders/brdf/FragmentShader.glsl");
  brdfShader_ = brdfShader; // assign to weak ptr
  
  scene->addDefShader(defShader);
  
  defShader->addCamera(cam);
  defShader->addLight(scene->getLights());
  cam->addShader(defShader);
  
  normalShader->addCamera(scene->getCamera());
  normalShader->addLight(scene->getLights());
  cam->addShader(normalShader);
  
  brdfShader->addCamera(scene->getCamera());
  brdfShader->addLight(scene->getLights());
  cam->addShader(brdfShader);


//  scene->addObject(std::make_shared<Object>(VertexBufferObject::cube, normalShader));
//  scene->addObject(std::make_shared<Object>(VertexBufferObject::cube, defShader));
//  scene->addObject(std::make_shared<Object>(VertexBufferObject::plane, defShader));
  
  const std::vector<Vertex> vertices = {glm::vec3(0, 0, 0), glm::vec3(0.5, 0.5, 0.5)};
  const std::vector<unsigned int> indices = {0, 1};
  
  auto incidentVectorVBO = std::make_shared<LineVertexBufferObject>(vertices, indices, 4);
  auto reflectedVectorVBO = std::make_shared<LineVertexBufferObject>(vertices, indices, 4);
  
  incidentVectorVBO_ = incidentVectorVBO; // Assign to weak ptr
  reflectedVectorVBO_ = reflectedVectorVBO; // Assign to weak ptr
  
  scene->addObject(
      std::make_shared<Object>(incidentVectorVBO, defShader, nullptr,
                               std::make_shared<Material>("Incident vector mtl",
                                                          Color3f{{0.1f, 0.1f, 0.1f}},
                                                          Color3f{{1.0f, 0.647f, 0.0f}})));
  
  scene->addObject(
      std::make_shared<Object>(reflectedVectorVBO, defShader, nullptr,
                               std::make_shared<Material>("Reflected vector mtl",
                                                          Color3f{{0.1f, 0.1f, 0.1f}},
                                                          Color3f{{0.0f, 0.349, 1.0f}})));
  
  scene->addObject(std::make_shared<Object>(disk, defShader));
//  scene->addObject(std::make_shared<Object>(std::make_shared<IcosphereVertexBufferObject>(0), normalShader));
//  scene->addObject(std::make_shared<Object>(std::make_shared<IcosphereVertexBufferObject>(3), defShader));
//  scene->addObject(std::make_shared<Object>(std::make_shared<IcosphereVertexBufferObject>(3), normalShader));
  scene->addObject(std::make_shared<Object>(std::make_shared<IcosphereVertexBufferObject>(6), brdfShader));
  scene->addObject(std::make_shared<Object>(gizmo, normalShader));
  
  scene->addLight(std::make_shared<Light>(
      std::make_shared<Transformation>(
          glm::vec3(0.f, 2000.f, 0.f)), //Pos
      glm::vec4(1.f), //Diffusion
      0.5f, //Ambient
      0.8f, //Specular
                      LIGHT_INTENSITY_10),
                  cube); //Intenzita
  
  renderer_->addScene(scene, true);
}

void Gui::glfw_error_callback(int error, const char *description) {
  spdlog::error("[GLFW] {} {}", error, description);
}

void Gui::ui() {
  // 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
  if (show_demo_window)
    ImGui::ShowDemoWindow(&show_demo_window);
  
  // 2. Show a simple window that we create ourselves. We use a Begin/End pair to created a named window.
  {
    ImGui::Begin("Hello, world!");// Create a window called "Hello, world!" and append into it.
    
    ImGui::Checkbox("Show Demo Window", &show_demo_window);      // Edit bools storing our window open/close state
    
    ImGui::ColorEdit3("clear color", (float *) &clear_color); // Edit 3 floats representing a color
    
    renderer_->clearColor[0] = clear_color.x;
    renderer_->clearColor[1] = clear_color.y;
    renderer_->clearColor[2] = clear_color.z;
    
    ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate,
                ImGui::GetIO().Framerate);
    
    
    ImGui::Separator();
    ImGui::Text("Rendering");
    ImGui::Checkbox("Render edges", &geometry);
    shallSave = ImGui::Button("Save");
    ImGui::Separator();
    
    ImGui::Text("Camera info");
//    ImGui::SliderFloat("pitch", &pitch, -M_PI / 2., M_PI / 2.);
    ImGui::SliderFloat("yaw", &yaw, -2. * M_PI, 2. * M_PI);
    ImGui::SliderFloat("pitch", &pitch, -1.553, 1.553);
    ImGui::SliderFloat("dist", &dist, 0.0f, 100);
    
    ImGui::Separator();
    
    ImGui::Text("Incident beam");
    ImGui::SliderFloat("thetha", &thetha, 0, M_PI / 2.);
    ImGui::SliderFloat("phi", &phi, 0, 2. * M_PI);
    
    ImGui::Separator();
    
    #pragma region "BRDF setting"
    if (auto brdfShader = brdfShader_.lock()) {
      ImGui::Text("BRDF");
      int *selectedIdx = reinterpret_cast<int *>(&brdfShader->currentBrdfIdx);
      
      shallInvalidateRTC |= ImGui::Combo("Selected BRDF",                           // const char* label,
                                         selectedIdx,                              // int* current_item,
                                         &BRDFShader::imguiSelectionGetter,         // bool(*items_getter)(void* data, int idx, const char** out_text),
                                         (void *) BRDFShader::brdfArray,            // void* data
                                         IM_ARRAYSIZE(BRDFShader::brdfArray));// int items_count
      
      switch (brdfShader->currentBrdfIdx) {
        case BRDFShader::BRDF::Phong:
        case BRDFShader::BRDF::BlinnPhong: {
          shallInvalidateRTC |= ImGui::SliderInt("Shininess", &brdfShader->getBrdfUniformLocations().shininess.getData(), 1, 100);
          break;
        }
        
        case BRDFShader::BRDF::TorranceSparrow: {
          shallInvalidateRTC |= ImGui::SliderFloat("Roughness", &brdfShader->getBrdfUniformLocations().roughness.getData(), 0.001, 1.0);
          shallInvalidateRTC |= ImGui::SliderFloat("f0", &brdfShader->getBrdfUniformLocations().f0.getData(), 0, 1);
          break;
        }
        
        case BRDFShader::BRDF::CountBrdf:break;
      }
    } else {
      spdlog::warn("[GUI] BRDFShader ptr expired");
    }
    
    #pragma endregion
    ImGui::End();
  }
  
  
  {
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoScrollWithMouse;
    ImGui::Begin("Test drawing into window", nullptr, window_flags);
    ImGuiIO &io = ImGui::GetIO();
    
    glm::vec2 fboUv = fbo_->getUV();
    ImGui::Image((void *) (intptr_t) fbo_->getFrameBufferColorId(),
                 ImVec2(fbo_->getWidth(), fbo_->getHeight()),
                 ImVec2(0, 0),
                 ImVec2(fboUv.x, fboUv.y));
    
    
    if (ImGui::IsItemClicked(0)) {
      mouseInput = true;
    }
    
    if (mouseInput) {
      if (!io.MouseDown[0]) {
        mouseInput = false;
      } else {
        // Draw debug line
        ImGui::GetForegroundDrawList()->AddLine(io.MouseClickedPos[0], io.MousePos, ImGui::GetColorU32(ImGuiCol_Button),
                                                4.0f);
        
        yaw += glm::radians(io.MouseDelta.x);
        pitch += glm::radians(io.MouseDelta.y);

//        pitch = std::min<float>(pitch, M_PI / 2.f);
//        pitch = std::max<float>(pitch, -M_PI / 2.f);
        
        pitch = std::min<float>(pitch, 1.553);
        pitch = std::max<float>(pitch, -1.553);
        
        if (yaw > 2 * M_PI) yaw -= 2 * M_PI;
        if (yaw < -2 * M_PI) yaw += 2 * M_PI;
        
      }
    }
    if (ImGui::IsItemHovered()) {
      dist -= (io.MouseWheel * 0.5f);
      dist = std::max(dist, 0.0f);
    }
    winSize = ImGui::GetWindowSize();
    ImGui::End();
  }
  
  
}

Gui::Gui(const char *winName) : winName_{winName} {
  init();
}

Gui::~Gui() {
  // Delete pointers before GLFW
  auto rendererRaw = renderer_.release();
  delete rendererRaw;
  
  auto fboRaw = fbo_.release();
  delete fboRaw;
//  auto incidentVectorVBOraw = incidentVectorVBO.release();
//  auto reflectedVectorVBOraw = reflectedVectorVBO.release();
//  auto brdfShaderraw = brdfShader.release();

//  LineVertexBufferObject::deleteStaticObjects();
//  VertexBufferObject::deleteStaticObjects();
  
  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();
  glfwDestroyWindow(window);
  glfwTerminate();
}

void Gui::render() {
  preRenderInit();
  renderLoop();
}

void Gui::preRenderInit() {
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO &io = ImGui::GetIO();
  (void) io;
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;       // Enable Keyboard Controls
  //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
  io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;           // Enable Docking
  io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;         // Enable Multi-Viewport / Platform Windows
  //io.ConfigViewportsNoAutoMerge = true;
  //io.ConfigViewportsNoTaskBarIcon = true;
  
  // Setup Dear ImGui style
  ImGui::StyleColorsDark();
  //ImGui::StyleColorsClassic();
  
  //clear_color = ImGui::GetStyle().Colors[ImGuiCol_WindowBg];
  
  // When viewports are enabled we tweak WindowRounding/WindowBg so platform windows can look identical to regular ones.
  ImGuiStyle &style = ImGui::GetStyle();
  if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
    style.WindowRounding = 0.0f;
    style.Colors[ImGuiCol_WindowBg].w = 1.0f;
  }
  
  // Setup Platform/Renderer bindings
  ImGui_ImplGlfw_InitForOpenGL(window, true);
  ImGui_ImplOpenGL3_Init(glsl_version);

//  initTextures();
}

void Gui::renderLoop() {
// Main loop
  std::thread producerThread(&EmbreeRenderer::producer, this->embreeRenderer_.get());
  while (!glfwWindowShouldClose(window)) {
    // Poll and handle events (inputs, window resize, etc.)
    // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
    // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application.
    // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application.
    // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
    glfwPollEvents();
    
    // Start the Dear ImGui frame
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    
    static bool p_open = true;
    static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_None;
    
    {
      ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
      
      ImGuiViewport *viewport = ImGui::GetMainViewport();
      ImGui::SetNextWindowPos(viewport->Pos);
      ImGui::SetNextWindowSize(viewport->Size);
      ImGui::SetNextWindowViewport(viewport->ID);
      ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
      ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
      window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize |
                      ImGuiWindowFlags_NoMove;
      window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
      
      
      if (dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode)
        window_flags |= ImGuiWindowFlags_NoBackground;
      
      ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
      ImGui::Begin("DockSpace Demo", &p_open, window_flags);
      ImGui::PopStyleVar();
      
      ImGui::PopStyleVar(2);
      
      ImGuiIO &io = ImGui::GetIO();
      if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable) {
        ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
        ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);
      }
      ImGui::End();
    }
    
    auto cam = renderer_->getCurrentScene()->getCamera();
    cam->ratio = fbo_->getRatio();
    
    
    glm::vec3 newCamPos;
    
    newCamPos.x = std::cos(yaw) * std::cos(pitch);
    newCamPos.y = std::sin(yaw) * std::cos(pitch);
    newCamPos.z = std::sin(pitch);
    
    glm::normalize(newCamPos);
    newCamPos *= dist;
    
    cam->transformation->setPosition(newCamPos);
    {
      std::vector<Vertex> incidentVertices;
      std::vector<Vertex> reflectedVertices;
      std::vector<unsigned int> indices = {0, 1};
      
      glm::vec3 incidentVector = glm::vec3(
          std::sin(thetha) * std::cos(phi),
          std::sin(thetha) * std::sin(phi),
          std::cos(thetha));
      if (auto brdfShader = brdfShader_.lock()) {
        brdfShader->getBrdfUniformLocations().incidentVector.getData() = incidentVector;
      } else {
        spdlog::warn("[GUI] BRDFShader ptr expired");
      }
      
      glm::vec3 reflectedVector = glm::vec3(-incidentVector[0], -incidentVector[1], incidentVector[2]);
      
      incidentVertices.emplace_back(glm::vec3(0, 0, 0));
      incidentVertices.emplace_back(incidentVector);
      
      reflectedVertices.emplace_back(glm::vec3(0, 0, 0));
      reflectedVertices.emplace_back(reflectedVector);
      
      
      if (auto incidentVectorVBO = incidentVectorVBO_.lock()) {
        incidentVectorVBO->recreate(incidentVertices, indices);
      } else {
        spdlog::warn("[GUI] incidentVectorVBO ptr expired");
      }
      
      if (auto reflectedVectorVBO = reflectedVectorVBO_.lock()) {
        reflectedVectorVBO->recreate(reflectedVertices, indices);
      } else {
        spdlog::warn("[GUI] reflectedVectorVBO ptr expired");
      }
    }
    
    if (shallInvalidateRTC) {
      embreeRenderer_->invalidateRendering();
    }
    
    fbo_->resize(winSize.x, winSize.y);
    fbo_->bind();
    renderer_->render(geometry);
    fbo_->unbind();
    if (shallSave) fbo_->saveScreen();
    
    shallInvalidateRTC = false;
    ui();
    embreeRenderer_->ui();
    
    // Rendering
    ImGui::Render();
    int display_w, display_h;
    glfwGetFramebufferSize(window, &display_w, &display_h);
    glViewport(0, 0, display_w, display_h);
    glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    
    // Update and Render additional Platform Windows
    // (Platform functions may change the current OpenGL context, so we save/restore it to make it easier to paste this code elsewhere.
    //  For this specific demo app we could also call glfwMakeContextCurrent(window) directly)
    if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
      GLFWwindow *backup_current_context = glfwGetCurrentContext();
      ImGui::UpdatePlatformWindows();
      ImGui::RenderPlatformWindowsDefault();
      glfwMakeContextCurrent(backup_current_context);
    }
    glfwSwapBuffers(window);
  }
  embreeRenderer_->finishRendering();
  producerThread.join();
}