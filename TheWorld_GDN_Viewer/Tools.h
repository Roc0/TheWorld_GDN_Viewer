#pragma once

#pragma warning(push, 0)
#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/classes/mesh_instance3d.hpp>
#include <godot_cpp/classes/immediate_mesh.hpp>
#include <godot_cpp/classes/standard_material3d.hpp>
#include <godot_cpp/classes/cylinder_mesh.hpp>
#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/classes/label3d.hpp>
#include <godot_cpp/classes/camera3d.hpp>
#include <godot_cpp/classes/sub_viewport.hpp>
#include <godot_cpp/classes/viewport_texture.hpp>
#include <godot_cpp/classes/shader_material.hpp>
#pragma warning(pop)

#include <map>
#include <memory>
#include <thread>

namespace godot
{
	class GDN_TheWorld_Viewer;

	class GDN_TheWorld_ShaderTexture
	{
	public:
		GDN_TheWorld_ShaderTexture();
		~GDN_TheWorld_ShaderTexture();

		void init(Node* parent, godot::Vector2i size, std::string shaderPath);
		void deinit(void);
		godot::Ref<godot::ViewportTexture> getTexture(void);
		godot::Ref<godot::ShaderMaterial> getMaterial(void);

	private:
		godot::SubViewport* m_subviewport = nullptr;
		godot::Ref<godot::ShaderMaterial> m_mat;
	};

	class GDN_TheWorld_Gizmo3d : public godot::Node3D
	{
		GDCLASS(GDN_TheWorld_Gizmo3d, Node3D)

	public:
		GDN_TheWorld_Gizmo3d();
		~GDN_TheWorld_Gizmo3d();

		static void _bind_methods();

		virtual void _ready(void) override;
		virtual void _process(double _delta) override;

		void set_viewer(GDN_TheWorld_Viewer* viewer)
		{
			m_viewer = viewer;
		}
		void set_camera(godot::Camera3D* camera)
		{
			m_camera = camera;
		}

	private:
		godot::MeshInstance3D* m_xAzisMesh = nullptr;
		godot::MeshInstance3D* m_xAxisArrowheadMesh = nullptr;
		godot::Label3D* m_xAxisLabel = nullptr;

		godot::MeshInstance3D* m_yAzisMesh = nullptr;
		godot::MeshInstance3D* m_yAxisArrowheadMesh = nullptr;
		godot::Label3D* m_yAxisLabel = nullptr;

		godot::MeshInstance3D* m_zAzisMesh = nullptr;
		godot::MeshInstance3D* m_zAxisArrowheadMesh = nullptr;
		godot::Label3D* m_zAxisLabel = nullptr;

		//godot::MeshInstance3D* m_normalMesh = nullptr;
		//godot::MeshInstance3D* m_normalArrowheadMesh = nullptr;

		float m_arrowRadius = 1;
		float m_arrowheadRadiusMultiplier = 2.5f;
		float m_arrowSize = 30;
		float m_arrowheadSize = 10;

		GDN_TheWorld_Viewer* m_viewer = nullptr;
		godot::Camera3D* m_camera = nullptr;
	};

	// Thanks to lokimckay: https://github.com/godotengine/godot-docs/issues/5901#issuecomment-1172923676
	// and JonathanDotCel https://github.com/godotengine/godot-docs/issues/5901#issuecomment-1742096560
	// C++ conversion
	class GDN_TheWorld_Drawer : public godot::MeshInstance3D
	{
		GDCLASS(GDN_TheWorld_Drawer, MeshInstance3D)

	public:
		class Drawing
		{
		public:
			Drawing();
			~Drawing();

			enum class DrawingType
			{
				not_set = -1
				, line = 0
				, sphere = 1
				, label2d = 2
			};
			enum class LabelPos
			{
				not_set = -1
				, start = 0
				, center = 1
				, end = 2
			};

			enum class DrawingType drawingType = DrawingType::not_set;
			godot::Vector3 start;
			godot::Vector3 end;
			float radius = -1;
			godot::Color color;
			bool label_3d = false;
			bool label_2d = false;
			godot::Label3D* label3d = nullptr;
			float label3dOffset = -1;
			godot::Label* label2d = nullptr;
			godot::Vector2i label2dOffset;
			enum class LabelPos labelPos = LabelPos::not_set;
			std::string labelText;
		};

	public:
		GDN_TheWorld_Drawer();
		~GDN_TheWorld_Drawer();

		static void _bind_methods();

		virtual void _ready(void) override;
		virtual void _process(double _delta) override;

		int32_t addLine(godot::Vector3 start, godot::Vector3 end, std::string labelText = "@", bool label3d = true, bool label2d = false, enum class Drawing::LabelPos labelPos = Drawing::LabelPos::not_set, float label3dOffset = -1, godot::Vector2i label2dOffset = godot::Vector2i(0, 0), godot::Color c = godot::Color(0.0f, 0.0f, 0.0f, 0.0f));
		void updateLine(int32_t idx, godot::Vector3 start, godot::Vector3 end, std::string labelText = "@", enum class Drawing::LabelPos labelPos = Drawing::LabelPos::not_set, float label3dOffset = -1, godot::Vector2i label2dOffset = godot::Vector2i(0, 0), godot::Color c = godot::Color(0.0f, 0.0f, 0.0f, 0.0f));
		
		int32_t addSphere(godot::Vector3 center, float radius = -1, godot::Color c = godot::Color(0.0f, 0.0f, 0.0f, 0.0f));
		void updateSphere(int32_t idx, godot::Vector3 center, float radius = -1, godot::Color c = godot::Color(0.0f, 0.0f, 0.0f, 0.0f));
		
		int32_t addLabel2d(godot::Vector3 pos, godot::Vector2i offset, std::string labelText = "@", godot::Color c = godot::Color(0.0f, 0.0f, 0.0f, 0.0f));
		void updateLabel2d(int32_t idx, godot::Vector3 pos, std::string labelText = "@", godot::Color c = godot::Color(0.0f, 0.0f, 0.0f, 0.0f));

		void set_camera(godot::Camera3D* camera)
		{
			m_camera = camera;
		}

	private:
		void removeDrawing(int32_t idx);
		void clearDrawings();
		void drawLine(godot::Vector3 start, godot::Vector3 end, godot::Color c = godot::Color(0.0f, 0.0f, 0.0f, 1.0f));
		void drawSphere(godot::Vector3 center, float radius, godot::Color c = godot::Color(0.0f, 0.0f, 0.0f, 1.0f));

	private:
		godot::Ref<godot::ImmediateMesh> m_mesh;
		godot::Ref<godot::StandardMaterial3D> m_mat;
		int32_t m_firstAvailableIdx = 0;
		std::map<int32_t, std::unique_ptr<Drawing>> m_drawings;
		godot::Camera3D* m_camera = nullptr;
	};
}
