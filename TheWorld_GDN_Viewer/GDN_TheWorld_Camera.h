#pragma once

#pragma warning (push, 0)
#include <godot_cpp/classes/camera3d.hpp>
#include <godot_cpp/classes/input_event.hpp>
#include <godot_cpp/classes/array_mesh.hpp>
#include <godot_cpp/classes/ref.hpp>
#pragma warning (pop)

//#include <Reference.hpp>
//#include <ArrayMesh.hpp>

#include "Utils.h"

#define GD_ACTIVE_CAMERA_GROUP	"ActiveCamera"

namespace godot {

	class GDN_TheWorld_Globals;

	class GDN_TheWorld_Camera : public Camera3D
	{
		GDCLASS(GDN_TheWorld_Camera, Camera3D)

	protected:
		static void _bind_methods();
		void _notification(int p_what);
		void _init(void);

	public:
		GDN_TheWorld_Camera();
		~GDN_TheWorld_Camera();

		virtual void _ready() override;
		virtual void _process(double _delta) override;
		virtual void _physics_process(double _delta) override;
		virtual void _input(const Ref<InputEvent>& event) override;
		
		bool initCameraInWorld(Vector3 cameraPos, Vector3 lookAt, float cameraYaw, float cameraPitch, float cameraRoll);
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
		float getAngleFromNorthDeg(void)
		{
			return m_angleFromNorthDeg;
		}
		float getYaw(bool radiant = true)
		{
			if (radiant)
				return m_yawEuler;
			else
				return (m_yawEuler * 180) / TheWorld_Utils::kPi;
		}
		void setYaw(float yaw, bool radiant = true)
		{
			float radiantYaw = radiant ? yaw : (yaw * TheWorld_Utils::kPi) / 180;
			Transform3D t = get_global_transform();
			godot::Vector3 v = t.basis.get_euler();
			v.y = radiantYaw;
			t.basis.set_euler(v);
			set_global_transform(t);
		}
		float getPitch(bool radiant = true)
		{
			if (radiant)
				return m_pitchEuler;
			else
				return (m_pitchEuler * 180) / TheWorld_Utils::kPi;
		}
		void setPitch(float pitch, bool radiant = true)
		{
			float radiantPitch = radiant ? pitch : (pitch * TheWorld_Utils::kPi) / 180;
			Transform3D t = get_global_transform();
			godot::Vector3 v = t.basis.get_euler();
			v.x = radiantPitch;
			t.basis.set_euler(v);
			set_global_transform(t);
		}
		float getRoll(bool radiant = true)
		{
			if (radiant)
				return m_rollEuler;
			else
				return (m_rollEuler * 180) / TheWorld_Utils::kPi;
		}
		void setRoll(float roll, bool radiant = true)
		{
			float radiantRoll = radiant ? roll : (roll * TheWorld_Utils::kPi) / 180;
			Transform3D t = get_global_transform();
			godot::Vector3 v = t.basis.get_euler();
			v.z = radiantRoll;
			t.basis.set_euler(v);
			set_global_transform(t);
		}

	private:

	private:
		bool m_ready;
		bool m_quitting;
		Transform3D m_lastCameraPosInWorld;
		bool m_PlayerCamera;
		bool m_OtherEntityCamera;
		bool m_WorldCamera;
		bool m_updateCameraRequired;
		bool m_isActive;
		int64_t m_instanceId;

		float m_angleFromNorthDeg;	// angle expressed in degrees of the projection of the camera on XZ plane: 0 ==> camera points to north, 90 camera points to east, 180 camera points to south, 270 camera points to west

		float m_yawEuler;	// radiant angle of the projection of the camera on XZ plane and Z axis (-Z direction)
							// 0 ==> projection of camera is aligned with Z axis and points to decreasing Z direction (north)
							// PI/2 ==> projection of camera is aligned with X axis and points to increasing X direction (east)
							// PI(/-PI) ==> projection of camera is aligned with Z axis and points to increasing Z direction (south)
							// -PI/2 ==> projection of camera is aligned with X axis and points to decreasing X direction (west)
							// -PI(/PI) ==> projection of camera is aligned with Z axis and points to decreasing Z direction (south)

		float m_pitchEuler;	// radiant angle of the projection of the camera on YZ plane and Z axis (-Z direction)
							// 0 ==> projection of camera is aligned with Z axis and points to decreasing Z direction (face forward to horizonth aligned with terrain)
							// PI/2 ==> projection of camera is aligned with Y axis and points to increasing Y direction (face up perpendicularly to terrain)
							// PI(/-PI) ==> projection of camera is aligned with Z axis and points to increasing Z direction (face backward to horizonth aligned with terrain)
							// -PI/2 ==> projection of camera is aligned with Y axis and points to decreasing Y direction (face down perpendicularly to terrain)
							// -PI(/PI) ==> projection of camera is aligned with Z axis and points to increasing Z direction (face backward to horizonth aligned with terrain)

		float m_rollEuler;

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

