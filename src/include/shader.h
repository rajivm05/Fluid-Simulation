#ifndef SHADER_H
#define SHADER_H

#include <glm/glm.hpp>

class Shader {
public:
    unsigned int ID;
  
    Shader(const char* vertexPath, const char* fragmentPath);
    void use();

    void setInt(const char* name, const int val);
    void setMatrix(const char* name, const glm::mat4& val);
    void setVec2(const char* name, const glm::vec2& val);
    void setVec4(const char* name, const glm::vec4& val);
    void setVec3(const char* name, const glm::vec3& val);
    void setFloat(const char* name, const float val);
};
  
#endif
