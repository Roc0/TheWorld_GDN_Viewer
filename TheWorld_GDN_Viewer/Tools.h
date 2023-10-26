#pragma once

#pragma warning(push, 0)
#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/classes/mesh_instance3d.hpp>
#include <godot_cpp/classes/immediate_mesh.hpp>
#include <godot_cpp/classes/standard_material3d.hpp>
#include <godot_cpp/classes/cylinder_mesh.hpp>
#include <godot_cpp/classes/label3d.hpp>
#pragma warning(pop)

#include <map>
#include <memory>
#include <thread>

namespace godot
{
	class GDN_TheWorld_Gizmo3d : public godot::Node3D
	{
		GDCLASS(GDN_TheWorld_Gizmo3d, Node3D)

	public:
		GDN_TheWorld_Gizmo3d();
		~GDN_TheWorld_Gizmo3d();

		static void _bind_methods();

		virtual void _ready(void) override;
		virtual void _process(double _delta) override;

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
	};

	// Thanks to lokimckay: https://github.com/godotengine/godot-docs/issues/5901#issuecomment-1172923676
	// and JonathanDotCel https://github.com/godotengine/godot-docs/issues/5901#issuecomment-1742096560
	// C++ conversion
	class GDN_TheWorld_Drawer : public godot::MeshInstance3D
	{
		GDCLASS(GDN_TheWorld_Drawer, MeshInstance3D)

	private:
		class Drawing
		{
		public:
			enum class DrawingType
			{
				not_set = -1
				, line = 0
				, sphere = 1
			};

			enum class DrawingType drawingType = DrawingType::not_set;
			godot::Vector3 start;
			godot::Vector3 end;
			float radius = 0;
			godot::Color color;
		};

	public:
		GDN_TheWorld_Drawer();
		~GDN_TheWorld_Drawer();

		static void _bind_methods();

		void clearDrawings();
		void drawLine(godot::Vector3 start, godot::Vector3 end, godot::Color c = godot::Color(0.0f, 0.0f, 0.0f, 1.0f));
		void drawSphere(godot::Vector3 center, float radius, godot::Color c = godot::Color(0.0f, 0.0f, 0.0f, 1.0f));

		virtual void _ready(void) override;
		virtual void _process(double _delta) override;

		int32_t addLine(godot::Vector3 start, godot::Vector3 end, godot::Color c = godot::Color(0.0f, 0.0f, 0.0f, 1.0f));
		int32_t addSphere(godot::Vector3 center, float radius, godot::Color c = godot::Color(0.0f, 0.0f, 0.0f, 1.0f));
		void removeDrawing(int32_t idx);

	private:
		godot::Ref<godot::ImmediateMesh> m_mesh;
		godot::Ref<godot::StandardMaterial3D> m_mat;
		int32_t m_firstAvailableIdx = 0;

		std::map<int32_t, std::unique_ptr<Drawing>> m_drawings;
	};
}
