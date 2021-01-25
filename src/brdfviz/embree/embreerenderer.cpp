/*
* This is a personal academic project. Dear PVS-Studio, please check it.
* PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com
*/
#include <pch.h>
#include <common/utils/rng.h>
#include <common/utils/rgb.h>
#include <common/utils/math.h>
#include "embreerenderer.h"
#include "rtccommonshader.h"
#include "rtcamera.h"
#include "pathtracerhelper.h"

EmbreeRenderer::EmbreeRenderer(const int width, const int height) : width_(width), height_(height) {
  texData_.reserve(width_ * height_);
  
  for (int y = 0; y < height_; ++y) {
    for (int x = 0; x < width_; ++x) {
      const int offset = (y * width_ + x);
      texData_[offset] = glm::vec4(0, 0, 1, 1);
    }
  }
  
  glCall(glCreateTextures(GL_TEXTURE_2D, 1, &texID_));
  glCall(glBindTexture(GL_TEXTURE_2D, texID_));
  glCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST));
  glCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST));
  glCall(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width_, height_, 0, GL_RGBA, GL_FLOAT, texData_.data()));
  glCall(glBindTexture(GL_TEXTURE_2D, 0));
  
  commonShader_ = std::make_unique<RTCCommonShader>();
  commonShader_->pathTracerHelper = std::make_unique<PathTracerHelper>(width, height);
  commonShader_->camera_ = std::make_unique<RTCamera>(width, height, deg2rad(45.0), glm::vec3(5, 5, 5), glm::vec3(0, 0, 0));
  auto mtl = std::make_shared<Material>();
  auto sphere = std::make_unique<Sphere>(glm::vec3(0, 0, 0), 1.0f, mtl);
  commonShader_->mathScene_ = std::make_unique<MathScene>();
  commonShader_->mathScene_->spheres.emplace_back(std::move(sphere));
  commonShader_->useShader = RTCShadingType::PathTracing;
}


void EmbreeRenderer::ui() {
  {
    std::lock_guard<std::mutex> lock(texDataLock_);
    
    glCall(glBindTexture(GL_TEXTURE_2D, texID_));
    glCall(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width_, height_, 0, GL_RGBA, GL_FLOAT, texData_.data()));
    glCall(glBindTexture(GL_TEXTURE_2D, 0));
  }
  
  ImGui::Begin("Image", nullptr, ImGuiWindowFlags_NoResize);
  ImGui::Image((void *) (intptr_t) texID_, ImVec2(float(width_), float(height_)));
  ImGui::End();
}

glm::vec4 EmbreeRenderer::getPixel(int x, int y, float t) {
  try {
    return commonShader_->getPixel(x, y);
  }
  catch (const std::exception &e) {
    return glm::vec4{rng(), rng(), rng(), 1.0f};
  }
}

void EmbreeRenderer::producer() {
  std::vector<glm::vec4> localData;
  localData.reserve(width_ * height_);
  
  float t = 0.0f; // time
  auto t0 = std::chrono::high_resolution_clock::now();
  
  // refinenment loop
  while (!finishRequest_.load(std::memory_order_acquire)) {
    auto t1 = std::chrono::high_resolution_clock::now();
    std::chrono::duration<float> dt = t1 - t0;
    producerTime_ = dt.count();
    t += producerTime_;
    t0 = t1;
    
    // compute rendering

#pragma omp parallel for schedule(dynamic, 4) shared(localData)
    for (int y = 0; y < height_; ++y) {
      for (int x = 0; x < width_; ++x) {
        const glm::vec4 pixel = getPixel(x, y, t);
        const int offset = (y * width_ + x);
        
        localData[offset].x = /*c_srgb(*/pixel.r/*, gamma_)*/;
        localData[offset].y = /*c_srgb(*/pixel.g/*, gamma_)*/;
        localData[offset].z = /*c_srgb(*/pixel.b/*, gamma_)*/;
        localData[offset].w = 1;//c_srgb(pixel.a);
        // TODO check
//        if (finishRequest_.load(std::memory_order_acquire)) {
//          return;
//        }
      }
    }
    
    // write rendering results
    {
      std::lock_guard<std::mutex> lock(texDataLock_);
      memcpy(texData_.data(), localData.data(), width_ * height_ * sizeof(glm::vec4));
    } // lock release
  }
}

const std::atomic<bool> &EmbreeRenderer::getFinishRequest() const {
  return finishRequest_;
}

void EmbreeRenderer::finishRendering() {
  finishRequest_.store(true, std::memory_order_release);
}