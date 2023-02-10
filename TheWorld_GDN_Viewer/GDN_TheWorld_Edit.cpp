//#include "pch.h"
#include <Godot.hpp>
#include <Node.hpp>
#include <MarginContainer.hpp>
#include <TabContainer.hpp>
#include <PanelContainer.hpp>
#include <VBoxContainer.hpp>
#include <HBoxContainer.hpp>
#include <Button.hpp>
#include <Label.hpp>
#include <HSeparator.hpp>
#include <VSeparator.hpp>

#include <string>
#include <iostream>

#include "GDN_TheWorld_Viewer.h"
#include "GDN_TheWorld_Edit.h"
#include "FastNoiseLite.h"
#include "half.h"

using namespace godot;

void GDN_TheWorld_Edit::_register_methods()
{
	register_method("_ready", &GDN_TheWorld_Edit::_ready);
	register_method("_process", &GDN_TheWorld_Edit::_process);
	register_method("_input", &GDN_TheWorld_Edit::_input);
	register_method("_notification", &GDN_TheWorld_Edit::_notification);

	//register_method("hello", &GDN_TheWorld_Edit::hello);
	register_method("edit_mode_generate", &GDN_TheWorld_Edit::editModeGenerateAction);
	register_method("edit_mode_mend", &GDN_TheWorld_Edit::editModeMendAction);
	register_method("edit_mode_gen_normals", &GDN_TheWorld_Edit::editModeGenNormalsAction);
	register_method("edit_mode_save", &GDN_TheWorld_Edit::editModeSaveAction);
	register_method("edit_mode_upload", &GDN_TheWorld_Edit::editModeUploadAction);
}

GDN_TheWorld_Edit::GDN_TheWorld_Edit()
{
	m_initialized = false;
	m_actionInProgress = false;
	m_onGoingElapsedLabel = false;
	m_viewer = nullptr;
	m_seed = nullptr;
	m_frequency = nullptr;
	m_fractalOctaves = nullptr;
	m_fractalLacunarity = nullptr;
	m_fractalGain = nullptr;
	m_fractalWeightedStrength = nullptr;
	m_fractalPingPongStrength = nullptr;
	m_amplitudeLabel = nullptr;
	m_minHeightLabel = nullptr;
	m_maxHeightLabel = nullptr;
	m_elapsedLabel = nullptr;
	m_mouseHitLabel = nullptr;
	m_mouseQuadHitLabel = nullptr;
	m_mouseQuadHitPosLabel = nullptr;
	m_mouseQuadSelLabel = nullptr;
	m_mouseQuadSelPosLabel = nullptr;
}

GDN_TheWorld_Edit::~GDN_TheWorld_Edit()
{
	deinit();
}

void GDN_TheWorld_Edit::init(GDN_TheWorld_Viewer* viewer)
{
	m_initialized = true;
	m_viewer = viewer;

	std::lock_guard<std::recursive_mutex> lock(m_viewer->getMainProcessingMutex());

	m_viewer->add_child(this);
	set_name(THEWORLD_EDIT_MODE_UI_CONTROL_NAME);
	resizeUI();

	godot::Control* marginContainer = nullptr;
	//godot::Control* vBoxContainer = nullptr;
	godot::Control* hBoxContainer = nullptr;
	godot::Control* separator = nullptr;
	godot::Button* button = nullptr;
	godot::Label* label = nullptr;

	godot::Control* mainTabContainer = godot::TabContainer::_new();
	mainTabContainer->set_name("EditModeTab");
	add_child(mainTabContainer);

	godot::Control* mainPanelContainer = godot::PanelContainer::_new();
	mainPanelContainer->set_name("Terrain");
	mainTabContainer->add_child(mainPanelContainer);

		godot::Control* mainVBoxContainer = godot::VBoxContainer::_new();
		mainPanelContainer->add_child(mainVBoxContainer);
			
		//	marginContainer = godot::MarginContainer::_new();
		//	mainVBoxContainer->add_child(marginContainer);
		//		label = godot::Label::_new();
		//		marginContainer->add_child(label);
		//		label->set_text("Terrain");
		//		label->set_align(godot::Label::Align::ALIGN_CENTER);

			separator = HSeparator::_new();
			mainVBoxContainer->add_child(separator);

			marginContainer = godot::MarginContainer::_new();
			mainVBoxContainer->add_child(marginContainer);
				hBoxContainer = godot::HBoxContainer::_new();
				marginContainer->add_child(hBoxContainer);
					button = godot::Button::_new();
					hBoxContainer->add_child(button);
					button->set_text("Generate");
					button->connect("pressed", this, "edit_mode_generate");
					button->set_focus_mode(FocusMode::FOCUS_NONE);
					separator = VSeparator::_new();
					hBoxContainer->add_child(separator);
					button = godot::Button::_new();
					hBoxContainer->add_child(button);
					button->set_text("Mend");
					button->connect("pressed", this, "edit_mode_mend");
					button->set_focus_mode(FocusMode::FOCUS_NONE);
					separator = VSeparator::_new();
					hBoxContainer->add_child(separator);
					button = godot::Button::_new();
					hBoxContainer->add_child(button);
					button->set_text("Gen. Normals");
					button->connect("pressed", this, "edit_mode_gen_normals");
					button->set_focus_mode(FocusMode::FOCUS_NONE);

			marginContainer = godot::MarginContainer::_new();
			mainVBoxContainer->add_child(marginContainer);
				hBoxContainer = godot::HBoxContainer::_new();
				marginContainer->add_child(hBoxContainer);
					label = godot::Label::_new();
					hBoxContainer->add_child(label);
					label->set_text("Seed");
					label->set_align(godot::Label::Align::ALIGN_LEFT);
					m_seed = godot::LineEdit::_new();
					hBoxContainer->add_child(m_seed);
					m_seed->set_align(godot::Label::Align::ALIGN_RIGHT);
					label = godot::Label::_new();
					hBoxContainer->add_child(label);
					label->set_text("Frequency");
					label->set_align(godot::Label::Align::ALIGN_LEFT);
					m_frequency = godot::LineEdit::_new();
					hBoxContainer->add_child(m_frequency);
					m_frequency->set_align(godot::Label::Align::ALIGN_RIGHT);

			//marginContainer = godot::MarginContainer::_new();
			//mainVBoxContainer->add_child(marginContainer);
			//	hBoxContainer = godot::HBoxContainer::_new();
			//	marginContainer->add_child(hBoxContainer);
			//		label = godot::Label::_new();
			//		hBoxContainer->add_child(label);
			//		label->set_text("Frequency");
			//		label->set_align(godot::Label::Align::ALIGN_LEFT);
			//		m_frequency = godot::LineEdit::_new();
			//		hBoxContainer->add_child(m_frequency);
			//		m_frequency->set_align(godot::Label::Align::ALIGN_RIGHT);
					
			marginContainer = godot::MarginContainer::_new();
			mainVBoxContainer->add_child(marginContainer);
				hBoxContainer = godot::HBoxContainer::_new();
				marginContainer->add_child(hBoxContainer);
					label = godot::Label::_new();
					hBoxContainer->add_child(label);
					label->set_text("Octaves");
					label->set_align(godot::Label::Align::ALIGN_LEFT);
					m_fractalOctaves = godot::LineEdit::_new();
					hBoxContainer->add_child(m_fractalOctaves);
					m_fractalOctaves->set_align(godot::Label::Align::ALIGN_RIGHT);
					label = godot::Label::_new();
					hBoxContainer->add_child(label);
					label->set_text("Lacunarity");
					label->set_align(godot::Label::Align::ALIGN_LEFT);
					m_fractalLacunarity = godot::LineEdit::_new();
					hBoxContainer->add_child(m_fractalLacunarity);
					m_fractalLacunarity->set_align(godot::Label::Align::ALIGN_RIGHT);

			//marginContainer = godot::MarginContainer::_new();
			//mainVBoxContainer->add_child(marginContainer);
			//	hBoxContainer = godot::HBoxContainer::_new();
			//	marginContainer->add_child(hBoxContainer);
			//		label = godot::Label::_new();
			//		hBoxContainer->add_child(label);
			//		label->set_text("Lacunarity");
			//		label->set_align(godot::Label::Align::ALIGN_LEFT);
			//		m_fractalLacunarity = godot::LineEdit::_new();
			//		hBoxContainer->add_child(m_fractalLacunarity);
			//		m_fractalLacunarity->set_align(godot::Label::Align::ALIGN_RIGHT);
					
			marginContainer = godot::MarginContainer::_new();
			mainVBoxContainer->add_child(marginContainer);
				hBoxContainer = godot::HBoxContainer::_new();
				marginContainer->add_child(hBoxContainer);
					label = godot::Label::_new();
					hBoxContainer->add_child(label);
					label->set_text("Gain");
					label->set_align(godot::Label::Align::ALIGN_LEFT);
					m_fractalGain = godot::LineEdit::_new();
					hBoxContainer->add_child(m_fractalGain);
					m_fractalGain->set_align(godot::Label::Align::ALIGN_RIGHT);
					label = godot::Label::_new();
					hBoxContainer->add_child(label);
					label->set_text("Amplitude");
					label->set_align(godot::Label::Align::ALIGN_LEFT);
					m_amplitudeLabel = godot::LineEdit::_new();
					hBoxContainer->add_child(m_amplitudeLabel);
					m_amplitudeLabel->set_align(godot::Label::Align::ALIGN_RIGHT);

			marginContainer = godot::MarginContainer::_new();
			mainVBoxContainer->add_child(marginContainer);
				hBoxContainer = godot::HBoxContainer::_new();
				marginContainer->add_child(hBoxContainer);
					label = godot::Label::_new();
					hBoxContainer->add_child(label);
					label->set_text("Weighted Strength");
					label->set_align(godot::Label::Align::ALIGN_LEFT);
					m_fractalWeightedStrength = godot::LineEdit::_new();
					hBoxContainer->add_child(m_fractalWeightedStrength);
					m_fractalWeightedStrength->set_align(godot::Label::Align::ALIGN_RIGHT);
					
			marginContainer = godot::MarginContainer::_new();
			mainVBoxContainer->add_child(marginContainer);
				hBoxContainer = godot::HBoxContainer::_new();
				marginContainer->add_child(hBoxContainer);
					label = godot::Label::_new();
					hBoxContainer->add_child(label);
					label->set_text("Ping Pong Strength");
					label->set_align(godot::Label::Align::ALIGN_LEFT);
					m_fractalPingPongStrength = godot::LineEdit::_new();
					hBoxContainer->add_child(m_fractalPingPongStrength);
					m_fractalPingPongStrength->set_align(godot::Label::Align::ALIGN_RIGHT);

			//marginContainer = godot::MarginContainer::_new();
			//mainVBoxContainer->add_child(marginContainer);
			//	hBoxContainer = godot::HBoxContainer::_new();
			//	marginContainer->add_child(hBoxContainer);
			//		label = godot::Label::_new();
			//		hBoxContainer->add_child(label);
			//		label->set_text("Amplitude");
			//		label->set_align(godot::Label::Align::ALIGN_LEFT);
			//		m_amplitudeLabel = godot::LineEdit::_new();
			//		hBoxContainer->add_child(m_amplitudeLabel);
			//		m_amplitudeLabel->set_align(godot::Label::Align::ALIGN_RIGHT);

			separator = HSeparator::_new();
			mainVBoxContainer->add_child(separator);

			marginContainer = godot::MarginContainer::_new();
			mainVBoxContainer->add_child(marginContainer);
				hBoxContainer = godot::HBoxContainer::_new();
				marginContainer->add_child(hBoxContainer);
					label = godot::Label::_new();
					hBoxContainer->add_child(label);
					label->set_text("Min");
					label->set_align(godot::Label::Align::ALIGN_LEFT);
					m_minHeightLabel = godot::Label::_new();
					hBoxContainer->add_child(m_minHeightLabel);
					m_minHeightLabel->set_align(godot::Label::Align::ALIGN_LEFT);
					label = godot::Label::_new();
					hBoxContainer->add_child(label);
					label->set_text("Max");
					label->set_align(godot::Label::Align::ALIGN_LEFT);
					m_maxHeightLabel = godot::Label::_new();
					hBoxContainer->add_child(m_maxHeightLabel);
					m_maxHeightLabel->set_align(godot::Label::Align::ALIGN_LEFT);
					label = godot::Label::_new();
					hBoxContainer->add_child(label);
					label->set_text("Elapsed");
					label->set_align(godot::Label::Align::ALIGN_LEFT);
					m_elapsedLabel = godot::Label::_new();
					hBoxContainer->add_child(m_elapsedLabel);
					m_elapsedLabel->set_align(godot::Label::Align::ALIGN_LEFT);

			separator = HSeparator::_new();
			mainVBoxContainer->add_child(separator);

			marginContainer = godot::MarginContainer::_new();
			mainVBoxContainer->add_child(marginContainer);
				hBoxContainer = godot::HBoxContainer::_new();
				marginContainer->add_child(hBoxContainer);
					label = godot::Label::_new();
					hBoxContainer->add_child(label);
					label->set_text("Hit");
					label->set_align(godot::Label::Align::ALIGN_LEFT);
					m_mouseHitLabel = godot::Label::_new();
					hBoxContainer->add_child(m_mouseHitLabel);
					m_mouseHitLabel->set_align(godot::Label::Align::ALIGN_RIGHT);

			marginContainer = godot::MarginContainer::_new();
			mainVBoxContainer->add_child(marginContainer);
				hBoxContainer = godot::HBoxContainer::_new();
				marginContainer->add_child(hBoxContainer);
					label = godot::Label::_new();
					hBoxContainer->add_child(label);
					label->set_text("Quad Hit");
					label->set_align(godot::Label::Align::ALIGN_LEFT);
					m_mouseQuadHitLabel = godot::Label::_new();
					hBoxContainer->add_child(m_mouseQuadHitLabel);
					m_mouseQuadHitLabel->set_align(godot::Label::Align::ALIGN_RIGHT);

			marginContainer = godot::MarginContainer::_new();
			mainVBoxContainer->add_child(marginContainer);
				hBoxContainer = godot::HBoxContainer::_new();
				marginContainer->add_child(hBoxContainer);
					label = godot::Label::_new();
					hBoxContainer->add_child(label);
					label->set_text("Pos Hit");
					label->set_align(godot::Label::Align::ALIGN_LEFT);
					m_mouseQuadHitPosLabel = godot::Label::_new();
					hBoxContainer->add_child(m_mouseQuadHitPosLabel);
					m_mouseQuadHitPosLabel->set_align(godot::Label::Align::ALIGN_RIGHT);

			marginContainer = godot::MarginContainer::_new();
			mainVBoxContainer->add_child(marginContainer);
				hBoxContainer = godot::HBoxContainer::_new();
				marginContainer->add_child(hBoxContainer);
					label = godot::Label::_new();
					hBoxContainer->add_child(label);
					label->set_text("Quad Sel");
					label->set_align(godot::Label::Align::ALIGN_LEFT);
					m_mouseQuadSelLabel = godot::Label::_new();
					hBoxContainer->add_child(m_mouseQuadSelLabel);
					m_mouseQuadSelLabel->set_align(godot::Label::Align::ALIGN_RIGHT);

			marginContainer = godot::MarginContainer::_new();
			mainVBoxContainer->add_child(marginContainer);
				hBoxContainer = godot::HBoxContainer::_new();
				marginContainer->add_child(hBoxContainer);
					label = godot::Label::_new();
					hBoxContainer->add_child(label);
					label->set_text("Pos Sel");
					label->set_align(godot::Label::Align::ALIGN_LEFT);
					m_mouseQuadSelPosLabel = godot::Label::_new();
					hBoxContainer->add_child(m_mouseQuadSelPosLabel);
					m_mouseQuadSelPosLabel->set_align(godot::Label::Align::ALIGN_RIGHT);

			separator = HSeparator::_new();
			mainVBoxContainer->add_child(separator);

			marginContainer = godot::MarginContainer::_new();
			mainVBoxContainer->add_child(marginContainer);
				hBoxContainer = godot::HBoxContainer::_new();
				marginContainer->add_child(hBoxContainer);
					button = godot::Button::_new();
					hBoxContainer->add_child(button);
					button->set_text("Save");
					button->connect("pressed", this, "edit_mode_save");
					button->set_focus_mode(FocusMode::FOCUS_NONE);
					separator = VSeparator::_new();
					hBoxContainer->add_child(separator);
					button = godot::Button::_new();
					hBoxContainer->add_child(button);
					button->set_text("Upload");
					button->connect("pressed", this, "edit_mode_upload");
					button->set_focus_mode(FocusMode::FOCUS_NONE);

			separator = HSeparator::_new();
			mainVBoxContainer->add_child(separator);

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
	if (m_actionInProgress)
	{
		setElapsed(m_actionClock.partialDuration().count(), true);
	}
}

void GDN_TheWorld_Edit::editModeSaveAction(void)
{
	if (m_actionInProgress)
		return;

	m_actionInProgress = true;
	setElapsed(0, true);

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

	for (auto& item : m_mapQuadToSave)
	{
		QuadTree* quadToSave = m_viewer->getQuadTree(item.first);
		if (quadToSave != nullptr)
		{
			TheWorld_Utils::MeshCacheBuffer& cache = quadToSave->getQuadrant()->getMeshCacheBuffer();
			TheWorld_Utils::MeshCacheBuffer::CacheData cacheData;
			cacheData.meshId = cache.getMeshId();
			TerrainEdit* terrainEdit = quadToSave->getQuadrant()->getTerrainEdit();
			TheWorld_Utils::MemoryBuffer terrainEditValuesBuffer((BYTE*)terrainEdit, terrainEdit->size);
			cacheData.minHeight = terrainEdit->minHeight;
			cacheData.maxHeight = terrainEdit->maxHeight;
			cacheData.terrainEditValues = &terrainEditValuesBuffer;
			cacheData.heights16Buffer = &quadToSave->getQuadrant()->getFloat16HeightsBuffer();
			cacheData.heights32Buffer = &quadToSave->getQuadrant()->getFloat32HeightsBuffer();
			cacheData.normalsBuffer = &quadToSave->getQuadrant()->getNormalsBuffer();
			QuadrantPos quadrantPos = item.first;
			TheWorld_Utils::MemoryBuffer buffer;
			cache.setBufferFromCacheData(quadrantPos.getNumVerticesPerSize(), quadrantPos.getGridStepInWU(), cacheData, buffer);
			cache.writeBufferToCache(buffer);
		}
	}

	m_mapQuadToSave.clear();

	m_actionClock.tock();

	size_t duration = m_actionClock.duration().count();
	setElapsed(duration, false);

	m_actionInProgress = false;
}

void GDN_TheWorld_Edit::editModeUploadAction(void)
{
	if (m_actionInProgress)
		return;

	m_actionInProgress = true;
	setElapsed(0, true);

	std::function<void(void)> f = std::bind(&GDN_TheWorld_Edit::editModeUpload, this);
	m_tp.QueueJob(f);
}

void GDN_TheWorld_Edit::editModeUpload(void)
{
	if (m_actionClock.counterStarted())
	{
		m_actionInProgress = false;
		return;
	}

	m_actionClock.tick();

	TheWorld_Utils::GuardProfiler profiler(std::string("EditUpload 1 ") + __FUNCTION__, "ALL");

	// TODO

	m_actionClock.tock();

	size_t duration = m_actionClock.duration().count();
	setElapsed(duration, false);

	m_actionInProgress = false;
}

void GDN_TheWorld_Edit::editModeGenerateAction(void)
{
	if (m_actionInProgress)
		return;

	m_actionInProgress = true;
	setElapsed(0, true);

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

	TheWorld_Utils::GuardProfiler profiler(std::string("EditGenerate 1 ") + __FUNCTION__, "ALL");

	TerrainEdit* terrainEdit = quadTreeSel->getQuadrant()->getTerrainEdit();
	terrainEdit->noiseSeed = seed();
	terrainEdit->frequency = frequency();
	terrainEdit->fractalOctaves = octaves();
	terrainEdit->fractalLacunarity = lacunarity();
	terrainEdit->fractalGain = gain();
	terrainEdit->fractalWeightedStrength = weightedStrength();
	terrainEdit->fractalPingPongStrength = pingPongStrength();
	terrainEdit->amplitude = amplitude();
	terrainEdit->needUploadToServer = true;

	FastNoiseLite noise(terrainEdit->noiseSeed);
	noise.SetNoiseType(FastNoiseLite::NoiseType::NoiseType_Perlin);
	//noise.SetRotationType3D();
	noise.SetFrequency(terrainEdit->frequency);
	noise.SetFractalType(FastNoiseLite::FractalType::FractalType_FBm);
	noise.SetFractalOctaves(terrainEdit->fractalOctaves);
	noise.SetFractalLacunarity(terrainEdit->fractalLacunarity);
	noise.SetFractalGain(terrainEdit->fractalGain);
	noise.SetFractalWeightedStrength(terrainEdit->fractalWeightedStrength);
	noise.SetFractalPingPongStrength(terrainEdit->fractalPingPongStrength);
	//noise.SetCellularDistanceFunction();
	//noise.SetCellularReturnType();
	//noise.SetCellularJitter()

	size_t numVerticesPerSize = quadrantSelPos.getNumVerticesPerSize();
	size_t numVertices = numVerticesPerSize * numVerticesPerSize;
	float gridStepInWU = quadrantSelPos.getGridStepInWU();
	float sizeInWU = quadrantSelPos.getSizeInWU();
	float lowerXGridVertex = quadrantSelPos.getLowerXGridVertex();
	float lowerZGridVertex = quadrantSelPos.getLowerZGridVertex();
	
	TheWorld_Utils::MemoryBuffer& heights16Buffer = quadTreeSel->getQuadrant()->getFloat16HeightsBuffer();
	size_t heights16BufferSize = numVertices * sizeof(uint16_t);
	heights16Buffer.reserve(heights16BufferSize);
	uint16_t* movingHeights16Buffer = (uint16_t*)heights16Buffer.ptr();
	//std::vector<float> vectGridHeights(numVertices);;
	//vectGridHeights.reserve(numVertices);
	TheWorld_Utils::MemoryBuffer& heights32Buffer = quadTreeSel->getQuadrant()->getFloat32HeightsBuffer();
	size_t heights32BufferSize = numVertices * sizeof(float);
	heights32Buffer.reserve(heights32BufferSize);
	float* movingHeights32Buffer = (float*)heights32Buffer.ptr();

	float minHeight = FLT_MAX;
	float maxHeight = FLT_MIN;

	{
		TheWorld_Utils::GuardProfiler profiler(std::string("EditGenerate 1.1 ") + __FUNCTION__, "Generate Heights");

		size_t idx = 0;
		for (int z = 0; z < numVerticesPerSize; z++)
			for (int x = 0; x < numVerticesPerSize; x++)
			{
				float altitude = 0.0f;

				{
					//TheWorld_Utils::GuardProfiler profiler(std::string("EditGenerate 1.1.1 ") + __FUNCTION__, "GetNoise");

					float xf = lowerXGridVertex + (x * gridStepInWU);
					float zf = lowerZGridVertex + (z * gridStepInWU);
					altitude = noise.GetNoise(xf, zf);
					// noises are value in range -1 to 1 we need to interpolate with amplitude
					altitude *= (terrainEdit->amplitude / 2);

					if (altitude < minHeight)
						minHeight = altitude;
					if (altitude > maxHeight)
						maxHeight = altitude;
				}

				{
					//TheWorld_Utils::GuardProfiler profiler(std::string("EditGenerate 1.1.2 ") + __FUNCTION__, "half_from_float");

					TheWorld_Utils::FLOAT_32 f(altitude);
					*movingHeights16Buffer = half_from_float(f.u32);;
					movingHeights16Buffer++;
				}

				*movingHeights32Buffer = altitude;
				movingHeights32Buffer++;
				//vectGridHeights[idx] = altitude;
				//vectGridHeights.push_back(altitude);

				idx++;
			}
	}

	my_assert((BYTE*)movingHeights16Buffer - heights16Buffer.ptr() == heights16BufferSize);
	heights16Buffer.adjustSize(heights16BufferSize);

	my_assert((BYTE*)movingHeights32Buffer - heights32Buffer.ptr() == heights32BufferSize);
	heights32Buffer.adjustSize(heights32BufferSize);

	quadTreeSel->getQuadrant()->getNormalsBuffer().clear();

	terrainEdit->minHeight = minHeight;
	terrainEdit->maxHeight = maxHeight;

	{
		PoolRealArray& heigths = quadTreeSel->getQuadrant()->getHeights();
		heigths.resize((int)numVertices);
		godot::PoolRealArray::Write w = heigths.write();
		memcpy((char*)w.ptr(), heights32Buffer.ptr(), heights32Buffer.size());
	}

	Vector3 startPosition(lowerXGridVertex, minHeight, lowerZGridVertex);
	Vector3 endPosition(startPosition.x + sizeInWU, maxHeight, startPosition.z + sizeInWU);
	Vector3 size = endPosition - startPosition;

	quadTreeSel->getQuadrant()->getGlobalCoordAABB().set_position(startPosition);
	quadTreeSel->getQuadrant()->getGlobalCoordAABB().set_size(size);

	quadTreeSel->getQuadrant()->setHeightsUpdated(true);
	quadTreeSel->getQuadrant()->setColorsUpdated(true);
	quadTreeSel->materialParamsNeedReset(true);

	m_mapQuadToSave[quadrantSelPos] = "";

	//std::string meshBuffer;
	//std::string meshId;

	//{
	//	TheWorld_Utils::GuardProfiler profiler(std::string("EditGenerate 1.2 ") + __FUNCTION__, "Quadrant reverse array to buffer");

	//	float minHeight = 0, maxHeight = 0;
	//	TheWorld_Utils::MeshCacheBuffer& cache = quadTreeSel->getQuadrant()->getMeshCacheBuffer();
	//	meshId = cache.getMeshId();
	//	TheWorld_Utils::MemoryBuffer terrainEditValuesBuffer((BYTE*)terrainEdit, terrainEdit->size);
	//	cache.setBufferFromHeights(meshId, numVerticesPerSize, gridStepInWU, terrainEditValuesBuffer, vectGridHeights, meshBuffer, minHeight, maxHeight, false);
	//	
	//	m_mapQuadToSave[quadrantSelPos] = meshBuffer;
	//}

	//{
	//	TheWorld_Utils::GuardProfiler profiler(std::string("EditGenerate 1.3 ") + __FUNCTION__, "Quadrant refreshGridVertices");

	//	quadTreeSel->getQuadrant()->refreshGridVertices(meshBuffer, meshId, meshId, false);
	//	quadTreeSel->materialParamsNeedReset(true);
	//}

	setMinHeight(minHeight);
	setMaxHeight(maxHeight);

	m_actionClock.tock();

	size_t duration = m_actionClock.duration().count();
	setElapsed(duration, false);
	
	m_actionInProgress = false;
}

void GDN_TheWorld_Edit::editModeMendAction(void)
{
	if (m_actionInProgress)
		return;

	m_actionInProgress = true;
	setElapsed(0, true);

	std::function<void(void)> f = std::bind(&GDN_TheWorld_Edit::editModeMend, this);
	m_tp.QueueJob(f);
}

void GDN_TheWorld_Edit::editModeMend(void)
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

	TheWorld_Utils::GuardProfiler profiler(std::string("EditMend 1 ") + __FUNCTION__, "ALL");

	// TODO

	m_actionClock.tock();

	size_t duration = m_actionClock.duration().count();
	setElapsed(duration, false);

	m_actionInProgress = false;
}

void GDN_TheWorld_Edit::editModeGenNormalsAction(void)
{
	if (m_actionInProgress)
		return;

	m_actionInProgress = true;
	setElapsed(0, true);

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

	std::vector<QuadrantPos> allQuandrantPos;
	m_viewer->getAllQuadrantPos(allQuandrantPos);

	for (auto& pos : allQuandrantPos)
	{
		QuadTree* quadTree = m_viewer->getQuadTree(pos);
		if (quadTree != nullptr)
		{
			TheWorld_Utils::MemoryBuffer& normalsBuffer = quadTree->getQuadrant()->getNormalsBuffer();
			if (normalsBuffer.size() == 0)
			{
				TheWorld_Utils::GuardProfiler profiler(std::string("EditGenMesh 1.1 ") + __FUNCTION__, "Single QuadTree");

				TheWorld_Utils::MeshCacheBuffer& cache = quadTree->getQuadrant()->getMeshCacheBuffer();
				TheWorld_Utils::MemoryBuffer& heightsBuffer = quadTree->getQuadrant()->getFloat32HeightsBuffer();
				size_t numElements = heightsBuffer.size() / sizeof(float);
				my_assert(numElements == pos.getNumVerticesPerSize() * pos.getNumVerticesPerSize());
				//std::vector<float> vectGridHeights((float*)heightsBuffer.ptr(), (float*)heightsBuffer.ptr() + numElements);
				std::vector<float> vectGridHeights;
				heightsBuffer.populateFloatVector(vectGridHeights);
				cache.generateNormals(pos.getNumVerticesPerSize(), pos.getGridStepInWU(), vectGridHeights, normalsBuffer);
				m_mapQuadToSave[pos] = "";
				quadTree->getQuadrant()->setNormalsUpdated(true);
				quadTree->materialParamsNeedReset(true);
			}
		}
	}

	m_actionClock.tock();

	size_t duration = m_actionClock.duration().count();
	setElapsed(duration, false);

	m_actionInProgress = false;
}
