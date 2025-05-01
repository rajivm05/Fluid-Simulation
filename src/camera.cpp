#include "camera.h"

Camera::Camera(glm::vec3 pos_i, glm::vec3 target_i, glm::vec3 up_i, float fov_i,
        float width_i, float height_i, float near_i, float far_i): pos(pos_i), target(target_i), up(up_i), fov(fov_i),
                                                                   width(width_i), height(height_i), near(near_i), far(far_i) {
    view = glm::lookAt(pos, target, up);
    projection = glm::perspective(fov, width / height, near, far);
}
