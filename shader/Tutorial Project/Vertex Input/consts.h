#pragma once
const int WINDOW_WIDTH = 1280;
const int WINDOW_HEIGHT = 720;

const int NUM_VERTEX_ATTRIBUTES = 4;
const int NUM_ATTACHMENTS = 2;

const int NUM_LIGHTS = 4;

const float Z_NEAR = 0.1f;
const float Z_FAR = 1024.f;

// One for the scene and one for renderables
const int NUM_DESCRIPTOR_SET_LAYOUTS = 2;

const std::string CUBE_MODEL_PATH = "Models/cube.obj";
const std::string SPHERE_MODEL_PATH = "Models/HQ_sphere.obj";
const std::string COEURL_TEXTURE_PATH = "Textures/coeurl.png";
const std::string DEFAULT_TEXTURE_PATH = "Textures/default.png";