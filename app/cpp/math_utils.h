#pragma once

#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>

namespace math {
struct Transform {
  glm::quat orientation;
  glm::vec3 position;
  glm::vec3 scale;
};

// The projection matrix transforms -Z=forward, +Y=up, +X=right to the appropriate clip space for the graphics API.
// The far plane is placed at infinity if far <= near.
// An infinite projection matrix is preferred for rasterization because, except for
// things *right* up against the near plane, it always provides better precision:
//              "Tightening the Precision of Perspective Rendering"
//              Paul Upchurch, Mathieu Desbrun
//              Journal of Graphics Tools, Volume 16, Issue 1, 2012
inline static glm::mat4 CreateProjectionFov(const XrFovf fov,
                                            const float near,
                                            const float far) {
  const float kTanAngleLeft = tanf(fov.angleLeft);
  const float kTanAngleRight = tanf(fov.angleRight);

  const float kTanAngleUp = tanf(fov.angleUp);
  const float kTanAngleDown = tanf(fov.angleDown);

  const float kTanAngleWidth = kTanAngleRight - kTanAngleLeft;
  const float kTanAngleHeight = kTanAngleUp - kTanAngleDown;
  glm::mat4 result;
  auto result_ptr = &result[0][0];
  if (far <= near) {
    // place the far plane at infinity
    result_ptr[0] = 2 / kTanAngleWidth;
    result_ptr[4] = 0;
    result_ptr[8] = (kTanAngleRight + kTanAngleLeft) / kTanAngleWidth;
    result_ptr[12] = 0;

    result_ptr[1] = 0;
    result_ptr[5] = 2 / kTanAngleHeight;
    result_ptr[9] = (kTanAngleUp + kTanAngleDown) / kTanAngleHeight;
    result_ptr[13] = 0;

    result_ptr[2] = 0;
    result_ptr[6] = 0;
    result_ptr[10] = -1;
    result_ptr[14] = -near;

    result_ptr[3] = 0;
    result_ptr[7] = 0;
    result_ptr[11] = -1;
    result_ptr[15] = 0;
  } else {
    // normal projection
    result_ptr[0] = 2 / kTanAngleWidth;
    result_ptr[4] = 0;
    result_ptr[8] = (kTanAngleRight + kTanAngleLeft) / kTanAngleWidth;
    result_ptr[12] = 0;

    result_ptr[1] = 0;
    result_ptr[5] = 2 / kTanAngleHeight;
    result_ptr[9] = (kTanAngleUp + kTanAngleDown) / kTanAngleHeight;
    result_ptr[13] = 0;

    result_ptr[2] = 0;
    result_ptr[6] = 0;
    result_ptr[10] = -far / (far - near);
    result_ptr[14] = -(far * near) / (far - near);

    result_ptr[3] = 0;
    result_ptr[7] = 0;
    result_ptr[11] = -1;
    result_ptr[15] = 0;
  }
  return result;
}
inline static glm::mat4 InvertRigidBody(const glm::mat4 src) {
  glm::mat4 result{};
  auto result_ptr = &result[0][0];
  auto src_ptr = &src[0][0];
  result_ptr[0] = src_ptr[0];
  result_ptr[1] = src_ptr[4];
  result_ptr[2] = src_ptr[8];
  result_ptr[3] = 0.0f;
  result_ptr[4] = src_ptr[1];
  result_ptr[5] = src_ptr[5];
  result_ptr[6] = src_ptr[9];
  result_ptr[7] = 0.0f;
  result_ptr[8] = src_ptr[2];
  result_ptr[9] = src_ptr[6];
  result_ptr[10] = src_ptr[10];
  result_ptr[11] = 0.0f;
  result_ptr[12] =
      -(src_ptr[0] * src_ptr[12] + src_ptr[1] * src_ptr[13] + src_ptr[2] * src_ptr[14]);
  result_ptr[13] =
      -(src_ptr[4] * src_ptr[12] + src_ptr[5] * src_ptr[13] + src_ptr[6] * src_ptr[14]);
  result_ptr[14] =
      -(src_ptr[8] * src_ptr[12] + src_ptr[9] * src_ptr[13] + src_ptr[10] * src_ptr[14]);
  result_ptr[15] = 1.0f;
  return result;
}
inline static glm::vec3 XrVector3FToGlm(XrVector3f xr_vector) {
  return {xr_vector.x, xr_vector.y, xr_vector.z};
}
inline static glm::quat XrQuaternionFToGlm(XrQuaternionf quaternion) {
  return {quaternion.w, quaternion.x, quaternion.y, quaternion.z};
}
}