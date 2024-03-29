//#include "pch.h"
#include "GDN_TheWorld_Globals.h"
#include "GDN_TheWorld_Camera.h"
#include "GDN_TheWorld_Viewer.h"

#pragma warning (push, 0)
//#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/input.hpp>
#include <godot_cpp/classes/input_event_mouse_motion.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/classes/viewport.hpp>
#include <godot_cpp/classes/window.hpp>
#include <godot_cpp/classes/array_mesh.hpp>
#pragma warning (pop)

#include <math.h> 

using namespace godot;

void GDN_TheWorld_Camera::_bind_methods()
{
	ClassDB::bind_method(D_METHOD("is_active_camera"), &GDN_TheWorld_Camera::isActiveCamera);
	ClassDB::bind_method(D_METHOD("get_active_camera"), &GDN_TheWorld_Camera::getActiveCamera);
	ClassDB::bind_method(D_METHOD("get_angle_from_north_degree"), &GDN_TheWorld_Camera::getAngleFromNorthDeg);
	ClassDB::bind_method(D_METHOD("get_yaw"), &GDN_TheWorld_Camera::getYaw);
	ClassDB::bind_method(D_METHOD("set_yaw"), &GDN_TheWorld_Camera::setYaw);
	ClassDB::bind_method(D_METHOD("get_pitch"), &GDN_TheWorld_Camera::getPitch);
	ClassDB::bind_method(D_METHOD("set_pitch"), &GDN_TheWorld_Camera::setPitch);
	ClassDB::bind_method(D_METHOD("get_roll"), &GDN_TheWorld_Camera::getRoll);
	ClassDB::bind_method(D_METHOD("set_roll"), &GDN_TheWorld_Camera::setRoll);
	ClassDB::bind_method(D_METHOD("init_camera_in_world"), &GDN_TheWorld_Camera::initCameraInWorld);
}

GDN_TheWorld_Camera::GDN_TheWorld_Camera()
{
	m_quitting = false;
	m_ready = false;
	std::string m = TheWorld_Utils::MeshCacheBuffer::generateNewMeshId();
	m_isActive = false;
	m_instanceId = -1;
	m_PlayerCamera = false;
	m_OtherEntityCamera = false;
	m_WorldCamera = false;
	m_updateCameraRequired = false;
	m_angleFromNorthDeg = 0;
	m_yawEuler = 0;
	m_pitchEuler = 0;
	m_rollEuler = 0;

	// Camera Movement
	m_numMoveStepForward = 0;
	m_numMoveStepLeft = 0;
	m_numMoveStepUp = 0;
	m_wheelFastVelocity = 20.0;	// 10.0;
	m_wheelNormalVelocity = 5.0;	// 10.0;
	m_wheelSlowVelocity = 0.3F;	// 10.0;
	//m_forwardMovementOn = false;
	//m_backwardMovementOn = false;
	//m_leftMovementOn = false;
	//m_rightMovementOn = false;
	//m_upwardMovementOn = false;
	//m_downwardMovementOn = false;
	// Camera Movement

	// Camera Rotation
	m_shiftOriCameraOn = false;
	m_shiftVertCameraOn = false;
	m_shiftPressed = false;
	m_ctrlPressed = false;
	m_altPressed = false;
	m_rotateCameraOn = false;
	m_mouseRelativePosToShiftOriz = Vector2(0, 0);
	m_mouseRelativePosToShiftVert = Vector2(0, 0);
	m_mouseRelativePosToRotate = Vector2(0, 0);
	m_sensitivity = 1.0;	// 0.5;
	m_smoothness = 0.5;
	m_yaw = 0.0;
	m_lastYaw = m_yaw;
	m_totalYaw = 0.0;
	m_yawLimit = 360;
	m_pitch = 0.0;
	m_lastPitch = m_pitch;
	m_totalPitch = 0.0;
	m_pitchLimit = 360;
	// Camera Rotation

	m_globals = NULL;

	_init();
}

GDN_TheWorld_Camera::~GDN_TheWorld_Camera()
{
}

void GDN_TheWorld_Camera::_init()
{
	//Cannot find Globals pointer as current node is not yet in the scene
	//godot::UtilityFunctions::print("GDN_TheWorld_Camera::_init");

	m_instanceId = get_instance_id();
	set_name(THEWORLD_CAMERA_NODE_NAME);
}

void GDN_TheWorld_Camera::_ready()
{
	GDN_TheWorld_Globals* globals = Globals();
	if (globals != nullptr)
	{
		globals->debugPrint("GDN_TheWorld_Camera::_ready");
		//Node* parent = get_parent();
		//String parentName = parent->get_name();
		//globals->debugPrint("GDN_TheWorld_Camera::_ready - Parent name: " + parentName);
	}

	m_ready = true;
}

void GDN_TheWorld_Camera::_notification(int p_what)
{
	//if (globals->status() != TheWorldStatus::sessionInitialized)
	//	return;

	switch (p_what)
	{
	case NOTIFICATION_WM_CLOSE_REQUEST:
	{
		m_quitting = true;
	}
	break;
	case NOTIFICATION_PREDELETE:
	{
		if (m_ready)
		{
			GDN_TheWorld_Globals* globals = Globals();
			if (globals != nullptr)
				if (m_quitting)
					globals->debugPrint("GDN_TheWorld_Camera::_notification (NOTIFICATION_PREDELETE) - Destroy Camera", false);
				else
					globals->debugPrint("GDN_TheWorld_Camera::_notification (NOTIFICATION_PREDELETE) - Destroy Camera");
		}
	}
	break;
	}
}

void GDN_TheWorld_Camera::_process(double _delta)
{
	//TheWorld_Utils::GuardProfiler profiler(std::string("_process 1 ") + __FUNCTION__, "ALL");

	// To activate _process method add this Node to a Godot Scene
	//Globals()->debugPrint("GDN_TheWorld_Camera::_process");

	GDN_TheWorld_Globals* globals = Globals();
	if (globals == nullptr)
		return;

	if ((int)globals->status() < (int)TheWorldStatus::sessionInitialized)
		return;

	if (!isActiveCamera())
	{
		return;
	}

	bool b = updateCamera();

	godot::Vector3 euler = get_global_transform().basis.get_euler();

	m_pitchEuler = euler.x;
	m_yawEuler = euler.y;		// 0 = camera aligned with z axis pointing to decresing z (north), - PI/2 = camera aligned with x axis pointing to incrasing x (east), 
								// - PI or PI = camera aligned with z axis pointing to increasing z (south), PI/2 = camera aligned with x axis pointing to decreasing X (west)
	m_rollEuler = euler.z;

	float angleFromNorthRad = (m_yawEuler < 0 ? -m_yawEuler : (m_yawEuler == 0 ? 0 : TheWorld_Utils::kPi * 2 - m_yawEuler));
	m_angleFromNorthDeg = (angleFromNorthRad * 180) / TheWorld_Utils::kPi;		// transform angle from radiant to degree
}

void GDN_TheWorld_Camera::_physics_process(double _delta)
{
	//TheWorld_Utils::GuardProfiler profiler(std::string("_physics_process 1 ") + __FUNCTION__, "ALL");

	// To activate _process method add this Node to a Godot Scene
	//Globals()->debugPrint("GDN_TheWorld_Camera::_physics_process");

	if (IS_EDITOR_HINT())
		return;

	GDN_TheWorld_Globals* globals = Globals();
	if (globals == nullptr)
		return;

	GDN_TheWorld_Viewer* viewer = Globals()->Viewer();
	if (viewer == nullptr)
		return;

	if ((int)globals->status() < (int)TheWorldStatus::sessionInitialized)
		return;

	if (!isActiveCamera())
	{
		return;
	}

	Input* input = Input::get_singleton();

	if (input->is_action_pressed("ui_mouse_button_right") /* && viewer->terrainShiftPermitted()*/ /* && !viewer->editMode()*/)
		m_rotateCameraOn = true;
	else
		m_rotateCameraOn = false;

	//input = Input::get_singleton();
	if (input->is_action_pressed("ui_mouse_button_left") /* && viewer->terrainShiftPermitted()*/ /* && !viewer->editMode()*/)
		m_shiftOriCameraOn = true;
	else
		m_shiftOriCameraOn = false;

	//input = Input::get_singleton();
	if (input->is_action_pressed("ui_mouse_button_mid") /* && viewer->terrainShiftPermitted()*/ /* && !viewer->editMode()*/)
		m_shiftVertCameraOn = true;
	else
		m_shiftVertCameraOn = false;

	if (input->is_action_pressed("ui_shift"))
		m_shiftPressed = true;
	else
		m_shiftPressed = false;

	if (input->is_action_pressed("ui_ctrl"))
		m_ctrlPressed = true;
	else
		m_ctrlPressed = false;

	if (input->is_action_pressed("ui_alt"))
		m_altPressed = true;
	else
		m_altPressed = false;

	if (input->is_action_pressed("ui_forward") /* && !m_altPressed*/)
	{
		if (viewer->terrainShiftPermitted() /* && !viewer->editMode()*/)
		{
			m_numMoveStepForward--;
			m_updateCameraRequired = true;
			//m_forwardMovementOn = true;
		}
	}
	//else
	//	m_forwardMovementOn = false;

	if (input->is_action_pressed("ui_backward") /* && !m_altPressed*/)
	{
		if (viewer->terrainShiftPermitted() /* && !viewer->editMode()*/)
		{
			m_numMoveStepForward++;
			m_updateCameraRequired = true;
			//m_backwardMovementOn = true;
		}
	}
	//else
	//	m_backwardMovementOn = false;

	if (input->is_action_pressed("ui_left") && !m_altPressed)
	{
		if (viewer->terrainShiftPermitted() /* && !viewer->editMode()*/)
		{
			m_numMoveStepLeft++;
			m_updateCameraRequired = true;
			//m_leftMovementOn = true;
		}
	}
	//else
	//	m_leftMovementOn = false;

	if (input->is_action_pressed("ui_right") && !m_altPressed)
	{
		if (viewer->terrainShiftPermitted() /* && !viewer->editMode()*/)
		{
			m_numMoveStepLeft--;
			m_updateCameraRequired = true;
			//m_rightMovementOn = true;
		}
	}
	//else
	//	m_rightMovementOn = false;

	if (input->is_action_pressed("ui_up") && !m_altPressed)
	{
		if (viewer->terrainShiftPermitted() /* && !viewer->editMode()*/)
		{
			m_numMoveStepUp++;
			m_updateCameraRequired = true;
			//m_upwardMovementOn = true;
		}
	}
	//else
	//	m_upwardMovementOn = false;

	if (input->is_action_pressed("ui_down") && !m_altPressed)
	{
		if (viewer->terrainShiftPermitted() /* && !viewer->editMode()*/)
		{
			m_numMoveStepUp--;
			m_updateCameraRequired = true;
			//m_downwardMovementOn = true;
		}
	}
	//else
	//	m_downwardMovementOn = false;

	//if (input->is_action_pressed("ui_mouse_wheel_up"))
	//{
	//	if (viewer->terrainShiftPermitted())
	//	{
	//		m_numMoveStepForward--;
	//		m_updateCameraRequired = true;
	//	}
	//}

	//if (input->is_action_pressed("ui_mouse_wheel_down"))
	//{
	//	if (viewer->terrainShiftPermitted())
	//	{
	//		m_numMoveStepForward++;
	//		m_updateCameraRequired = true;
	//	}
	//}
}

void GDN_TheWorld_Camera::_input(const Ref<godot::InputEvent>& event)
{
	//Globals()->debugPrint("GDN_TheWorld_Camera::_input: " + event->as_text());

	GDN_TheWorld_Globals* globals = Globals();
	if (globals == nullptr)
		return;

	GDN_TheWorld_Viewer* viewer = Globals()->Viewer();
	if (viewer == nullptr)
		return;

	if ((int)globals->status() < (int)TheWorldStatus::sessionInitialized)
		return;

	if (!isActiveCamera())
	{
		return;
	}

	if (viewer->mouseInsideMainEditPanel())
		return;

	const InputEventMouseMotion* eventMouseMotion = godot::Object::cast_to<InputEventMouseMotion>(event.ptr());
	if (eventMouseMotion != nullptr)
	{
		//Globals()->debugPrint("GDN_TheWorld_Camera::_input: " + event->as_text());
		if (m_rotateCameraOn)
		{
			m_mouseRelativePosToRotate = eventMouseMotion->get_relative();
			m_updateCameraRequired = true;
		}
		if (m_shiftOriCameraOn)
		{
			m_mouseRelativePosToShiftOriz = eventMouseMotion->get_relative();
			m_updateCameraRequired = true;
		}
		if (m_shiftVertCameraOn)
		{
			m_mouseRelativePosToShiftVert = eventMouseMotion->get_relative();
			m_updateCameraRequired = true;
		}
	}

	if (event->is_action_pressed("ui_mouse_wheel_up"))
	{
		if (viewer->terrainShiftPermitted())
		{
			m_numMoveStepForward--;
			m_updateCameraRequired = true;
		}
	}

	if (event->is_action_pressed("ui_mouse_wheel_down"))
	{
		if (viewer->terrainShiftPermitted())
		{
			m_numMoveStepForward++;
			m_updateCameraRequired = true;
		}
	}
}

void GDN_TheWorld_Camera::activateCamera(void)
{
	if (IS_EDITOR_HINT())
		return;

	if (!isActiveCamera())
	{
		GDN_TheWorld_Camera* activeCamera = (GDN_TheWorld_Camera*)getActiveCamera();
		if (activeCamera)
		{
			activeCamera->set_process_input(false);
			activeCamera->set_process(false);
			activeCamera->set_physics_process(false);
			activeCamera->remove_from_group(GD_ACTIVE_CAMERA_GROUP);
			activeCamera->notifyActiveCameraFlag(false);
		}

		make_current();
		set_process_input(true);
		set_process(true);
		set_physics_process(true);
		add_to_group(GD_ACTIVE_CAMERA_GROUP);
		notifyActiveCameraFlag(true);
	}
}

void GDN_TheWorld_Camera::deactivateCamera(void)
{
	if (IS_EDITOR_HINT())
		return;

	if (isActiveCamera())
	{
		clear_current(true);
		set_process_input(false);
		set_process(false);
		set_physics_process(false);
		remove_from_group(GD_ACTIVE_CAMERA_GROUP);
		notifyActiveCameraFlag(false);
	}
}

bool GDN_TheWorld_Camera::isActiveCamera(void)
{
	return m_isActive;
	//return is_in_group(GD_ACTIVE_CAMERA_GROUP);
}

void GDN_TheWorld_Camera::notifyActiveCameraFlag(bool active)
{
	m_isActive = active;
}


GDN_TheWorld_Camera* GDN_TheWorld_Camera::getActiveCamera(void)
{
	SceneTree* scene = get_tree();
	if (scene == NULL)
		return NULL;

	godot::Array camArray = scene->get_nodes_in_group(GD_ACTIVE_CAMERA_GROUP);

	
	if (camArray.size() > 1)
	{
		Globals()->_setAppInError(THEWORLD_VIEWER_GENERIC_ERROR, "Too many active cameras");
		return NULL;
	}

	if (camArray.size() == 1)
		return godot::Object::cast_to<GDN_TheWorld_Camera>(camArray[0]);
	else
		return NULL;
}

bool GDN_TheWorld_Camera::initPlayerCamera(void)
{
	set_name("Camera");

	m_PlayerCamera = true;

	return true;
}

bool GDN_TheWorld_Camera::initOtherEntityCamera(void)
{
	set_name("Camera");

	m_OtherEntityCamera = true;

	return true;
}


bool GDN_TheWorld_Camera::initCameraInWorld(Vector3 cameraPos, Vector3 lookAt, float cameraYaw, float cameraPitch, float cameraRoll)
{
	//set_name("WorldCamera");

	activateCamera();

	// cameraPos and lookAt are in WorldNode local coordinates and must be transformed in godot global coordinates
	Vector3 worldNodePosGlobalCoord = Globals()->Viewer()->getWorldNode()->get_global_transform().origin;
	Vector3 cameraPosGlobalCoord = cameraPos + worldNodePosGlobalCoord;
	Vector3 lookAtGlobalCoord = lookAt + worldNodePosGlobalCoord;
	if (cameraPosGlobalCoord != lookAtGlobalCoord)
		Camera3D::look_at_from_position(cameraPosGlobalCoord, lookAtGlobalCoord, Vector3(0, 1, 0));

	setYaw(cameraYaw, false);
	setPitch(cameraPitch, false);
	setRoll(cameraRoll, false);

	//real_t z_near = Camera::get_znear();
	//real_t z_far = Camera::get_zfar();
	//real_t size = Camera::get_size();
	//Vector2 offset = Camera::get_frustum_offset();

	Camera3D::set_far(20000.0);

	// TODORIC mah
	//Camera::set_frustum(size / 2, offset, z_near, z_far * 100);
	//Camera::set_frustum(size / 10.0F, offset, z_near, z_far * 100);
	//Camera::set_frustum(size, offset, z_near, z_far * 100);

	//Camera::set_orthogonal(size, z_near, z_far);

	m_WorldCamera = true;

	return true;
}

bool GDN_TheWorld_Camera::updateCamera()
{
	if (isPlayerCamera() || isOtherEntityCamera())
		return true;

	if (!m_updateCameraRequired)
		return true;

	m_updateCameraRequired = false;

	if (m_mouseRelativePosToRotate != Vector2(0, 0))
	{
		float sensitivity = m_sensitivity;
		if (m_shiftPressed)
			sensitivity /= 4;
		if (m_ctrlPressed)
			sensitivity *= 2;
		m_mouseRelativePosToRotate *= sensitivity;
		m_yaw = m_yaw * m_smoothness + m_mouseRelativePosToRotate.x * (1.0F - m_smoothness);
		m_pitch = m_pitch * m_smoothness + m_mouseRelativePosToRotate.y * (1.0F - m_smoothness);
		m_mouseRelativePosToRotate = Vector2(0, 0);

		if (m_yawLimit < 360)
			m_yaw = TheWorld_Viewer_Utils::Utils::clamp(m_yaw, -m_yawLimit - m_totalYaw, m_yawLimit - m_totalYaw);
		if (m_pitchLimit < 360)
			m_pitch = TheWorld_Viewer_Utils::Utils::clamp(m_pitch, -m_pitchLimit - m_totalPitch, m_pitchLimit - m_totalPitch);
		m_totalYaw *= m_yaw;
		m_totalPitch *= m_pitch;

		float pitch = Math::deg_to_rad(-m_pitch);
		if ((pitch + get_rotation().x) > kPi / 2)
			pitch = (kPi / 2) - get_rotation().x;
		if ((pitch + get_rotation().x) < -(kPi / 2))
			pitch = (-(kPi / 2)) - get_rotation().x;

		rotate_y(Math::deg_to_rad(-m_yaw));
		rotate_object_local(Vector3(1, 0, 0), pitch);

		/*if (m_lastYaw != m_yaw)
			Godot::print("Camera Yaw: " + String(std::to_string(m_yaw).c_str()));
		if (m_lastPitch != m_pitch)
			Godot::print("Camera Pitch: " + String(std::to_string(m_pitch).c_str()));*/

		m_lastYaw = m_yaw;
		m_lastPitch = m_pitch;
	}

	if (m_mouseRelativePosToShiftOriz != Vector2(0, 0))
	{
		m_mouseRelativePosToShiftOriz *= m_sensitivity;

		translate_object_local(Vector3(-m_mouseRelativePosToShiftOriz.x, 0, 0));

		m_mouseRelativePosToShiftOriz = Vector2(0, 0);
	}

	if (m_mouseRelativePosToShiftVert != Vector2(0, 0))
	{
		m_mouseRelativePosToShiftVert *= m_sensitivity;

		translate_object_local(Vector3(0, 0, -m_mouseRelativePosToShiftVert.y));

		m_mouseRelativePosToShiftVert = Vector2(0, 0);
	}

	if (m_numMoveStepForward)
	{
		if (m_altPressed)
		{
			if (m_shiftPressed)
			{
				translate_object_local(Vector3(0, -m_numMoveStepForward * m_wheelSlowVelocity, 0));
			}
			else if (m_ctrlPressed)
			{
				translate_object_local(Vector3(0, -m_numMoveStepForward * m_wheelFastVelocity, 0));
			}
			else
			{
				translate_object_local(Vector3(0, -m_numMoveStepForward * m_wheelNormalVelocity, 0));
			}
		}
		else
		{
			if (m_shiftPressed)
			{
				translate_object_local(Vector3(0, 0, m_numMoveStepForward * m_wheelSlowVelocity));
			}
			else if (m_ctrlPressed)
			{
				translate_object_local(Vector3(0, 0, m_numMoveStepForward * m_wheelFastVelocity));
			}
			else
			{
				translate_object_local(Vector3(0, 0, m_numMoveStepForward * m_wheelNormalVelocity));
			}
		}
		m_numMoveStepForward = 0;
	}

	if (m_numMoveStepLeft)
	{
		//if (m_altPressed)
		//{
		//	Transform t = get_global_transform();
		//	if (m_shiftPressed)
		//	{
		//		t.origin += Vector3(-m_numMoveStepLeft * m_wheelSlowVelocity, 0, 0);
		//	}
		//	else if (m_ctrlPressed)
		//	{
		//		t.origin += Vector3(-m_numMoveStepLeft * m_wheelFastVelocity, 0, 0);
		//	}
		//	else
		//	{
		//		t.origin += Vector3(-m_numMoveStepLeft * m_wheelNormalVelocity, 0, 0);
		//	}
		//	set_global_transform(t);
		//}
		//else
		//{
			if (m_shiftPressed)
			{
				translate_object_local(Vector3(-m_numMoveStepLeft * m_wheelSlowVelocity, 0, 0));
			}
			else if (m_ctrlPressed)
			{
				translate_object_local(Vector3(-m_numMoveStepLeft * m_wheelFastVelocity, 0, 0));
			}
			else
			{
				translate_object_local(Vector3(-m_numMoveStepLeft * m_wheelNormalVelocity, 0, 0));
			}
		//}
		m_numMoveStepLeft = 0;
	}

	if (m_numMoveStepUp)
	{
		Transform3D t = get_global_transform();
		if (m_shiftPressed)
		{
			t.origin += Vector3(0, m_numMoveStepUp * m_wheelSlowVelocity, 0);
		}
		else if (m_ctrlPressed)
		{
			t.origin += Vector3(0, m_numMoveStepUp * m_wheelFastVelocity, 0);
		}
		else
		{
			t.origin += Vector3(0, m_numMoveStepUp * m_wheelNormalVelocity, 0);
		}
		set_global_transform(t);
		m_numMoveStepUp = 0;
	}

	return true;
}

GDN_TheWorld_Globals* GDN_TheWorld_Camera::Globals(bool useCache)
{
	if (m_globals == NULL || !useCache)
	{
		SceneTree* scene = get_tree();
		if (!scene)
			return NULL;
		Viewport* root = scene->get_root();
		if (!root)
			return NULL;
		m_globals = Object::cast_to<GDN_TheWorld_Globals>(root->find_child(THEWORLD_GLOBALS_NODE_NAME, true, false));
	}

	return m_globals;
}

Ref<ArrayMesh> GDN_TheWorld_Camera::DrawViewFrustum(Color c)
{
	GDN_TheWorld_Viewer* viewer = Globals()->Viewer();

	float m_FarOffset = (float)godot::Camera3D::get_far();
	float m_NearOffset = (float)godot::Camera3D::get_near();
	float m_ScreenWidth = godot::Camera3D::get_viewport()->get_visible_rect().get_size().x;
	float m_ScreenHeight = godot::Camera3D::get_viewport()->get_visible_rect().get_size().y;

	float m_AspectRatio = (m_ScreenWidth > m_ScreenHeight) ? (m_ScreenWidth / m_ScreenHeight) : (m_ScreenHeight / m_ScreenWidth);

	float m_FarHeight = 2.0f * ((float)tan(godot::Camera3D::get_fov() / 2.0f)) * m_FarOffset;
	//float m_FarWidth = m_FarOffset * m_AspectRatio;	// TODORIC mah
	float m_FarWidth = m_FarHeight * m_AspectRatio;

	m_FarHeight = m_FarHeight + ((m_FarHeight / 7) * 3);
	m_FarWidth = m_FarWidth + ((m_FarWidth / 7) * 3);

	Vector3 m_Forward = godot::Camera3D::get_global_transform().origin + Vector3Forward * m_FarOffset;

	//--> Near World
	Vector3 m_NearTopLeft = godot::Camera3D::project_position(Vector2(0, 0), godot::Camera3D::get_near());
	Vector3 m_NearTopRight = godot::Camera3D::project_position(Vector2(m_ScreenWidth, 0), godot::Camera3D::get_near());
	Vector3 m_NearBottomLeft = godot::Camera3D::project_position(Vector2(0, m_ScreenHeight), godot::Camera3D::get_near());
	Vector3 m_NearBottomRight = godot::Camera3D::project_position(Vector2(m_ScreenWidth, m_ScreenHeight), godot::Camera3D::get_near());
	
	//--> Far LOCAL
	Vector3 m_FarTopLeft = m_Forward + (Vector3Up * m_FarHeight / 2.0f) - (Vector3Right * m_FarWidth / 2.0f);
	Vector3 m_FarTopRight = m_Forward + (Vector3Up * m_FarHeight / 2.0f) + (Vector3Right * m_FarWidth / 2.0f);
	Vector3 m_FarBottomLeft = m_Forward - (Vector3Up * m_FarHeight / 2.0f) - (Vector3Right * m_FarWidth / 2.0f);
	Vector3 m_FarBottomRight = m_Forward - (Vector3Up * m_FarHeight / 2.0f) + (Vector3Right * m_FarWidth / 2.0f);

	//--> Far WORLD
	m_FarTopLeft = godot::Camera3D::get_transform().xform(m_FarTopLeft);
	m_FarTopRight = godot::Camera3D::get_transform().xform(m_FarTopRight);
	m_FarBottomRight = godot::Camera3D::get_transform().xform(m_FarBottomRight);
	m_FarBottomLeft = godot::Camera3D::get_transform().xform(m_FarBottomLeft);
	
	godot::PackedVector3Array positions;
	godot::PackedColorArray colors;
	godot::PackedInt32Array indices;

	//--> Draw lines
	positions.append(m_FarTopLeft);			colors.append(c);	// index 0
	positions.append(m_FarTopRight);		colors.append(c);	// index 1
	positions.append(m_FarBottomRight);		colors.append(c);	// index 2
	positions.append(m_FarBottomLeft);		colors.append(c);	// index 3
	positions.append(m_NearTopLeft);		colors.append(c);	// index 4
	positions.append(m_NearTopRight);		colors.append(c);	// index 5
	positions.append(m_NearBottomRight);	colors.append(c);	// index 6
	positions.append(m_NearBottomLeft);		colors.append(c);	// index 7

	// Far rect lines
	indices.append(0); indices.append(1);
	indices.append(1); indices.append(2);
	indices.append(2); indices.append(3);
	indices.append(3); indices.append(0);
	// Near rect lines
	indices.append(4); indices.append(5);
	indices.append(5); indices.append(6);
	indices.append(6); indices.append(7);
	indices.append(7); indices.append(4);
	// Left lines        
	indices.append(4); indices.append(0);
	indices.append(7); indices.append(3);
	// Right lines        
	indices.append(5); indices.append(1);
	indices.append(6); indices.append(2);

	godot::Array arrays;
	arrays.resize(ArrayMesh::ARRAY_MAX);
	arrays[ArrayMesh::ARRAY_VERTEX] = positions;
	arrays[ArrayMesh::ARRAY_COLOR] = colors;
	arrays[ArrayMesh::ARRAY_INDEX] = indices;

	if (viewer != nullptr)
		viewer->getMainProcessingMutex().lock();
	Ref<ArrayMesh> mesh = memnew(ArrayMesh);
	if (viewer != nullptr)
		viewer->getMainProcessingMutex().unlock();
	mesh->add_surface_from_arrays(Mesh::PRIMITIVE_LINES, arrays);

	return mesh;
}