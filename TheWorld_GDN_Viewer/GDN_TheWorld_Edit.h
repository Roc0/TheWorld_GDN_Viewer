#pragma once
#include <Godot.hpp>
#include <Node.hpp>
#include <Reference.hpp>
#include <InputEvent.hpp>
#include <MarginContainer.hpp>
#include <Label.hpp>
#include <LineEdit.hpp>

namespace godot
{
	//class GDN_TheWorld_Viewer;

	class GDN_TheWorld_Edit : public MarginContainer
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

		static void _register_methods();

		//
		// Godot Standard Functions
		//
		void _init(void); // our initializer called by Godot
		void _ready(void);
		void _process(float _delta);
		void _input(const Ref<InputEvent> event);
		void _notification(int p_what);

		//
		// Test
		//
		//String hello(String target1, String target2, int target3)
		//{
		//	return String("Test, {0} {1} {2}!").format(Array::make(target1, target2, target3)); 
		//};

		void resizeUI(void);
		void editModeGenerate(void);
		void editModeSave(void);
		void editModeUpload(void);

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
		void setMinHeight(float minHeight);
		float minHeight(void);
		void setMaxHeight(float maxHeight);
		float maxHeight(void);
		void setElapsed(size_t elapsed);
		size_t elapsed(void);

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

		bool m_terrainGenerationInProgress;
		TheWorld_Utils::TimerMs m_clockTerrainGeneration;

		godot::LineEdit* m_seed;
		godot::LineEdit* m_frequency;
		godot::LineEdit* m_fractalOctaves;
		godot::LineEdit* m_fractalLacunarity;
		godot::LineEdit* m_fractalGain;
		godot::LineEdit* m_fractalWeightedStrength;
		godot::LineEdit* m_fractalPingPongStrength;

		godot::LineEdit* m_amplitudeLabel;
		godot::Label* m_minHeightLabel;
		godot::Label* m_maxHeightLabel;
		godot::Label* m_elapsedLabel;

		godot::Label* m_mouseHitLabel;
		godot::Label* m_mouseQuadHitLabel;
		godot::Label* m_mouseQuadHitPosLabel;
		godot::Label* m_mouseQuadSelLabel;
		godot::Label* m_mouseQuadSelPosLabel;

		std::map<QuadrantPos, std::string> m_mapQuadToSave;
	};

}

