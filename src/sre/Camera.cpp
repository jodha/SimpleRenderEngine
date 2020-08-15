/*
 *  SimpleRenderEngine (https://github.com/mortennobel/SimpleRenderEngine)
 *
 *  Created by Morten Nobel-Jørgensen ( http://www.nobel-joergensen.com/ )
 *  License: MIT
 */

#include "sre/Camera.hpp"

#include "sre/impl/GL.hpp"

#include "sre/Renderer.hpp"
#include <glm/gtc/matrix_transform.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/transform.hpp>
#include <sre/Camera.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/string_cast.hpp>
#include <glm/gtx/euler_angles.hpp>
#include "sre/Log.hpp"

namespace sre {
    Camera::Camera()
    : viewTransform{1.0f}
    {
        projectionValue.orthographic.orthographicSize = 1;
        projectionValue.orthographic.nearPlane = -1;
        projectionValue.orthographic.farPlane = 1;
    }

    void Camera::setPerspectiveProjection(float fieldOfViewY, float nearPlane, float farPlane) {
        projectionValue.perspective.fieldOfViewY = glm::radians( fieldOfViewY);
        projectionValue.perspective.nearPlane    = nearPlane;
        projectionValue.perspective.farPlane     = farPlane;
        projectionType = ProjectionType::Perspective;
    }

    void Camera::setOrthographicProjection(float orthographicSize, float nearPlane, float farPlane) {
        projectionValue.orthographic.orthographicSize  = orthographicSize;
        projectionValue.orthographic.nearPlane = nearPlane;
        projectionValue.orthographic.farPlane  = farPlane;
        projectionType = ProjectionType::Orthographic;
    }

    void Camera::setWindowCoordinates(){
        projectionType = ProjectionType::OrthographicWindow;
    }

    void Camera::lookAt(glm::vec3 eye, glm::vec3 at, glm::vec3 up) {
        if (glm::length(eye-at)<std::numeric_limits<float>::epsilon()){
            auto eyeStr = glm::to_string(eye);
            auto atStr = glm::to_string(at);
            LOG_WARNING("Camera::lookAt() invalid parameters. eye (%s) must be different from at (%s)",eyeStr.c_str(),atStr.c_str());
        }
        setViewTransform(glm::lookAt<float>(eye, at, up));
    }

    glm::mat4 Camera::getViewTransform() {
        return viewTransform;
    }

    glm::mat4 Camera::getProjectionTransform(glm::uvec2 viewportSize) {
        switch (projectionType){
            case ProjectionType::Custom:
                return glm::make_mat4(projectionValue.customProjectionMatrix);
            case ProjectionType::Orthographic:
            {
                float aspect = viewportSize.x/(float)viewportSize.y;
                float sizeX = aspect * projectionValue.orthographic.orthographicSize;
                return glm::ortho<float>	(-sizeX, sizeX, -projectionValue.orthographic.orthographicSize, projectionValue.orthographic.orthographicSize, projectionValue.orthographic.nearPlane, projectionValue.orthographic.farPlane);
            }
            case ProjectionType::OrthographicWindow:
                return glm::ortho<float>	(0, float(viewportSize.x), 0, float(viewportSize.y), 1.0f,-1.0f);
            case ProjectionType::Perspective:
                return glm::perspectiveFov<float>(projectionValue.perspective.fieldOfViewY,
                                                  float(viewportSize.x),
                                                  float(viewportSize.y),
                                                  projectionValue.perspective.nearPlane,
                                                  projectionValue.perspective.farPlane);
            default:
                return glm::mat4(1);
        }
    }

    glm::mat4 Camera::getInfiniteProjectionTransform(glm::uvec2 viewportSize) {
        switch (projectionType){
            case ProjectionType::Perspective:
                return glm::tweakedInfinitePerspective(projectionValue.perspective.fieldOfViewY,float(viewportSize.x)/float(viewportSize.y),projectionValue.perspective.nearPlane);
            default:
                return getProjectionTransform(viewportSize);
        }
    }

    void Camera::setViewTransform(const glm::mat4 &viewTransform) {
        Camera::viewTransform = viewTransform;
    }

    void Camera::setProjectionTransform(const glm::mat4 &projectionTransform) {
        memcpy(projectionValue.customProjectionMatrix, glm::value_ptr(projectionTransform), sizeof(glm::mat4));
        projectionType = ProjectionType::Custom;
    }

    void Camera::setViewport(glm::vec2 offset, glm::vec2 size) {
        viewportOffset = offset;
        viewportSize = size;
    }

    void Camera::setPositionAndRotation(glm::vec3 position, glm::vec3 rotationEulersDegrees) {
        auto rotationEulersRadians = glm::radians(rotationEulersDegrees);
        auto viewTransform = glm::translate(position) * glm::eulerAngleXYZ(rotationEulersRadians.x, rotationEulersRadians.y, rotationEulersRadians.z);
        setViewTransform(glm::inverse(viewTransform));
    }

    glm::vec3 Camera::getPosition() {
        glm::vec3 scale;
        glm::quat orientation;
        glm::vec3 translation;
        glm::vec3 skew;
        glm::vec4 perspective;
        glm::decompose(glm::inverse(viewTransform),
                scale,
                orientation,
                translation,
                skew,
                perspective);
        return translation;
    }

    glm::vec3 Camera::getRotationEuler() {
        glm::vec3 scale;
        glm::quat orientation;
        glm::vec3 translation;
        glm::vec3 skew;
        glm::vec4 perspective;
        glm::decompose(glm::inverse(viewTransform),
                           scale,
                           orientation,
                           translation,
                           skew,
                           perspective);

        return glm::degrees( -glm::eulerAngles(orientation));
    }

    std::array<glm::vec3, 2> Camera::screenPointToRay(glm::vec2 position) {
        glm::vec2 scaledWindowsSize = (glm::vec2)Renderer::instance->getWindowSize() * viewportSize;

        position = (position / scaledWindowsSize - viewportOffset/viewportSize)*2.0f-glm::vec2(1.0f);

        auto viewProjection = getProjectionTransform(scaledWindowsSize) * viewTransform;
        auto invViewProjection = glm::inverse(viewProjection);

        glm::vec4 originClipSpace{position,-1,1};
        glm::vec4 destClipSpace{position,1,1};
        glm::vec4 originClipSpaceWS = invViewProjection * originClipSpace;
        glm::vec4 destClipSpaceWS   = invViewProjection * destClipSpace;
        glm::vec3 originClipSpaceWS3 = glm::vec3(originClipSpaceWS)/originClipSpaceWS.w;
        glm::vec3 destClipSpaceWS3   = glm::vec3(destClipSpaceWS)/destClipSpaceWS.w;

        return {originClipSpaceWS3, glm::normalize(destClipSpaceWS3-originClipSpaceWS3)};

    }

// FPS_Camera Class Method Implementations

FPS_Camera::FPS_Camera(): Camera() {
	position = {0.0, 0.0, 0.0};
	direction = {0.0, 0.0, -1.0};
	worldUp = {0.0, 1.0, 0.0};
	up = worldUp;
	right = glm::cross(direction, up);
	forward = direction - glm::dot(direction, worldUp) * worldUp;
	forwardLen = glm::length(forward);
	fieldOfView = 45.0;
	speed = 1.0;
	float nearPlane = 0.1f;
	float farPlane = 150.0f;
    setPerspectiveProjection(fieldOfView, nearPlane, farPlane);
    lookAt(position, position + direction, up);
}	
	
void
FPS_Camera::init(glm::vec3 positionIn, glm::vec3 directionIn,
				 glm::vec3 worldUpIn, float speedIn, float fieldOfViewIn,
				 float nearPlane, float farPlane) {
	position = positionIn;
	direction = directionIn;
	worldUp = worldUpIn;
	up = worldUp;
	right = glm::cross(direction, up);
	forward = direction - glm::dot(direction, worldUp) * worldUp;
	forwardLen = glm::length(forward);
	fieldOfView = fieldOfViewIn;
	speed = speedIn;
    setPerspectiveProjection(fieldOfView, nearPlane, farPlane);
    lookAt(position, position + direction, up);
}

float
FPS_Camera::Speed() {
	return speed;
}

void
FPS_Camera::setSpeed(float speedIn) {
	speed = speedIn;
}

void
FPS_Camera::move(Direction directionToMove, float distance) {
	glm::vec3 moveDirection;
	switch(directionToMove) {
		case Direction::Forward:
			moveDirection = forward;	
			break;
		case Direction::Backward:
			moveDirection = -forward;	
			break;
		case Direction::Left:
			moveDirection = -right;	
			break;
		case Direction::Right:
			moveDirection = right;	
			break;
		case Direction::Up:
			moveDirection = up;	
			break;
		case Direction::Down:
			moveDirection = -up;	
			break;
	}

	// Add the movement vector to the camera position
	position += distance * moveDirection;
	// Set the camera view transform
	lookAt(position, position + direction, up);
}

void
FPS_Camera::pitchAndYaw(float pitchIncrement, float yawIncrement) {
    // Note that pitch and yaw are expected to be in degrees
    float pitch = glm::radians(pitchIncrement);
	// Scale the amount of yaw by the un-normalized length of the forward vector
    float yaw = forwardLen * glm::radians(yawIncrement);

    // Rotate direction & up according to pitch around right
    direction = glm::normalize(direction + glm::tan(pitch)*up);
    up = glm::cross(right, direction);

	// Check if the angle theta between forward and up > thetaMax
	float cosThetaMax = glm::cos(glm::radians(89.9));
	auto cosTheta = glm::dot(forward, direction);
	if (cosTheta < cosThetaMax) { // cos(theta) gets smaller as theta > 90
		direction = direction + (cosThetaMax - cosTheta) * forward;
		direction = glm::normalize(direction);
		up = glm::cross(right, direction);
	}

    // Rotate direction & right according to yaw around up vector
    direction = glm::normalize(direction + glm::tan(yaw)*right);
	// Use worldUp to ensure that right vector stays on horizontal plane
    right = glm::cross(direction, worldUp);
	// Normalize because cross product was not with ortho-normal vectors
	right = glm::normalize(glm::cross(direction, worldUp));

	// Adjust up vector to be consistent with right and direction
	up = glm::cross(right, direction);

	// Calculate projection and length of direction onto horizontal plane 
	forward = direction - glm::dot(direction, worldUp) * worldUp;
	forwardLen = glm::length(forward);
	forward = glm::normalize(forward);

    // Calculate the view transform
    lookAt(position, position + direction, up);
}

void
FPS_Camera::zoom(float zoomIncrement) {
	fieldOfView -= zoomIncrement; // Decreasing the FOV increases the "zoom"
	fieldOfView = glm::min(fieldOfView,45.0f); // USE CLASS MAX FOV!!!!!
	fieldOfView = glm::max(fieldOfView,1.0f);
	setPerspectiveProjection(fieldOfView,0.1,150); // USE CLASS NEAR & FAR!!!!
}

}
