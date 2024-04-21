#pragma once
#include <glm/glm.hpp>
#include <unordered_map>
#include <string>

std::vector<std::string> split(std::string line, std::string delimiter);
int random_int_in_range(int range);
float random_float();

enum class ObjectType {
    ePeste,

};

static std::vector<const char*> objNames = {
    "models/peste.obj",       //Peste

};

static std::vector<const char*> mtlNames = {
     "models/peste.mtl",         //Peste

};

static std::vector<float> scales = {
    1.0f,  //Peste

};

struct Mesh {
    uint32_t elementCount, VAO, VBO, EBO, material;
};

constexpr float windowWidth = 1920.0f;
constexpr float windowHeight = 1080.0f;

constexpr uint32_t maxObjectCount = 1;
constexpr uint32_t objectTypeCount = 1;