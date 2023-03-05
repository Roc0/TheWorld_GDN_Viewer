#pragma once
#include <Godot.hpp>
#include <Node.hpp>
#include <Camera.hpp>
#include <Reference.hpp>
#include <InputEvent.hpp>
#include <ArrayMesh.hpp>

#define GD_ACTIVE_CAMERA_GROUP	"ActiveCamera"

namespace godot {

	class GDN_TheWorld_Globals;

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
		void _notification(int p_what);
		bool initCameraInWorld(Vector3 cameraPos, Vector3 lookAt);
		bool initPlayerCamera(void);
		bool initOtherEntityCamera(void);
		bool updateCamera(void);
		void activateCamera(void);
		void deactivateCamera(void);
		GDN_TheWorld_Camera* getActiveCamera(void);
		bool isActiveCamera(void);
		bool isPlayerCamera()
		{
			return m_PlayerCamera; 
		}
		bool isOtherEntityCamera() 
		{
			return m_OtherEntityCamera;
		}
		bool isWorldCamera()
		{
			return m_WorldCamera;
		}
		void notifyActiveCameraFlag(bool active);
		GDN_TheWorld_Globals* Globals(bool useCache = true);
		Ref<ArrayMesh> DrawViewFrustum(Color c = Color(1, 1, 1));
		float getAngleFromNorth(void)
		{
			return m_angleFromNorth;
		}

	private:

	private:
		Transform m_lastCameraPosInWorld;
		bool m_PlayerCamera;
		bool m_OtherEntityCamera;
		bool m_WorldCamera;
		bool m_updateCameraRequired;
		bool m_isActive;
		int64_t m_instanceId;
		float m_angleFromNorth;		// 0 ==> camera points to north, 90 camera points to east, 180 camera points to south, 270 camera points to west

		// Camera Movement
		int m_numMoveStepForward;
		int m_numMoveStepLeft;
		int m_numMoveStepUp;
		float m_wheelFastVelocity;
		float m_wheelNormalVelocity;
		float m_wheelSlowVelocity;
		// Camera Movement

		// Camera Rotation
		bool m_shiftOriCameraOn;
		bool m_shiftVertCameraOn;
		bool m_rotateCameraOn;
		bool m_shiftPressed;
		bool m_ctrlPressed;
		bool m_altPressed;
		//bool m_forwardMovementOn;
		//bool m_backwardMovementOn;
		//bool m_leftMovementOn;
		//bool m_rightMovementOn;
		//bool m_upwardMovementOn;
		//bool m_downwardMovementOn;
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
		
		// Node cache
		GDN_TheWorld_Globals* m_globals;
	};

}

