#pragma once
#include <Godot.hpp>
#include <Node.hpp>
#include <Reference.hpp>
#include <InputEvent.hpp>
#include <PanelContainer.hpp>
#include <TabContainer.hpp>
#include <MarginContainer.hpp>
#include <Label.hpp>
#include <LineEdit.hpp>
#include <CheckBox.hpp>
#include <OptionButton.hpp>

#include <WorldModifier.h>

namespace godot
{
	//class GDN_TheWorld_Viewer;

	class GDN_TheWorld_Edit : public MarginContainer, public TheWorld_Utils::ThreadInitDeinit, public TheWorld_ClientServer::ClientCallback
	{
		GODOT_CLASS(GDN_TheWorld_Edit, MarginContainer)

		enum Margin {
			MARGIN_LEFT,
			MARGIN_TOP,
			MARGIN_RIGHT,
			MARGIN_BOTTOM
		};

	public:
		GDN_TheWorld_Edit();
		~GDN_TheWorld_Edit();
		void init(GDN_TheWorld_Viewer* viewer);
		void deinit(void);

		virtual void threadInit(void) {}
		virtual void threadDeinit(void) {}

		static void _register_methods();

		//
		// Godot Standard Functions
		//
		void _init(void); // our initializer called by Godot
		void _ready(void);
		void _process(float _delta);
		void _input(const Ref<InputEvent> event);
		void _notification(int p_what);

		virtual void replyFromServer(TheWorld_ClientServer::ClientServerExecution& reply);

		//
		// Test
		//
		//String hello(String target1, String target2, int target3)
		//{
		//	return String("Test, {0} {1} {2}!").format(Array::make(target1, target2, target3)); 
		//};

		void resizeUI(void);
		void editModeGenerateAction(void);
		void editModeBlendAction(void);
		void editModeGenNormalsAction(void);
		void editModeSaveAction(void);
		void editModeUploadAction(void);
		void editModeStopAction(void);
		void editModeSelectTerrainTypeAction(int64_t index);

		void editModeGenerate(void);
		void editModeBlend(void);
		void editModeGenNormals(void);
		void editModeSave(void);
		void editModeUpload(void);

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

		//std::map<QuadrantPos, bool>& getMapQuadToSave(void)
		//{
		//	return m_mapQuadToSave;
		//}

	private:
		bool m_initialized;
		GDN_TheWorld_Viewer* m_viewer;

		map<TheWorld_Utils::WorldModifierPos, std::unique_ptr<TheWorld_Utils::WorldModifier>> m_wms;
		
		bool m_actionInProgress;
		bool m_actionStopRequested;
		TheWorld_Utils::TimerMs m_actionClock;
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

		godot::CheckBox* m_genAllNormals;
		godot::OptionButton* m_terrTypeOptionButton;

		std::map<QuadrantPos, std::string> m_mapQuadToSave;

		TheWorld_Utils::ThreadPool m_tp;
	};
}

