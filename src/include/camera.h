#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

class Camera {
private:
    glm::vec3 pos;
    glm::vec3 target;
    glm::vec3 up;

    float fov;
    float width;
    float height;
    float near;
    float far;

public:
    glm::mat4 transform;

    Camera(glm::vec3 pos, glm::vec3 target, glm::vec3 up, float fov,
        float width, float height, float near, float far);
};
