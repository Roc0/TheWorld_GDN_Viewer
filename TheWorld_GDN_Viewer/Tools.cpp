#include "GDN_TheWorld_Globals.h"
#include "GDN_TheWorld_Viewer.h"
#include "Tools.h"
#include "Utils.h"
#include "Viewer_Utils.h"

#pragma warning(push, 0)
#include <godot_cpp/classes/label3d.hpp>
#include <godot_cpp/classes/sphere_mesh.hpp>
#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/classes/sub_viewport.hpp>
#include <godot_cpp/classes/world3d.hpp>
#include <godot_cpp/classes/shader_material.hpp>
#include <godot_cpp/classes/color_rect.hpp>
#include <godot_cpp/classes/viewport_texture.hpp>
#pragma warning(pop)

using namespace godot;

GDN_TheWorld_ShaderTexture::GDN_TheWorld_ShaderTexture()
{
}

GDN_TheWorld_ShaderTexture::~GDN_TheWorld_ShaderTexture()
{
	deinit();
}

void GDN_TheWorld_ShaderTexture::init(Node* parent, godot::Vector2i size, std::string shaderPath)
{
	deinit();

	m_subviewport = memnew(godot::SubViewport);
	parent->add_child(m_subviewport);
	m_subviewport->set_size(size);
	//m_subviewport->set_update_mode(godot::SubViewport::UpdateMode::UPDATE_WHEN_VISIBLE);
	//m_subviewport->set_clear_mode(godot::SubViewport::ClearMode::CLEAR_MODE_ALWAYS);
	godot::Ref<godot::World3D> w = memnew(godot::World3D);
	m_subviewport->set_world_3d(w);
	m_subviewport->set_use_own_world_3d(true);

	godot::ResourceLoader* resLoader = godot::ResourceLoader::get_singleton();
	godot::Ref<godot::ShaderMaterial> mat = memnew(godot::ShaderMaterial);
	godot::Ref<godot::Shader> shader = resLoader->load(shaderPath.c_str()/*, "", godot::ResourceLoader::CacheMode::CACHE_MODE_IGNORE*/);
	mat->set_shader(shader);
	m_mat = mat;

	godot::ColorRect* colorRect = memnew(godot::ColorRect);
	m_subviewport->add_child(colorRect);
	colorRect->set_size(m_subviewport->get_size());
	colorRect->set_anchors_preset(godot::Control::LayoutPreset::PRESET_FULL_RECT);
	colorRect->set_material(mat);
}

void GDN_TheWorld_ShaderTexture::deinit(void)
{
	if (m_subviewport != nullptr)
	{
		Node* parent = m_subviewport->get_parent();
		if (parent != nullptr)
			parent->remove_child(m_subviewport);
		m_subviewport->queue_free();
		m_subviewport = nullptr;
		m_mat.unref();
	}
}

godot::Ref<godot::ViewportTexture> GDN_TheWorld_ShaderTexture::getTexture(void)
{
	return m_subviewport->get_texture();
}

godot::Ref<godot::ShaderMaterial> GDN_TheWorld_ShaderTexture::getMaterial(void)
{
	return m_mat;
}

GDN_TheWorld_Gizmo3d::GDN_TheWorld_Gizmo3d()
{
}

GDN_TheWorld_Gizmo3d::~GDN_TheWorld_Gizmo3d()
{
}

void GDN_TheWorld_Gizmo3d::_bind_methods()
{
}

void GDN_TheWorld_Gizmo3d::_ready(void)
{
	const double pixel_size = 0.05;		// 0.005
	const int32_t font_size = 128;		// 32
	const int32_t outline_size = 36;	// 12

	// X Axis GDN_TheWorld_Globals::g_color_red 
	godot::Ref<godot::CylinderMesh> xAzisMesh = memnew(godot::CylinderMesh);
	xAzisMesh->set_cap_bottom(false);
	xAzisMesh->set_cap_top(false);
	xAzisMesh->set_bottom_radius(m_arrowRadius);
	xAzisMesh->set_top_radius(m_arrowRadius);
	xAzisMesh->set_height(m_arrowSize);
	godot::Ref<godot::StandardMaterial3D> xAxisMat = memnew(godot::StandardMaterial3D);
	xAxisMat->set_albedo(GDN_TheWorld_Globals::g_color_red);
	m_xAzisMesh = memnew(godot::MeshInstance3D);
	m_xAzisMesh->set_mesh(xAzisMesh);
	m_xAzisMesh->set_material_override(xAxisMat);
	add_child(m_xAzisMesh);
	m_xAzisMesh->set_rotation_degrees(godot::Vector3(0.0f, 0.0f, 270.0f));
	m_xAzisMesh->set_position(godot::Vector3(m_arrowSize / 2, 0.0f, 0.0f));

	godot::Ref<godot::CylinderMesh> xAzisArrowheadMesh = memnew(godot::CylinderMesh);
	xAzisArrowheadMesh->set_cap_bottom(true);
	xAzisArrowheadMesh->set_cap_top(false);
	xAzisArrowheadMesh->set_bottom_radius(m_arrowRadius * m_arrowheadRadiusMultiplier);
	xAzisArrowheadMesh->set_top_radius(0.0f);
	xAzisArrowheadMesh->set_height(m_arrowheadSize);
	m_xAxisArrowheadMesh = memnew(godot::MeshInstance3D);
	m_xAxisArrowheadMesh->set_mesh(xAzisArrowheadMesh);
	m_xAxisArrowheadMesh->set_material_override(xAxisMat);
	add_child(m_xAxisArrowheadMesh);
	m_xAxisArrowheadMesh->set_rotation_degrees(godot::Vector3(0.0f, 0.0f, 270.0f));
	m_xAxisArrowheadMesh->set_position(godot::Vector3(m_arrowheadSize / 2 + m_arrowSize, 0.0f, 0.0f));

	m_xAxisLabel = memnew(godot::Label3D);
	add_child(m_xAxisLabel);
	m_xAxisLabel->set_text("X");
	m_xAxisLabel->set_position(godot::Vector3(m_arrowheadSize + m_arrowSize + 5, 0.0f, 0.0f));
	m_xAxisLabel->set_font_size(font_size);
	m_xAxisLabel->set_pixel_size(pixel_size);
	m_xAxisLabel->set_outline_size(outline_size);
	m_xAxisLabel->set_modulate(GDN_TheWorld_Globals::g_color_red);
	m_xAxisLabel->set_outline_modulate(godot::Color(0.27f, 0.27f, 0.27f, 1.0f));

	//godot::SphereMesh* xAxisSphereMesh = memnew(godot::SphereMesh);
	//xAxisSphereMesh->set_height(m_arrowRadius * m_arrowheadRadiusMultiplier);
	//xAxisSphereMesh->set_radius((m_arrowRadius * m_arrowheadRadiusMultiplier) / 2);
	//godot::MeshInstance3D* xAxisSphereMeshI3d = memnew(godot::MeshInstance3D);
	//xAxisSphereMeshI3d->set_mesh(xAxisSphereMesh);
	//xAxisSphereMeshI3d->set_material_override(xAxisMat);
	//add_child(xAxisSphereMeshI3d);
	//xAxisSphereMeshI3d->set_position(godot::Vector3(m_arrowheadSize + m_arrowSize + 5, 0.0f, 0.0f));

	// Y Axis GDN_TheWorld_Globals::g_color_green 
	godot::Ref<godot::CylinderMesh> yAzisMesh = memnew(godot::CylinderMesh);
	yAzisMesh->set_cap_bottom(false);
	yAzisMesh->set_cap_top(false);
	yAzisMesh->set_bottom_radius(m_arrowRadius);
	yAzisMesh->set_top_radius(m_arrowRadius);
	yAzisMesh->set_height(m_arrowSize);
	godot::Ref<godot::StandardMaterial3D> yAxisMat = memnew(godot::StandardMaterial3D);
	yAxisMat->set_albedo(GDN_TheWorld_Globals::g_color_green);
	m_yAzisMesh = memnew(godot::MeshInstance3D);
	m_yAzisMesh->set_mesh(yAzisMesh);
	m_yAzisMesh->set_material_override(yAxisMat);
	add_child(m_yAzisMesh);
	m_yAzisMesh->set_position(godot::Vector3(0.0f, m_arrowSize / 2, 0.0f));

	godot::Ref<godot::CylinderMesh> yAzisArrowheadMesh = memnew(godot::CylinderMesh);
	yAzisArrowheadMesh->set_cap_bottom(true);
	yAzisArrowheadMesh->set_cap_top(false);
	yAzisArrowheadMesh->set_bottom_radius(m_arrowRadius * m_arrowheadRadiusMultiplier);
	yAzisArrowheadMesh->set_top_radius(0.0f);
	yAzisArrowheadMesh->set_height(m_arrowheadSize);
	m_yAxisArrowheadMesh = memnew(godot::MeshInstance3D);
	m_yAxisArrowheadMesh->set_mesh(yAzisArrowheadMesh);
	m_yAxisArrowheadMesh->set_material_override(yAxisMat);
	add_child(m_yAxisArrowheadMesh);
	m_yAxisArrowheadMesh->set_position(godot::Vector3(0.0f, m_arrowheadSize / 2 + m_arrowSize, 0.0f));

	m_yAxisLabel = memnew(godot::Label3D);
	add_child(m_yAxisLabel);
	m_yAxisLabel->set_text("Y");
	m_yAxisLabel->set_position(godot::Vector3(0.0f, m_arrowheadSize + m_arrowSize + 5, 0.0f));
	m_yAxisLabel->set_font_size(font_size);
	m_yAxisLabel->set_pixel_size(pixel_size);
	m_yAxisLabel->set_outline_size(outline_size);
	m_yAxisLabel->set_modulate(GDN_TheWorld_Globals::g_color_green);
	m_yAxisLabel->set_outline_modulate(godot::Color(0.27f, 0.27f, 0.27f, 1.0f));

	// Z Axis GDN_TheWorld_Globals::g_color_blue 
	godot::Ref<godot::CylinderMesh> zAzisMesh = memnew(godot::CylinderMesh);
	zAzisMesh->set_cap_bottom(false);
	zAzisMesh->set_cap_top(true);
	zAzisMesh->set_bottom_radius(m_arrowRadius);
	zAzisMesh->set_top_radius(m_arrowRadius);
	zAzisMesh->set_height(m_arrowSize);
	godot::Ref<godot::StandardMaterial3D> zAxisMat = memnew(godot::StandardMaterial3D);
	zAxisMat->set_albedo(GDN_TheWorld_Globals::g_color_blue);
	m_zAzisMesh = memnew(godot::MeshInstance3D);
	m_zAzisMesh->set_mesh(zAzisMesh);
	m_zAzisMesh->set_material_override(zAxisMat);
	add_child(m_zAzisMesh);
	m_zAzisMesh->set_rotation_degrees(godot::Vector3(90.0f, 0.0f, 0.0f));
	m_zAzisMesh->set_position(godot::Vector3(0.0f, 0.0f, m_arrowSize / 2));

	godot::Ref<godot::CylinderMesh> zAzisArrowheadMesh = memnew(godot::CylinderMesh);
	zAzisArrowheadMesh->set_cap_bottom(true);
	zAzisArrowheadMesh->set_cap_top(false);
	zAzisArrowheadMesh->set_bottom_radius(m_arrowRadius * m_arrowheadRadiusMultiplier);
	zAzisArrowheadMesh->set_top_radius(0.0f);
	zAzisArrowheadMesh->set_height(m_arrowheadSize);
	m_zAxisArrowheadMesh = memnew(godot::MeshInstance3D);
	m_zAxisArrowheadMesh->set_mesh(zAzisArrowheadMesh);
	m_zAxisArrowheadMesh->set_material_override(zAxisMat);
	add_child(m_zAxisArrowheadMesh);
	m_zAxisArrowheadMesh->set_rotation_degrees(godot::Vector3(90.0f, 0.0f, 0.0f));
	m_zAxisArrowheadMesh->set_position(godot::Vector3(0.0f, 0.0f, m_arrowheadSize / 2 + m_arrowSize));

	m_zAxisLabel = memnew(godot::Label3D);
	add_child(m_zAxisLabel);
	m_zAxisLabel->set_text("Z");
	m_zAxisLabel->set_position(godot::Vector3(0.0f, 0.0f, m_arrowheadSize + m_arrowSize + 5));
	m_zAxisLabel->set_font_size(font_size);
	m_zAxisLabel->set_pixel_size(pixel_size);
	m_zAxisLabel->set_outline_size(outline_size);
	m_zAxisLabel->set_modulate(GDN_TheWorld_Globals::g_color_blue);
	m_zAxisLabel->set_outline_modulate(godot::Color(0.27f, 0.27f, 0.27f, 1.0f));
}

void GDN_TheWorld_Gizmo3d::_process(double _delta)
{
	if (m_camera != nullptr)
	{
		godot::Vector3 cameraPos = m_camera->get_global_transform().origin;
		m_xAxisLabel->look_at(cameraPos, godot::Vector3(0, 1, 0));
		m_xAxisLabel->rotate_y(TheWorld_Utils::kPi);
		m_yAxisLabel->look_at(cameraPos, godot::Vector3(0, 1, 0));
		m_yAxisLabel->rotate_y(TheWorld_Utils::kPi);
		m_zAxisLabel->look_at(cameraPos, godot::Vector3(0, 1, 0));
		m_zAxisLabel->rotate_y(TheWorld_Utils::kPi);
	}
}

GDN_TheWorld_Drawer::Drawing::Drawing()
{
}

GDN_TheWorld_Drawer::Drawing::~Drawing()
{
	if (label3d != nullptr)
	{
		Node* parent = label3d->get_parent();
		if (parent != nullptr)
			parent->remove_child(label3d);
		label3d->queue_free();
		label3d = nullptr;
	}

	if (label2d != nullptr)
	{
		Node* parent = label2d->get_parent();
		if (parent != nullptr)
			parent->remove_child(label2d);
		label2d->queue_free();
		label2d = nullptr;
	}
}

GDN_TheWorld_Drawer::GDN_TheWorld_Drawer()
{
}

GDN_TheWorld_Drawer::~GDN_TheWorld_Drawer()
{
}

void GDN_TheWorld_Drawer::_bind_methods()
{
}

void GDN_TheWorld_Drawer::_ready(void)
{
	godot::Ref<godot::StandardMaterial3D> mat = memnew(godot::StandardMaterial3D);
	m_mat = mat;
	m_mat->set("use_point_size", false);
	m_mat->set("point_size", 5.0f);
	m_mat->set("no_depth_test", false);
	//m_mat->set("shading_mode", (int)godot::BaseMaterial3D::ShadingMode::SHADING_MODE_PER_PIXEL);
	m_mat->set("shading_mode", (int)godot::BaseMaterial3D::ShadingMode::SHADING_MODE_UNSHADED);
	m_mat->set("vertex_color_use_as_albedo", true);
	//m_mat->set("transparency", (int)godot::BaseMaterial3D::Transparency::TRANSPARENCY_ALPHA);
	m_mat->set("transparency", (int)godot::BaseMaterial3D::Transparency::TRANSPARENCY_DISABLED);

	godot::Ref<godot::ImmediateMesh> mesh = memnew(godot::ImmediateMesh);
	m_mesh = mesh;
	//m_mesh->surface_set_material(0, m_mat);

	set_mesh(m_mesh);
	set_material_override(m_mat);
}

void GDN_TheWorld_Drawer::_process(double _delta)
{
	if (!m_drawings.empty())
	{
		if (is_visible())
		{
			const double pixel_size = 0.05;		// 0.005
			const int32_t font_size = 128;		// 32
			const int32_t outline_size = 36;	// 12

			m_mesh->clear_surfaces();
			for (auto& item : m_drawings)
			{
				if (item.second->drawingType == Drawing::DrawingType::line && item.second->start != item.second->end)
				{
					drawLine(item.second->start, item.second->end, item.second->color);
					if (item.second->labelPos == Drawing::LabelPos::not_set)
					{
						if (item.second->label3d != nullptr)
						{
							Node* parent = item.second->label3d->get_parent();
							if (parent != nullptr)
								parent->remove_child(item.second->label3d);
							item.second->label3d->queue_free();
							item.second->label3d = nullptr;
						}
						if (item.second->label2d != nullptr)
						{
							Node* parent = item.second->label2d->get_parent();
							if (parent != nullptr)
								parent->remove_child(item.second->label2d);
							item.second->label2d->queue_free();
							item.second->label2d = nullptr;
						}
					}
					else
					{
						if (item.second->label_3d)
						{
							if (item.second->label3d == nullptr)
							{
								item.second->label3d = memnew(godot::Label3D);
								add_child(item.second->label3d);
								item.second->label3d->set_font_size(font_size);
								item.second->label3d->set_pixel_size(pixel_size);
								item.second->label3d->set_outline_size(outline_size);
								item.second->label3d->set_outline_modulate(godot::Color(0.27f, 0.27f, 0.27f, 1.0f));
							}

							item.second->label3d->set_text(item.second->labelText.c_str());
							item.second->label3d->set_modulate(item.second->color);

							if (item.second->labelPos == Drawing::LabelPos::start)
							{
								// TODO
							}
							else if (item.second->labelPos == Drawing::LabelPos::center)
							{
								// TODO
							}
							else if (item.second->labelPos == Drawing::LabelPos::end)
							{
								item.second->label3d->set_position(item.second->end + (item.second->end - item.second->start).normalized() * item.second->label3dOffset);
							}

							if (m_camera != nullptr)
							{
								godot::Vector3 cameraPos = m_camera->get_global_transform().origin;
								item.second->label3d->look_at(cameraPos, godot::Vector3(0, 1, 0));
								item.second->label3d->rotate_y(TheWorld_Utils::kPi);
							}
						}

						if (item.second->label_2d)
						{
							if (m_camera != nullptr && m_viewer != nullptr)
							{
								if (item.second->label2d == nullptr)
								{
									item.second->label2d = memnew(godot::Label);
									add_child(item.second->label2d);
								}

								item.second->label2d->set_text(item.second->labelText.c_str());
								item.second->label2d->add_theme_color_override("font_color", item.second->color);

								//godot::Vector3 position;
								godot::Vector2 viewportPos;
								if (item.second->labelPos == Drawing::LabelPos::start)
								{
									// TODO
								}
								else if (item.second->labelPos == Drawing::LabelPos::center)
								{
									godot::Vector2 end = m_camera->unproject_position(item.second->end);
									godot::Vector2 start = m_camera->unproject_position(item.second->start);
									godot::Size2 viewportSize = m_viewer->getViewportSize();
									bool startBehindCamera = m_camera->is_position_behind(item.second->start);

									if (startBehindCamera)
									{
										godot::Vector3 newStart = item.second->start;
										while (startBehindCamera)
										{
											newStart = newStart + (item.second->end - newStart) / 10;
											startBehindCamera = m_camera->is_position_behind(newStart);
										}
										start = m_camera->unproject_position(newStart);
									}

									viewportPos = start + (end - start) / 2;
										
									if (viewportPos.x < 0 || viewportPos.x > viewportSize.x || viewportPos.y < 0 || viewportPos.y > viewportSize.y)
									{
										// pos outside viewport

										if (end.x >= 0 && end.x <= viewportSize.x && end.y >= 0 && end.y <= viewportSize.y)
										{
											// end inside viewport: we need to approach end until label will be inside viewport
											while (viewportPos.x < 0 || viewportPos.x > viewportSize.x || viewportPos.y < 0 || viewportPos.y > viewportSize.y)
												viewportPos = viewportPos + (end - viewportPos) / 2;
										}
										else if (start.x >= 0 && start.x <= viewportSize.x && start.y >= 0 && start.y <= viewportSize.y)
										{
											// start inside viewport: we need to approach start until label will be inside viewport
											while (viewportPos.x < 0 || viewportPos.x > viewportSize.x || viewportPos.y < 0 || viewportPos.y > viewportSize.y)
												viewportPos = viewportPos + (start - viewportPos) / 2;
										}
										else
										{
											// label not visible
											Node* parent = item.second->label2d->get_parent();
											if (parent != nullptr)
												parent->remove_child(item.second->label2d);
											item.second->label2d->queue_free();
											item.second->label2d = nullptr;
										}
									}
								}
								else if (item.second->labelPos == Drawing::LabelPos::end)
								{
									godot::Vector3 position = item.second->end + (item.second->end - item.second->start).normalized() * item.second->label3dOffset;
									viewportPos = m_camera->unproject_position(position);
								}

								if (item.second->label2d != nullptr)
								{
									viewportPos += item.second->label2dOffset;
									item.second->label2d->set_position(viewportPos);
								}
							}
						}
					}
				}
				else if (item.second->drawingType == Drawing::DrawingType::sphere && item.second->radius != 0.0f)
					drawSphere(item.second->start, item.second->radius, item.second->color);
				else if (item.second->drawingType == Drawing::DrawingType::label2d)
				{
					if (m_camera != nullptr)
					{
						if (item.second->label2d == nullptr)
						{
							item.second->label2d = memnew(godot::Label);
							add_child(item.second->label2d);
						}
						godot::Vector2 viewportPos = m_camera->unproject_position(item.second->start);
						viewportPos += item.second->label2dOffset;
						item.second->label2d->set_position(viewportPos);
						item.second->label2d->set_text(item.second->labelText.c_str());
						item.second->label2d->add_theme_color_override("font_color", item.second->color);
					}
				}
			}
		}
		else
		{
			for (auto& item : m_drawings)
			{
				if (item.second->label3d != nullptr)
				{
					Node* parent = item.second->label3d->get_parent();
					if (parent != nullptr)
						parent->remove_child(item.second->label3d);
					item.second->label3d->queue_free();
					item.second->label3d = nullptr;
				}
				if (item.second->label2d != nullptr)
				{
					Node* parent = item.second->label2d->get_parent();
					if (parent != nullptr)
						parent->remove_child(item.second->label2d);
					item.second->label2d->queue_free();
					item.second->label2d = nullptr;
				}
			}
		}
	}
}

void GDN_TheWorld_Drawer::clearDrawings()
{
	m_mesh->clear_surfaces();
}

void GDN_TheWorld_Drawer::drawLine(godot::Vector3 start, godot::Vector3 end, godot::Color c)
{
	m_mesh->surface_begin(godot::Mesh::PrimitiveType::PRIMITIVE_LINES);
	m_mesh->surface_set_color(c);
	m_mesh->surface_add_vertex(start);
	m_mesh->surface_add_vertex(end);
	m_mesh->surface_end();
}

void GDN_TheWorld_Drawer::drawSphere(godot::Vector3 center, float radius, godot::Color c)
{
	float numSteps = 16;
	float stepSize = (TheWorld_Utils::kPi2) / numSteps;

	// move here from the center
	std::vector<godot::Vector3> primaryAxis;
	primaryAxis.push_back(Vector3Up);
	primaryAxis.push_back(Vector3Right);
	primaryAxis.push_back(Vector3Forward);

	// then rotate around this axis
	std::vector<godot::Vector3> rotationAxis;
	rotationAxis.push_back(Vector3Right);
	rotationAxis.push_back(Vector3Forward);
	rotationAxis.push_back(Vector3Up);

	m_mesh->surface_begin(godot::Mesh::PrimitiveType::PRIMITIVE_LINE_STRIP);
	m_mesh->surface_set_color(c);

	for (int axis = 0; axis < primaryAxis.size(); axis++)
	{
		// ad an extra step to close the gap on the final disc of each rotation
		for (float i = 0; i <= numSteps; i++)
		{
			Vector3 vert = (primaryAxis[axis] * radius);
			vert = vert.rotated(rotationAxis[axis], stepSize * i);
			vert += center;
			m_mesh->surface_add_vertex(vert);
		}
		// throw in an extra center vert to tidy up the lines between axes
		m_mesh->surface_add_vertex(center);

	}
	m_mesh->surface_end();
}

int32_t GDN_TheWorld_Drawer::addLine(godot::Vector3 start, godot::Vector3 end, std::string labelText, bool label3d, bool label2d, enum class Drawing::LabelPos labelPos, float label3dOffset, godot::Vector2i label2dOffset, godot::Color c)
{
	int32_t idx = m_firstAvailableIdx++;
	m_drawings[idx] = std::make_unique<Drawing>();
	Drawing* drawing = m_drawings[idx].get();

	drawing->drawingType = Drawing::DrawingType::line;
	drawing->start = start;
	drawing->end = end;
	drawing->color = c;
	drawing->labelPos = labelPos;
	drawing->label_3d = label3d;
	drawing->label_2d = label2d;
	drawing->label3dOffset = label3dOffset;
	drawing->label2dOffset = label2dOffset;
	if (labelText == "@")
		drawing->labelText = "";
	else
		drawing->labelText = labelText;

	return idx;
}

void GDN_TheWorld_Drawer::updateLine(int32_t idx, godot::Vector3 start, godot::Vector3 end, std::string labelText, enum class Drawing::LabelPos labelPos, float label3dOffset, godot::Vector2i label2dOffset, godot::Color c)
{
	if (!m_drawings.contains(idx))
		return;

	Drawing* drawing = m_drawings[idx].get();

	if (drawing->drawingType != Drawing::DrawingType::line)
		return;

	drawing->start = start;
	drawing->end = end;
	if (c.a != 0.0f)
		drawing->color = c;
	if (labelPos != Drawing::LabelPos::not_set)
		drawing->labelPos = labelPos;
	if (label3dOffset >= 0)
		drawing->label3dOffset = label3dOffset;
	if (label2dOffset != godot::Vector2i(0, 0))
		drawing->label2dOffset = label2dOffset;
	if (labelText != "@")
		drawing->labelText = labelText;
	if (start == end)
	{
		if (drawing->label3d != nullptr)
		{
			Node* parent = drawing->label3d->get_parent();
			if (parent != nullptr)
				parent->remove_child(drawing->label3d);
			drawing->label3d->queue_free();
			drawing->label3d = nullptr;
		}

		if (drawing->label2d != nullptr)
		{
			Node* parent = drawing->label2d->get_parent();
			if (parent != nullptr)
				parent->remove_child(drawing->label2d);
			drawing->label2d->queue_free();
			drawing->label2d = nullptr;
		}

	}
}

int32_t GDN_TheWorld_Drawer::addSphere(godot::Vector3 center, float radius, godot::Color c)
{
	int32_t idx = m_firstAvailableIdx++;
	m_drawings[idx] = std::make_unique<Drawing>();
	Drawing* drawing = m_drawings[idx].get();

	drawing->drawingType = Drawing::DrawingType::sphere;
	drawing->start = center;
	drawing->radius = radius;
	drawing->color = c;

	return idx;
}

void GDN_TheWorld_Drawer::updateSphere(int32_t idx, godot::Vector3 center, float radius, godot::Color c)
{
	if (!m_drawings.contains(idx))
		return;

	Drawing* drawing = m_drawings[idx].get();

	if (drawing->drawingType != Drawing::DrawingType::sphere)
		return;

	drawing->start = center;
	if (radius >= 0)
		drawing->radius = radius;
	if (c.a != 0.0f)
		drawing->color = c;
}

int32_t GDN_TheWorld_Drawer::addLabel2d(godot::Vector3 pos, godot::Vector2i offset, std::string labelText, godot::Color c)
{
	int32_t idx = m_firstAvailableIdx++;
	m_drawings[idx] = std::make_unique<Drawing>();
	Drawing* drawing = m_drawings[idx].get();

	drawing->drawingType = Drawing::DrawingType::label2d;
	drawing->start = pos;
	if (labelText == "@")
		drawing->labelText = "";
	else
		drawing->labelText = labelText;
	drawing->label_3d = false;
	drawing->label_2d = true;
	drawing->label2dOffset = offset;
	drawing->color = c;

	return idx;
}

void GDN_TheWorld_Drawer::updateLabel2d(int32_t idx, godot::Vector3 pos, std::string labelText, godot::Color c)
{
	if (!m_drawings.contains(idx))
		return;

	Drawing* drawing = m_drawings[idx].get();

	if (drawing->drawingType != Drawing::DrawingType::label2d)
		return;

	drawing->start = pos;
	if (labelText != "@")
		drawing->labelText = labelText;

	if (c.a != 0.0f)
		drawing->color = c;
}

void GDN_TheWorld_Drawer::removeDrawing(int32_t idx)
{
	if (m_drawings.contains(idx))
		m_drawings.erase(idx);
}
