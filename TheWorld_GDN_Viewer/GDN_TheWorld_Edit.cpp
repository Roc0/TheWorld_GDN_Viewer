//#include "pch.h"
#include <Godot.hpp>
#include <Node.hpp>
#include <MarginContainer.hpp>
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

using namespace godot;

void GDN_TheWorld_Edit::_register_methods()
{
	register_method("_ready", &GDN_TheWorld_Edit::_ready);
	register_method("_process", &GDN_TheWorld_Edit::_process);
	register_method("_input", &GDN_TheWorld_Edit::_input);
	register_method("_notification", &GDN_TheWorld_Edit::_notification);

	//register_method("hello", &GDN_TheWorld_Edit::hello);
	register_method("edit_mode_generate", &GDN_TheWorld_Edit::editModeGenerate);
}

GDN_TheWorld_Edit::GDN_TheWorld_Edit()
{
	m_initialized = false;
	m_viewer = nullptr;
	m_seed = nullptr;
	m_frequency = nullptr;
	m_fractalOctaves = nullptr;
	m_fractalLacunarity = nullptr;
	m_fractalGain = nullptr;
	m_fractalWeightedStrength = nullptr;
	m_fractalPingPongStrength = nullptr;
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

	m_viewer->add_child(this);
	set_name(THEWORLD_EDIT_MODE_UI_CONTROL_NAME);
	resizeUI();

	godot::Control* marginContainer = nullptr;
	//godot::Control* vBoxContainer = nullptr;
	godot::Control* hBoxContainer = nullptr;
	godot::Control* separator = nullptr;
	godot::Button* button = nullptr;
	godot::Label* label = nullptr;

	godot::Control* mainPanelContainer = godot::PanelContainer::_new();
	add_child(mainPanelContainer);

		godot::Control* mainVBoxContainer = godot::VBoxContainer::_new();
		mainPanelContainer->add_child(mainVBoxContainer);
			marginContainer = godot::MarginContainer::_new();
			mainVBoxContainer->add_child(marginContainer);
				label = godot::Label::_new();
				marginContainer->add_child(label);
				label->set_text("Edit Mode");
				label->set_align(godot::Label::Align::ALIGN_CENTER);

			marginContainer = godot::MarginContainer::_new();
			mainVBoxContainer->add_child(marginContainer);
			//marginContainer->set_margin(GDN_TheWorld_Viewer::Margin::MARGIN_RIGHT, 50.0);
			//marginContainer->set_margin(GDN_TheWorld_Viewer::Margin::MARGIN_TOP, 5.0);
			//marginContainer->set_margin(GDN_TheWorld_Viewer::Margin::MARGIN_LEFT, 50.0);
			//marginContainer->set_margin(GDN_TheWorld_Viewer::Margin::MARGIN_BOTTOM, 5.0);
				hBoxContainer = godot::HBoxContainer::_new();
				marginContainer->add_child(hBoxContainer);
					button = godot::Button::_new();
					hBoxContainer->add_child(button);
					button->set_text("Generate");
					button->connect("pressed", this, "edit_mode_generate");
					separator = VSeparator::_new();
					hBoxContainer->add_child(separator);
					button = godot::Button::_new();
					hBoxContainer->add_child(button);
					button->set_text("Save");
					button->connect("pressed", this, "edit_mode_generate");
					separator = VSeparator::_new();
					hBoxContainer->add_child(separator);
					button = godot::Button::_new();
					hBoxContainer->add_child(button);
					button->set_text("Upload");
					button->connect("pressed", this, "edit_mode_generate");
					
			separator = HSeparator::_new();
			mainVBoxContainer->add_child(separator);

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

			marginContainer = godot::MarginContainer::_new();
			mainVBoxContainer->add_child(marginContainer);
				hBoxContainer = godot::HBoxContainer::_new();
				marginContainer->add_child(hBoxContainer);
					label = godot::Label::_new();
					hBoxContainer->add_child(label);
					label->set_text("Frequency");
					label->set_align(godot::Label::Align::ALIGN_LEFT);
					m_frequency = godot::LineEdit::_new();
					hBoxContainer->add_child(m_frequency);
					m_frequency->set_align(godot::Label::Align::ALIGN_RIGHT);
					
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

			marginContainer = godot::MarginContainer::_new();
			mainVBoxContainer->add_child(marginContainer);
				hBoxContainer = godot::HBoxContainer::_new();
				marginContainer->add_child(hBoxContainer);
					label = godot::Label::_new();
					hBoxContainer->add_child(label);
					label->set_text("Lacunarity");
					label->set_align(godot::Label::Align::ALIGN_LEFT);
					m_fractalLacunarity = godot::LineEdit::_new();
					hBoxContainer->add_child(m_fractalLacunarity);
					m_fractalLacunarity->set_align(godot::Label::Align::ALIGN_RIGHT);
					
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

void GDN_TheWorld_Edit::deinit(void)
{
	if (m_initialized)
	{
		m_initialized = false;
	}
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
	// To activate _process method add this Node to a Godot Scene
	//Godot::print("GDN_Template::_process");
}

void GDN_TheWorld_Edit::editModeGenerate(void)
{
	m_viewer->GenerateHeigths();
}