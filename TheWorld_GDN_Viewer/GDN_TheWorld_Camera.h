#pragma once
#include <Godot.hpp>
#include <Node.hpp>
#include <Camera.hpp>
#include <Reference.hpp>
#include <InputEvent.hpp>

#define GD_ACTIVE_CAMERA_GROUP	"ActiveCamera"

namespace godot {

	class GDN_TheWorld_Camera : public Camera
	{
		GODOT_CLASS(GDN_TheWorld_Camera, Camera)

	public:
		static void _register_methods();

		GDN_TheWorld_Camera();
		~GDN_TheWorld_Camera();

		void _init(); // our initializer called by Godot
		void _ready();
		void _process(float _delta);
		void _physics_process(float _delta);
		void _input(const Ref<InputEvent> event);
		bool initCameraInWorld(Node* pSpaceWorld);
		bool initPlayerCamera(void);
		bool initOtherEntityCamera(void);
		bool updateCamera(void);
		void activateCamera(void);
		Node* getActiveCamera(void);
		bool isActiveCamera(void);
		bool isPlayerCamera() { return m_PlayerCamera; }
		bool isOtherEntityCamera() { return m_OtherEntityCamera; }
		bool isWorldCamera() { return m_WorldCamera; }
		//bool isDebugEnabled(void);
		//void resetDebugEnabled(void);

	private:
		Node* m_pSpaceWorldNode;
		Transform m_lastCameraPosInWorld;
		bool m_PlayerCamera;
		bool m_OtherEntityCamera;
		bool m_WorldCamera;
		bool m_updateCameraRequired;
		bool m_isActive;
		int64_t m_instanceId;
		//int m_isDebugEnabled;

		// Camera Movement
		int m_numMoveStep;
		float m_wheelVelocity;
		// Camera Movement

		// Camera Rotation
		bool m_shiftOriCameraOn;
		bool m_shiftVertCameraOn;
		bool m_rotateCameraOn;
		Vector2 m_mouseRelativePosToRotate;
		Vector2 m_mouseRelativePosToShiftOriz;
		Vector2 m_mouseRelativePosToShiftVert;
		float m_sensitivity;			// 0.0 - 1.0
		float m_smoothness;				// 0.0 - 0.999 tick 0.001
		float m_yaw;
		float m_lastYaw;
		float m_totalYaw;
		int m_yawLimit;
		float m_pitch;
		float m_lastPitch;
		float m_totalPitch;
		int m_pitchLimit;
		// Camera Rotation
	};

}

