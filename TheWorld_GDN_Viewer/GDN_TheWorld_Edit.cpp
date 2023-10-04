//#include "pch.h"

#include "GDN_TheWorld_Viewer.h"
#include "GDN_TheWorld_Edit.h"
#include "WorldModifier.h"
#include "Utils.h"
#include "FastNoiseLite.h"
#include "half.h"
#include "UnicodeDefine.h"
#include <typeinfo>

#pragma warning (push, 0)
#include <godot_cpp/classes/v_box_container.hpp>
#include <godot_cpp/classes/h_box_container.hpp>
#include <godot_cpp/classes/scroll_container.hpp>
#include <godot_cpp/classes/margin_container.hpp>
#include <godot_cpp/classes/panel.hpp>
#include <godot_cpp/classes/button.hpp>
#include <godot_cpp/classes/h_separator.hpp>
#include <godot_cpp/classes/texture_rect.hpp>
#include <godot_cpp/classes/v_separator.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/input_event_mouse_motion.hpp>
#include <godot_cpp/classes/input_event_mouse_button.hpp>
#include <godot_cpp/classes/input_event_key.hpp>
#include <godot_cpp/classes/style_box_flat.hpp>
#include <godot_cpp/classes/style_box_empty.hpp>
#pragma warning (pop)

#include <string>
#include <iostream>

#define INFO_BUTTON_TEXT L"Info"
#define NOISE_BUTTON_TEXT L"Terrain Generation"
#define TERRAIN_EDIT_BUTTON_TEXT L"Terrain Edit"

using namespace godot;

const std::string c_lowElevationEditTextureName = "Low Elevation";
const std::string c_highElevationEditTextureName = "High Elevation";
const std::string c_dirtEditTextureName = "Dirt";
const std::string c_rocksEditTextureName = "Rocks";

void GDN_TheWorld_Edit::_bind_methods()
{
	ClassDB::bind_method(D_METHOD("edit_mode_generate"), &GDN_TheWorld_Edit::editModeGenerateAction);
	ClassDB::bind_method(D_METHOD("edit_mode_blend"), &GDN_TheWorld_Edit::editModeBlendAction);
	ClassDB::bind_method(D_METHOD("edit_mode_gen_normals"), &GDN_TheWorld_Edit::editModeGenNormalsAction);
	ClassDB::bind_method(D_METHOD("edit_mode_apply_textures"), &GDN_TheWorld_Edit::editModeApplyTexturesAction);
	ClassDB::bind_method(D_METHOD("edit_mode_save"), &GDN_TheWorld_Edit::editModeSaveAction);
	ClassDB::bind_method(D_METHOD("edit_mode_upload"), &GDN_TheWorld_Edit::editModeUploadAction);
	ClassDB::bind_method(D_METHOD("edit_mode_stop"), &GDN_TheWorld_Edit::editModeStopAction);
	ClassDB::bind_method(D_METHOD("edit_mode_sel_terr_type"), &GDN_TheWorld_Edit::editModeSelectTerrainTypeAction);
	ClassDB::bind_method(D_METHOD("edit_mode_sel_lookdev"), &GDN_TheWorld_Edit::editModeSelectLookDevAction);
	ClassDB::bind_method(D_METHOD("mouse_entered_main_panel"), &GDN_TheWorld_Edit::editModeMouseEnteredMainPanel);
	ClassDB::bind_method(D_METHOD("mouse_exited_main_panel"), &GDN_TheWorld_Edit::editModeMouseExitedMainPanel);
	ClassDB::bind_method(D_METHOD("control_need_resize"), &GDN_TheWorld_Edit::controlNeedResize);
	ClassDB::bind_method(D_METHOD("void_signal_manager", "p_object", "p_signal", "p_class_name", "p_instance_id", "p_object_name", "p_custom1", "p_custom2", "p_custom3"), &GDN_TheWorld_Edit::voidSignalManager);
	ClassDB::bind_method(D_METHOD("gui_input_event", "p_event, p_object", "p_signal", "p_class_name", "p_instance_id", "p_object_name", "p_custom1", "p_custom2", "p_custom3"), &GDN_TheWorld_Edit::guiInputEvent);
}

GDN_TheWorld_Edit::GDN_TheWorld_Edit()
{
	m_innerData = std::make_unique<InnerData>();
	m_quitting = false;
	m_initialized = false;
	m_ready = false;
	m_actionInProgress = false;
	m_actionStopRequested = false;
	m_onGoingElapsedLabel = false;
	m_viewer = nullptr;
	m_controlNeedResize = false;
	m_scrollPanelsNeedResize = false;
	m_infoLabelTextChanged = false;
	m_minHeightStr = new std::string();
	m_maxHeightStr = new std::string();
	m_counterStr = new std::string();
	m_note1Str = new std::string();
	m_mouseHitStr = new std::string();
	m_mouseQuadHitStr = new std::string();
	m_mouseQuadHitPosStr = new std::string();
	m_mouseQuadSelStr = new std::string();
	m_mouseQuadSelPosStr = new std::string();
	m_allItems = 0;
	m_completedItems = 0;
	m_elapsedCompleted = 0;
	m_lastElapsed = 0;
	m_UIAcceptingFocus = false;
	m_requiredUIAcceptFocus = false;
	m_lastMessageChanged = false;

	_init();
}

GDN_TheWorld_Edit::~GDN_TheWorld_Edit()
{
	deinit();

	delete m_minHeightStr;
	delete m_maxHeightStr;
	delete m_counterStr;
	delete m_note1Str;
	delete m_mouseHitStr;
	delete m_mouseQuadHitStr;
	delete m_mouseQuadHitPosStr;
	delete m_mouseQuadSelStr;
	delete m_mouseQuadSelPosStr;
}

void GDN_TheWorld_Edit::replyFromServer(TheWorld_ClientServer::ClientServerExecution& reply)
{
	std::string method = reply.getMethod();

	if (reply.error())
	{
		m_viewer->Globals()->setStatus(TheWorldStatus::error);
		m_viewer->Globals()->errorPrint((std::string("GDN_TheWorld_Edit::replyFromServer: error method ") + method + std::string(" rc=") + std::to_string(reply.getErrorCode()) + std::string(" ") + reply.getErrorMessage()).c_str());
		return;
	}

	try
	{
		if (method == THEWORLD_CLIENTSERVER_METHOD_MAPM_UPLOADCACHEBUFFER)
		{
			ClientServerVariant v = reply.getInputParam(0);
			const auto _lowerXGridVertex(std::get_if<float>(&v));
			if (_lowerXGridVertex == NULL)
			{
				std::string m = std::string("Reply MapManager::getVertices did not have a float as first input param");
				throw(GDN_TheWorld_Exception(__FUNCTION__, m.c_str()));
			}
			float lowerXGridVertex = *_lowerXGridVertex;

			v = reply.getInputParam(1);
			const auto _lowerZGridVertex(std::get_if<float>(&v));
			if (_lowerZGridVertex == NULL)
			{
				std::string m = std::string("Reply MapManager::getVertices did not have a float as second input param");
				throw(GDN_TheWorld_Exception(__FUNCTION__, m.c_str()));
			}
			float lowerZGridVertex = *_lowerZGridVertex;

			v = reply.getInputParam(2);
			const auto _numVerticesPerSize(std::get_if<int>(&v));
			if (_numVerticesPerSize == NULL)
			{
				std::string m = std::string("Reply MapManager::getVertices did not have an int as third input param");
				throw(GDN_TheWorld_Exception(__FUNCTION__, m.c_str()));
			}
			int numVerticesPerSize = *_numVerticesPerSize;

			v = reply.getInputParam(3);
			const auto _gridStepinWU(std::get_if<float>(&v));
			if (_gridStepinWU == NULL)
			{
				std::string m = std::string("Reply MapManager::getVertices did not have a float as forth input param");
				throw(GDN_TheWorld_Exception(__FUNCTION__, m.c_str()));
			}
			float gridStepInWU = *_gridStepinWU;

			v = reply.getInputParam(4);
			const auto _level(std::get_if<int>(&v));
			if (_level == NULL)
			{
				std::string m = std::string("Reply MapManager::getVertices did not have an int as fifth input param");
				throw(GDN_TheWorld_Exception(__FUNCTION__, m.c_str()));
			}
			int level = *_level;

			QuadrantPos pos(lowerXGridVertex, lowerZGridVertex, level, numVerticesPerSize, gridStepInWU);
			QuadTree* quadTree = m_viewer->getQuadTree(pos);
			if (quadTree != nullptr)
			{
				quadTree->getQuadrant()->setNeedUploadToServer(false);
				quadTree->setStatus(QuadrantStatus::refreshTerrainDataNeeded);
			}

			m_completedItems++;
			size_t partialCount = m_actionClock.partialDuration().count();
			m_lastElapsed = partialCount - m_elapsedCompleted;
			m_elapsedCompleted = partialCount;
		}
		else
		{
			m_viewer->Globals()->setStatus(TheWorldStatus::error);
			m_viewer->Globals()->errorPrint((std::string("GDN_TheWorld_Edit::replyFromServer: unknow method ") + method).c_str());
		}
	}
	catch (GDN_TheWorld_Exception ex)
	{
		m_viewer->Globals()->setStatus(TheWorldStatus::error);
		m_viewer->Globals()->errorPrint(String(ex.exceptionName()) + String(" caught - ") + ex.what());
	}
	catch (...)
	{
		m_viewer->Globals()->setStatus(TheWorldStatus::error);
		m_viewer->Globals()->errorPrint("GDN_TheWorld_Globals::GDN_TheWorld_Edit: exception caught");
	}
}

void GDN_TheWorld_Edit::editModeMouseEnteredMainPanel(void)
{
	m_requiredUIAcceptFocus = true;
	//setUIAcceptFocus(true);
}

void GDN_TheWorld_Edit::editModeMouseExitedMainPanel(void)
{
	m_requiredUIAcceptFocus = false;
	//setUIAcceptFocus(false);
}

bool GDN_TheWorld_Edit::UIAcceptingFocus(void)
{
	return m_UIAcceptingFocus;
}

void GDN_TheWorld_Edit::setUIAcceptFocus(bool b)
{
	if (m_UIAcceptingFocus == b)
		return;
		
	enum godot::Control::FocusMode focusMode = godot::Control::FocusMode::FOCUS_ALL;

	if (!b)
	{
		focusMode = godot::Control::FocusMode::FOCUS_NONE;
	}

	//if (b)
	//	setSeed(1);
	//else
	//	setSeed(0);

	m_innerData->m_allCheckBox->set_focus_mode(focusMode);
	m_innerData->m_seed->set_focus_mode(focusMode);
	m_innerData->m_frequency->set_focus_mode(focusMode);
	m_innerData->m_fractalOctaves->set_focus_mode(focusMode);
	m_innerData->m_fractalLacunarity->set_focus_mode(focusMode);
	m_innerData->m_fractalGain->set_focus_mode(focusMode);
	m_innerData->m_amplitudeLabel->set_focus_mode(focusMode);
	m_innerData->m_scaleFactorLabel->set_focus_mode(focusMode);
	m_innerData->m_desideredMinHeightLabel->set_focus_mode(focusMode);
	m_innerData->m_fractalWeightedStrength->set_focus_mode(focusMode);
	m_innerData->m_fractalPingPongStrength->set_focus_mode(focusMode);

	m_UIAcceptingFocus = b;
}

void GDN_TheWorld_Edit::init(GDN_TheWorld_Viewer* viewer)
{
	m_viewer = viewer;

	//TheWorld_Utils::WorldModifierPos pos(0, 4096, 4096, TheWorld_Utils::WMType::elevator, 0);
	//m_wms[pos] = std::make_unique<TheWorld_Utils::WorldModifier>(pos, TheWorld_Utils::WMFunctionType::ConsiderMinMax, 10000.0f, 0.0f, 200.0f, TheWorld_Utils::WMOrder::MaxEffectOnWM);

	std::lock_guard<std::recursive_mutex> lock(m_viewer->getMainProcessingMutex());

	int32_t border_size = 2;
	{
		godot::Ref<godot::StyleBoxFlat> styleBox = memnew(godot::StyleBoxFlat);
		m_innerData->m_styleBoxSelectedFrame = styleBox;
		Color c = m_innerData->m_styleBoxSelectedFrame->get_bg_color();
		c.a = 0.0f;
		m_innerData->m_styleBoxSelectedFrame->set_bg_color(c);
		m_innerData->m_styleBoxSelectedFrame->set_border_color(GDN_TheWorld_Globals::g_color_yellow_amber);
		m_innerData->m_styleBoxSelectedFrame->set_border_width(godot::Side::SIDE_TOP, border_size);
		m_innerData->m_styleBoxSelectedFrame->set_border_width(godot::Side::SIDE_BOTTOM, border_size);
		m_innerData->m_styleBoxSelectedFrame->set_border_width(godot::Side::SIDE_RIGHT, border_size);
		m_innerData->m_styleBoxSelectedFrame->set_border_width(godot::Side::SIDE_LEFT, border_size);
	}
	{
		godot::Ref<godot::StyleBoxFlat> styleBox = memnew(godot::StyleBoxFlat);
		m_innerData->m_styleBoxHighlightedFrame = styleBox;
		Color c = m_innerData->m_styleBoxHighlightedFrame->get_bg_color();
		c.a = 0.0f;
		m_innerData->m_styleBoxHighlightedFrame->set_bg_color(c);
		m_innerData->m_styleBoxHighlightedFrame->set_border_color(GDN_TheWorld_Globals::g_color_white);
		m_innerData->m_styleBoxHighlightedFrame->set_border_width(godot::Side::SIDE_TOP, border_size);
		m_innerData->m_styleBoxHighlightedFrame->set_border_width(godot::Side::SIDE_BOTTOM, border_size);
		m_innerData->m_styleBoxHighlightedFrame->set_border_width(godot::Side::SIDE_RIGHT, border_size);
		m_innerData->m_styleBoxHighlightedFrame->set_border_width(godot::Side::SIDE_LEFT, border_size);
	}

	//if (!IS_EDITOR_HINT())
	m_viewer->add_child(this);
	set_name(THEWORLD_EDIT_MODE_UI_CONTROL_NAME);

	//const Color self_modulate = get_self_modulate();
	//const Color self_modulate_to_transparency(1.0f, 1.0f, 1.0f, 0.5f);
	const Color self_modulate_to_transparency(0.0f, 0.0f, 0.0f, 0.5f);
	//const Color self_modulate(1.0f, 1.0f, 1.0f, 1.0f);

	godot::HBoxContainer* hBoxContainer = nullptr;
	godot::Control* separator = nullptr;
	godot::Button* button = nullptr;
	godot::Label* label = nullptr;
	godot::Error e;

	m_innerData->m_mainPanelContainer = createControl<godot::PanelContainer>(this, "");

	m_innerData->m_mainTabContainer = createControl<godot::TabContainer>(m_innerData->m_mainPanelContainer, "", "", "", nullptr, "", "", "", "", self_modulate_to_transparency);
	//m_innerData->m_mainTabContainer = createControl<godot::TabContainer>(m_innerData->m_mainPanelContainer, "");

	m_innerData->m_mainTerrainScrollContainer = createControl<godot::ScrollContainer>(m_innerData->m_mainTabContainer, "Terrain");
	m_innerData->m_mainTerrainScrollContainer->set_h_size_flags(godot::Control::SizeFlags::SIZE_FILL);
	m_innerData->m_mainTerrainScrollContainer->set_v_size_flags(godot::Control::SizeFlags::SIZE_FILL);
	m_innerData->m_mainTerrainScrollContainer->set_horizontal_scroll_mode(godot::ScrollContainer::SCROLL_MODE_AUTO);
	m_innerData->m_mainTerrainScrollContainer->set_vertical_scroll_mode(godot::ScrollContainer::SCROLL_MODE_AUTO);

	godot::MarginContainer* marginContainer = nullptr;
	marginContainer = createControl<godot::MarginContainer>(m_innerData->m_mainTerrainScrollContainer, "");
	if (!IS_EDITOR_HINT())
	{
		marginContainer->add_theme_constant_override("margin_left", 5);
		marginContainer->add_theme_constant_override("margin_right", 5);
	}
	//marginContainer->add_theme_constant_override("margin_top", 5);
	//marginContainer->add_theme_constant_override("margin_bottom", 5);

	m_innerData->m_mainTerrainVBoxContainer = createControl<godot::VBoxContainer>(marginContainer);

	{
		hBoxContainer = createControl<godot::HBoxContainer>(m_innerData->m_mainTerrainVBoxContainer);
		label = createControl<godot::Label>(hBoxContainer, "", "Quads to save");
		m_innerData->m_numQuadrantToSaveLabel = createControl<godot::Label>(hBoxContainer);
		m_innerData->m_numQuadrantToSaveLabel->set_horizontal_alignment(godot::HorizontalAlignment::HORIZONTAL_ALIGNMENT_RIGHT);
		label = createControl<godot::Label>(hBoxContainer, "", "Quads to upload");
		m_innerData->m_numQuadrantToUploadLabel = createControl<godot::Label>(hBoxContainer);
		m_innerData->m_numQuadrantToUploadLabel->set_horizontal_alignment(godot::HorizontalAlignment::HORIZONTAL_ALIGNMENT_RIGHT);

		hBoxContainer = createControl<godot::HBoxContainer>(m_innerData->m_mainTerrainVBoxContainer);
		button = createControl<godot::Button>(hBoxContainer, "", "Save", "pressed", this, "edit_mode_save");
		separator = createControl<godot::VSeparator>(hBoxContainer);
		button = createControl<godot::Button>(hBoxContainer, "", "Upload", "pressed", this, "edit_mode_upload");
		separator = createControl<godot::VSeparator>(hBoxContainer);
		button = createControl<godot::Button>(hBoxContainer, "", "Stop", "pressed", this, "edit_mode_stop");

		hBoxContainer = createControl<godot::HBoxContainer>(m_innerData->m_mainTerrainVBoxContainer);
		button = createControl<godot::Button>(hBoxContainer, "", "Blend", "pressed", this, "edit_mode_blend");
		separator = createControl<godot::VSeparator>(hBoxContainer);
		button = createControl<godot::Button>(hBoxContainer, "", "Gen. Normals", "pressed", this, "edit_mode_gen_normals");
		separator = createControl<godot::VSeparator>(hBoxContainer);
		button = createControl<godot::Button>(hBoxContainer, "", "Apply Texs", "pressed", this, "edit_mode_apply_textures");
		m_innerData->m_allCheckBox = createControl<godot::CheckBox>(hBoxContainer);
		m_innerData->m_allCheckBox->set_text("All");
		m_innerData->m_allCheckBox->set_toggle_mode(true);

		hBoxContainer = createControl<godot::HBoxContainer>(m_innerData->m_mainTerrainVBoxContainer);
		m_innerData->m_message = createControl<godot::Label>(hBoxContainer);

		hBoxContainer = createControl<godot::HBoxContainer>(m_innerData->m_mainTerrainVBoxContainer);
		label = createControl<godot::Label>(hBoxContainer, "", "Elapsed");
		m_innerData->m_elapsedLabel = createControl<godot::Label>(hBoxContainer);
		m_innerData->m_elapsed1Label = createControl<godot::Label>(hBoxContainer);
	}
	//setMessage(std::string("AAAAAAAAAAAAAAAAAAAAAAA"));

	{
		//m_innerData->m_infoButton = createControl<godot::Button>(m_innerData->m_mainTerrainVBoxContainer, "", (std::wstring(DOWN_ARROW) + L" " + INFO_BUTTON_TEXT).c_str(), "pressed", this, "edit_mode_info_panel");
		m_innerData->m_infoButton = createControl<godot::Button>(m_innerData->m_mainTerrainVBoxContainer, "InfoPanelToggle", (std::wstring(DOWN_ARROW) + L" " + INFO_BUTTON_TEXT).c_str(), "pressed", this, "void_signal_manager");
		m_innerData->m_infoButton->set_text_alignment(godot::HorizontalAlignment::HORIZONTAL_ALIGNMENT_LEFT);

		m_innerData->m_infoVBoxContainer = createControl<godot::VBoxContainer>(m_innerData->m_mainTerrainVBoxContainer);

		m_innerData->m_infoLabel = createControl<godot::Label>(m_innerData->m_infoVBoxContainer);
	}

	{
		hBoxContainer = createControl<godot::HBoxContainer>(m_innerData->m_mainTerrainVBoxContainer);
		m_innerData->m_lookDevOptionButton = createControl<godot::OptionButton>(hBoxContainer, "", "", "item_selected", this, "edit_mode_sel_lookdev");
		m_innerData->m_lookDevOptionButton->set_h_size_flags(godot::OptionButton::SizeFlags::SIZE_EXPAND_FILL);
		m_innerData->m_lookDevOptionButton->set_toggle_mode(true);
		//m_innerData->m_lookDevOptionButton->connect("item_selected", Callable(this, "edit_mode_sel_lookdev"));
		//m_innerData->m_lookDevOptionButton->set_focus_mode(godot::Control::FocusMode::FOCUS_NONE);
		m_innerData->m_lookDevOptionButton->add_item("Lookdev disabled", (int64_t)ShaderTerrainData::LookDev::NotSet);
		m_innerData->m_lookDevOptionButton->add_separator();
		m_innerData->m_lookDevOptionButton->add_item("Lookdev heights", (int64_t)ShaderTerrainData::LookDev::Heights);
		m_innerData->m_lookDevOptionButton->add_item("Lookdev normals", (int64_t)ShaderTerrainData::LookDev::Normals);
		m_innerData->m_lookDevOptionButton->add_item("Lookdev splatmap", (int64_t)ShaderTerrainData::LookDev::Splat);
		m_innerData->m_lookDevOptionButton->add_item("Lookdev colors", (int64_t)ShaderTerrainData::LookDev::Color);
		m_innerData->m_lookDevOptionButton->add_item("Lookdev globalmap", (int64_t)ShaderTerrainData::LookDev::Global);
	}

	{
		//m_innerData->m_noiseButton = createControl<godot::Button>(m_innerData->m_mainTerrainVBoxContainer, "NoisePanelToggle", (std::wstring(RIGHT_ARROW) + L" " + NOISE_BUTTON_TEXT).c_str(), "pressed", this, "edit_mode_noise_panel");
		m_innerData->m_noiseButton = createControl<godot::Button>(m_innerData->m_mainTerrainVBoxContainer, "NoisePanelToggle", (std::wstring(RIGHT_ARROW) + L" " + NOISE_BUTTON_TEXT).c_str(), "pressed", this, "void_signal_manager");
		m_innerData->m_noiseButton->set_text_alignment(godot::HorizontalAlignment::HORIZONTAL_ALIGNMENT_LEFT);

		m_innerData->m_noiseVBoxContainer = createControl<godot::VBoxContainer>(m_innerData->m_mainTerrainVBoxContainer);
		m_innerData->m_noiseVBoxContainer->hide();

		separator = createControl<HSeparator>(m_innerData->m_noiseVBoxContainer);
		
		hBoxContainer = createControl<godot::HBoxContainer>(m_innerData->m_noiseVBoxContainer);
		m_innerData->m_terrTypeOptionButton = createControl<godot::OptionButton>(hBoxContainer, "", "", "item_selected", this, "edit_mode_sel_terr_type");
		m_innerData->m_terrTypeOptionButton->add_item(TheWorld_Utils::TerrainEdit::terrainTypeString(TheWorld_Utils::TerrainEdit::TerrainType::unknown).c_str(), (int64_t)TheWorld_Utils::TerrainEdit::TerrainType::unknown);
		m_innerData->m_terrTypeOptionButton->add_separator();
		m_innerData->m_terrTypeOptionButton->add_item(TheWorld_Utils::TerrainEdit::terrainTypeString(TheWorld_Utils::TerrainEdit::TerrainType::campaign_1).c_str(), (int64_t)TheWorld_Utils::TerrainEdit::TerrainType::campaign_1);
		m_innerData->m_terrTypeOptionButton->add_item(TheWorld_Utils::TerrainEdit::terrainTypeString(TheWorld_Utils::TerrainEdit::TerrainType::plateau_1).c_str(), (int64_t)TheWorld_Utils::TerrainEdit::TerrainType::plateau_1);
		m_innerData->m_terrTypeOptionButton->add_item(TheWorld_Utils::TerrainEdit::terrainTypeString(TheWorld_Utils::TerrainEdit::TerrainType::low_hills).c_str(), (int64_t)TheWorld_Utils::TerrainEdit::TerrainType::low_hills);
		m_innerData->m_terrTypeOptionButton->add_item(TheWorld_Utils::TerrainEdit::terrainTypeString(TheWorld_Utils::TerrainEdit::TerrainType::high_hills).c_str(), (int64_t)TheWorld_Utils::TerrainEdit::TerrainType::high_hills);
		m_innerData->m_terrTypeOptionButton->add_item(TheWorld_Utils::TerrainEdit::terrainTypeString(TheWorld_Utils::TerrainEdit::TerrainType::low_mountains).c_str(), (int64_t)TheWorld_Utils::TerrainEdit::TerrainType::low_mountains);
		m_innerData->m_terrTypeOptionButton->add_item(TheWorld_Utils::TerrainEdit::terrainTypeString(TheWorld_Utils::TerrainEdit::TerrainType::low_mountains_grow).c_str(), (int64_t)TheWorld_Utils::TerrainEdit::TerrainType::low_mountains_grow);
		m_innerData->m_terrTypeOptionButton->add_item(TheWorld_Utils::TerrainEdit::terrainTypeString(TheWorld_Utils::TerrainEdit::TerrainType::high_mountains_1).c_str(), (int64_t)TheWorld_Utils::TerrainEdit::TerrainType::high_mountains_1);
		m_innerData->m_terrTypeOptionButton->add_item(TheWorld_Utils::TerrainEdit::terrainTypeString(TheWorld_Utils::TerrainEdit::TerrainType::high_mountains_1_grow).c_str(), (int64_t)TheWorld_Utils::TerrainEdit::TerrainType::high_mountains_1_grow);
		m_innerData->m_terrTypeOptionButton->add_item(TheWorld_Utils::TerrainEdit::terrainTypeString(TheWorld_Utils::TerrainEdit::TerrainType::high_mountains_2).c_str(), (int64_t)TheWorld_Utils::TerrainEdit::TerrainType::high_mountains_2);
		m_innerData->m_terrTypeOptionButton->add_item(TheWorld_Utils::TerrainEdit::terrainTypeString(TheWorld_Utils::TerrainEdit::TerrainType::high_mountains_2_grow).c_str(), (int64_t)TheWorld_Utils::TerrainEdit::TerrainType::high_mountains_2_grow);
		m_innerData->m_terrTypeOptionButton->add_separator();
		m_innerData->m_terrTypeOptionButton->add_item(TheWorld_Utils::TerrainEdit::terrainTypeString(TheWorld_Utils::TerrainEdit::TerrainType::noise_1).c_str(), (int64_t)TheWorld_Utils::TerrainEdit::TerrainType::noise_1);
		separator = createControl<godot::VSeparator>(hBoxContainer);
		button = createControl<godot::Button>(hBoxContainer, "", "Generate", "pressed", this, "edit_mode_generate");
		separator = createControl<godot::VSeparator>(hBoxContainer);

		hBoxContainer = createControl<godot::HBoxContainer>(m_innerData->m_noiseVBoxContainer);
		label = createControl<godot::Label>(hBoxContainer, "", "Seed");
		m_innerData->m_seed = createControl<godot::LineEdit>(hBoxContainer);
		label = createControl<godot::Label>(hBoxContainer, "", "Frequency");
		m_innerData->m_frequency = createControl<godot::LineEdit>(hBoxContainer);
		label = createControl<godot::Label>(hBoxContainer, "", "Gain");
		m_innerData->m_fractalGain = createControl<godot::LineEdit>(hBoxContainer);

		hBoxContainer = createControl<godot::HBoxContainer>(m_innerData->m_noiseVBoxContainer);
		label = createControl<godot::Label>(hBoxContainer, "", "Octaves");
		m_innerData->m_fractalOctaves = createControl<godot::LineEdit>(hBoxContainer);
		label = createControl<godot::Label>(hBoxContainer, "", "Lacunarity");
		m_innerData->m_fractalLacunarity = createControl<godot::LineEdit>(hBoxContainer);

		hBoxContainer = createControl<godot::HBoxContainer>(m_innerData->m_noiseVBoxContainer);
		label = createControl<godot::Label>(hBoxContainer, "", "Amplitude");
		m_innerData->m_amplitudeLabel = createControl<godot::LineEdit>(hBoxContainer);
		label = createControl<godot::Label>(hBoxContainer, "", "Scale");
		m_innerData->m_scaleFactorLabel = createControl<godot::LineEdit>(hBoxContainer);
		label = createControl<godot::Label>(hBoxContainer, "", "Start H");
		m_innerData->m_desideredMinHeightLabel = createControl<godot::LineEdit>(hBoxContainer);

		hBoxContainer = createControl<godot::HBoxContainer>(m_innerData->m_noiseVBoxContainer);
		label = createControl<godot::Label>(hBoxContainer, "", "Weighted Strength");
		m_innerData->m_fractalWeightedStrength = createControl<godot::LineEdit>(hBoxContainer);

		hBoxContainer = createControl<godot::HBoxContainer>(m_innerData->m_noiseVBoxContainer);
		label = createControl<godot::Label>(hBoxContainer, "", "Ping Pong Strength");
		m_innerData->m_fractalPingPongStrength = createControl<godot::LineEdit>(hBoxContainer);
	}

	{
		//m_innerData->m_terrEditButton = createControl<godot::Button>(m_innerData->m_mainTerrainVBoxContainer, "", (std::wstring(RIGHT_ARROW) + L" " + TERRAIN_EDIT_BUTTON_TEXT).c_str(), "pressed", this, "edit_mode_terredit_panel");
		m_innerData->m_terrEditButton = createControl<godot::Button>(m_innerData->m_mainTerrainVBoxContainer, "TerrEditPanelToggle", (std::wstring(RIGHT_ARROW) + L" " + TERRAIN_EDIT_BUTTON_TEXT).c_str(), "pressed", this, "void_signal_manager");
		m_innerData->m_terrEditButton->set_text_alignment(godot::HorizontalAlignment::HORIZONTAL_ALIGNMENT_LEFT);

		m_innerData->m_terrEditVBoxContainer = createControl<godot::VBoxContainer>(m_innerData->m_mainTerrainVBoxContainer);
		m_innerData->m_terrEditVBoxContainer->hide();

		godot::Vector2 size(70, 70);
		
		label = createControl<godot::Label>(m_innerData->m_terrEditVBoxContainer, "", c_lowElevationEditTextureName.c_str());
		m_innerData->m_lowElevationTexPanel = createControl<godot::PanelContainer>(m_innerData->m_terrEditVBoxContainer, "LowElevationTexPanel");
		e = GDN_TheWorld_Globals::connectSignal(m_innerData->m_lowElevationTexPanel, "godot::PanelContainer", "mouse_entered", this, "void_signal_manager", c_lowElevationEditTextureName.c_str());
		e = GDN_TheWorld_Globals::connectSignal(m_innerData->m_lowElevationTexPanel, "godot::PanelContainer", "mouse_exited", this, "void_signal_manager", c_lowElevationEditTextureName.c_str());
		e = GDN_TheWorld_Globals::connectSignal(m_innerData->m_lowElevationTexPanel, "godot::PanelContainer", "gui_input", this, "gui_input_event", c_lowElevationEditTextureName.c_str());
		hBoxContainer = createControl<godot::HBoxContainer>(m_innerData->m_lowElevationTexPanel);
		m_innerData->m_lowElevationTex = createControl<godot::TextureRect>(hBoxContainer);
		m_innerData->m_lowElevationTex->set_expand_mode(godot::TextureRect::EXPAND_IGNORE_SIZE);
		m_innerData->m_lowElevationTex->set_custom_minimum_size(size);
		m_innerData->m_lowElevationTex->set_stretch_mode(godot::TextureRect::STRETCH_SCALE);
		m_innerData->m_lowElevationTexName = createControl<godot::Label>(hBoxContainer);
		//button = createControl<godot::Button>(hBoxContainer, "", "Change", "pressed", this, "void_signal_manager");

		label = createControl<godot::Label>(m_innerData->m_terrEditVBoxContainer, "", c_highElevationEditTextureName.c_str());
		m_innerData->m_highElevationTexPanel = createControl<godot::PanelContainer>(m_innerData->m_terrEditVBoxContainer, "HighElevationTexPanel");
		e = GDN_TheWorld_Globals::connectSignal(m_innerData->m_highElevationTexPanel, "godot::PanelContainer", "mouse_entered", this, "void_signal_manager", c_highElevationEditTextureName.c_str());
		e = GDN_TheWorld_Globals::connectSignal(m_innerData->m_highElevationTexPanel, "godot::PanelContainer", "mouse_exited", this, "void_signal_manager", c_highElevationEditTextureName.c_str());
		e = GDN_TheWorld_Globals::connectSignal(m_innerData->m_highElevationTexPanel, "godot::PanelContainer", "gui_input", this, "gui_input_event", c_highElevationEditTextureName.c_str());
		hBoxContainer = createControl<godot::HBoxContainer>(m_innerData->m_highElevationTexPanel);
		m_innerData->m_highElevationTex = createControl<godot::TextureRect>(hBoxContainer);
		m_innerData->m_highElevationTex->set_expand_mode(godot::TextureRect::EXPAND_IGNORE_SIZE);
		m_innerData->m_highElevationTex->set_custom_minimum_size(size);
		m_innerData->m_highElevationTex->set_stretch_mode(godot::TextureRect::STRETCH_SCALE);
		m_innerData->m_highElevationTexName = createControl<godot::Label>(hBoxContainer);
		//button = createControl<godot::Button>(hBoxContainer, "", "Change", "pressed", this, "void_signal_manager");

		label = createControl<godot::Label>(m_innerData->m_terrEditVBoxContainer, "", c_dirtEditTextureName.c_str());
		m_innerData->m_dirtTexPanel = createControl<godot::PanelContainer>(m_innerData->m_terrEditVBoxContainer, "DirtTexPanel");
		e = GDN_TheWorld_Globals::connectSignal(m_innerData->m_dirtTexPanel, "godot::PanelContainer", "mouse_entered", this, "void_signal_manager", c_dirtEditTextureName.c_str());
		e = GDN_TheWorld_Globals::connectSignal(m_innerData->m_dirtTexPanel, "godot::PanelContainer", "mouse_exited", this, "void_signal_manager", c_dirtEditTextureName.c_str());
		e = GDN_TheWorld_Globals::connectSignal(m_innerData->m_dirtTexPanel, "godot::PanelContainer", "gui_input", this, "gui_input_event", c_dirtEditTextureName.c_str());
		hBoxContainer = createControl<godot::HBoxContainer>(m_innerData->m_dirtTexPanel);
		m_innerData->m_dirtTex = createControl<godot::TextureRect>(hBoxContainer);
		m_innerData->m_dirtTex->set_expand_mode(godot::TextureRect::EXPAND_IGNORE_SIZE);
		m_innerData->m_dirtTex->set_custom_minimum_size(size);
		m_innerData->m_dirtTex->set_stretch_mode(godot::TextureRect::STRETCH_SCALE);
		m_innerData->m_dirtTexName = createControl<godot::Label>(hBoxContainer);
		//button = createControl<godot::Button>(hBoxContainer, "", "Change", "pressed", this, "void_signal_manager");

		label = createControl<godot::Label>(m_innerData->m_terrEditVBoxContainer, "", c_rocksEditTextureName.c_str());
		m_innerData->m_rocksTexPanel = createControl<godot::PanelContainer>(m_innerData->m_terrEditVBoxContainer, "RocksTexPanel");
		e = GDN_TheWorld_Globals::connectSignal(m_innerData->m_rocksTexPanel, "godot::PanelContainer", "mouse_entered", this, "void_signal_manager", c_rocksEditTextureName.c_str());
		e = GDN_TheWorld_Globals::connectSignal(m_innerData->m_rocksTexPanel, "godot::PanelContainer", "mouse_exited", this, "void_signal_manager", c_rocksEditTextureName.c_str());
		e = GDN_TheWorld_Globals::connectSignal(m_innerData->m_rocksTexPanel, "godot::PanelContainer", "gui_input", this, "gui_input_event", c_rocksEditTextureName.c_str());
		hBoxContainer = createControl<godot::HBoxContainer>(m_innerData->m_rocksTexPanel);
		m_innerData->m_rocksTex = createControl<godot::TextureRect>(hBoxContainer);
		m_innerData->m_rocksTex->set_expand_mode(godot::TextureRect::EXPAND_IGNORE_SIZE);
		m_innerData->m_rocksTex->set_custom_minimum_size(size);
		m_innerData->m_rocksTex->set_stretch_mode(godot::TextureRect::STRETCH_SCALE);
		m_innerData->m_rocksTexName = createControl<godot::Label>(hBoxContainer);
		//button = createControl<godot::Button>(hBoxContainer, "", "Change", "pressed", this, "void_signal_manager");
	}
	
	separator = createControl<Label>(m_innerData->m_mainTerrainVBoxContainer, "", " ");

	controlNeedResize();

	size_t numToSave = 0;
	size_t numToUpload = 0;
	refreshNumToSaveUpload(numToSave, numToUpload);

	m_tp.Start("EditModeWorker", 1, this);

	m_initialized = true;
}

template <class T>
T* GDN_TheWorld_Edit::createControl(godot::Node* parent, std::string name, godot::String text, std::string signal, godot::Object* callableObject, std::string callableMethod, godot::Variant custom1, godot::Variant custom2, godot::Variant custom3, Color selfModulateColor)
{
	T* control = memnew(T);
	parent->add_child(control);
	godot::Error e = control->connect("mouse_entered", Callable(this, "mouse_entered_main_panel"));
	e = control->connect("mouse_exited", Callable(this, "mouse_exited_main_panel"));
	
	if (name.length() > 0)
		control->set_name(name.c_str());

	//Color c = control->get_self_modulate();
	if (selfModulateColor != Color(0.0f, 0.0f, 0.0f, 0.0f))
		control->set_self_modulate(selfModulateColor);
	//c = control->get_self_modulate();

	if constexpr (std::is_same_v<T, godot::Label>)
	{
		if (text.length() > 0)
			control->set_text(text);
		control->set_horizontal_alignment(godot::HorizontalAlignment::HORIZONTAL_ALIGNMENT_LEFT);
	}
	else if constexpr (std::is_same_v<T, godot::MarginContainer>)
	{
		control->set_h_size_flags(godot::MarginContainer::SizeFlags::SIZE_EXPAND_FILL);
		control->set_v_size_flags(godot::MarginContainer::SizeFlags::SIZE_EXPAND_FILL);
	}
	else if constexpr (std::is_same_v<T, godot::VBoxContainer>)
	{
		control->set_h_size_flags(godot::VBoxContainer::SizeFlags::SIZE_EXPAND_FILL);
		control->set_v_size_flags(godot::VBoxContainer::SizeFlags::SIZE_EXPAND_FILL);
	}
	else if constexpr (std::is_same_v<T, godot::Button> || std::is_same_v<T, godot::OptionButton>)
	{
		if (text.length() > 0)
			control->set_text(text);
		if (signal.length() > 0 && callableObject != nullptr && callableMethod.length() > 0 && callableMethod != "void_signal_manager")
			e = control->connect(signal.c_str(), Callable(callableObject, callableMethod.c_str()));
		control->set_focus_mode(godot::Control::FocusMode::FOCUS_NONE);
		//control->set_flat(true);

	}
	else if constexpr (std::is_same_v<T, godot::LineEdit>)
	{
		control->set_horizontal_alignment(godot::HorizontalAlignment::HORIZONTAL_ALIGNMENT_RIGHT);
	}

	if (signal.length() > 0 && callableObject != nullptr && callableMethod.length() > 0 && callableMethod == "void_signal_manager")
	{
		std::string typeName = typeid(T).name();
		e = GDN_TheWorld_Globals::connectSignal(control, typeName.c_str(), signal.c_str(), callableObject, callableMethod.c_str(), custom1, custom2, custom3);
	}

	return control;
}

void GDN_TheWorld_Edit::deinit(void)
{
	if (m_initialized)
	{
		m_tp.Stop();
		m_initialized = false;
	}
}

void GDN_TheWorld_Edit::setSizeUI(void)
{
	set_anchor(godot::Side::SIDE_RIGHT, 1.0);
	set_offset(godot::Side::SIDE_RIGHT, 0.0);
	set_offset(godot::Side::SIDE_TOP, 0.0);
	set_offset(godot::Side::SIDE_BOTTOM, 0.0);

	if (IS_EDITOR_HINT())
	{
		std::string classsName;
		godot::Node* parent = get_parent();
		if (parent != nullptr)
		{
			godot::String s = parent->get_class();
			classsName = m_viewer->to_string(s);
		}
		if (classsName == "Control")
			set_offset(godot::Side::SIDE_LEFT, ((godot::Control*)get_parent())->get_size().x - 350);
		else
			set_offset(godot::Side::SIDE_LEFT, get_viewport()->get_visible_rect().get_size().x - 350);
	}
	else
		set_offset(godot::Side::SIDE_LEFT, get_viewport()->get_visible_rect().get_size().x - 350);

	m_scrollPanelsNeedResize = true;
	m_infoLabelTextChanged = true;
}

void GDN_TheWorld_Edit::setEmptyTerrainEditValues(void)
{
	m_innerData->m_terrTypeOptionButton->select(m_innerData->m_terrTypeOptionButton->get_item_index((int64_t)TheWorld_Utils::TerrainEdit::TerrainType::unknown));

	setSeed(0);
	setFrequency(0.0f);
	setOctaves(0);
	setLacunarity(0.0f);
	setGain(0.0f);
	setWeightedStrength(0.0f);
	setPingPongStrength(0.0f);
	setAmplitude(0);
	setScaleFactor(1.0f);
	setMinHeight(0.0f);
	setMaxHeight(0.0f);

	m_innerData->m_lowElevationTexName->set_text("");
	m_innerData->m_highElevationTexName->set_text("");
	m_innerData->m_dirtTexName->set_text("");
	m_innerData->m_rocksTexName->set_text("");
	godot::Ref<godot::Texture2D> emptyTexture;
	m_innerData->m_lowElevationTex->set_texture(emptyTexture);
	m_innerData->m_highElevationTex->set_texture(emptyTexture);
	m_innerData->m_dirtTex->set_texture(emptyTexture);
	m_innerData->m_rocksTex->set_texture(emptyTexture);
}

void GDN_TheWorld_Edit::setTerrainEditValues(TheWorld_Utils::TerrainEdit& terrainEdit)
{
	int64_t id = m_innerData->m_terrTypeOptionButton->get_selected_id();
	if (id != -1)
	{
		TheWorld_Utils::TerrainEdit::TerrainType selectedTerrainType = (TheWorld_Utils::TerrainEdit::TerrainType)id;
		if (selectedTerrainType != terrainEdit.terrainType)
		{
			//std::string text = TheWorld_Utils::TerrainEdit::terrainTypeString(terrainEdit.terrainType);
			//m_terrTypeOptionButton->set_text(text.c_str());
			m_innerData->m_terrTypeOptionButton->select(m_innerData->m_terrTypeOptionButton->get_item_index((int64_t)terrainEdit.terrainType));
		}
	}
	
	setSeed(terrainEdit.noise.noiseSeed);
	setFrequency(terrainEdit.noise.frequency);
	setOctaves(terrainEdit.noise.fractalOctaves);
	setLacunarity(terrainEdit.noise.fractalLacunarity);
	setGain(terrainEdit.noise.fractalGain);
	setWeightedStrength(terrainEdit.noise.fractalWeightedStrength);
	setPingPongStrength(terrainEdit.noise.fractalPingPongStrength);
	setAmplitude(terrainEdit.noise.amplitude);
	setScaleFactor(terrainEdit.noise.scaleFactor);
	setDesideredMinHeight(terrainEdit.noise.desideredMinHeight);
	setMinHeight(terrainEdit.minHeight);
	setMaxHeight(terrainEdit.maxHeight);

	std::map<std::string, std::unique_ptr<ShaderTerrainData::GroundTexture>>& groundTextures = ShaderTerrainData::getGroundTextures();

	std::map<std::string, std::unique_ptr<ShaderTerrainData::GroundTexture>>::iterator it = groundTextures.find(terrainEdit.extraValues.lowElevationTexName_r);
	if (it != groundTextures.end())
	{
		m_innerData->m_lowElevationTexName->set_text(it->first.c_str());
		m_innerData->m_lowElevationTex->set_texture(it->second->m_tex);
	}
	it = groundTextures.find(terrainEdit.extraValues.highElevationTexName_g);
	if (it != groundTextures.end())
	{
		m_innerData->m_highElevationTexName->set_text(it->first.c_str());
		m_innerData->m_highElevationTex->set_texture(it->second->m_tex);
	}
	it = groundTextures.find(terrainEdit.extraValues.dirtTexName_b);
	if (it != groundTextures.end())
	{
		m_innerData->m_dirtTexName->set_text(it->first.c_str());
		m_innerData->m_dirtTex->set_texture(it->second->m_tex);
	}
	it = groundTextures.find(terrainEdit.extraValues.rocksTexName_a);
	if (it != groundTextures.end())
	{
		m_innerData->m_rocksTexName->set_text(it->first.c_str());
		m_innerData->m_rocksTex->set_texture(it->second->m_tex);
	}
}

void GDN_TheWorld_Edit::setSeed(int seed)
{
	m_innerData->m_seed->set_text(std::to_string(seed).c_str());
}

int GDN_TheWorld_Edit::seed(void)
{
	godot::String s = m_innerData->m_seed->get_text();
	const char* str = s.utf8().get_data();
	int ret = std::stoi(std::string(str));
	return ret;
}

void GDN_TheWorld_Edit::setFrequency(float frequency)
{
	m_innerData->m_frequency->set_text(std::to_string(frequency).c_str());
}

float GDN_TheWorld_Edit::frequency(void)
{
	godot::String s = m_innerData->m_frequency->get_text();
	const char* str = s.utf8().get_data();
	float ret = std::stof(std::string(str));
	return ret;
}

void GDN_TheWorld_Edit::setOctaves(int octaves)
{
	m_innerData->m_fractalOctaves->set_text(std::to_string(octaves).c_str());
}

int GDN_TheWorld_Edit::octaves(void)
{
	godot::String s = m_innerData->m_fractalOctaves->get_text();
	const char* str = s.utf8().get_data();
	int ret = std::stoi(std::string(str));
	return ret;
}

void GDN_TheWorld_Edit::setLacunarity(float lacunarity)
{
	m_innerData->m_fractalLacunarity->set_text(std::to_string(lacunarity).c_str());
}

float GDN_TheWorld_Edit::lacunarity(void)
{
	godot::String s = m_innerData->m_fractalLacunarity->get_text();
	const char* str = s.utf8().get_data();
	float ret = std::stof(std::string(str));
	return ret;
}

void GDN_TheWorld_Edit::setGain(float gain)
{
	m_innerData->m_fractalGain->set_text(std::to_string(gain).c_str());
}

float GDN_TheWorld_Edit::gain(void)
{
	godot::String s = m_innerData->m_fractalGain->get_text();
	const char* str = s.utf8().get_data();
	float ret = std::stof(std::string(str));
	return ret;
}

void GDN_TheWorld_Edit::setWeightedStrength(float weightedStrength)
{
	m_innerData->m_fractalWeightedStrength->set_text(std::to_string(weightedStrength).c_str());
}

float GDN_TheWorld_Edit::weightedStrength(void)
{
	godot::String s = m_innerData->m_fractalWeightedStrength->get_text();
	const char* str = s.utf8().get_data();
	float ret = std::stof(std::string(str));
	return ret;
}

void GDN_TheWorld_Edit::setPingPongStrength(float pingPongStrength)
{
	m_innerData->m_fractalPingPongStrength->set_text(std::to_string(pingPongStrength).c_str());
}

float GDN_TheWorld_Edit::pingPongStrength(void)
{
	godot::String s = m_innerData->m_fractalPingPongStrength->get_text();
	const char* str = s.utf8().get_data();
	float ret = std::stof(std::string(str));
	return ret;
}

void GDN_TheWorld_Edit::setAmplitude(unsigned int amplitude)
{
	m_innerData->m_amplitudeLabel->set_text(std::to_string(amplitude).c_str());
}

unsigned int GDN_TheWorld_Edit::amplitude(void)
{
	godot::String s = m_innerData->m_amplitudeLabel->get_text();
	const char* str = s.utf8().get_data();
	unsigned int ret = std::stoi(std::string(str));
	return ret;
}

void GDN_TheWorld_Edit::setScaleFactor(float scaleFactor)
{
	m_innerData->m_scaleFactorLabel->set_text(std::to_string(scaleFactor).c_str());
}

float GDN_TheWorld_Edit::scaleFactor(void)
{
	godot::String s = m_innerData->m_scaleFactorLabel->get_text();
	const char* str = s.utf8().get_data();
	float ret = std::stof(std::string(str));
	return ret;
}

void GDN_TheWorld_Edit::setDesideredMinHeight(float desideredMinHeight)
{
	m_innerData->m_desideredMinHeightLabel->set_text(std::to_string(desideredMinHeight).c_str());
}

float GDN_TheWorld_Edit::desideredMinHeight(void)
{
	godot::String s = m_innerData->m_desideredMinHeightLabel->get_text();
	const char* str = s.utf8().get_data();
	float ret = std::stof(std::string(str));
	return ret;
}

void GDN_TheWorld_Edit::setMinHeight(float minHeight)
{
	*m_minHeightStr = std::to_string(minHeight);
	m_infoLabelTextChanged = true;
}

float GDN_TheWorld_Edit::minHeight(void)
{
	float ret = std::stof(*m_minHeightStr);
	return ret;
	return 0;
}

void GDN_TheWorld_Edit::setMaxHeight(float maxHeight)
{
	*m_maxHeightStr = std::to_string(maxHeight);
	m_infoLabelTextChanged = true;
}

float GDN_TheWorld_Edit::maxHeight(void)
{
	float ret = std::stof(*m_maxHeightStr);
	return ret;
	return 0;
}

void GDN_TheWorld_Edit::setElapsed(size_t elapsed, bool onGoing)
{
	std::lock_guard<std::recursive_mutex> lock(m_mtxUI);

	if (onGoing)
	{
		if (!m_onGoingElapsedLabel)
		{
			//m_elapsedLabelNormalColor = m_elapsedLabel->get("custom_colors/font_color");
			m_innerData->m_elapsedLabel->set("custom_colors/font_color", Color(1, 0, 0));
			m_onGoingElapsedLabel = true;
		}
	}
	else
	{
		if (m_onGoingElapsedLabel)
		{
			m_innerData->m_elapsedLabel->set("custom_colors/font_color", Color(1, 1, 1));
			m_onGoingElapsedLabel = false;
		}
	}
	
	if (elapsed == 0)
	{
		m_innerData->m_elapsedLabel->set_text("");
	}
	else
	{
		m_innerData->m_elapsedLabel->set_text(std::to_string(elapsed).c_str());
	}
}

void GDN_TheWorld_Edit::setCounter(size_t current, size_t all)
{
	*m_counterStr = std::to_string(current) + "/" + std::to_string(all);
	m_infoLabelTextChanged = true;
}

void GDN_TheWorld_Edit::setNote1(size_t num)
{
	if (num == 0)
	{
		*m_note1Str = "";
	}
	else
	{
		*m_note1Str = std::to_string(num);
	}
	m_infoLabelTextChanged = true;
}

void GDN_TheWorld_Edit::setNote1(std::string msg)
{
	*m_note1Str = msg;
	m_infoLabelTextChanged = true;
}

size_t GDN_TheWorld_Edit::elapsed(void)
{
	godot::String s = m_innerData->m_elapsedLabel->get_text();
	const char* str = s.utf8().get_data();
	size_t ret = std::stoll(std::string(str));
	return ret;
	return 0;
}

void GDN_TheWorld_Edit::setMouseHitLabelText(std::string text)
{
	*m_mouseHitStr = text;
	m_infoLabelTextChanged = true;
}

void GDN_TheWorld_Edit::setMouseQuadHitLabelText(std::string text)
{
	*m_mouseQuadHitStr = text;
	m_infoLabelTextChanged = true;
}

void GDN_TheWorld_Edit::setMouseQuadHitPosLabelText(std::string text)
{
	*m_mouseQuadHitPosStr = text;
	m_infoLabelTextChanged = true;
}

void GDN_TheWorld_Edit::setMouseQuadSelLabelText(std::string text)
{
	*m_mouseQuadSelStr = text;
	m_infoLabelTextChanged = true;
}

void GDN_TheWorld_Edit::setMouseQuadSelPosLabelText(std::string text)
{
	*m_mouseQuadSelPosStr = text;
	m_infoLabelTextChanged = true;
}

void GDN_TheWorld_Edit::setNumQuadrantToSave(size_t num)
{
	m_innerData->m_numQuadrantToSaveLabel->set_text(std::to_string(num).c_str());
}

void GDN_TheWorld_Edit::setNumQuadrantToUpload(size_t num)
{
	m_innerData->m_numQuadrantToUploadLabel->set_text(std::to_string(num).c_str());
}

void GDN_TheWorld_Edit::refreshNumToSaveUpload(size_t& numToSave, size_t& numToUpload)
{
	numToSave = m_mapQuadToSave.size();

	numToUpload = 0;

	std::vector<QuadrantPos> allQuandrantPos;
	m_viewer->getAllQuadrantPos(allQuandrantPos);

	for (auto& pos : allQuandrantPos)
	{
		QuadTree* q = m_viewer->getQuadTree(pos);
		if (q != nullptr && q->getQuadrant()->getTerrainEdit()->needUploadToServer)
			numToUpload++;

	}

	setNumQuadrantToSave(numToSave);
	setNumQuadrantToUpload(numToUpload);

}

void GDN_TheWorld_Edit::_init(void)
{
	//Cannot find Globals pointer as current node is not yet in the scene
	//godot::UtilityFunctions::print("GDN_TheWorld_Edit::Init");

	set_name(THEWORLD_EDIT_MODE_UI_CONTROL_NAME);
}

void GDN_TheWorld_Edit::_ready(void)
{
	//Cannot find Globals pointer as current node is not yet in the scene
	//godot::UtilityFunctions::print("GDN_TheWorld_Edit::_ready");

	m_ready = true;
}

void GDN_TheWorld_Edit::controlNeedResize(void)
{
	m_controlNeedResize = true;
}

void GDN_TheWorld_Edit::_input(const Ref<InputEvent>& event)
{
}

void GDN_TheWorld_Edit::_notification(int p_what)
{
	switch (p_what)
	{
	case NOTIFICATION_ENTER_TREE:
	{
		std::string classsName;
		godot::Node* parent = get_parent();
		if (parent != nullptr)
		{
			godot::String s = parent->get_class();
			classsName = m_viewer->to_string(s);
		}
		if (IS_EDITOR_HINT())
		{
			if (classsName == "Control")
			{
				//godot::Error e = parent->connect("size_changed", this, "setSizeUI");
				//std::string s = "";
			}
		}
	}
	break;
	case NOTIFICATION_WM_CLOSE_REQUEST:
	{
		m_quitting = true;
	}
	break;
	case NOTIFICATION_PREDELETE:
	{
		if (m_ready && m_viewer != nullptr)
		{
			GDN_TheWorld_Globals* globals = m_viewer->Globals();
			if (globals != nullptr)
				if (m_quitting)
					globals->debugPrint("GDN_TheWorld_Edit::_notification (NOTIFICATION_PREDELETE) - Destroy Edit", false);
				else
					globals->debugPrint("GDN_TheWorld_Edit::_notification (NOTIFICATION_PREDELETE) - Destroy Edit");
		}
	}
	break;
	case NOTIFICATION_RESIZED:
	{
		//setSizeUI();
		controlNeedResize();
	}
	break;
	}
}

godot::Size2 GDN_TheWorld_Edit::getTerrainScrollPanelSize(void)
{
	if (m_viewer == nullptr || m_innerData->m_mainTerrainVBoxContainer == nullptr)
		return godot::Size2(0.0f, 0.0f);

	godot::Size2 viewportSize = m_viewer->getViewportSize();
	godot::Rect2 contentRect = m_innerData->m_mainTerrainVBoxContainer->get_rect();
	godot::Size2 contentSize = contentRect.size;
	
	godot::Size2 ret = viewportSize;

	if (contentRect.size != godot::Size2(0.0f, 0.0f))
	{
		if (viewportSize.height > contentSize.height)
			ret.height = contentSize.height;
		if (viewportSize.width > contentSize.width)
			ret.width = contentSize.width;
	}

	ret.width = 0;	// ???
	
	return ret;
}

void GDN_TheWorld_Edit::_process(double _delta)
{
	//if (is_visible_in_tree())
	//{
	//	godot::Vector2 mousePos = get_viewport()->get_mouse_position();
	//	
	//	godot::Vector2 globalPos = m_mainTabContainer->get_global_position();
	//	godot::Vector2 globalMousePos = m_mainTabContainer->get_global_mouse_position();
	//	godot::Rect2 viewportRect = m_mainTabContainer->get_viewport_rect();
	//	godot::Transform2D viewportTransform = m_mainTabContainer->get_viewport_transform();

	//	globalPos = m_mainPanelContainer->get_global_position();
	//	globalMousePos = m_mainPanelContainer->get_global_mouse_position();
	//	viewportRect = m_mainPanelContainer->get_viewport_rect();
	//	viewportTransform = m_mainPanelContainer->get_viewport_transform();

	//	viewportTransform = m_mainTabContainer->get_viewport_transform();
	//}

	if (!m_initialized)
		return;

	if (m_controlNeedResize)
	{
		setSizeUI();
		m_controlNeedResize = false;
	}

	if (m_scrollPanelsNeedResize)
	{
		if (m_innerData->m_mainTerrainVBoxContainer != nullptr && m_innerData->m_mainTerrainVBoxContainer->get_rect().size != godot::Size2(0.0f, 0.0f))
		{
			m_innerData->m_mainTerrainScrollContainer->set_custom_minimum_size(getTerrainScrollPanelSize());
			m_scrollPanelsNeedResize = false;
		}
	}

	if (m_viewer->editMode() && (!m_refreshUI.counterStarted() || m_refreshUI.partialDuration().count() > 1000))
	{
		m_refreshUI.tick();

		size_t numToSave = 0, numToUpload = 0;
		refreshNumToSaveUpload(numToSave, numToUpload);
	}

	if (m_requiredUIAcceptFocus != m_UIAcceptingFocus)
	{
		setUIAcceptFocus(m_requiredUIAcceptFocus);
	}

	if (m_actionInProgress)
	{
		setElapsed(m_actionClock.partialDuration().count(), true);
		if (m_allItems != 0)
		{
			setCounter(m_completedItems, m_allItems);
			setNote1(m_lastElapsed);
		}
	}

	if (m_lastMessageChanged)
	{
		std::lock_guard<std::recursive_mutex> lock(m_mtxUI);

		if (m_lastMessageChanged)
		{
			m_innerData->m_message->set_text(m_innerData->m_lastMessage);
			m_lastMessageChanged = false;
		}
	}

	if (m_innerData->m_selTexturePanel->m_closeSelTexturesRequired)
	{
		if (m_innerData->m_selTexturePanel->m_wnd != nullptr)
		{
			m_innerData->m_selTexturePanel->m_wnd->hide();
			godot::Node* parent = m_innerData->m_selTexturePanel->m_wnd->get_parent();
			if (parent != nullptr)
			{
				//parent->remove_child(m_innerData->m_selTexturePanel->m_wnd);
				parent->call_deferred("remove_child", m_innerData->m_selTexturePanel->m_wnd);
			}
			m_innerData->m_selTexturePanel->m_wnd->queue_free();
			m_innerData->m_selTexturePanel->m_wnd = nullptr;
			m_innerData->m_selTexturePanel->m_textureItems.clear();

			m_innerData->m_lowElevationTexPanel->remove_theme_stylebox_override("panel");
			m_innerData->m_highElevationTexPanel->remove_theme_stylebox_override("panel");
			m_innerData->m_dirtTexPanel->remove_theme_stylebox_override("panel");
			m_innerData->m_rocksTexPanel->remove_theme_stylebox_override("panel");
			
			if (!m_innerData->m_selTexturePanel->m_openSelTexturesRequired)
			{
				m_innerData->m_selTexturePanel->m_selectedTexSlotName = "";
				m_innerData->m_lowElevationTexSelected = false;
				m_innerData->m_highElevationTexSelected = false;
				m_innerData->m_dirtTexSelected = false;
				m_innerData->m_rocksTexSelected = false;
			}
		}

		m_innerData->m_selTexturePanel->m_closeSelTexturesRequired = false;
	}

	if (m_innerData->m_selTexturePanel->m_openSelTexturesRequired)
	{
		if (m_innerData->m_selTexturePanel->m_wnd == nullptr)
		{
			if (m_innerData->m_lowElevationTexSelected)
				m_innerData->m_lowElevationTexPanel->add_theme_stylebox_override("panel", m_innerData->m_styleBoxSelectedFrame);
			else if (m_innerData->m_highElevationTexSelected)
				m_innerData->m_highElevationTexPanel->add_theme_stylebox_override("panel", m_innerData->m_styleBoxSelectedFrame);
			else if (m_innerData->m_dirtTexSelected)
				m_innerData->m_dirtTexPanel->add_theme_stylebox_override("panel", m_innerData->m_styleBoxSelectedFrame);
			else if (m_innerData->m_rocksTexSelected)
				m_innerData->m_rocksTexPanel->add_theme_stylebox_override("panel", m_innerData->m_styleBoxSelectedFrame);

			m_innerData->m_selTexturePanel->m_wnd = initSelTexturePanel();
		}

		m_innerData->m_selTexturePanel->m_openSelTexturesRequired = false;
	}

	if (m_innerData->m_selTexturePanel->m_changeTextureRequired)
	{
		std::string s = m_innerData->m_selTexturePanel->m_newTexSlotName;
		s = m_innerData->m_selTexturePanel->m_newTexName;

		m_innerData->m_selTexturePanel->m_changeTextureRequired = false;
	}
	
	if (m_infoLabelTextChanged)
	{
		std::string infoLabelText = "Min: " + *m_minHeightStr + " Max: " + *m_maxHeightStr + "\n"
			+ "Hit: " + *m_mouseHitStr + "\n"
			+ "Quad Hit: " + *m_mouseQuadHitStr + "\n"
			+ "Pos Hit: " + *m_mouseQuadHitPosStr + "\n"
			+ "Quad Sel: " + *m_mouseQuadSelStr + "\n"
			+ "Pos Sel: " + *m_mouseQuadSelPosStr;
		m_innerData->m_infoLabel->set_text(infoLabelText.c_str());
		m_innerData->m_elapsed1Label->set_text((*m_counterStr + " " + *m_note1Str).c_str());
		
		m_infoLabelTextChanged = false;
	}
}

godot::Window* GDN_TheWorld_Edit::initSelTexturePanel(void)
{
	godot::Error e;

	m_innerData->m_selTexturePanel->m_textureItems.clear();
	
	godot::Window* wnd = memnew(godot::Window);
	this->add_child(wnd);
	godot::Size2 windowSize(300.0f, 400.0f);
	wnd->set_flag(godot::Window::Flags::FLAG_ALWAYS_ON_TOP, true);
	wnd->set_initial_position(godot::Window::WindowInitialPosition::WINDOW_INITIAL_POSITION_ABSOLUTE);
	wnd->set_position(get_viewport_rect().size * godot::Size2(0.5f, 0.3f));
	wnd->set_flag(godot::Window::Flags::FLAG_BORDERLESS, true);
	wnd->set_min_size(windowSize);
	e = GDN_TheWorld_Globals::connectSignal(wnd, "godot::Window", "window_input", this, "gui_input_event", "SelTexturePanel_Window");
	
	godot::PanelContainer* panelContainer = createControl<godot::PanelContainer>(wnd, "SelTexturesPanel");
	godot::VBoxContainer* vBoxContainer = createControl<godot::VBoxContainer>(panelContainer);
	godot::Label* label = createControl<godot::Label>(vBoxContainer, "", m_innerData->m_selTexturePanel->m_selectedTexSlotName.c_str());
	godot::HBoxContainer* hBoxContainer = createControl<godot::HBoxContainer>(vBoxContainer);
	godot::Button* button = createControl<godot::Button>(hBoxContainer, "", "Set");
	e = GDN_TheWorld_Globals::connectSignal(button, "godot::Button", "pressed", this, "void_signal_manager", "SelTexturePanel_SetButton");
	button = createControl<godot::Button>(hBoxContainer, "", "Cancel");
	e = GDN_TheWorld_Globals::connectSignal(button, "godot::Button", "pressed", this, "void_signal_manager", "SelTexturePanel_CancelButton");
	godot::ScrollContainer* scrollContainer = createControl<godot::ScrollContainer>(vBoxContainer);
	godot::MarginContainer* marginContainer = createControl<godot::MarginContainer>(scrollContainer);
	scrollContainer->set_custom_minimum_size(windowSize - godot::Size2(0, 40));
	scrollContainer->set_h_size_flags(godot::Control::SizeFlags::SIZE_FILL);
	scrollContainer->set_v_size_flags(godot::Control::SizeFlags::SIZE_FILL);
	vBoxContainer = createControl<godot::VBoxContainer>(marginContainer);


	godot::Vector2 size(70, 70);

	std::map<std::string, std::unique_ptr<ShaderTerrainData::GroundTexture>>& groundTextures = ShaderTerrainData::getGroundTextures();
	int32_t idx = 0;
	for (const auto& groundTexture : groundTextures)
	{
		if (groundTexture.second->initialized)
		{
			m_innerData->m_selTexturePanel->m_textureItems[groundTexture.first] = std::make_unique<InnerData::SelTexturePanel::TextureItemPanel>();
			InnerData::SelTexturePanel::TextureItemPanel* texItemPanel = m_innerData->m_selTexturePanel->m_textureItems[groundTexture.first].get();

			texItemPanel->m_panel = createControl<godot::PanelContainer>(vBoxContainer, (std::string("SelTexturePanel_TexFrame_") + std::to_string(idx)).c_str());
			texItemPanel->m_idx = idx;
			if (m_innerData->m_selTexturePanel->m_selectedTexName == groundTexture.first)
			{
				texItemPanel->m_panel->add_theme_stylebox_override("panel", m_innerData->m_styleBoxSelectedFrame);
				texItemPanel->m_selected = true;
			}
			e = GDN_TheWorld_Globals::connectSignal(texItemPanel->m_panel, "godot::PanelContainer", "gui_input", this, "gui_input_event", "SelTexturePanel_ItemPanelGuiInput", groundTexture.first.c_str(), idx);
			e = GDN_TheWorld_Globals::connectSignal(texItemPanel->m_panel, "godot::PanelContainer", "mouse_entered", this, "void_signal_manager", "SelTexturePanel_ItemPanelGuiInput", groundTexture.first.c_str(), idx);
			e = GDN_TheWorld_Globals::connectSignal(texItemPanel->m_panel, "godot::PanelContainer", "mouse_exited", this, "void_signal_manager", "SelTexturePanel_ItemPanelGuiInput", groundTexture.first.c_str(), idx);
			godot::HBoxContainer* hBoxContainer = createControl<godot::HBoxContainer>(texItemPanel->m_panel);
			//int i = hBoxContainer->get_theme_constant("separation");
			godot::TextureRect* tex = createControl<godot::TextureRect>(hBoxContainer);
			tex->set_expand_mode(godot::TextureRect::EXPAND_IGNORE_SIZE);
			tex->set_custom_minimum_size(size);
			tex->set_stretch_mode(godot::TextureRect::STRETCH_SCALE);
			tex->set_texture(groundTexture.second->m_tex);
			godot::Label* label = createControl<godot::Label>(hBoxContainer, "", groundTexture.first.c_str());

			idx++;
		}
	}

	label = createControl<godot::Label>(vBoxContainer, "", " ");

	return wnd;
}

void GDN_TheWorld_Edit::voidSignalManager(godot::Object* obj, godot::String signal, godot::String className, int instanceId, godot::String objectName, godot::Variant custom1, godot::Variant custom2, godot::Variant custom3)
{
	//m_viewer->Globals()->debugPrint("GDN_TheWorld_Edit::voidSignalManager", true);

	std::string _signal = signal.utf8().get_data();
	std::string _className = className.utf8().get_data();
	std::string _objectName = objectName.utf8().get_data();

	if (TheWorld_Utils::to_lower(_signal) == "pressed")
	{
		if (_objectName == "NoisePanelToggle" && _className == "class godot::Button")
		{
			if (m_innerData->m_noiseVBoxContainer->is_visible())
			{
				m_innerData->m_noiseVBoxContainer->hide();
				((godot::Button*)obj)->set_text((std::wstring(RIGHT_ARROW) + L" " + NOISE_BUTTON_TEXT).c_str());
			}
			else
			{
				m_innerData->m_noiseVBoxContainer->show();
				((godot::Button*)obj)->set_text((std::wstring(DOWN_ARROW) + L" " + NOISE_BUTTON_TEXT).c_str());
			}

			m_scrollPanelsNeedResize = true;
		}
		else if (_objectName == "InfoPanelToggle" && _className == "class godot::Button")
		{
			if (m_innerData->m_infoVBoxContainer->is_visible())
			{
				m_innerData->m_infoVBoxContainer->hide();
				((godot::Button*)obj)->set_text((std::wstring(RIGHT_ARROW) + L" " + INFO_BUTTON_TEXT).c_str());
			}
			else
			{
				m_innerData->m_infoVBoxContainer->show();
				((godot::Button*)obj)->set_text((std::wstring(DOWN_ARROW) + L" " + INFO_BUTTON_TEXT).c_str());
			}

			m_scrollPanelsNeedResize = true;
		}
		else if (_objectName == "TerrEditPanelToggle" && _className == "class godot::Button")
		{
			if (m_innerData->m_terrEditVBoxContainer->is_visible())
			{
				m_innerData->m_terrEditVBoxContainer->hide();
				((godot::Button*)obj)->set_text((std::wstring(RIGHT_ARROW) + L" " + TERRAIN_EDIT_BUTTON_TEXT).c_str());
			}
			else
			{
				m_innerData->m_terrEditVBoxContainer->show();
				((godot::Button*)obj)->set_text((std::wstring(DOWN_ARROW) + L" " + TERRAIN_EDIT_BUTTON_TEXT).c_str());
			}

			m_scrollPanelsNeedResize = true;
		}
		else if (godot::String(custom1) == "SelTexturePanel_SetButton")
		{
			for (auto& texturePanel : m_innerData->m_selTexturePanel->m_textureItems)
			{
				if (texturePanel.second->m_selected)
				{
					m_innerData->m_selTexturePanel->m_changeTextureRequired = true;
					m_innerData->m_selTexturePanel->m_newTexSlotName = m_innerData->m_selTexturePanel->m_selectedTexSlotName;
					m_innerData->m_selTexturePanel->m_newTexName = texturePanel.first;
					break;
				}
			}
		}
		else if (godot::String(custom1) == "SelTexturePanel_CancelButton")
		{
			m_innerData->m_selTexturePanel->m_closeSelTexturesRequired = true;
		}
	}
	else if (TheWorld_Utils::to_lower(_signal) == "mouse_entered")
	{
		if (_className == "godot::PanelContainer")
		{
			if (godot::String(custom1) == c_lowElevationEditTextureName.c_str())
			{
				if (!m_innerData->m_lowElevationTexSelected)
					m_innerData->m_lowElevationTexPanel->remove_theme_stylebox_override("panel");
				if (!m_innerData->m_highElevationTexSelected)
					m_innerData->m_highElevationTexPanel->remove_theme_stylebox_override("panel");
				if (!m_innerData->m_dirtTexSelected)
					m_innerData->m_dirtTexPanel->remove_theme_stylebox_override("panel");
				if (!m_innerData->m_rocksTexSelected)
					m_innerData->m_rocksTexPanel->remove_theme_stylebox_override("panel");

				if (m_innerData->m_lowElevationTexSelected)
					m_innerData->m_lowElevationTexPanel->add_theme_stylebox_override("panel", m_innerData->m_styleBoxSelectedFrame);
				else
					m_innerData->m_lowElevationTexPanel->add_theme_stylebox_override("panel", m_innerData->m_styleBoxHighlightedFrame);
			}
			else if (godot::String(custom1) == c_highElevationEditTextureName.c_str())
			{
				if (!m_innerData->m_lowElevationTexSelected)
					m_innerData->m_lowElevationTexPanel->remove_theme_stylebox_override("panel");
				if (!m_innerData->m_highElevationTexSelected)
					m_innerData->m_highElevationTexPanel->remove_theme_stylebox_override("panel");
				if (!m_innerData->m_dirtTexSelected)
					m_innerData->m_dirtTexPanel->remove_theme_stylebox_override("panel");
				if (!m_innerData->m_rocksTexSelected)
					m_innerData->m_rocksTexPanel->remove_theme_stylebox_override("panel");

				if (m_innerData->m_highElevationTexSelected)
					m_innerData->m_highElevationTexPanel->add_theme_stylebox_override("panel", m_innerData->m_styleBoxSelectedFrame);
				else
					m_innerData->m_highElevationTexPanel->add_theme_stylebox_override("panel", m_innerData->m_styleBoxHighlightedFrame);
			}
			else if (godot::String(custom1) == c_dirtEditTextureName.c_str())
			{
				if (!m_innerData->m_lowElevationTexSelected)
					m_innerData->m_lowElevationTexPanel->remove_theme_stylebox_override("panel");
				if (!m_innerData->m_highElevationTexSelected)
					m_innerData->m_highElevationTexPanel->remove_theme_stylebox_override("panel");
				if (!m_innerData->m_dirtTexSelected)
					m_innerData->m_dirtTexPanel->remove_theme_stylebox_override("panel");
				if (!m_innerData->m_rocksTexSelected)
					m_innerData->m_rocksTexPanel->remove_theme_stylebox_override("panel");

				if (m_innerData->m_dirtTexSelected)
					m_innerData->m_dirtTexPanel->add_theme_stylebox_override("panel", m_innerData->m_styleBoxSelectedFrame);
				else
					m_innerData->m_dirtTexPanel->add_theme_stylebox_override("panel", m_innerData->m_styleBoxHighlightedFrame);
			}
			else if (godot::String(custom1) == c_rocksEditTextureName.c_str())
			{
				if (!m_innerData->m_lowElevationTexSelected)
					m_innerData->m_lowElevationTexPanel->remove_theme_stylebox_override("panel");
				if (!m_innerData->m_highElevationTexSelected)
					m_innerData->m_highElevationTexPanel->remove_theme_stylebox_override("panel");
				if (!m_innerData->m_dirtTexSelected)
					m_innerData->m_dirtTexPanel->remove_theme_stylebox_override("panel");
				if (!m_innerData->m_rocksTexSelected)
					m_innerData->m_rocksTexPanel->remove_theme_stylebox_override("panel");

				if (m_innerData->m_rocksTexSelected)
					m_innerData->m_rocksTexPanel->add_theme_stylebox_override("panel", m_innerData->m_styleBoxSelectedFrame);
				else
					m_innerData->m_rocksTexPanel->add_theme_stylebox_override("panel", m_innerData->m_styleBoxHighlightedFrame);
			}
			else if (godot::String(custom1) == "SelTexturePanel_ItemPanelGuiInput")
			{
				std::string textureName = godot::String(custom2).utf8().get_data();
				if (m_innerData->m_selTexturePanel->m_textureItems.contains(textureName) && !m_innerData->m_selTexturePanel->m_textureItems[textureName]->m_selected)
					m_innerData->m_selTexturePanel->m_textureItems[textureName]->m_panel->add_theme_stylebox_override("panel", m_innerData->m_styleBoxHighlightedFrame);
			}
		}
	}
	else if (TheWorld_Utils::to_lower(_signal) == "mouse_exited")
	{
		if (_className == "godot::PanelContainer")
		{
			if (godot::String(custom1) == c_lowElevationEditTextureName.c_str()
				|| godot::String(custom1) == c_highElevationEditTextureName.c_str()
				|| godot::String(custom1) == c_dirtEditTextureName.c_str()
				|| godot::String(custom1) == c_rocksEditTextureName.c_str())
			{
				if (!m_innerData->m_lowElevationTexSelected)
					m_innerData->m_lowElevationTexPanel->remove_theme_stylebox_override("panel");
				if (!m_innerData->m_highElevationTexSelected)
					m_innerData->m_highElevationTexPanel->remove_theme_stylebox_override("panel");
				if (!m_innerData->m_dirtTexSelected)
					m_innerData->m_dirtTexPanel->remove_theme_stylebox_override("panel");
				if (!m_innerData->m_rocksTexSelected)
					m_innerData->m_rocksTexPanel->remove_theme_stylebox_override("panel");
				//if (m_innerData->m_lowElevationTexSelected && godot::String(custom1) == c_lowElevationEditTextureName.c_str())
				//	m_innerData->m_lowElevationTexPanel->add_theme_stylebox_override("panel", m_innerData->m_styleBoxSelectedFrame);
				//else if (m_innerData->m_highElevationTexSelected && godot::String(custom1) == c_highElevationEditTextureName.c_str())
				//	m_innerData->m_highElevationTexPanel->add_theme_stylebox_override("panel", m_innerData->m_styleBoxSelectedFrame);
				//else if (m_innerData->m_dirtTexSelected && godot::String(custom1) == c_dirtEditTextureName.c_str())
				//	m_innerData->m_dirtTexPanel->add_theme_stylebox_override("panel", m_innerData->m_styleBoxSelectedFrame);
				//else if (m_innerData->m_rocksTexSelected && godot::String(custom1) == c_rocksEditTextureName.c_str())
				//	m_innerData->m_rocksTexPanel->add_theme_stylebox_override("panel", m_innerData->m_styleBoxSelectedFrame);
			}
			else if (godot::String(custom1) == "SelTexturePanel_ItemPanelGuiInput")
			{
				std::string textureName = godot::String(custom2).utf8().get_data();
				if (m_innerData->m_selTexturePanel->m_textureItems.contains(textureName) && !m_innerData->m_selTexturePanel->m_textureItems[textureName]->m_selected)
					m_innerData->m_selTexturePanel->m_textureItems[textureName]->m_panel->remove_theme_stylebox_override("panel");
			}
		}
	}
}

void GDN_TheWorld_Edit::guiInputEvent(const godot::Ref<godot::InputEvent>& event, godot::Object* obj, godot::String signal, godot::String className, int instanceId, godot::String objectName, godot::Variant custom1, godot::Variant custom2, godot::Variant custom3)
{
	//m_viewer->Globals()->debugPrint("GDN_TheWorld_Edit::guiInputEvent", true);

	std::string _signal = signal.utf8().get_data();
	std::string _className = className.utf8().get_data();
	std::string _objectName = objectName.utf8().get_data();

	if (TheWorld_Utils::to_lower(_signal) == "gui_input")
	{
		const godot::InputEventMouseMotion* eventMouseMotion = godot::Object::cast_to<godot::InputEventMouseMotion>(event.ptr());
		const godot::InputEventMouseButton* eventMouseButton = godot::Object::cast_to<godot::InputEventMouseButton>(event.ptr());
		
		if ( (_objectName == "LowElevationTexPanel" || _objectName == "HighElevationTexPanel" || _objectName == "DirtTexPanel" || _objectName == "RocksTexPanel")
			&& eventMouseButton != nullptr && eventMouseButton->is_double_click() && eventMouseButton->get_button_index() == godot::MouseButton::MOUSE_BUTTON_LEFT
			&& m_viewer->isQuadrantSelectedForEdit())
		{
			m_innerData->m_selTexturePanel->m_closeSelTexturesRequired = true;
			m_innerData->m_selTexturePanel->m_openSelTexturesRequired = true;
			m_innerData->m_selTexturePanel->m_selectedTexSlotName = godot::String(custom1).utf8().get_data();
			if (_objectName == "LowElevationTexPanel")
			{
				m_innerData->m_selTexturePanel->m_selectedTexName = m_innerData->m_lowElevationTexName->get_text().utf8().get_data();
				m_innerData->m_lowElevationTexSelected = true;
				m_innerData->m_highElevationTexSelected = false;
				m_innerData->m_dirtTexSelected = false;
				m_innerData->m_rocksTexSelected = false;
			}
			else if (_objectName == "HighElevationTexPanel")
			{
				m_innerData->m_selTexturePanel->m_selectedTexName = m_innerData->m_highElevationTexName->get_text().utf8().get_data();
				m_innerData->m_lowElevationTexSelected = false;
				m_innerData->m_highElevationTexSelected = true;
				m_innerData->m_dirtTexSelected = false;
				m_innerData->m_rocksTexSelected = false;
			}
			else if (_objectName == "DirtTexPanel")
			{
				m_innerData->m_selTexturePanel->m_selectedTexName = m_innerData->m_dirtTexName->get_text().utf8().get_data();
				m_innerData->m_lowElevationTexSelected = false;
				m_innerData->m_highElevationTexSelected = false;
				m_innerData->m_dirtTexSelected = true;
				m_innerData->m_rocksTexSelected = false;
			}
			else if (_objectName == "RocksTexPanel")
			{
				m_innerData->m_selTexturePanel->m_selectedTexName = m_innerData->m_rocksTexName->get_text().utf8().get_data();
				m_innerData->m_lowElevationTexSelected = false;
				m_innerData->m_highElevationTexSelected = false;
				m_innerData->m_dirtTexSelected = false;
				m_innerData->m_rocksTexSelected = true;
			}

			//m_innerData->m_lowElevationTexPanel->remove_theme_stylebox_override("panel");
			//m_innerData->m_highElevationTexPanel->remove_theme_stylebox_override("panel");
			//m_innerData->m_dirtTexPanel->remove_theme_stylebox_override("panel");
			//m_innerData->m_rocksTexPanel->remove_theme_stylebox_override("panel");
			//if (m_innerData->m_lowElevationTexSelected)
			//	m_innerData->m_lowElevationTexPanel->add_theme_stylebox_override("panel", m_innerData->m_styleBoxSelectedFrame);
			//else if (m_innerData->m_highElevationTexSelected)
			//	m_innerData->m_highElevationTexPanel->add_theme_stylebox_override("panel", m_innerData->m_styleBoxSelectedFrame);
			//else if (m_innerData->m_dirtTexSelected)
			//	m_innerData->m_dirtTexPanel->add_theme_stylebox_override("panel", m_innerData->m_styleBoxSelectedFrame);
			//else if (m_innerData->m_rocksTexSelected)
			//	m_innerData->m_rocksTexPanel->add_theme_stylebox_override("panel", m_innerData->m_styleBoxSelectedFrame);
		}
		else if (godot::String(custom1) == "SelTexturePanel_ItemPanelGuiInput")
		{
			if (eventMouseButton != nullptr && eventMouseButton->is_double_click() && eventMouseButton->get_button_index() == godot::MouseButton::MOUSE_BUTTON_LEFT)
			{
				std::string selectedTextureName = godot::String(custom2).utf8().get_data();
				std::map<std::string, std::unique_ptr<ShaderTerrainData::GroundTexture>>& groundTextures = ShaderTerrainData::getGroundTextures();
				int32_t idx = int32_t(custom3);
				if (groundTextures.contains(selectedTextureName))
				{
					for (auto& texturePanel : m_innerData->m_selTexturePanel->m_textureItems)
					{
						if (texturePanel.second->m_selected)
						{
							texturePanel.second->m_selected = false;
							texturePanel.second->m_panel->remove_theme_stylebox_override("panel");
						}
					}
					m_innerData->m_selTexturePanel->m_textureItems[selectedTextureName]->m_selected = true;
					m_innerData->m_selTexturePanel->m_changeTextureRequired = true;
					m_innerData->m_selTexturePanel->m_newTexSlotName = m_innerData->m_selTexturePanel->m_selectedTexSlotName;
					m_innerData->m_selTexturePanel->m_newTexName = selectedTextureName;

					//godot::Ref<godot::Texture> selectedTexture = groundTextures[selectedTextureName]->m_tex;
				}
			}
			else if (eventMouseButton != nullptr && eventMouseButton->is_pressed() && eventMouseButton->get_button_index() == godot::MouseButton::MOUSE_BUTTON_LEFT)
			{
				std::string selectedTextureName = godot::String(custom2).utf8().get_data();
				std::map<std::string, std::unique_ptr<ShaderTerrainData::GroundTexture>>& groundTextures = ShaderTerrainData::getGroundTextures();
				int32_t idx = int32_t(custom3);
				if (groundTextures.contains(selectedTextureName))
				{
					if (m_innerData->m_selTexturePanel->m_textureItems.contains(selectedTextureName) && !m_innerData->m_selTexturePanel->m_textureItems[selectedTextureName]->m_selected)
					{
						for (auto& texturePanel : m_innerData->m_selTexturePanel->m_textureItems)
						{
							if (texturePanel.second->m_selected)
							{
								texturePanel.second->m_selected = false;
								texturePanel.second->m_panel->remove_theme_stylebox_override("panel");
							}
						}
						m_innerData->m_selTexturePanel->m_textureItems[selectedTextureName]->m_selected = true;
						m_innerData->m_selTexturePanel->m_textureItems[selectedTextureName]->m_panel->add_theme_stylebox_override("panel", m_innerData->m_styleBoxSelectedFrame);
					}
				}
			}

		}
	}
	else if (TheWorld_Utils::to_lower(_signal) == "window_input")
	{
		if (godot::String(custom1) == "SelTexturePanel_Window")
		{
			const godot::InputEventKey* eventKey = godot::Object::cast_to<godot::InputEventKey>(event.ptr());

			if (eventKey != nullptr && eventKey->is_pressed() && eventKey->get_keycode() == godot::Key::KEY_ESCAPE)
			{
				m_innerData->m_selTexturePanel->m_closeSelTexturesRequired = true;
			}
		}
	}
}

void GDN_TheWorld_Edit::editModeStopAction(void)
{
	if (!m_actionInProgress)
		return;

	m_actionStopRequested = true;
}

void GDN_TheWorld_Edit::editModeSaveAction(void)
{
	if (m_actionInProgress)
		return;

	m_actionInProgress = true;
	m_allItems = 0;
	setElapsed(0, true);
	setNote1(0);

	std::function<void(void)> f = std::bind(&GDN_TheWorld_Edit::editModeSave, this);
	m_tp.QueueJob(f);
}

void GDN_TheWorld_Edit::editModeSave(void)
{
	if (m_actionClock.counterStarted())
	{
		m_actionInProgress = false;
		return;
	}

	setMessage(godot::String("Saving..."));

	m_actionClock.tick();

	TheWorld_Utils::GuardProfiler profiler(std::string("EditSave 1 ") + __FUNCTION__, "ALL");

	m_completedItems = 0;
	m_elapsedCompleted = 0;
	m_lastElapsed = 0;
	m_allItems = m_mapQuadToSave.size();
	for (auto& item : m_mapQuadToSave)
	{
		if (m_actionStopRequested)
		{
			m_actionStopRequested = false;
			break;
		}
		
		QuadTree* quadToSave = m_viewer->getQuadTree(item.first);
		if (quadToSave != nullptr && !quadToSave->getQuadrant()->empty())
		{
			TheWorld_Utils::MeshCacheBuffer& cache = quadToSave->getQuadrant()->getMeshCacheBuffer();
			TheWorld_Utils::MeshCacheBuffer::CacheQuadrantData cacheQuadrantData;
			cacheQuadrantData.meshId = cache.getMeshId();
			TheWorld_Utils::TerrainEdit* terrainEdit = quadToSave->getQuadrant()->getTerrainEdit();
			
			terrainEdit->needUploadToServer = true;
			quadToSave->getQuadrant()->setNeedUploadToServer(true);
			
			TheWorld_Utils::MemoryBuffer terrainEditValuesBuffer((BYTE*)terrainEdit, terrainEdit->size);
			cacheQuadrantData.minHeight = terrainEdit->minHeight;
			cacheQuadrantData.maxHeight = terrainEdit->maxHeight;
			cacheQuadrantData.terrainEditValues = &terrainEditValuesBuffer;
			cacheQuadrantData.heights16Buffer = &quadToSave->getQuadrant()->getFloat16HeightsBuffer(false);
			cacheQuadrantData.heights32Buffer = &quadToSave->getQuadrant()->getFloat32HeightsBuffer(false);
			cacheQuadrantData.normalsBuffer = &quadToSave->getQuadrant()->getNormalsBuffer(false);
			cacheQuadrantData.splatmapBuffer = &quadToSave->getQuadrant()->getSplatmapBuffer(false);
			cacheQuadrantData.colormapBuffer = &quadToSave->getQuadrant()->getColormapBuffer(false);
			cacheQuadrantData.globalmapBuffer = &quadToSave->getQuadrant()->getGlobalmapBuffer(false);
			QuadrantPos quadrantPos = item.first;
			TheWorld_Utils::MemoryBuffer buffer;
			cache.setBufferFromCacheQuadrantData(quadrantPos.getNumVerticesPerSize(), cacheQuadrantData, buffer);
			cache.writeBufferToDiskCache(buffer);

			m_completedItems++;
			size_t partialCount = m_actionClock.partialDuration().count();
			m_lastElapsed = partialCount - m_elapsedCompleted;
			m_elapsedCompleted = partialCount;
			m_lastElapsed = m_actionClock.partialDuration().count() - m_lastElapsed;
		}
	}

	m_mapQuadToSave.clear();

	m_actionClock.tock();

	m_actionInProgress = false;

	size_t duration = m_actionClock.duration().count();
	setElapsed(duration, false);
	setCounter(m_completedItems, m_allItems);
	setNote1(m_lastElapsed);

	size_t numToSave = 0;
	size_t numToUpload = 0;
	refreshNumToSaveUpload(numToSave, numToUpload);

	setMessage(godot::String("Completed!"), true);
}

void GDN_TheWorld_Edit::editModeUploadAction(void)
{
	if (m_actionInProgress)
		return;

	m_actionInProgress = true;
	m_allItems = 0;
	setElapsed(0, true);
	setNote1(0);

	std::function<void(void)> f = std::bind(&GDN_TheWorld_Edit::editModeUpload, this);
	m_tp.QueueJob(f);
}

void GDN_TheWorld_Edit::editModeUpload(void)
{
	editModeSave();
		
	if (m_actionClock.counterStarted())
	{
		m_actionInProgress = false;
		return;
	}

	setMessage(godot::String("Uploading..."));

	m_actionClock.tick();

	TheWorld_Utils::GuardProfiler profiler(std::string("EditUpload 1 ") + __FUNCTION__, "ALL");

	std::vector<QuadrantPos> allQuandrantPos;
	m_viewer->getAllQuadrantPos(allQuandrantPos);

	std::vector<QuadrantPos> quandrantPos;
	for (auto& pos : allQuandrantPos)
	{
		QuadTree* quadTree = m_viewer->getQuadTree(pos);
		if (quadTree != nullptr)
		{
			if (quadTree->getQuadrant()->needUploadToServer())
			{
				quandrantPos.push_back(pos);
			}
		}
	}

	m_completedItems = 0;
	m_elapsedCompleted = 0;
	m_lastElapsed = 0;
	m_allItems = quandrantPos.size();
	for (auto& pos : quandrantPos)
	{
		if (m_actionStopRequested)
		{
			m_actionStopRequested = false;
			break;
		}

		QuadTree* quadTree = m_viewer->getQuadTree(pos);
		if (quadTree != nullptr)
		{
			TheWorld_Utils::GuardProfiler profiler(std::string("EditUpload 1.1 ") + __FUNCTION__, "Single QuadTree");

			TheWorld_Utils::MeshCacheBuffer& cache = quadTree->getQuadrant()->getMeshCacheBuffer();
			TheWorld_Utils::MeshCacheBuffer::CacheQuadrantData cacheQuadrantData;
			cacheQuadrantData.meshId = cache.getMeshId();
			TheWorld_Utils::TerrainEdit* terrainEdit = quadTree->getQuadrant()->getTerrainEdit();
			terrainEdit->needUploadToServer = false;
			TheWorld_Utils::MemoryBuffer terrainEditValuesBuffer((BYTE*)terrainEdit, terrainEdit->size);
			cacheQuadrantData.minHeight = terrainEdit->minHeight;
			cacheQuadrantData.maxHeight = terrainEdit->maxHeight;
			cacheQuadrantData.terrainEditValues = &terrainEditValuesBuffer;
			cacheQuadrantData.heights16Buffer = &quadTree->getQuadrant()->getFloat16HeightsBuffer(false);
			cacheQuadrantData.heights32Buffer = &quadTree->getQuadrant()->getFloat32HeightsBuffer(false);
			cacheQuadrantData.normalsBuffer = &quadTree->getQuadrant()->getNormalsBuffer(false);
			cacheQuadrantData.splatmapBuffer = &quadTree->getQuadrant()->getSplatmapBuffer(false);
			cacheQuadrantData.colormapBuffer = &quadTree->getQuadrant()->getColormapBuffer(false);
			cacheQuadrantData.globalmapBuffer = &quadTree->getQuadrant()->getGlobalmapBuffer(false);
			std::string buffer;
			cache.setBufferFromCacheQuadrantData(pos.getNumVerticesPerSize(), cacheQuadrantData, buffer);
			
			float lowerXGridVertex = pos.getLowerXGridVertex();
			float lowerZGridVertex = pos.getLowerZGridVertex();
			int numVerticesPerSize = pos.getNumVerticesPerSize();
			float gridStepinWU = pos.getGridStepInWU();
			int lvl = pos.getLevel();
			quadTree->Viewer()->Globals()->Client()->MapManagerUploadBuffer(quadTree->Viewer()->isInEditor(), lowerXGridVertex, lowerZGridVertex, numVerticesPerSize, gridStepinWU, lvl, buffer);

			cache.writeBufferToDiskCache(buffer);

			//quadTree->getQuadrant()->setNeedUploadToServer(false);
		}
	}
	
	while (!m_actionStopRequested && m_allItems > m_completedItems)
		Sleep(10);
	
	m_actionClock.tock();

	m_actionInProgress = false;

	size_t duration = m_actionClock.duration().count();
	setElapsed(duration, false);
	setCounter(m_completedItems, m_allItems);
	setNote1(m_lastElapsed);

	size_t numToSave = 0;
	size_t numToUpload = 0;
	refreshNumToSaveUpload(numToSave, numToUpload);

	setMessage(godot::String("Completed!"), true);
}

void GDN_TheWorld_Edit::editModeGenerateAction(void)
{
	if (m_actionInProgress)
		return;

	m_actionInProgress = true;
	m_allItems = 0;
	setElapsed(0, true);
	setNote1(0);

	std::function<void(void)> f = std::bind(&GDN_TheWorld_Edit::editModeGenerate, this);
	m_tp.QueueJob(f);
}

void GDN_TheWorld_Edit::editModeGenerate(void)
{
	if (m_actionClock.counterStarted())
	{
		m_actionInProgress = false;
		return;
	}

	setMessage(godot::String("Generating heights..."));

	m_actionClock.tick();

	QuadTree* quadTreeSel = nullptr;
	QuadrantPos quadrantSelPos = m_viewer->getQuadrantSelForEdit(&quadTreeSel);

	if (quadrantSelPos.empty())
	{
		m_actionClock.tock();
		m_actionInProgress = false;
		setMessage(godot::String("Completed!"), true);
		return;
	}

	m_completedItems = 0;
	m_elapsedCompleted = 0;
	m_lastElapsed = 0;
	m_allItems = 1;

	TheWorld_Utils::GuardProfiler profiler(std::string("EditGenerate 1 ") + __FUNCTION__, "ALL");

	int64_t id = m_innerData->m_terrTypeOptionButton->get_selected_id();
	enum class TheWorld_Utils::TerrainEdit::TerrainType terrainType = (enum class TheWorld_Utils::TerrainEdit::TerrainType)id;

	TheWorld_Utils::TerrainEdit* terrainEdit = quadTreeSel->getQuadrant()->getTerrainEdit();
	terrainEdit->terrainType = terrainType;
	terrainEdit->noise.noiseSeed = seed();
	terrainEdit->noise.frequency = frequency();
	terrainEdit->noise.fractalOctaves = octaves();
	terrainEdit->noise.fractalLacunarity = lacunarity();
	terrainEdit->noise.fractalGain = gain();
	terrainEdit->noise.fractalWeightedStrength = weightedStrength();
	terrainEdit->noise.fractalPingPongStrength = pingPongStrength();
	terrainEdit->noise.amplitude = amplitude();
	terrainEdit->noise.scaleFactor = scaleFactor();
	terrainEdit->noise.desideredMinHeight = desideredMinHeight();
	//terrainEdit->needUploadToServer = true;

	int level = quadrantSelPos.getLevel();
	size_t numVerticesPerSize = quadrantSelPos.getNumVerticesPerSize();
	size_t numVertices = numVerticesPerSize * numVerticesPerSize;
	float gridStepInWU = quadrantSelPos.getGridStepInWU();
	float sizeInWU = quadrantSelPos.getSizeInWU();
	float lowerXGridVertex = quadrantSelPos.getLowerXGridVertex();
	float lowerZGridVertex = quadrantSelPos.getLowerZGridVertex();
	
	// 1. Lock internal data to prevent shader using them while they are incomplete 
	quadTreeSel->getQuadrant()->lockInternalData();

	// 2. generate new heights
	std::vector<float> vectGridHeights;
	TheWorld_Utils::MeshCacheBuffer& cache = quadTreeSel->getQuadrant()->getMeshCacheBuffer();
	{
		TheWorld_Utils::GuardProfiler profiler(std::string("EditGenerate 1.1 ") + __FUNCTION__, "Generate Heights");
		cache.generateHeightsWithNoise(numVerticesPerSize, gridStepInWU, lowerXGridVertex, lowerZGridVertex, terrainEdit, vectGridHeights);

		for (auto& item : m_wms)
		{
			cache.applyWorldModifier(level, numVerticesPerSize, gridStepInWU, lowerXGridVertex, lowerZGridVertex, terrainEdit, vectGridHeights, *item.second.get());
		}
	}

	// 3. remember quadrant has to be saved and uploaded
	quadTreeSel->getQuadrant()->setNeedUploadToServer(true);
	terrainEdit->needUploadToServer = true;
	m_mapQuadToSave[quadrantSelPos] = "";

	// 4. refresh new heights on internal datas 16-bit heights (for shader), 32-bit heigths (for collider), terrain data for editing and set flag according (new heigths have to be passed to shader)
	terrainEdit->northSideZMinus.needBlend = true;
	terrainEdit->southSideZPlus.needBlend = true;
	terrainEdit->westSideXMinus.needBlend = true;
	terrainEdit->eastSideXPlus.needBlend = true;

	TheWorld_Utils::MemoryBuffer& heights16Buffer = quadTreeSel->getQuadrant()->getFloat16HeightsBuffer(false);
	size_t heights16BufferSize = numVertices * sizeof(uint16_t);
	heights16Buffer.reserve(heights16BufferSize);
	uint16_t* movingHeights16Buffer = (uint16_t*)heights16Buffer.ptr();
	
	TheWorld_Utils::MemoryBuffer& heights32Buffer = quadTreeSel->getQuadrant()->getFloat32HeightsBuffer(false);
	size_t heights32BufferSize = numVertices * sizeof(float);
	heights32Buffer.reserve(heights32BufferSize);

	{
		TheWorld_Utils::GuardProfiler profiler(std::string("EditGenerate 1.2 ") + __FUNCTION__, "Move Heights to float16/float32 buffer");

		size_t idx = 0;
		for (int z = 0; z < numVerticesPerSize; z++)
			for (int x = 0; x < numVerticesPerSize; x++)
			{
				float altitude = vectGridHeights[idx];
				TheWorld_Utils::FLOAT_32 f(altitude);
				*movingHeights16Buffer = half_from_float(f.u32);;
				movingHeights16Buffer++;
				idx++;
			}

		memcpy(heights32Buffer.ptr(), &vectGridHeights[0], heights32BufferSize);
	}

	my_assert((BYTE*)movingHeights16Buffer - heights16Buffer.ptr() == heights16BufferSize);
	heights16Buffer.adjustSize(heights16BufferSize);
	heights32Buffer.adjustSize(heights32BufferSize);
	quadTreeSel->getQuadrant()->setHeightsUpdated(true);

	// 5. invalidata normals and textures as heigths have been changed and set flags according (normals need regen and empty normals have to be passed to shader)
	quadTreeSel->getQuadrant()->resetNormalsBuffer();
	quadTreeSel->getQuadrant()->resetSplatmapBuffer();
	quadTreeSel->getQuadrant()->resetColormapBuffer();
	quadTreeSel->getQuadrant()->resetGlobalmapBuffer();

	// 6. prepare new heigths for collider (PackedFloat32Array)
	{
		PackedFloat32Array& heightsForCollider = quadTreeSel->getQuadrant()->getHeightsForCollider();
		heightsForCollider.resize((int)numVertices);
		memcpy((char*)heightsForCollider.ptrw(), heights32Buffer.ptr(), heights32Buffer.size());
	}


	// 7. calc new quadrant AABB for collider
	Vector3 startPosition(lowerXGridVertex, terrainEdit->minHeight, lowerZGridVertex);
	Vector3 endPosition(startPosition.x + sizeInWU, terrainEdit->maxHeight, startPosition.z + sizeInWU);
	Vector3 size = endPosition - startPosition;
	quadTreeSel->getQuadrant()->getGlobalCoordAABB().set_position(startPosition);
	quadTreeSel->getQuadrant()->getGlobalCoordAABB().set_size(size);

	
	// 8. notify all chunk of the quadrant that heigths have been changed (invalidates all cached chunk and quad AABB)
	Chunk::HeightsChangedChunkAction action(m_viewer->is_visible_in_tree());
	m_viewer->m_refreshRequired = true;
	quadTreeSel->ForAllChunk(action);

	// 9. update quadrant flags
	quadTreeSel->getQuadrant()->setEmpty(false);
	quadTreeSel->getQuadrant()->setColorsUpdated(true);
	quadTreeSel->materialParamsNeedReset(true);

	// 10. unlock internal datas so that they can be passed to the shader/collider
	quadTreeSel->getQuadrant()->unlockInternalData();

	setMinHeight(terrainEdit->minHeight);
	setMaxHeight(terrainEdit->maxHeight);

	m_completedItems++;
	size_t partialCount = m_actionClock.partialDuration().count();
	m_lastElapsed = partialCount - m_elapsedCompleted;
	m_elapsedCompleted = partialCount;

	m_actionClock.tock();

	m_actionInProgress = false;

	size_t duration = m_actionClock.duration().count();
	setElapsed(duration, false);
	setCounter(m_completedItems, m_allItems);
	setNote1(m_lastElapsed);

	size_t numToSave = 0;
	size_t numToUpload = 0;
	refreshNumToSaveUpload(numToSave, numToUpload);

	setMessage(godot::String("Completed!"), true);
}

void GDN_TheWorld_Edit::editModeBlendAction(void)
{
	if (m_actionInProgress)
		return;

	m_actionInProgress = true;
	m_allItems = 0;
	setElapsed(0, true);
	setNote1(0);

	std::function<void(void)> f = std::bind(&GDN_TheWorld_Edit::editModeBlend, this);
	m_tp.QueueJob(f);
}

void GDN_TheWorld_Edit::editModeBlend(void)
{
	if (m_actionClock.counterStarted())
	{
		m_actionInProgress = false;
		return;
	}

	setMessage(godot::String("Blending heigths..."));
	
	m_actionClock.tick();

	TheWorld_Utils::GuardProfiler profiler(std::string("EditBlend 1 ") + __FUNCTION__, "ALL");

	bool allCheched = m_innerData->m_allCheckBox->is_pressed();

	QuadTree* quadTreeSel = nullptr;
	QuadrantPos quadrantSelPos = m_viewer->getQuadrantSelForEdit(&quadTreeSel);

	if (!allCheched && quadrantSelPos.empty())
	{
		m_actionClock.tock();
		m_actionInProgress = false;
		setMessage(godot::String("Completed!"), true);
		return;
	}

	std::vector<QuadrantPos> allQuandrantPos;
	m_viewer->getAllQuadrantPos(allQuandrantPos);

	std::vector<QuadrantPos> quandrantPos;
	for (auto& pos : allQuandrantPos)
	{
		QuadTree* quadTree = m_viewer->getQuadTree(pos);
		if (quadTree != nullptr)
		{
			if (allCheched || pos == quadrantSelPos)
			{
				quandrantPos.push_back(pos);

				if (!allCheched)
					break;
			}
		}
	}

	m_elapsedCompleted = 0;
	m_lastElapsed = 0;
	m_allItems = quandrantPos.size();

	size_t numRounds = 2;
	for (size_t round = 0; round < numRounds; round++)
	{
		m_completedItems = 0;

		for (auto& pos : quandrantPos)
		{
			if (m_actionStopRequested)
			{
				m_actionStopRequested = false;
				break;
			}

			QuadTree* quadTree = m_viewer->getQuadTree(pos);
			if (quadTree != nullptr)
			{
				TheWorld_Utils::GuardProfiler profiler(std::string("EditBlend 1.1 ") + __FUNCTION__, "Single QuadTree");

				quadTree->getQuadrant()->lockInternalData();

				TheWorld_Utils::MemoryBuffer terrainEditValuesBuffer;
				TheWorld_Utils::MemoryBuffer heights16Buffer;
				TheWorld_Utils::MemoryBuffer heights32Buffer;
				TheWorld_Utils::MeshCacheBuffer::CacheQuadrantData quadrantData;
				quadTree->getQuadrant()->getTerrainEdit()->serialize(terrainEditValuesBuffer);
				heights16Buffer.copyFrom(quadTree->getQuadrant()->getFloat16HeightsBuffer());
				heights32Buffer.copyFrom(quadTree->getQuadrant()->getFloat32HeightsBuffer());

				quadrantData.meshId = quadTree->getQuadrant()->getMeshCacheBuffer().getMeshId();
				quadrantData.terrainEditValues = &terrainEditValuesBuffer;
				quadrantData.minHeight = quadTree->getQuadrant()->getTerrainEdit()->minHeight;
				quadrantData.maxHeight = quadTree->getQuadrant()->getTerrainEdit()->maxHeight;
				quadrantData.heights16Buffer = &heights16Buffer;
				quadrantData.heights32Buffer = &heights32Buffer;
				quadrantData.normalsBuffer = nullptr;
				quadrantData.splatmapBuffer = nullptr;
				quadrantData.colormapBuffer = nullptr;
				quadrantData.globalmapBuffer = nullptr;

				TheWorld_Utils::MemoryBuffer northTerrainEditValuesBuffer;
				TheWorld_Utils::MemoryBuffer northHeights16Buffer;
				TheWorld_Utils::MemoryBuffer northHeights32Buffer;
				TheWorld_Utils::MeshCacheBuffer::CacheQuadrantData northQuadrantData;
				QuadrantPos p = pos.getQuadrantPos(QuadrantPos::DirectionSlot::ZMinus);
				QuadTree* northQuadTree = m_viewer->getQuadTree(p);
				if (northQuadTree != nullptr)
				{
					northQuadTree->getQuadrant()->lockInternalData();

					northQuadTree->getQuadrant()->getTerrainEdit()->serialize(northTerrainEditValuesBuffer);
					northHeights16Buffer.copyFrom(northQuadTree->getQuadrant()->getFloat16HeightsBuffer());
					northHeights32Buffer.copyFrom(northQuadTree->getQuadrant()->getFloat32HeightsBuffer());

					northQuadrantData.meshId = northQuadTree->getQuadrant()->getMeshCacheBuffer().getMeshId();
					northQuadrantData.terrainEditValues = &northTerrainEditValuesBuffer;
					northQuadrantData.minHeight = northQuadTree->getQuadrant()->getTerrainEdit()->minHeight;
					northQuadrantData.maxHeight = northQuadTree->getQuadrant()->getTerrainEdit()->maxHeight;
					northQuadrantData.heights16Buffer = &northHeights16Buffer;
					northQuadrantData.heights32Buffer = &northHeights32Buffer;
					northQuadrantData.normalsBuffer = nullptr;
					northQuadrantData.splatmapBuffer = nullptr;
					northQuadrantData.colormapBuffer = nullptr;
					northQuadrantData.globalmapBuffer = nullptr;
				}
				else
					northQuadTree = nullptr;

				TheWorld_Utils::MemoryBuffer southTerrainEditValuesBuffer;
				TheWorld_Utils::MemoryBuffer southHeights16Buffer;
				TheWorld_Utils::MemoryBuffer southHeights32Buffer;
				TheWorld_Utils::MeshCacheBuffer::CacheQuadrantData southQuadrantData;
				p = pos.getQuadrantPos(QuadrantPos::DirectionSlot::ZPlus);
				QuadTree* southQuadTree = m_viewer->getQuadTree(p);
				if (southQuadTree != nullptr)
				{
					southQuadTree->getQuadrant()->lockInternalData();

					southQuadTree->getQuadrant()->getTerrainEdit()->serialize(southTerrainEditValuesBuffer);
					southHeights16Buffer.copyFrom(southQuadTree->getQuadrant()->getFloat16HeightsBuffer());
					southHeights32Buffer.copyFrom(southQuadTree->getQuadrant()->getFloat32HeightsBuffer());

					southQuadrantData.meshId = southQuadTree->getQuadrant()->getMeshCacheBuffer().getMeshId();
					southQuadrantData.terrainEditValues = &southTerrainEditValuesBuffer;
					southQuadrantData.minHeight = southQuadTree->getQuadrant()->getTerrainEdit()->minHeight;
					southQuadrantData.maxHeight = southQuadTree->getQuadrant()->getTerrainEdit()->maxHeight;
					southQuadrantData.heights16Buffer = &southHeights16Buffer;
					southQuadrantData.heights32Buffer = &southHeights32Buffer;
					southQuadrantData.normalsBuffer = nullptr;
					southQuadrantData.splatmapBuffer = nullptr;
					southQuadrantData.colormapBuffer = nullptr;
					southQuadrantData.globalmapBuffer = nullptr;
				}
				else
					southQuadTree = nullptr;

				TheWorld_Utils::MemoryBuffer westTerrainEditValuesBuffer;
				TheWorld_Utils::MemoryBuffer westHeights16Buffer;
				TheWorld_Utils::MemoryBuffer westHeights32Buffer;
				TheWorld_Utils::MeshCacheBuffer::CacheQuadrantData westQuadrantData;
				p = pos.getQuadrantPos(QuadrantPos::DirectionSlot::XMinus);
				QuadTree* westQuadTree = m_viewer->getQuadTree(p);
				if (westQuadTree != nullptr)
				{
					westQuadTree->getQuadrant()->lockInternalData();

					westQuadTree->getQuadrant()->getTerrainEdit()->serialize(westTerrainEditValuesBuffer);
					westHeights16Buffer.copyFrom(westQuadTree->getQuadrant()->getFloat16HeightsBuffer());
					westHeights32Buffer.copyFrom(westQuadTree->getQuadrant()->getFloat32HeightsBuffer());

					westQuadrantData.meshId = westQuadTree->getQuadrant()->getMeshCacheBuffer().getMeshId();
					westQuadrantData.terrainEditValues = &westTerrainEditValuesBuffer;
					westQuadrantData.minHeight = westQuadTree->getQuadrant()->getTerrainEdit()->minHeight;
					westQuadrantData.maxHeight = westQuadTree->getQuadrant()->getTerrainEdit()->maxHeight;
					westQuadrantData.heights16Buffer = &westHeights16Buffer;
					westQuadrantData.heights32Buffer = &westHeights32Buffer;
					westQuadrantData.normalsBuffer = nullptr;
					westQuadrantData.splatmapBuffer = nullptr;
					westQuadrantData.colormapBuffer = nullptr;
					westQuadrantData.globalmapBuffer = nullptr;
				}
				else
					westQuadTree = nullptr;

				TheWorld_Utils::MemoryBuffer eastTerrainEditValuesBuffer;
				TheWorld_Utils::MemoryBuffer eastHeights16Buffer;
				TheWorld_Utils::MemoryBuffer eastHeights32Buffer;
				TheWorld_Utils::MeshCacheBuffer::CacheQuadrantData eastQuadrantData;
				p = pos.getQuadrantPos(QuadrantPos::DirectionSlot::XPlus);
				QuadTree* eastQuadTree = m_viewer->getQuadTree(p);
				if (eastQuadTree != nullptr)
				{
					eastQuadTree->getQuadrant()->lockInternalData();

					eastQuadTree->getQuadrant()->getTerrainEdit()->serialize(eastTerrainEditValuesBuffer);
					eastHeights16Buffer.copyFrom(eastQuadTree->getQuadrant()->getFloat16HeightsBuffer());
					eastHeights32Buffer.copyFrom(eastQuadTree->getQuadrant()->getFloat32HeightsBuffer());

					eastQuadrantData.meshId = eastQuadTree->getQuadrant()->getMeshCacheBuffer().getMeshId();
					eastQuadrantData.terrainEditValues = &eastTerrainEditValuesBuffer;
					eastQuadrantData.minHeight = eastQuadTree->getQuadrant()->getTerrainEdit()->minHeight;
					eastQuadrantData.maxHeight = eastQuadTree->getQuadrant()->getTerrainEdit()->maxHeight;
					eastQuadrantData.heights16Buffer = &eastHeights16Buffer;
					eastQuadrantData.heights32Buffer = &eastHeights32Buffer;
					eastQuadrantData.normalsBuffer = nullptr;
					eastQuadrantData.splatmapBuffer = nullptr;
					eastQuadrantData.colormapBuffer = nullptr;
					eastQuadrantData.globalmapBuffer = nullptr;
				}
				else
					eastQuadTree = nullptr;

				TheWorld_Utils::MemoryBuffer northwestTerrainEditValuesBuffer;
				TheWorld_Utils::MemoryBuffer northwestHeights16Buffer;
				TheWorld_Utils::MemoryBuffer northwestHeights32Buffer;
				TheWorld_Utils::MeshCacheBuffer::CacheQuadrantData northwestQuadrantData;
				p = pos.getQuadrantPos(QuadrantPos::DirectionSlot::ZMinusXMinus);
				QuadTree* northwestQuadTree = m_viewer->getQuadTree(p);
				if (northwestQuadTree != nullptr)
				{
					northwestQuadTree->getQuadrant()->lockInternalData();

					northwestQuadTree->getQuadrant()->getTerrainEdit()->serialize(northwestTerrainEditValuesBuffer);
					northwestHeights16Buffer.copyFrom(northwestQuadTree->getQuadrant()->getFloat16HeightsBuffer());
					northwestHeights32Buffer.copyFrom(northwestQuadTree->getQuadrant()->getFloat32HeightsBuffer());

					northwestQuadrantData.meshId = northwestQuadTree->getQuadrant()->getMeshCacheBuffer().getMeshId();
					northwestQuadrantData.terrainEditValues = &northwestTerrainEditValuesBuffer;
					northwestQuadrantData.minHeight = northwestQuadTree->getQuadrant()->getTerrainEdit()->minHeight;
					northwestQuadrantData.maxHeight = northwestQuadTree->getQuadrant()->getTerrainEdit()->maxHeight;
					northwestQuadrantData.heights16Buffer = &northwestHeights16Buffer;
					northwestQuadrantData.heights32Buffer = &northwestHeights32Buffer;
					northwestQuadrantData.normalsBuffer = nullptr;
					northwestQuadrantData.splatmapBuffer = nullptr;
					northwestQuadrantData.colormapBuffer = nullptr;
					northwestQuadrantData.globalmapBuffer = nullptr;
				}
				else
					northwestQuadTree = nullptr;

				TheWorld_Utils::MemoryBuffer northeastTerrainEditValuesBuffer;
				TheWorld_Utils::MemoryBuffer northeastHeights16Buffer;
				TheWorld_Utils::MemoryBuffer northeastHeights32Buffer;
				TheWorld_Utils::MeshCacheBuffer::CacheQuadrantData northeastQuadrantData;
				p = pos.getQuadrantPos(QuadrantPos::DirectionSlot::ZMinusXPlus);
				QuadTree* northeastQuadTree = m_viewer->getQuadTree(p);
				if (northeastQuadTree != nullptr)
				{
					northeastQuadTree->getQuadrant()->lockInternalData();

					northeastQuadTree->getQuadrant()->getTerrainEdit()->serialize(northeastTerrainEditValuesBuffer);
					northeastHeights16Buffer.copyFrom(northeastQuadTree->getQuadrant()->getFloat16HeightsBuffer());
					northeastHeights32Buffer.copyFrom(northeastQuadTree->getQuadrant()->getFloat32HeightsBuffer());

					northeastQuadrantData.meshId = northeastQuadTree->getQuadrant()->getMeshCacheBuffer().getMeshId();
					northeastQuadrantData.terrainEditValues = &northeastTerrainEditValuesBuffer;
					northeastQuadrantData.minHeight = northeastQuadTree->getQuadrant()->getTerrainEdit()->minHeight;
					northeastQuadrantData.maxHeight = northeastQuadTree->getQuadrant()->getTerrainEdit()->maxHeight;
					northeastQuadrantData.heights16Buffer = &northeastHeights16Buffer;
					northeastQuadrantData.heights32Buffer = &northeastHeights32Buffer;
					northeastQuadrantData.normalsBuffer = nullptr;
					northeastQuadrantData.splatmapBuffer = nullptr;
					northeastQuadrantData.colormapBuffer = nullptr;
					northeastQuadrantData.globalmapBuffer = nullptr;
				}
				else
					northeastQuadTree = nullptr;

				TheWorld_Utils::MemoryBuffer southwestTerrainEditValuesBuffer;
				TheWorld_Utils::MemoryBuffer southwestHeights16Buffer;
				TheWorld_Utils::MemoryBuffer southwestHeights32Buffer;
				TheWorld_Utils::MeshCacheBuffer::CacheQuadrantData southwestQuadrantData;
				p = pos.getQuadrantPos(QuadrantPos::DirectionSlot::ZPlusXMinus);
				QuadTree* southwestQuadTree = m_viewer->getQuadTree(p);
				if (southwestQuadTree != nullptr)
				{
					southwestQuadTree->getQuadrant()->lockInternalData();

					southwestQuadTree->getQuadrant()->getTerrainEdit()->serialize(southwestTerrainEditValuesBuffer);
					southwestHeights16Buffer.copyFrom(southwestQuadTree->getQuadrant()->getFloat16HeightsBuffer());
					southwestHeights32Buffer.copyFrom(southwestQuadTree->getQuadrant()->getFloat32HeightsBuffer());

					southwestQuadrantData.meshId = southwestQuadTree->getQuadrant()->getMeshCacheBuffer().getMeshId();
					southwestQuadrantData.terrainEditValues = &southwestTerrainEditValuesBuffer;
					southwestQuadrantData.minHeight = southwestQuadTree->getQuadrant()->getTerrainEdit()->minHeight;
					southwestQuadrantData.maxHeight = southwestQuadTree->getQuadrant()->getTerrainEdit()->maxHeight;
					southwestQuadrantData.heights16Buffer = &southwestHeights16Buffer;
					southwestQuadrantData.heights32Buffer = &southwestHeights32Buffer;
					southwestQuadrantData.normalsBuffer = nullptr;
					southwestQuadrantData.splatmapBuffer = nullptr;
					southwestQuadrantData.colormapBuffer = nullptr;
					southwestQuadrantData.globalmapBuffer = nullptr;
				}
				else
					southwestQuadTree = nullptr;

				TheWorld_Utils::MemoryBuffer southeastTerrainEditValuesBuffer;
				TheWorld_Utils::MemoryBuffer southeastHeights16Buffer;
				TheWorld_Utils::MemoryBuffer southeastHeights32Buffer;

				TheWorld_Utils::MeshCacheBuffer::CacheQuadrantData southeastQuadrantData;
				p = pos.getQuadrantPos(QuadrantPos::DirectionSlot::ZPlusXPlus);
				QuadTree* southeastQuadTree = m_viewer->getQuadTree(p);
				if (southeastQuadTree != nullptr)
				{
					southeastQuadTree->getQuadrant()->lockInternalData();

					southeastQuadTree->getQuadrant()->getTerrainEdit()->serialize(southeastTerrainEditValuesBuffer);
					southeastHeights16Buffer.copyFrom(southeastQuadTree->getQuadrant()->getFloat16HeightsBuffer());
					southeastHeights32Buffer.copyFrom(southeastQuadTree->getQuadrant()->getFloat32HeightsBuffer());

					southeastQuadrantData.meshId = southeastQuadTree->getQuadrant()->getMeshCacheBuffer().getMeshId();
					southeastQuadrantData.terrainEditValues = &southeastTerrainEditValuesBuffer;
					southeastQuadrantData.minHeight = southeastQuadTree->getQuadrant()->getTerrainEdit()->minHeight;
					southeastQuadrantData.maxHeight = southeastQuadTree->getQuadrant()->getTerrainEdit()->maxHeight;
					southeastQuadrantData.heights16Buffer = &southeastHeights16Buffer;
					southeastQuadrantData.heights32Buffer = &southeastHeights32Buffer;
					southeastQuadrantData.normalsBuffer = nullptr;
					southeastQuadrantData.splatmapBuffer = nullptr;
					southeastQuadrantData.colormapBuffer = nullptr;
					southeastQuadrantData.globalmapBuffer = nullptr;
				}
				else
					southeastQuadTree = nullptr;

				bool lastPhase = false;
				if (round == numRounds - 1)
					lastPhase = true;
				
				bool updated = quadTree->getQuadrant()->getMeshCacheBuffer().blendQuadrant(pos.getNumVerticesPerSize(), pos.getGridStepInWU(), lastPhase,
					quadrantData,
					northQuadrantData,
					southQuadrantData,
					westQuadrantData,
					eastQuadrantData,
					northwestQuadrantData,
					northeastQuadrantData,
					southwestQuadrantData,
					southeastQuadrantData);

				manageUpdatedHeights(quadrantData, quadTree, terrainEditValuesBuffer, heights16Buffer, heights32Buffer);
				quadTree->getQuadrant()->unlockInternalData();

				if (northQuadTree != nullptr)
				{
					manageUpdatedHeights(northQuadrantData, northQuadTree, northTerrainEditValuesBuffer, northHeights16Buffer, northHeights32Buffer);
					northQuadTree->getQuadrant()->unlockInternalData();
				}
				if (southQuadTree != nullptr)
				{
					manageUpdatedHeights(southQuadrantData, southQuadTree, southTerrainEditValuesBuffer, southHeights16Buffer, southHeights32Buffer);
					southQuadTree->getQuadrant()->unlockInternalData();
				}
				if (westQuadTree != nullptr)
				{
					manageUpdatedHeights(westQuadrantData, westQuadTree, westTerrainEditValuesBuffer, westHeights16Buffer, westHeights32Buffer);
					westQuadTree->getQuadrant()->unlockInternalData();
				}
				if (eastQuadTree != nullptr)
				{
					manageUpdatedHeights(eastQuadrantData, eastQuadTree, eastTerrainEditValuesBuffer, eastHeights16Buffer, eastHeights32Buffer);
					eastQuadTree->getQuadrant()->unlockInternalData();
				}
				if (northwestQuadTree != nullptr)
				{
					manageUpdatedHeights(northwestQuadrantData, northwestQuadTree, northwestTerrainEditValuesBuffer, northwestHeights16Buffer, northwestHeights32Buffer);
					northwestQuadTree->getQuadrant()->unlockInternalData();
				}
				if (northeastQuadTree != nullptr)
				{
					manageUpdatedHeights(northeastQuadrantData, northeastQuadTree, northeastTerrainEditValuesBuffer, northeastHeights16Buffer, northeastHeights32Buffer);
					northeastQuadTree->getQuadrant()->unlockInternalData();
				}
				if (southwestQuadTree != nullptr)
				{
					manageUpdatedHeights(southwestQuadrantData, southwestQuadTree, southwestTerrainEditValuesBuffer, southwestHeights16Buffer, southwestHeights32Buffer);
					southwestQuadTree->getQuadrant()->unlockInternalData();
				}
				if (southeastQuadTree != nullptr)
				{
					manageUpdatedHeights(southeastQuadrantData, southeastQuadTree, southeastTerrainEditValuesBuffer, southeastHeights16Buffer, southeastHeights32Buffer);
					southeastQuadTree->getQuadrant()->unlockInternalData();
				}

				m_completedItems++;
				size_t partialCount = m_actionClock.partialDuration().count();
				m_lastElapsed = partialCount - m_elapsedCompleted;
				m_elapsedCompleted = partialCount;
			}
		}
	}

	size_t partialCount = m_actionClock.partialDuration().count();
	m_lastElapsed = partialCount - m_elapsedCompleted;
	m_elapsedCompleted = partialCount;

	m_actionClock.tock();

	m_actionInProgress = false;

	size_t duration = m_actionClock.duration().count();
	setElapsed(duration, false);
	setCounter(m_completedItems, m_allItems);
	setNote1(m_lastElapsed);

	size_t numToSave = 0;
	size_t numToUpload = 0;
	refreshNumToSaveUpload(numToSave, numToUpload);

	setMessage(godot::String("Completed!"), true);
}

void GDN_TheWorld_Edit::manageUpdatedHeights(TheWorld_Utils::MeshCacheBuffer::CacheQuadrantData& quadrantData, QuadTree* quadTree, TheWorld_Utils::MemoryBuffer& terrainEditValuesBuffer, TheWorld_Utils::MemoryBuffer& heights16Buffer, TheWorld_Utils::MemoryBuffer& heights32Buffer)
{
	if (quadrantData.heightsUpdated)
	{
		quadTree->getQuadrant()->getTerrainEdit()->deserialize(terrainEditValuesBuffer);
		quadTree->getQuadrant()->getFloat16HeightsBuffer().copyFrom(heights16Buffer);
		quadTree->getQuadrant()->getFloat32HeightsBuffer().copyFrom(heights32Buffer);

		m_mapQuadToSave[quadTree->getQuadrant()->getPos()] = "";
		quadTree->getQuadrant()->setEmpty(false);
		quadTree->getQuadrant()->getTerrainEdit()->needUploadToServer = true;
		quadTree->getQuadrant()->setNeedUploadToServer(true);

		quadTree->getQuadrant()->setHeightsUpdated(true);

		quadTree->getQuadrant()->resetNormalsBuffer();

		quadTree->getQuadrant()->resetSplatmapBuffer();

		quadTree->getQuadrant()->resetGlobalmapBuffer();


		{
			godot::PackedFloat32Array& heightsForCollider = quadTree->getQuadrant()->getHeightsForCollider(false);
			heightsForCollider.resize((int)(quadTree->getQuadrant()->getPos().getNumVerticesPerSize() * quadTree->getQuadrant()->getPos().getNumVerticesPerSize()));
			memcpy((char*)heightsForCollider.ptrw(), quadTree->getQuadrant()->getFloat32HeightsBuffer().ptr(), quadTree->getQuadrant()->getFloat32HeightsBuffer().size());
		}

		Vector3 startPosition(quadTree->getQuadrant()->getPos().getLowerXGridVertex(), quadTree->getQuadrant()->getTerrainEdit()->minHeight, quadTree->getQuadrant()->getPos().getLowerZGridVertex());
		Vector3 endPosition(startPosition.x + quadTree->getQuadrant()->getPos().getSizeInWU(), quadTree->getQuadrant()->getTerrainEdit()->maxHeight, startPosition.z + quadTree->getQuadrant()->getPos().getSizeInWU());
		Vector3 size = endPosition - startPosition;
		quadTree->getQuadrant()->getGlobalCoordAABB().set_position(startPosition);
		quadTree->getQuadrant()->getGlobalCoordAABB().set_size(size);

		Chunk::HeightsChangedChunkAction action(m_viewer->is_visible_in_tree());
		m_viewer->m_refreshRequired = true;
		quadTree->ForAllChunk(action);

		quadTree->materialParamsNeedReset(true);
	}
}

void GDN_TheWorld_Edit::editModeGenNormalsAction(void)
{
	if (m_actionInProgress)
		return;

	m_actionInProgress = true;
	m_allItems = 0;
	setElapsed(0, true);
	setNote1(0);

	std::function<void(void)> f = std::bind(&GDN_TheWorld_Edit::editModeGenNormals, this);
	m_tp.QueueJob(f);
}

void GDN_TheWorld_Edit::editModeGenNormals(void)
{
	editModeGenNormals_1(true);
}

void GDN_TheWorld_Edit::editModeGenNormals_1(bool force)
{
	if (m_actionClock.counterStarted())
	{
		m_actionInProgress = false;
		return;
	}

	TheWorld_Utils::GuardProfiler profiler(std::string("EditGenNormals 1 ") + __FUNCTION__, "ALL");

	setMessage(godot::String("Generating normals..."));
	
	m_actionClock.tick();

	QuadTree* quadTreeSel = nullptr;
	QuadrantPos quadrantSelPos = m_viewer->getQuadrantSelForEdit(&quadTreeSel);

	bool allCheched = m_innerData->m_allCheckBox->is_pressed();

	if (!allCheched && quadrantSelPos.empty())
	{
		m_actionClock.tock();
		m_actionInProgress = false;
		setMessage(godot::String("Completed!"), true);
		return;
	}

	if (quadTreeSel != nullptr && force)
		quadTreeSel->getQuadrant()->getTerrainEdit()->normalsNeedRegen = true;

	std::vector<QuadrantPos> allQuandrantPos;
	m_viewer->getAllQuadrantPos(allQuandrantPos);

	std::vector<QuadrantPos> quandrantPos;
	for (auto& pos : allQuandrantPos)
	{
		QuadTree* quadTree = m_viewer->getQuadTree(pos);
		if (quadTree != nullptr && !quadTree->getQuadrant()->empty())
		{
			//quadTree->getQuadrant()->getTerrainEdit()->normalsNeedRegen = true;		// SUPERDEBUGRIC: force to generate normals
			TheWorld_Utils::MemoryBuffer& normalsBuffer = quadTree->getQuadrant()->getNormalsBuffer(true);
			if (normalsBuffer.size() == 0 || quadTree->getQuadrant()->getTerrainEdit()->normalsNeedRegen)
			{
				if (allCheched || pos == quadrantSelPos)
				{
					quandrantPos.push_back(pos);
					quadTree->getQuadrant()->setNeedUploadToServer(true);

					if (!allCheched)
						break;
				}
			}
		}
	}

	m_completedItems = 0;
	m_elapsedCompleted = 0;
	m_lastElapsed = 0;
	m_allItems = quandrantPos.size();
	for (auto& pos : quandrantPos)
	{
		if (m_actionStopRequested)
		{
			m_actionStopRequested = false;
			break;
		}

		QuadTree* quadTree = m_viewer->getQuadTree(pos);
		if (quadTree != nullptr)
		{
			quadTree->getQuadrant()->lockInternalData();

			TheWorld_Utils::MemoryBuffer& normalsBuffer = quadTree->getQuadrant()->getNormalsBuffer(true);
			if (normalsBuffer.size() == 0 || quadTree->getQuadrant()->getTerrainEdit()->normalsNeedRegen)
			{
				TheWorld_Utils::GuardProfiler profiler(std::string("EditGenNormals 1.1 ") + __FUNCTION__, "Single QuadTree");

				TheWorld_Utils::MeshCacheBuffer& cache = quadTree->getQuadrant()->getMeshCacheBuffer();
				TheWorld_Utils::MemoryBuffer& heightsBuffer = quadTree->getQuadrant()->getFloat32HeightsBuffer();
				size_t numElements = heightsBuffer.size() / sizeof(float);
				my_assert(numElements == pos.getNumVerticesPerSize() * pos.getNumVerticesPerSize());

				TheWorld_Utils::MemoryBuffer emptyHeights32Buffer;
				TheWorld_Utils::MemoryBuffer* eastHeights32Buffer = &emptyHeights32Buffer;
				QuadrantPos p = pos.getQuadrantPos(QuadrantPos::DirectionSlot::EastXPlus);
				QuadTree* eastQuadTree = m_viewer->getQuadTree(p);
				if (eastQuadTree != nullptr)
				{
					eastQuadTree->getQuadrant()->lockInternalData();
					eastHeights32Buffer = &eastQuadTree->getQuadrant()->getFloat32HeightsBuffer();
				}
				TheWorld_Utils::MemoryBuffer* southHeights32Buffer = &emptyHeights32Buffer;
				p = pos.getQuadrantPos(QuadrantPos::DirectionSlot::SouthZPlus);
				QuadTree* southQuadTree = m_viewer->getQuadTree(p);
				if (southQuadTree != nullptr)
				{
					southQuadTree->getQuadrant()->lockInternalData();
					southHeights32Buffer = &southQuadTree->getQuadrant()->getFloat32HeightsBuffer();
				}
				TheWorld_Utils::MemoryBuffer& float32HeigthsBuffer = quadTree->getQuadrant()->getFloat32HeightsBuffer(true);
				cache.generateNormalsForBlendedQuadrants(pos.getNumVerticesPerSize(), pos.getGridStepInWU(), float32HeigthsBuffer, *eastHeights32Buffer, *southHeights32Buffer, normalsBuffer);
				if (eastQuadTree != nullptr)
					eastQuadTree->getQuadrant()->unlockInternalData();
				if (southQuadTree != nullptr)
					southQuadTree->getQuadrant()->unlockInternalData();

				//std::vector<float> vectGridHeights;
				//heightsBuffer.populateFloatVector(vectGridHeights);
				//cache.deprecated_generateNormals(pos.getNumVerticesPerSize(), pos.getGridStepInWU(), vectGridHeights, normalsBuffer);
				
				m_mapQuadToSave[pos] = "";
				quadTree->getQuadrant()->getTerrainEdit()->needUploadToServer = true;
				quadTree->getQuadrant()->setNeedUploadToServer(true);
				
				quadTree->getQuadrant()->getTerrainEdit()->normalsNeedRegen = false;
				quadTree->getQuadrant()->setNormalsUpdated(true);

				quadTree->getQuadrant()->resetSplatmapBuffer();

				quadTree->getQuadrant()->resetGlobalmapBuffer();

				quadTree->materialParamsNeedReset(true);

				m_completedItems++;
				size_t partialCount = m_actionClock.partialDuration().count();
				m_lastElapsed = partialCount - m_elapsedCompleted;
				m_elapsedCompleted = partialCount;
			}

			quadTree->getQuadrant()->unlockInternalData();
		}
	}

	m_actionClock.tock();

	m_actionInProgress = false;

	size_t duration = m_actionClock.duration().count();
	setElapsed(duration, false);
	setCounter(m_completedItems, m_allItems);
	setNote1(m_lastElapsed);

	size_t numToSave = 0;
	size_t numToUpload = 0;
	refreshNumToSaveUpload(numToSave, numToUpload);

	setMessage(godot::String("Completed!"), true);
}

void GDN_TheWorld_Edit::editModeApplyTexturesAction(void)
{
	if (m_actionInProgress)
		return;

	m_actionInProgress = true;
	m_allItems = 0;
	setElapsed(0, true);
	setNote1(0);

	std::function<void(void)> f = std::bind(&GDN_TheWorld_Edit::editModeApplyTextures, this);
	m_tp.QueueJob(f);
}

void GDN_TheWorld_Edit::editModeApplyTextures(void)
{
	if (m_actionClock.counterStarted())
	{
		m_actionInProgress = false;
		return;
	}

	editModeGenNormals_1(false);

	m_actionInProgress = true;

	TheWorld_Utils::GuardProfiler profiler(std::string("EditSetTextures 1 ") + __FUNCTION__, "ALL");

	setMessage(godot::String("Applying textures..."));

	m_actionClock.tick();

	QuadTree* quadTreeSel = nullptr;
	QuadrantPos quadrantSelPos = m_viewer->getQuadrantSelForEdit(&quadTreeSel);

	bool allCheched = m_innerData->m_allCheckBox->is_pressed();

	if (!allCheched && quadrantSelPos.empty())
	{
		m_actionClock.tock();
		m_actionInProgress = false;
		setMessage(godot::String("Completed!"), true);
		return;
	}

	if (quadTreeSel != nullptr)
		quadTreeSel->getQuadrant()->getTerrainEdit()->extraValues.splatmapNeedRegen = true;

	std::vector<QuadrantPos> allQuandrantPos;
	m_viewer->getAllQuadrantPos(allQuandrantPos);

	std::vector<QuadrantPos> quandrantPos;
	for (auto& pos : allQuandrantPos)
	{
		QuadTree* quadTree = m_viewer->getQuadTree(pos);
		if (quadTree != nullptr && !quadTree->getQuadrant()->empty())
		{
			//quadTree->getQuadrant()->getTerrainEdit()->extraValues.splatmapNeedRegen = true;		// SUPERDEBUGRIC: force to generate textures
			TheWorld_Utils::MemoryBuffer& splatmapBuffer = quadTree->getQuadrant()->getSplatmapBuffer(true);
			if (splatmapBuffer.size() == 0 || quadTree->getQuadrant()->getTerrainEdit()->extraValues.splatmapNeedRegen)
			{
				if (allCheched || pos == quadrantSelPos)
				{
					quandrantPos.push_back(pos);
					quadTree->getQuadrant()->setNeedUploadToServer(true);

					if (!allCheched)
						break;
				}
			}
		}
	}

	m_completedItems = 0;
	m_elapsedCompleted = 0;
	m_lastElapsed = 0;
	m_allItems = quandrantPos.size();
	for (auto& pos : quandrantPos)
	{
		if (m_actionStopRequested)
		{
			m_actionStopRequested = false;
			break;
		}

		QuadTree* quadTree = m_viewer->getQuadTree(pos);
		if (quadTree != nullptr)
		{
			quadTree->getQuadrant()->lockInternalData();

			TheWorld_Utils::MemoryBuffer& splatmapBuffer = quadTree->getQuadrant()->getSplatmapBuffer(true);
			if (splatmapBuffer.size() == 0 || quadTree->getQuadrant()->getTerrainEdit()->extraValues.splatmapNeedRegen)
			{
				TheWorld_Utils::GuardProfiler profiler(std::string("EditSetTextures 1.1 ") + __FUNCTION__, "Single QuadTree");

				
				TheWorld_Utils::MeshCacheBuffer& cache = quadTree->getQuadrant()->getMeshCacheBuffer();
				
				TheWorld_Utils::MemoryBuffer& normalsBuffer = quadTree->getQuadrant()->getNormalsBuffer(true);
				TheWorld_Utils::MemoryBuffer& float32HeigthsBuffer = quadTree->getQuadrant()->getFloat32HeightsBuffer();
				TheWorld_Utils::MemoryBuffer& splatmapBuffer = quadTree->getQuadrant()->getSplatmapBuffer();
				size_t numElements = float32HeigthsBuffer.size() / sizeof(float);
				my_assert(numElements == pos.getNumVerticesPerSize() * pos.getNumVerticesPerSize());

				cache.generateSplatmap(pos.getNumVerticesPerSize(), pos.getGridStepInWU(), quadTree->getQuadrant()->getTerrainEdit(), float32HeigthsBuffer, normalsBuffer, splatmapBuffer);

				m_mapQuadToSave[pos] = "";
				quadTree->getQuadrant()->getTerrainEdit()->needUploadToServer = true;
				quadTree->getQuadrant()->setNeedUploadToServer(true);

				quadTree->getQuadrant()->getTerrainEdit()->extraValues.splatmapNeedRegen = false;
				quadTree->getQuadrant()->setSplatmapUpdated(true);

				quadTree->getQuadrant()->resetGlobalmapBuffer();

				quadTree->materialParamsNeedReset(true);

				{
					quadTree->getQuadrant()->getTerrainEdit()->setTextureNameForTerrainType(TheWorld_Utils::TerrainEdit::TextureType::lowElevation);
					quadTree->getQuadrant()->getTerrainEdit()->setTextureNameForTerrainType(TheWorld_Utils::TerrainEdit::TextureType::highElevation);
					quadTree->getQuadrant()->getTerrainEdit()->setTextureNameForTerrainType(TheWorld_Utils::TerrainEdit::TextureType::dirt);
					quadTree->getQuadrant()->getTerrainEdit()->setTextureNameForTerrainType(TheWorld_Utils::TerrainEdit::TextureType::rocks);
				}

				m_completedItems++;
				size_t partialCount = m_actionClock.partialDuration().count();
				m_lastElapsed = partialCount - m_elapsedCompleted;
				m_elapsedCompleted = partialCount;
			}

			quadTree->getQuadrant()->unlockInternalData();
		}
	}

	m_actionClock.tock();

	m_actionInProgress = false;

	size_t duration = m_actionClock.duration().count();
	setElapsed(duration, false);
	setCounter(m_completedItems, m_allItems);
	setNote1(m_lastElapsed);

	size_t numToSave = 0;
	size_t numToUpload = 0;
	refreshNumToSaveUpload(numToSave, numToUpload);

	setMessage(godot::String("Completed!"), true);

}

void GDN_TheWorld_Edit::editModeSelectLookDevAction(int32_t index)
{
	enum class ShaderTerrainData::LookDev lookDev = (enum class ShaderTerrainData::LookDev)m_innerData->m_lookDevOptionButton->get_item_id(index);
	m_viewer->setDesideredLookDev(lookDev);
}

void GDN_TheWorld_Edit::editModeSelectTerrainTypeAction(int32_t index)
{
	enum class TheWorld_Utils::TerrainEdit::TerrainType terrainType = (enum class TheWorld_Utils::TerrainEdit::TerrainType)m_innerData->m_terrTypeOptionButton->get_item_id(index);
	std::string s = TheWorld_Utils::TerrainEdit::terrainTypeString(terrainType);

	TheWorld_Utils::TerrainEdit terrainEdit(terrainType);

	QuadTree* selectedQuadTree = nullptr;
	QuadrantPos selectedQuadrantPos = m_viewer->getQuadrantSelForEdit(&selectedQuadTree);
	if (!selectedQuadrantPos.empty() && selectedQuadTree != nullptr)
	{
		TheWorld_Utils::TerrainEdit* northSideTerrainEdit = nullptr;
		QuadrantPos pos = selectedQuadrantPos.getQuadrantPos(QuadrantPos::DirectionSlot::NorthZMinus);
		QuadTree* q = m_viewer->getQuadTree(pos);
		if (q != nullptr && !q->getQuadrant()->empty())
			northSideTerrainEdit = q->getQuadrant()->getTerrainEdit();
		TheWorld_Utils::TerrainEdit* southSideTerrainEdit = nullptr;
		pos = selectedQuadrantPos.getQuadrantPos(QuadrantPos::DirectionSlot::SouthZPlus);
		q = m_viewer->getQuadTree(pos);
		if (q != nullptr && !q->getQuadrant()->empty())
			southSideTerrainEdit = q->getQuadrant()->getTerrainEdit();
		TheWorld_Utils::TerrainEdit* westSideTerrainEdit = nullptr;
		pos = selectedQuadrantPos.getQuadrantPos(QuadrantPos::DirectionSlot::WestXMinus);
		q = m_viewer->getQuadTree(pos);
		if (q != nullptr && !q->getQuadrant()->empty())
			westSideTerrainEdit = q->getQuadrant()->getTerrainEdit();
		TheWorld_Utils::TerrainEdit* eastSideTerrainEdit = nullptr;
		pos = selectedQuadrantPos.getQuadrantPos(QuadrantPos::DirectionSlot::EastXPlus);
		q = m_viewer->getQuadTree(pos);
		if (q != nullptr && !q->getQuadrant()->empty())
			eastSideTerrainEdit = q->getQuadrant()->getTerrainEdit();
		terrainEdit.adjustValues(northSideTerrainEdit, southSideTerrainEdit, westSideTerrainEdit, eastSideTerrainEdit);

	}

	setTerrainEditValues(terrainEdit);
}

void GDN_TheWorld_Edit::setMessage(std::string text, bool add)
{
	setMessage(godot::String(text.c_str()), add);
}

void GDN_TheWorld_Edit::setMessage(godot::String text, bool add)
{
	std::lock_guard<std::recursive_mutex> lock(m_mtxUI);

	if (add)
	{
		godot::String t = m_innerData->m_message->get_text();
		t += " ";
		t += text;
		m_innerData->m_lastMessage = t;
		//m_message->set_text(t);
	}
	else
		m_innerData->m_lastMessage = text;
		//m_message->set_text(text);

	m_lastMessageChanged = true;
}
