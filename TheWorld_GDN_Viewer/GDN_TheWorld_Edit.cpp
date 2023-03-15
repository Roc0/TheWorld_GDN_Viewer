//#include "pch.h"
#include <Godot.hpp>
#include <Node.hpp>
//#include <MarginContainer.hpp>
#include <TabContainer.hpp>
#include <PanelContainer.hpp>
#include <VBoxContainer.hpp>
#include <HBoxContainer.hpp>
#include <Button.hpp>
#include <Label.hpp>
#include <CheckBox.hpp>
#include <OptionButton.hpp>
#include <HSeparator.hpp>
#include <VSeparator.hpp>

#include <string>
#include <iostream>

#include "GDN_TheWorld_Viewer.h"
#include "GDN_TheWorld_Edit.h"
#include "WorldModifier.h"
#include "Utils.h"
#include "FastNoiseLite.h"
#include "half.h"

using namespace godot;

void GDN_TheWorld_Edit::_register_methods()
{
	register_method("_ready", &GDN_TheWorld_Edit::_ready);
	register_method("_process", &GDN_TheWorld_Edit::_process);
	register_method("_input", &GDN_TheWorld_Edit::_input);
	register_method("_notification", &GDN_TheWorld_Edit::_notification);

	register_method("edit_mode_noise_panel", &GDN_TheWorld_Edit::editModeNoisePanel);

	//register_method("hello", &GDN_TheWorld_Edit::hello);
	register_method("edit_mode_generate", &GDN_TheWorld_Edit::editModeGenerateAction);
	register_method("edit_mode_blend", &GDN_TheWorld_Edit::editModeBlendAction);
	register_method("edit_mode_gen_normals", &GDN_TheWorld_Edit::editModeGenNormalsAction);
	register_method("edit_mode_save", &GDN_TheWorld_Edit::editModeSaveAction);
	register_method("edit_mode_upload", &GDN_TheWorld_Edit::editModeUploadAction);
	register_method("edit_mode_stop", &GDN_TheWorld_Edit::editModeStopAction);
	register_method("edit_mode_sel_terr_type", &GDN_TheWorld_Edit::editModeSelectTerrainTypeAction);
	register_method("mouse_entered_main_panel", &GDN_TheWorld_Edit::editModeMouseEnteredMainPanel);
	register_method("mouse_exited_main_panel", &GDN_TheWorld_Edit::editModeMouseExitedMainPanel);
}

GDN_TheWorld_Edit::GDN_TheWorld_Edit()
{
	m_initialized = false;
	m_actionInProgress = false;
	m_actionStopRequested = false;
	m_onGoingElapsedLabel = false;
	m_viewer = nullptr;
	m_mainPanelContainer = nullptr;
	m_noiseVBoxContainer = nullptr;
	m_noiseButton = nullptr;
	//m_noiseContainerShowing = false;
	m_mainTabContainer = nullptr;
	m_seed = nullptr;
	m_frequency = nullptr;
	m_fractalOctaves = nullptr;
	m_fractalLacunarity = nullptr;
	m_fractalGain = nullptr;
	m_fractalWeightedStrength = nullptr;
	m_fractalPingPongStrength = nullptr;
	m_amplitudeLabel = nullptr;
	m_scaleFactorLabel = nullptr;
	m_desideredMinHeightLabel = nullptr;
	m_minHeightLabel = nullptr;
	m_maxHeightLabel = nullptr;
	m_elapsedLabel = nullptr;
	m_counterLabel = nullptr;
	m_note1Label = nullptr;
	m_mouseHitLabel = nullptr;
	m_mouseQuadHitLabel = nullptr;
	m_mouseQuadHitPosLabel = nullptr;
	m_mouseQuadSelLabel = nullptr;
	m_mouseQuadSelPosLabel = nullptr;
	m_numQuadrantToSaveLabel = nullptr;
	m_numQuadrantToUploadLabel = nullptr;
	m_genAllNormals = nullptr;
	m_allItems = 0;
	m_completedItems = 0;
	m_elapsedCompleted = 0;
	m_lastElapsed = 0;
	m_terrTypeOptionButton = nullptr;
	m_UIAcceptingFocus = false;
	m_requiredUIAcceptFocus = false;
}

GDN_TheWorld_Edit::~GDN_TheWorld_Edit()
{
	deinit();
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

	m_genAllNormals->set_focus_mode(focusMode);
	m_seed->set_focus_mode(focusMode);
	m_frequency->set_focus_mode(focusMode);
	m_fractalOctaves->set_focus_mode(focusMode);
	m_fractalLacunarity->set_focus_mode(focusMode);
	m_fractalGain->set_focus_mode(focusMode);
	m_amplitudeLabel->set_focus_mode(focusMode);
	m_scaleFactorLabel->set_focus_mode(focusMode);
	m_desideredMinHeightLabel->set_focus_mode(focusMode);
	m_fractalWeightedStrength->set_focus_mode(focusMode);
	m_fractalPingPongStrength->set_focus_mode(focusMode);

	m_UIAcceptingFocus = b;
}

void GDN_TheWorld_Edit::init(GDN_TheWorld_Viewer* viewer)
{
	m_initialized = true;
	m_viewer = viewer;

	//TheWorld_Utils::WorldModifierPos pos(0, 4096, 4096, TheWorld_Utils::WMType::elevator, 0);
	//m_wms[pos] = std::make_unique<TheWorld_Utils::WorldModifier>(pos, TheWorld_Utils::WMFunctionType::ConsiderMinMax, 10000.0f, 0.0f, 200.0f, TheWorld_Utils::WMOrder::MaxEffectOnWM);


	std::lock_guard<std::recursive_mutex> lock(m_viewer->getMainProcessingMutex());

	m_viewer->add_child(this);
	set_name(THEWORLD_EDIT_MODE_UI_CONTROL_NAME);
	resizeUI();

	//godot::Control* marginContainer = nullptr;
	//godot::Control* vBoxContainer = nullptr;
	godot::Control* hBoxContainer = nullptr;
	godot::Control* separator = nullptr;
	godot::Button* button = nullptr;
	godot::Label* label = nullptr;
	//godot::OptionButton* optionButton = nullptr;
	//godot::CheckBox* checkBox = nullptr;

	m_mainPanelContainer = godot::PanelContainer::_new();
	m_mainPanelContainer->set_name("EditBox");
	add_child(m_mainPanelContainer);
	m_mainPanelContainer->connect("mouse_entered", this, "mouse_entered_main_panel");
	m_mainPanelContainer->connect("mouse_exited", this, "mouse_exited_main_panel");

	m_mainTabContainer = godot::TabContainer::_new();
	m_mainTabContainer->set_name("EditModeTab");
	m_mainPanelContainer->add_child(m_mainTabContainer);
	m_mainTabContainer->connect("mouse_entered", this, "mouse_entered_main_panel");
	m_mainTabContainer->connect("mouse_exited", this, "mouse_exited_main_panel");

	//godot::Control* mainPanelContainer = godot::PanelContainer::_new();
	//mainPanelContainer->set_name("Terrain");
	//m_mainTabContainer->add_child(mainPanelContainer);

		godot::Control* mainVBoxContainer = godot::VBoxContainer::_new();
		mainVBoxContainer->set_name("Terrain");
		m_mainTabContainer->add_child(mainVBoxContainer);
		//mainVBoxContainer->connect("mouse_entered", this, "mouse_entered_main_panel");
		//mainVBoxContainer->connect("mouse_exited", this, "mouse_exited_main_panel");

			separator = HSeparator::_new();
			mainVBoxContainer->add_child(separator);
			separator->connect("mouse_entered", this, "mouse_entered_main_panel");
			separator->connect("mouse_exited", this, "mouse_exited_main_panel");

			hBoxContainer = godot::HBoxContainer::_new();
			mainVBoxContainer->add_child(hBoxContainer);
				button = godot::Button::_new();
				hBoxContainer->add_child(button);
				button->set_text("Blend");
				button->connect("pressed", this, "edit_mode_blend");
				button->connect("mouse_entered", this, "mouse_entered_main_panel");
				button->connect("mouse_exited", this, "mouse_exited_main_panel");
				button->set_focus_mode(godot::Control::FocusMode::FOCUS_NONE);
				separator = VSeparator::_new();
				hBoxContainer->add_child(separator);
				separator->connect("mouse_entered", this, "mouse_entered_main_panel");
				separator->connect("mouse_exited", this, "mouse_exited_main_panel");
				button = godot::Button::_new();
				hBoxContainer->add_child(button);
				button->set_text("Gen. Normals");
				button->connect("pressed", this, "edit_mode_gen_normals");
				button->connect("mouse_entered", this, "mouse_entered_main_panel");
				button->connect("mouse_exited", this, "mouse_exited_main_panel");
				button->set_focus_mode(godot::Control::FocusMode::FOCUS_NONE);
				m_genAllNormals = godot::CheckBox::_new();
				hBoxContainer->add_child(m_genAllNormals);
				m_genAllNormals->connect("mouse_entered", this, "mouse_entered_main_panel");
				m_genAllNormals->connect("mouse_exited", this, "mouse_exited_main_panel");
				m_genAllNormals->set_text("All");
				m_genAllNormals->set_toggle_mode(true);

			separator = HSeparator::_new();
			mainVBoxContainer->add_child(separator);
			separator->connect("mouse_entered", this, "mouse_entered_main_panel");
			separator->connect("mouse_exited", this, "mouse_exited_main_panel");

			m_noiseButton = godot::Button::_new();
			mainVBoxContainer->add_child(m_noiseButton);
			//m_noiseButton->set_flat(true);
			m_noiseButton->set_text_align(godot::Button::ALIGN_LEFT);
			m_noiseButton->set_text("> Noise");
			m_noiseButton->connect("pressed", this, "edit_mode_noise_panel");
			m_noiseButton->connect("mouse_entered", this, "mouse_entered_main_panel");
			m_noiseButton->connect("mouse_exited", this, "mouse_exited_main_panel");
			m_noiseButton->set_focus_mode(godot::Control::FocusMode::FOCUS_NONE);

			m_noiseVBoxContainer = godot::VBoxContainer::_new();
			m_noiseVBoxContainer->set_name("Noise");
			mainVBoxContainer->add_child(m_noiseVBoxContainer);
			m_noiseVBoxContainer->hide();
			//m_noiseContainerShowing = false;

				hBoxContainer = godot::HBoxContainer::_new();
				m_noiseVBoxContainer->add_child(hBoxContainer);
					m_terrTypeOptionButton = godot::OptionButton::_new();
					hBoxContainer->add_child(m_terrTypeOptionButton);
					//optionButton->connect("pressed", this, "edit_mode_sel_terr_type");
					m_terrTypeOptionButton->connect("item_selected", this, "edit_mode_sel_terr_type");
					m_terrTypeOptionButton->connect("mouse_entered", this, "mouse_entered_main_panel");
					m_terrTypeOptionButton->connect("mouse_exited", this, "mouse_exited_main_panel");
					m_terrTypeOptionButton->set_focus_mode(godot::Control::FocusMode::FOCUS_NONE);
					m_terrTypeOptionButton->add_item(TheWorld_Utils::TerrainEdit::terrainTypeString(TheWorld_Utils::TerrainEdit::TerrainType::unknown).c_str(), (int64_t)TheWorld_Utils::TerrainEdit::TerrainType::unknown);
					m_terrTypeOptionButton->add_separator();
					m_terrTypeOptionButton->add_item(TheWorld_Utils::TerrainEdit::terrainTypeString(TheWorld_Utils::TerrainEdit::TerrainType::campaign_1).c_str(), (int64_t)TheWorld_Utils::TerrainEdit::TerrainType::campaign_1);
					m_terrTypeOptionButton->add_item(TheWorld_Utils::TerrainEdit::terrainTypeString(TheWorld_Utils::TerrainEdit::TerrainType::plateau_1).c_str(), (int64_t)TheWorld_Utils::TerrainEdit::TerrainType::plateau_1);
					m_terrTypeOptionButton->add_item(TheWorld_Utils::TerrainEdit::terrainTypeString(TheWorld_Utils::TerrainEdit::TerrainType::low_hills).c_str(), (int64_t)TheWorld_Utils::TerrainEdit::TerrainType::low_hills);
					m_terrTypeOptionButton->add_item(TheWorld_Utils::TerrainEdit::terrainTypeString(TheWorld_Utils::TerrainEdit::TerrainType::high_hills).c_str(), (int64_t)TheWorld_Utils::TerrainEdit::TerrainType::high_hills);
					m_terrTypeOptionButton->add_item(TheWorld_Utils::TerrainEdit::terrainTypeString(TheWorld_Utils::TerrainEdit::TerrainType::low_mountains).c_str(), (int64_t)TheWorld_Utils::TerrainEdit::TerrainType::low_mountains);
					m_terrTypeOptionButton->add_item(TheWorld_Utils::TerrainEdit::terrainTypeString(TheWorld_Utils::TerrainEdit::TerrainType::low_mountains_grow).c_str(), (int64_t)TheWorld_Utils::TerrainEdit::TerrainType::low_mountains_grow);
					m_terrTypeOptionButton->add_item(TheWorld_Utils::TerrainEdit::terrainTypeString(TheWorld_Utils::TerrainEdit::TerrainType::high_mountains_1).c_str(), (int64_t)TheWorld_Utils::TerrainEdit::TerrainType::high_mountains_1);
					m_terrTypeOptionButton->add_item(TheWorld_Utils::TerrainEdit::terrainTypeString(TheWorld_Utils::TerrainEdit::TerrainType::high_mountains_1_grow).c_str(), (int64_t)TheWorld_Utils::TerrainEdit::TerrainType::high_mountains_1_grow);
					m_terrTypeOptionButton->add_item(TheWorld_Utils::TerrainEdit::terrainTypeString(TheWorld_Utils::TerrainEdit::TerrainType::high_mountains_2).c_str(), (int64_t)TheWorld_Utils::TerrainEdit::TerrainType::high_mountains_2);
					m_terrTypeOptionButton->add_item(TheWorld_Utils::TerrainEdit::terrainTypeString(TheWorld_Utils::TerrainEdit::TerrainType::high_mountains_2_grow).c_str(), (int64_t)TheWorld_Utils::TerrainEdit::TerrainType::high_mountains_2_grow);
					m_terrTypeOptionButton->add_separator();
					m_terrTypeOptionButton->add_item(TheWorld_Utils::TerrainEdit::terrainTypeString(TheWorld_Utils::TerrainEdit::TerrainType::noise_1).c_str(), (int64_t)TheWorld_Utils::TerrainEdit::TerrainType::noise_1);
					separator = VSeparator::_new();
					hBoxContainer->add_child(separator);
					separator->connect("mouse_entered", this, "mouse_entered_main_panel");
					separator->connect("mouse_exited", this, "mouse_exited_main_panel");
					button = godot::Button::_new();
					hBoxContainer->add_child(button);
					button->set_text("Generate");
					button->connect("pressed", this, "edit_mode_generate");
					button->connect("mouse_entered", this, "mouse_entered_main_panel");
					button->connect("mouse_exited", this, "mouse_exited_main_panel");
					button->set_focus_mode(godot::Control::FocusMode::FOCUS_NONE);

				hBoxContainer = godot::HBoxContainer::_new();
				m_noiseVBoxContainer->add_child(hBoxContainer);
					label = godot::Label::_new();
					hBoxContainer->add_child(label);
					label->set_text("Seed");
					label->set_align(godot::Label::Align::ALIGN_LEFT);
					label->connect("mouse_entered", this, "mouse_entered_main_panel");
					label->connect("mouse_exited", this, "mouse_exited_main_panel");
					m_seed = godot::LineEdit::_new();
					hBoxContainer->add_child(m_seed);
					m_seed->set_align(godot::Label::Align::ALIGN_RIGHT);
					m_seed->connect("mouse_entered", this, "mouse_entered_main_panel");
					m_seed->connect("mouse_exited", this, "mouse_exited_main_panel");
					label = godot::Label::_new();
					hBoxContainer->add_child(label);
					label->set_text("Frequency");
					label->set_align(godot::Label::Align::ALIGN_LEFT);
					label->connect("mouse_entered", this, "mouse_entered_main_panel");
					label->connect("mouse_exited", this, "mouse_exited_main_panel");
					m_frequency = godot::LineEdit::_new();
					hBoxContainer->add_child(m_frequency);
					m_frequency->set_align(godot::Label::Align::ALIGN_RIGHT);
					m_frequency->connect("mouse_entered", this, "mouse_entered_main_panel");
					m_frequency->connect("mouse_exited", this, "mouse_exited_main_panel");
					label = godot::Label::_new();
					hBoxContainer->add_child(label);
					label->set_text("Gain");
					label->set_align(godot::Label::Align::ALIGN_LEFT);
					label->connect("mouse_entered", this, "mouse_entered_main_panel");
					label->connect("mouse_exited", this, "mouse_exited_main_panel");
					m_fractalGain = godot::LineEdit::_new();
					hBoxContainer->add_child(m_fractalGain);
					m_fractalGain->set_align(godot::Label::Align::ALIGN_RIGHT);
					m_fractalGain->connect("mouse_entered", this, "mouse_entered_main_panel");
					m_fractalGain->connect("mouse_exited", this, "mouse_exited_main_panel");

				hBoxContainer = godot::HBoxContainer::_new();
				m_noiseVBoxContainer->add_child(hBoxContainer);
					label = godot::Label::_new();
					hBoxContainer->add_child(label);
					label->set_text("Octaves");
					label->set_align(godot::Label::Align::ALIGN_LEFT);
					label->connect("mouse_entered", this, "mouse_entered_main_panel");
					label->connect("mouse_exited", this, "mouse_exited_main_panel");
					m_fractalOctaves = godot::LineEdit::_new();
					hBoxContainer->add_child(m_fractalOctaves);
					m_fractalOctaves->set_align(godot::Label::Align::ALIGN_RIGHT);
					m_fractalOctaves->connect("mouse_entered", this, "mouse_entered_main_panel");
					m_fractalOctaves->connect("mouse_exited", this, "mouse_exited_main_panel");
					label = godot::Label::_new();
					hBoxContainer->add_child(label);
					label->set_text("Lacunarity");
					label->set_align(godot::Label::Align::ALIGN_LEFT);
					label->connect("mouse_entered", this, "mouse_entered_main_panel");
					label->connect("mouse_exited", this, "mouse_exited_main_panel");
					m_fractalLacunarity = godot::LineEdit::_new();
					hBoxContainer->add_child(m_fractalLacunarity);
					m_fractalLacunarity->set_align(godot::Label::Align::ALIGN_RIGHT);
					m_fractalLacunarity->connect("mouse_entered", this, "mouse_entered_main_panel");
					m_fractalLacunarity->connect("mouse_exited", this, "mouse_exited_main_panel");

				hBoxContainer = godot::HBoxContainer::_new();
				m_noiseVBoxContainer->add_child(hBoxContainer);
					label = godot::Label::_new();
					hBoxContainer->add_child(label);
					label->set_text("Amplitude");
					label->set_align(godot::Label::Align::ALIGN_LEFT);
					label->connect("mouse_entered", this, "mouse_entered_main_panel");
					label->connect("mouse_exited", this, "mouse_exited_main_panel");
					m_amplitudeLabel = godot::LineEdit::_new();
					hBoxContainer->add_child(m_amplitudeLabel);
					m_amplitudeLabel->set_align(godot::Label::Align::ALIGN_RIGHT);
					m_amplitudeLabel->connect("mouse_entered", this, "mouse_entered_main_panel");
					m_amplitudeLabel->connect("mouse_exited", this, "mouse_exited_main_panel");
					label = godot::Label::_new();
					hBoxContainer->add_child(label);
					label->set_text("Scale");
					label->set_align(godot::Label::Align::ALIGN_LEFT);
					label->connect("mouse_entered", this, "mouse_entered_main_panel");
					label->connect("mouse_exited", this, "mouse_exited_main_panel");
					m_scaleFactorLabel = godot::LineEdit::_new();
					hBoxContainer->add_child(m_scaleFactorLabel);
					m_scaleFactorLabel->set_align(godot::Label::Align::ALIGN_RIGHT);
					m_scaleFactorLabel->connect("mouse_entered", this, "mouse_entered_main_panel");
					m_scaleFactorLabel->connect("mouse_exited", this, "mouse_exited_main_panel");
					label = godot::Label::_new();
					hBoxContainer->add_child(label);
					label->set_text("Start H");
					label->set_align(godot::Label::Align::ALIGN_LEFT);
					label->connect("mouse_entered", this, "mouse_entered_main_panel");
					label->connect("mouse_exited", this, "mouse_exited_main_panel");
					m_desideredMinHeightLabel = godot::LineEdit::_new();
					hBoxContainer->add_child(m_desideredMinHeightLabel);
					m_desideredMinHeightLabel->set_align(godot::Label::Align::ALIGN_RIGHT);
					m_desideredMinHeightLabel->connect("mouse_entered", this, "mouse_entered_main_panel");
					m_desideredMinHeightLabel->connect("mouse_exited", this, "mouse_exited_main_panel");

				hBoxContainer = godot::HBoxContainer::_new();
				m_noiseVBoxContainer->add_child(hBoxContainer);
					label = godot::Label::_new();
					hBoxContainer->add_child(label);
					label->set_text("Weighted Strength");
					label->set_align(godot::Label::Align::ALIGN_LEFT);
					label->connect("mouse_entered", this, "mouse_entered_main_panel");
					label->connect("mouse_exited", this, "mouse_exited_main_panel");
					m_fractalWeightedStrength = godot::LineEdit::_new();
					hBoxContainer->add_child(m_fractalWeightedStrength);
					m_fractalWeightedStrength->set_align(godot::Label::Align::ALIGN_RIGHT);
					m_fractalWeightedStrength->connect("mouse_entered", this, "mouse_entered_main_panel");
					m_fractalWeightedStrength->connect("mouse_exited", this, "mouse_exited_main_panel");

				hBoxContainer = godot::HBoxContainer::_new();
				m_noiseVBoxContainer->add_child(hBoxContainer);
					label = godot::Label::_new();
					hBoxContainer->add_child(label);
					label->set_text("Ping Pong Strength");
					label->set_align(godot::Label::Align::ALIGN_LEFT);
					label->connect("mouse_entered", this, "mouse_entered_main_panel");
					label->connect("mouse_exited", this, "mouse_exited_main_panel");
					m_fractalPingPongStrength = godot::LineEdit::_new();
					hBoxContainer->add_child(m_fractalPingPongStrength);
					m_fractalPingPongStrength->set_align(godot::Label::Align::ALIGN_RIGHT);
					m_fractalPingPongStrength->connect("mouse_entered", this, "mouse_entered_main_panel");
					m_fractalPingPongStrength->connect("mouse_exited", this, "mouse_exited_main_panel");

			separator = HSeparator::_new();
			mainVBoxContainer->add_child(separator);
			separator->connect("mouse_entered", this, "mouse_entered_main_panel");
			separator->connect("mouse_exited", this, "mouse_exited_main_panel");

			hBoxContainer = godot::HBoxContainer::_new();
			mainVBoxContainer->add_child(hBoxContainer);
				label = godot::Label::_new();
				hBoxContainer->add_child(label);
				label->set_text("Min");
				label->set_align(godot::Label::Align::ALIGN_LEFT);
				label->connect("mouse_entered", this, "mouse_entered_main_panel");
				label->connect("mouse_exited", this, "mouse_exited_main_panel");
				m_minHeightLabel = godot::Label::_new();
				hBoxContainer->add_child(m_minHeightLabel);
				m_minHeightLabel->set_align(godot::Label::Align::ALIGN_LEFT);
				m_minHeightLabel->connect("mouse_entered", this, "mouse_entered_main_panel");
				m_minHeightLabel->connect("mouse_exited", this, "mouse_exited_main_panel");
				label = godot::Label::_new();
				hBoxContainer->add_child(label);
				label->set_text("Max");
				label->set_align(godot::Label::Align::ALIGN_LEFT);
				label->connect("mouse_entered", this, "mouse_entered_main_panel");
				label->connect("mouse_exited", this, "mouse_exited_main_panel");
				m_maxHeightLabel = godot::Label::_new();
				hBoxContainer->add_child(m_maxHeightLabel);
				m_maxHeightLabel->set_align(godot::Label::Align::ALIGN_LEFT);
				m_maxHeightLabel->connect("mouse_entered", this, "mouse_entered_main_panel");
				m_maxHeightLabel->connect("mouse_exited", this, "mouse_exited_main_panel");

			hBoxContainer = godot::HBoxContainer::_new();
			mainVBoxContainer->add_child(hBoxContainer);
				label = godot::Label::_new();
				hBoxContainer->add_child(label);
				label->set_text("Elapsed");
				label->set_align(godot::Label::Align::ALIGN_LEFT);
				label->connect("mouse_entered", this, "mouse_entered_main_panel");
				label->connect("mouse_exited", this, "mouse_exited_main_panel");
				m_elapsedLabel = godot::Label::_new();
				hBoxContainer->add_child(m_elapsedLabel);
				m_elapsedLabel->set_align(godot::Label::Align::ALIGN_LEFT);
				m_elapsedLabel->connect("mouse_entered", this, "mouse_entered_main_panel");
				m_elapsedLabel->connect("mouse_exited", this, "mouse_exited_main_panel");
				m_counterLabel = godot::Label::_new();
				hBoxContainer->add_child(m_counterLabel);
				m_counterLabel->set_align(godot::Label::Align::ALIGN_LEFT);
				m_counterLabel->connect("mouse_entered", this, "mouse_entered_main_panel");
				m_counterLabel->connect("mouse_exited", this, "mouse_exited_main_panel");
				m_note1Label = godot::Label::_new();
				hBoxContainer->add_child(m_note1Label);
				m_note1Label->set_align(godot::Label::Align::ALIGN_LEFT);
				m_note1Label->connect("mouse_entered", this, "mouse_entered_main_panel");
				m_note1Label->connect("mouse_exited", this, "mouse_exited_main_panel");

			separator = HSeparator::_new();
			mainVBoxContainer->add_child(separator);
			separator->connect("mouse_entered", this, "mouse_entered_main_panel");
			separator->connect("mouse_exited", this, "mouse_exited_main_panel");

			hBoxContainer = godot::HBoxContainer::_new();
			mainVBoxContainer->add_child(hBoxContainer);
				label = godot::Label::_new();
				hBoxContainer->add_child(label);
				label->set_text("Hit");
				label->set_align(godot::Label::Align::ALIGN_LEFT);
				label->connect("mouse_entered", this, "mouse_entered_main_panel");
				label->connect("mouse_exited", this, "mouse_exited_main_panel");
				m_mouseHitLabel = godot::Label::_new();
				hBoxContainer->add_child(m_mouseHitLabel);
				m_mouseHitLabel->set_align(godot::Label::Align::ALIGN_RIGHT);
				m_mouseHitLabel->connect("mouse_entered", this, "mouse_entered_main_panel");
				m_mouseHitLabel->connect("mouse_exited", this, "mouse_exited_main_panel");

			hBoxContainer = godot::HBoxContainer::_new();
			mainVBoxContainer->add_child(hBoxContainer);
				label = godot::Label::_new();
				hBoxContainer->add_child(label);
				label->set_text("Quad Hit");
				label->set_align(godot::Label::Align::ALIGN_LEFT);
				label->connect("mouse_entered", this, "mouse_entered_main_panel");
				label->connect("mouse_exited", this, "mouse_exited_main_panel");
				m_mouseQuadHitLabel = godot::Label::_new();
				hBoxContainer->add_child(m_mouseQuadHitLabel);
				m_mouseQuadHitLabel->set_align(godot::Label::Align::ALIGN_RIGHT);
				m_mouseQuadHitLabel->connect("mouse_entered", this, "mouse_entered_main_panel");
				m_mouseQuadHitLabel->connect("mouse_exited", this, "mouse_exited_main_panel");

			hBoxContainer = godot::HBoxContainer::_new();
			mainVBoxContainer->add_child(hBoxContainer);
				label = godot::Label::_new();
				hBoxContainer->add_child(label);
				label->set_text("Pos Hit");
				label->set_align(godot::Label::Align::ALIGN_LEFT);
				label->connect("mouse_entered", this, "mouse_entered_main_panel");
				label->connect("mouse_exited", this, "mouse_exited_main_panel");
				m_mouseQuadHitPosLabel = godot::Label::_new();
				hBoxContainer->add_child(m_mouseQuadHitPosLabel);
				m_mouseQuadHitPosLabel->set_align(godot::Label::Align::ALIGN_RIGHT);
				m_mouseQuadHitPosLabel->connect("mouse_entered", this, "mouse_entered_main_panel");
				m_mouseQuadHitPosLabel->connect("mouse_exited", this, "mouse_exited_main_panel");

			hBoxContainer = godot::HBoxContainer::_new();
			mainVBoxContainer->add_child(hBoxContainer);
				label = godot::Label::_new();
				hBoxContainer->add_child(label);
				label->set_text("Quad Sel");
				label->set_align(godot::Label::Align::ALIGN_LEFT);
				label->connect("mouse_entered", this, "mouse_entered_main_panel");
				label->connect("mouse_exited", this, "mouse_exited_main_panel");
				m_mouseQuadSelLabel = godot::Label::_new();
				hBoxContainer->add_child(m_mouseQuadSelLabel);
				m_mouseQuadSelLabel->set_align(godot::Label::Align::ALIGN_RIGHT);
				m_mouseQuadSelLabel->connect("mouse_entered", this, "mouse_entered_main_panel");
				m_mouseQuadSelLabel->connect("mouse_exited", this, "mouse_exited_main_panel");

			hBoxContainer = godot::HBoxContainer::_new();
			mainVBoxContainer->add_child(hBoxContainer);
				label = godot::Label::_new();
				hBoxContainer->add_child(label);
				label->set_text("Pos Sel");
				label->set_align(godot::Label::Align::ALIGN_LEFT);
				label->connect("mouse_entered", this, "mouse_entered_main_panel");
				label->connect("mouse_exited", this, "mouse_exited_main_panel");
				m_mouseQuadSelPosLabel = godot::Label::_new();
				hBoxContainer->add_child(m_mouseQuadSelPosLabel);
				m_mouseQuadSelPosLabel->set_align(godot::Label::Align::ALIGN_RIGHT);
				m_mouseQuadSelPosLabel->connect("mouse_entered", this, "mouse_entered_main_panel");
				m_mouseQuadSelPosLabel->connect("mouse_exited", this, "mouse_exited_main_panel");

			separator = HSeparator::_new();
			mainVBoxContainer->add_child(separator);
			separator->connect("mouse_entered", this, "mouse_entered_main_panel");
			separator->connect("mouse_exited", this, "mouse_exited_main_panel");

			hBoxContainer = godot::HBoxContainer::_new();
			mainVBoxContainer->add_child(hBoxContainer);
				label = godot::Label::_new();
				hBoxContainer->add_child(label);
				label->set_text("Quads to save");
				label->set_align(godot::Label::Align::ALIGN_LEFT);
				label->connect("mouse_entered", this, "mouse_entered_main_panel");
				label->connect("mouse_exited", this, "mouse_exited_main_panel");
				m_numQuadrantToSaveLabel = godot::Label::_new();
				hBoxContainer->add_child(m_numQuadrantToSaveLabel);
				m_numQuadrantToSaveLabel->set_align(godot::Label::Align::ALIGN_RIGHT);
				m_numQuadrantToSaveLabel->connect("mouse_entered", this, "mouse_entered_main_panel");
				m_numQuadrantToSaveLabel->connect("mouse_exited", this, "mouse_exited_main_panel");
				label = godot::Label::_new();
				hBoxContainer->add_child(label);
				label->set_text("Quads to upload");
				label->set_align(godot::Label::Align::ALIGN_LEFT);
				label->connect("mouse_entered", this, "mouse_entered_main_panel");
				label->connect("mouse_exited", this, "mouse_exited_main_panel");
				m_numQuadrantToUploadLabel = godot::Label::_new();
				hBoxContainer->add_child(m_numQuadrantToUploadLabel);
				m_numQuadrantToUploadLabel->set_align(godot::Label::Align::ALIGN_RIGHT);
				m_numQuadrantToUploadLabel->connect("mouse_entered", this, "mouse_entered_main_panel");
				m_numQuadrantToUploadLabel->connect("mouse_exited", this, "mouse_exited_main_panel");

			separator = HSeparator::_new();
			mainVBoxContainer->add_child(separator);
			separator->connect("mouse_entered", this, "mouse_entered_main_panel");
			separator->connect("mouse_exited", this, "mouse_exited_main_panel");

			hBoxContainer = godot::HBoxContainer::_new();
			mainVBoxContainer->add_child(hBoxContainer);
				button = godot::Button::_new();
				hBoxContainer->add_child(button);
				button->set_text("Save");
				button->connect("pressed", this, "edit_mode_save");
				button->connect("mouse_entered", this, "mouse_entered_main_panel");
				button->connect("mouse_exited", this, "mouse_exited_main_panel");
				button->set_focus_mode(godot::Control::FocusMode::FOCUS_NONE);
				separator = VSeparator::_new();
				hBoxContainer->add_child(separator);
				separator->connect("mouse_entered", this, "mouse_entered_main_panel");
				separator->connect("mouse_exited", this, "mouse_exited_main_panel");
				button = godot::Button::_new();
				hBoxContainer->add_child(button);
				button->set_text("Upload");
				button->connect("pressed", this, "edit_mode_upload");
				button->connect("mouse_entered", this, "mouse_entered_main_panel");
				button->connect("mouse_exited", this, "mouse_exited_main_panel");
				button->set_focus_mode(godot::Control::FocusMode::FOCUS_NONE);
				separator = VSeparator::_new();
				hBoxContainer->add_child(separator);
				separator->connect("mouse_entered", this, "mouse_entered_main_panel");
				separator->connect("mouse_exited", this, "mouse_exited_main_panel");
				button = godot::Button::_new();
				hBoxContainer->add_child(button);
				button->set_text("Stop");
				button->connect("pressed", this, "edit_mode_stop");
				button->connect("mouse_entered", this, "mouse_entered_main_panel");
				button->connect("mouse_exited", this, "mouse_exited_main_panel");
				button->set_focus_mode(godot::Control::FocusMode::FOCUS_NONE);

			separator = HSeparator::_new();
			mainVBoxContainer->add_child(separator);
			separator->connect("mouse_entered", this, "mouse_entered_main_panel");
			separator->connect("mouse_exited", this, "mouse_exited_main_panel");

			size_t numToSave = 0;
			size_t numToUpload = 0;
			refreshNumToSaveUpload(numToSave, numToUpload);

			m_tp.Start("EditModeWorker", 1, this);
}

void GDN_TheWorld_Edit::deinit(void)
{
	if (m_initialized)
	{
		m_tp.Stop();
		m_initialized = false;
	}
}

void GDN_TheWorld_Edit::resizeUI(void)
{
	set_anchor(GDN_TheWorld_Edit::Margin::MARGIN_RIGHT, 1.0);
	set_margin(GDN_TheWorld_Edit::Margin::MARGIN_RIGHT, 0.0);
	set_margin(GDN_TheWorld_Edit::Margin::MARGIN_TOP, 0.0);
	set_margin(GDN_TheWorld_Edit::Margin::MARGIN_LEFT, get_viewport()->get_size().x - 350);
	set_margin(GDN_TheWorld_Edit::Margin::MARGIN_BOTTOM, 0.0);
}

void GDN_TheWorld_Edit::setEmptyTerrainEditValues(void)
{
	m_terrTypeOptionButton->select(m_terrTypeOptionButton->get_item_index((int64_t)TheWorld_Utils::TerrainEdit::TerrainType::unknown));

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
}

void GDN_TheWorld_Edit::setTerrainEditValues(TheWorld_Utils::TerrainEdit& terrainEdit)
{
	int64_t id = m_terrTypeOptionButton->get_selected_id();
	if (id != -1)
	{
		TheWorld_Utils::TerrainEdit::TerrainType selectedTerrainType = (TheWorld_Utils::TerrainEdit::TerrainType)id;
		if (selectedTerrainType != terrainEdit.terrainType)
		{
			//std::string text = TheWorld_Utils::TerrainEdit::terrainTypeString(terrainEdit.terrainType);
			//m_terrTypeOptionButton->set_text(text.c_str());
			m_terrTypeOptionButton->select(m_terrTypeOptionButton->get_item_index((int64_t)terrainEdit.terrainType));
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
}

void GDN_TheWorld_Edit::setSeed(int seed)
{
	m_seed->set_text(std::to_string(seed).c_str());
}

int GDN_TheWorld_Edit::seed(void)
{
	godot::String s = m_seed->get_text();
	char* str = s.alloc_c_string();
	int ret = std::stoi(std::string(str));
	godot::api->godot_free(str);
	return ret;
}

void GDN_TheWorld_Edit::setFrequency(float frequency)
{
	m_frequency->set_text(std::to_string(frequency).c_str());
}

float GDN_TheWorld_Edit::frequency(void)
{
	godot::String s = m_frequency->get_text();
	char* str = s.alloc_c_string();
	float ret = std::stof(std::string(str));
	godot::api->godot_free(str);
	return ret;
}

void GDN_TheWorld_Edit::setOctaves(int octaves)
{
	m_fractalOctaves->set_text(std::to_string(octaves).c_str());
}

int GDN_TheWorld_Edit::octaves(void)
{
	godot::String s = m_fractalOctaves->get_text();
	char* str = s.alloc_c_string();
	int ret = std::stoi(std::string(str));
	godot::api->godot_free(str);
	return ret;
}

void GDN_TheWorld_Edit::setLacunarity(float lacunarity)
{
	m_fractalLacunarity->set_text(std::to_string(lacunarity).c_str());
}

float GDN_TheWorld_Edit::lacunarity(void)
{
	godot::String s = m_fractalLacunarity->get_text();
	char* str = s.alloc_c_string();
	float ret = std::stof(std::string(str));
	godot::api->godot_free(str);
	return ret;
}

void GDN_TheWorld_Edit::setGain(float gain)
{
	m_fractalGain->set_text(std::to_string(gain).c_str());
}

float GDN_TheWorld_Edit::gain(void)
{
	godot::String s = m_fractalGain->get_text();
	char* str = s.alloc_c_string();
	float ret = std::stof(std::string(str));
	godot::api->godot_free(str);
	return ret;
}

void GDN_TheWorld_Edit::setWeightedStrength(float weightedStrength)
{
	m_fractalWeightedStrength->set_text(std::to_string(weightedStrength).c_str());
}

float GDN_TheWorld_Edit::weightedStrength(void)
{
	godot::String s = m_fractalWeightedStrength->get_text();
	char* str = s.alloc_c_string();
	float ret = std::stof(std::string(str));
	godot::api->godot_free(str);
	return ret;
}

void GDN_TheWorld_Edit::setPingPongStrength(float pingPongStrength)
{
	m_fractalPingPongStrength->set_text(std::to_string(pingPongStrength).c_str());
}

float GDN_TheWorld_Edit::pingPongStrength(void)
{
	godot::String s = m_fractalPingPongStrength->get_text();
	char* str = s.alloc_c_string();
	float ret = std::stof(std::string(str));
	godot::api->godot_free(str);
	return ret;
}

void GDN_TheWorld_Edit::setAmplitude(unsigned int amplitude)
{
	m_amplitudeLabel->set_text(std::to_string(amplitude).c_str());
}

unsigned int GDN_TheWorld_Edit::amplitude(void)
{
	godot::String s = m_amplitudeLabel->get_text();
	char* str = s.alloc_c_string();
	unsigned int ret = std::stoi(std::string(str));
	godot::api->godot_free(str);
	return ret;
}

void GDN_TheWorld_Edit::setScaleFactor(float scaleFactor)
{
	m_scaleFactorLabel->set_text(std::to_string(scaleFactor).c_str());
}

float GDN_TheWorld_Edit::scaleFactor(void)
{
	godot::String s = m_scaleFactorLabel->get_text();
	char* str = s.alloc_c_string();
	float ret = std::stof(std::string(str));
	godot::api->godot_free(str);
	return ret;
}

void GDN_TheWorld_Edit::setDesideredMinHeight(float desideredMinHeight)
{
	m_desideredMinHeightLabel->set_text(std::to_string(desideredMinHeight).c_str());
}

float GDN_TheWorld_Edit::desideredMinHeight(void)
{
	godot::String s = m_desideredMinHeightLabel->get_text();
	char* str = s.alloc_c_string();
	float ret = std::stof(std::string(str));
	godot::api->godot_free(str);
	return ret;
}

void GDN_TheWorld_Edit::setMinHeight(float minHeight)
{
	m_minHeightLabel->set_text(std::to_string(minHeight).c_str());
}

float GDN_TheWorld_Edit::minHeight(void)
{
	godot::String s = m_minHeightLabel->get_text();
	char* str = s.alloc_c_string();
	float ret = std::stof(std::string(str));
	godot::api->godot_free(str);
	return ret;
}

void GDN_TheWorld_Edit::setMaxHeight(float maxHeight)
{
	m_maxHeightLabel->set_text(std::to_string(maxHeight).c_str());
}

float GDN_TheWorld_Edit::maxHeight(void)
{
	godot::String s = m_maxHeightLabel->get_text();
	char* str = s.alloc_c_string();
	float ret = std::stof(std::string(str));
	godot::api->godot_free(str);
	return ret;
}

void GDN_TheWorld_Edit::setElapsed(size_t elapsed, bool onGoing)
{
	std::lock_guard<std::recursive_mutex> lock(m_mtxUI);

	if (onGoing)
	{
		if (!m_onGoingElapsedLabel)
		{
			//m_elapsedLabelNormalColor = m_elapsedLabel->get("custom_colors/font_color");
			m_elapsedLabel->set("custom_colors/font_color", Color(1, 0, 0));
			m_onGoingElapsedLabel = true;
		}
	}
	else
	{
		if (m_onGoingElapsedLabel)
		{
			m_elapsedLabel->set("custom_colors/font_color", Color(1, 1, 1));
			m_onGoingElapsedLabel = false;
		}
	}
	
	if (elapsed == 0)
		m_elapsedLabel->set_text("");
	else
		m_elapsedLabel->set_text(std::to_string(elapsed).c_str());
}

void GDN_TheWorld_Edit::setCounter(size_t current, size_t all)
{
	m_counterLabel->set_text((std::to_string(current) + "/" + std::to_string(all)).c_str());
}

void GDN_TheWorld_Edit::setNote1(size_t num)
{
	if (num == 0)
		m_note1Label->set_text("");
	else
		m_note1Label->set_text(std::to_string(num).c_str());
}

void GDN_TheWorld_Edit::setNote1(std::string msg)
{
	m_note1Label->set_text(msg.c_str());
}

size_t GDN_TheWorld_Edit::elapsed(void)
{
	godot::String s = m_elapsedLabel->get_text();
	char* str = s.alloc_c_string();
	size_t ret = std::stoll(std::string(str));
	godot::api->godot_free(str);
	return ret;
}

void GDN_TheWorld_Edit::setMouseHitLabelText(std::string text)
{
	m_mouseHitLabel->set_text(text.c_str());
}

void GDN_TheWorld_Edit::setMouseQuadHitLabelText(std::string text)
{
	m_mouseQuadHitLabel->set_text(text.c_str());
}

void GDN_TheWorld_Edit::setMouseQuadHitPosLabelText(std::string text)
{
	m_mouseQuadHitPosLabel->set_text(text.c_str());
}

void GDN_TheWorld_Edit::setMouseQuadSelLabelText(std::string text)
{
	m_mouseQuadSelLabel->set_text(text.c_str());
}

void GDN_TheWorld_Edit::setMouseQuadSelPosLabelText(std::string text)
{
	m_mouseQuadSelPosLabel->set_text(text.c_str());
}

void GDN_TheWorld_Edit::setNumQuadrantToSave(size_t num)
{
	m_numQuadrantToSaveLabel->set_text(std::to_string(num).c_str());
}

void GDN_TheWorld_Edit::setNumQuadrantToUpload(size_t num)
{
	m_numQuadrantToUploadLabel->set_text(std::to_string(num).c_str());
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
	//Godot::print("GDN_Template::Init");
}

void GDN_TheWorld_Edit::_ready(void)
{
	//Godot::print("GDN_Template::_ready");
	//get_node(NodePath("/root/Main/Reset"))->connect("pressed", this, "on_Reset_pressed");
}

void GDN_TheWorld_Edit::_input(const Ref<InputEvent> event)
{
}

void GDN_TheWorld_Edit::_notification(int p_what)
{
	switch (p_what)
	{
	case NOTIFICATION_RESIZED:
	{
		resizeUI();
	}
	break;
	}
}

void GDN_TheWorld_Edit::_process(float _delta)
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
}

void GDN_TheWorld_Edit::editModeNoisePanel(void)
{
	if (m_noiseVBoxContainer->is_visible())
	{
		m_noiseVBoxContainer->hide();
		m_noiseButton->set_text("> Noise");
		//m_noiseContainerShowing = false;
	}
	else
	{
		m_noiseVBoxContainer->show();
		m_noiseButton->set_text("- Noise");
		//m_noiseContainerShowing = true;
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
			QuadrantPos quadrantPos = item.first;
			TheWorld_Utils::MemoryBuffer buffer;
			cache.setBufferFromCacheQuadrantData(quadrantPos.getNumVerticesPerSize(), cacheQuadrantData, buffer);
			cache.writeBufferToCache(buffer);

			m_completedItems++;
			size_t partialCount = m_actionClock.partialDuration().count();
			m_lastElapsed = partialCount - m_elapsedCompleted;
			m_elapsedCompleted = partialCount;
			m_lastElapsed = m_actionClock.partialDuration().count() - m_lastElapsed;
		}
	}

	m_mapQuadToSave.clear();

	m_actionClock.tock();

	size_t duration = m_actionClock.duration().count();
	setElapsed(duration, false);
	setCounter(m_completedItems, m_allItems);
	setNote1(m_lastElapsed);

	size_t numToSave = 0;
	size_t numToUpload = 0;
	refreshNumToSaveUpload(numToSave, numToUpload);

	m_actionInProgress = false;
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
			std::string buffer;
			cache.setBufferFromCacheQuadrantData(pos.getNumVerticesPerSize(), cacheQuadrantData, buffer);
			
			float lowerXGridVertex = pos.getLowerXGridVertex();
			float lowerZGridVertex = pos.getLowerZGridVertex();
			int numVerticesPerSize = pos.getNumVerticesPerSize();
			float gridStepinWU = pos.getGridStepInWU();
			int lvl = pos.getLevel();
			quadTree->Viewer()->Globals()->Client()->MapManagerUploadBuffer(lowerXGridVertex, lowerZGridVertex, numVerticesPerSize, gridStepinWU, lvl, buffer);

			cache.writeBufferToCache(buffer);

			//quadTree->getQuadrant()->setNeedUploadToServer(false);
		}
	}
	
	while (!m_actionStopRequested && m_allItems > m_completedItems)
		Sleep(10);
	
	m_actionClock.tock();

	size_t duration = m_actionClock.duration().count();
	setElapsed(duration, false);
	setCounter(m_completedItems, m_allItems);
	setNote1(m_lastElapsed);

	size_t numToSave = 0;
	size_t numToUpload = 0;
	refreshNumToSaveUpload(numToSave, numToUpload);

	m_actionInProgress = false;
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

	m_actionClock.tick();

	QuadTree* quadTreeSel = nullptr;
	QuadrantPos quadrantSelPos = m_viewer->getQuadrantSelForEdit(&quadTreeSel);

	if (quadrantSelPos.empty())
	{
		m_actionClock.tock();
		m_actionInProgress = false;
		return;
	}

	m_completedItems = 0;
	m_elapsedCompleted = 0;
	m_lastElapsed = 0;
	m_allItems = 1;

	TheWorld_Utils::GuardProfiler profiler(std::string("EditGenerate 1 ") + __FUNCTION__, "ALL");

	int64_t id = m_terrTypeOptionButton->get_selected_id();
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

	// 5. invalidata normals as heigths have been changed and set flags according (normals need regen and empty normals have to be passed to shader)
	quadTreeSel->getQuadrant()->getNormalsBuffer(false).clear();
	terrainEdit->normalsNeedRegen = true;
	quadTreeSel->getQuadrant()->setNormalsUpdated(true);

	// 6. prepare new heigths for collider (PoolRealArray)
	{
		PoolRealArray& heightsForCollider = quadTreeSel->getQuadrant()->getHeightsForCollider();
		heightsForCollider.resize((int)numVertices);
		godot::PoolRealArray::Write w = heightsForCollider.write();
		memcpy((char*)w.ptr(), heights32Buffer.ptr(), heights32Buffer.size());
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

	size_t duration = m_actionClock.duration().count();
	setElapsed(duration, false);
	setCounter(m_completedItems, m_allItems);
	setNote1(m_lastElapsed);

	size_t numToSave = 0;
	size_t numToUpload = 0;
	refreshNumToSaveUpload(numToSave, numToUpload);

	m_actionInProgress = false;
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

	m_actionClock.tick();

	TheWorld_Utils::GuardProfiler profiler(std::string("EditBlend 1 ") + __FUNCTION__, "ALL");

	bool genAllNormals = m_genAllNormals->is_pressed();

	QuadTree* quadTreeSel = nullptr;
	QuadrantPos quadrantSelPos = m_viewer->getQuadrantSelForEdit(&quadTreeSel);

	if (!genAllNormals && quadrantSelPos.empty())
	{
		m_actionClock.tock();
		m_actionInProgress = false;
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
			if (genAllNormals || pos == quadrantSelPos)
			{
				quandrantPos.push_back(pos);

				if (!genAllNormals)
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
				}
				else
					southeastQuadTree = nullptr;

				bool lastPhase = false;
				if (round == numRounds - 1)
					lastPhase = true;
				;
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

	size_t duration = m_actionClock.duration().count();
	setElapsed(duration, false);
	setCounter(m_completedItems, m_allItems);
	setNote1(m_lastElapsed);

	size_t numToSave = 0;
	size_t numToUpload = 0;
	refreshNumToSaveUpload(numToSave, numToUpload);

	m_actionInProgress = false;
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

		quadTree->getQuadrant()->getNormalsBuffer(false).clear();
		quadTree->getQuadrant()->getTerrainEdit()->normalsNeedRegen = true;
		quadTree->getQuadrant()->setNormalsUpdated(true);

		{
			PoolRealArray& heightsForCollider = quadTree->getQuadrant()->getHeightsForCollider(false);
			heightsForCollider.resize((int)(quadTree->getQuadrant()->getPos().getNumVerticesPerSize() * quadTree->getQuadrant()->getPos().getNumVerticesPerSize()));
			godot::PoolRealArray::Write w = heightsForCollider.write();
			memcpy((char*)w.ptr(), quadTree->getQuadrant()->getFloat32HeightsBuffer().ptr(), quadTree->getQuadrant()->getFloat32HeightsBuffer().size());
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
	if (m_actionClock.counterStarted())
	{
		m_actionInProgress = false;
		return;
	}

	TheWorld_Utils::GuardProfiler profiler(std::string("EditGenMesh 1 ") + __FUNCTION__, "ALL");

	m_actionClock.tick();

	QuadTree* quadTreeSel = nullptr;
	QuadrantPos quadrantSelPos = m_viewer->getQuadrantSelForEdit(&quadTreeSel);

	bool genAllNormals = m_genAllNormals->is_pressed();

	if (!genAllNormals && quadrantSelPos.empty())
	{
		m_actionClock.tock();
		m_actionInProgress = false;
		return;
	}

	if (quadTreeSel != nullptr)
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
				if (genAllNormals || pos == quadrantSelPos)
				{
					quandrantPos.push_back(pos);
					quadTree->getQuadrant()->setNeedUploadToServer(true);

					if (!genAllNormals)
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
				TheWorld_Utils::GuardProfiler profiler(std::string("EditGenMesh 1.1 ") + __FUNCTION__, "Single QuadTree");

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

	size_t duration = m_actionClock.duration().count();
	setElapsed(duration, false);
	setCounter(m_completedItems, m_allItems);
	setNote1(m_lastElapsed);

	size_t numToSave = 0;
	size_t numToUpload = 0;
	refreshNumToSaveUpload(numToSave, numToUpload);

	m_actionInProgress = false;
}

void GDN_TheWorld_Edit::editModeSelectTerrainTypeAction(int64_t index)
{
	enum class TheWorld_Utils::TerrainEdit::TerrainType terrainType = (enum class TheWorld_Utils::TerrainEdit::TerrainType)m_terrTypeOptionButton->get_item_id(index);
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