#pragma once
#include "Viewer_Utils.h"

#pragma warning (push, 0)
#include <godot_cpp/classes/margin_container.hpp>
#include <godot_cpp/classes/panel_container.hpp>
#include <godot_cpp/classes/tab_container.hpp>
#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/classes/line_Edit.hpp>
#include <godot_cpp/classes/check_box.hpp>
#include <godot_cpp/classes/option_button.hpp>
#include <godot_cpp/classes/input_event.hpp>
#include <godot_cpp/classes/ref.hpp>
#pragma warning (pop)

#include <WorldModifier.h>

namespace godot
{
	//class GDN_TheWorld_Viewer;

	class GDN_TheWorld_Edit : public MarginContainer, public TheWorld_Utils::ThreadInitDeinit, public TheWorld_ClientServer::ClientCallback
	{
		GDCLASS(GDN_TheWorld_Edit, MarginContainer)

		enum Margin {
			MARGIN_LEFT,
			MARGIN_TOP,
			MARGIN_RIGHT,
			MARGIN_BOTTOM
		};

	protected:
		static void _bind_methods();
		void _notification(int p_what);
		void _init(void);

	public:
		GDN_TheWorld_Edit();
		~GDN_TheWorld_Edit();
		void init(GDN_TheWorld_Viewer* viewer);
		void deinit(void);

		template <class T> T* createControl(godot::Node* parent);
			
		virtual void threadInit(void) {}
		virtual void threadDeinit(void) {}

		//
		// Godot Standard Functions
		//
		virtual void _ready(void) override;
		virtual void _process(double _delta) override;
		virtual void _input(const Ref<InputEvent>& event) override;

		virtual void replyFromServer(TheWorld_ClientServer::ClientServerExecution& reply);

		//
		// Test
		//
		//String hello(String target1, String target2, int target3)
		//{
		//	return String("Test, {0} {1} {2}!").format(Array::make(target1, target2, target3)); 
		//};

		bool initilized(void)
		{
			return m_initialized;
		}
		//void controlNeedResize(void);
		void editModeNoisePanel(void);
		void setSizeUI(void);
		void editModeGenerateAction(void);
		void editModeBlendAction(void);
		void editModeGenNormalsAction(void);
		void editModeSetTexturesAction(void);
		void editModeSaveAction(void);
		void editModeUploadAction(void);
		void editModeStopAction(void);
		void editModeSelectTerrainTypeAction(int32_t index);
		void editModeSelectLookDevAction(int32_t index);

		void editModeGenerate(void);
		void editModeBlend(void);
		void editModeGenNormals(void);
		void editModeGenNormals_1(bool force);
		void editModeSetTextures(void);
		void editModeSave(void);
		void editModeUpload(void);

		void manageUpdatedHeights(TheWorld_Utils::MeshCacheBuffer::CacheQuadrantData& quadrantData, QuadTree* quadTree, TheWorld_Utils::MemoryBuffer& terrainEditValuesBuffer, TheWorld_Utils::MemoryBuffer& heights16Buffer, TheWorld_Utils::MemoryBuffer& heights32Buffer);

		void setEmptyTerrainEditValues(void);
		void setTerrainEditValues(TheWorld_Utils::TerrainEdit& terrainEdit);

		void editModeMouseEnteredMainPanel(void);
		void editModeMouseExitedMainPanel(void);
		void setUIAcceptFocus(bool b);
		bool UIAcceptingFocus(void);

		void setSeed(int seed);
		int seed(void);
		void setFrequency(float frequency);
		float frequency(void);
		void setOctaves(int octaves);
		int octaves(void);
		void setLacunarity(float lacunarity);
		float lacunarity(void);
		void setGain(float gain);
		float gain(void);
		void setWeightedStrength(float weightedStrength);
		float weightedStrength(void);
		void setPingPongStrength(float pingPongStrength);
		float pingPongStrength(void);
		void setAmplitude(unsigned int amplitude);
		unsigned int amplitude(void);
		void setScaleFactor(float scaleFactor);
		float scaleFactor(void);
		void setDesideredMinHeight(float desideredMinHeight);
		float desideredMinHeight(void);
		void setMinHeight(float minHeight);
		float minHeight(void);
		void setMaxHeight(float maxHeight);
		float maxHeight(void);
		void setElapsed(size_t elapsed, bool onGoing);
		size_t elapsed(void);
		void setCounter(size_t curr, size_t all);
		void setNote1(size_t num);
		void setNote1(std::string msg);

		bool actionInProgress(void)
		{
			return m_actionInProgress;
		}

		void setMouseHitLabelText(std::string text);
		void setMouseQuadHitLabelText(std::string text);
		void setMouseQuadHitPosLabelText(std::string text);
		void setMouseQuadSelLabelText(std::string text);
		void setMouseQuadSelPosLabelText(std::string text);

		void setNumQuadrantToSave(size_t num);
		void setNumQuadrantToUpload(size_t num);

		void refreshNumToSaveUpload(size_t& numToSave, size_t& numToUpload);

		void setMessage(std::string text, bool add = false);
		void setMessage(godot::String text, bool add = false);

		//std::map<QuadrantPos, bool>& getMapQuadToSave(void)
		//{
		//	return m_mapQuadToSave;
		//}

	private:
		bool m_initialized;
		GDN_TheWorld_Viewer* m_viewer;
		std::recursive_mutex m_mtxUI;

		map<TheWorld_Utils::WorldModifierPos, std::unique_ptr<TheWorld_Utils::WorldModifier>> m_wms;
		
		bool m_actionInProgress;
		bool m_actionStopRequested;
		TheWorld_Utils::TimerMs m_actionClock;
		TheWorld_Utils::TimerMs m_refreshUI;
		bool m_onGoingElapsedLabel;
		Color m_elapsedLabelNormalColor;
		size_t m_completedItems;
		size_t m_elapsedCompleted;
		size_t m_allItems;
		size_t m_lastElapsed;
		bool m_requiredUIAcceptFocus;
		bool m_UIAcceptingFocus;

		godot::PanelContainer* m_mainPanelContainer;
		godot::TabContainer* m_mainTabContainer;

		godot::Control* m_noiseVBoxContainer;
		//bool m_noiseContainerShowing;
		godot::Button* m_noiseButton;
		
		godot::LineEdit* m_seed;
		godot::LineEdit* m_frequency;
		godot::LineEdit* m_fractalOctaves;
		godot::LineEdit* m_fractalLacunarity;
		godot::LineEdit* m_fractalGain;
		godot::LineEdit* m_fractalWeightedStrength;
		godot::LineEdit* m_fractalPingPongStrength;

		godot::LineEdit* m_amplitudeLabel;
		godot::LineEdit* m_scaleFactorLabel;
		godot::LineEdit* m_desideredMinHeightLabel;
		godot::Label* m_minHeightLabel;
		godot::Label* m_maxHeightLabel;
		godot::Label* m_elapsedLabel;
		godot::Label* m_counterLabel;
		godot::Label* m_note1Label;

		godot::Label* m_mouseHitLabel;
		godot::Label* m_mouseQuadHitLabel;
		godot::Label* m_mouseQuadHitPosLabel;
		godot::Label* m_mouseQuadSelLabel;
		godot::Label* m_mouseQuadSelPosLabel;

		godot::Label* m_numQuadrantToSaveLabel;
		godot::Label* m_numQuadrantToUploadLabel;

		godot::Label* m_message;
		godot::String m_lastMessage;
		bool m_lastMessageChanged;

		godot::CheckBox* m_allCheckBox;
		godot::OptionButton* m_terrTypeOptionButton;
		godot::OptionButton* m_lookDevOptionButton;

		std::map<QuadrantPos, std::string> m_mapQuadToSave;

		TheWorld_Utils::ThreadPool m_tp;
	};
}

