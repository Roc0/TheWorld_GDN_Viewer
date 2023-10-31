#pragma once
#include "Viewer_Utils.h"

#pragma warning (push, 0)
#include <godot_cpp/classes/margin_container.hpp>
#include <godot_cpp/classes/panel_container.hpp>
#include <godot_cpp/classes/tab_container.hpp>
#include <godot_cpp/classes/scroll_container.hpp>
#include <godot_cpp/classes/popup_panel.hpp>
#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/classes/line_Edit.hpp>
#include <godot_cpp/classes/check_box.hpp>
#include <godot_cpp/classes/option_button.hpp>
#include <godot_cpp/classes/texture_rect.hpp>
#include <godot_cpp/classes/input_event.hpp>
#include <godot_cpp/classes/ref.hpp>
#include <godot_cpp/classes/style_box_flat.hpp>
#include <godot_cpp/classes/style_box_empty.hpp>
#include <godot_cpp/classes/h_split_container.hpp>
#include <godot_cpp/classes/sub_viewport.hpp>
#include <godot_cpp/classes/sprite2d.hpp>
#pragma warning (pop)

#include <WorldModifier.h>

namespace godot
{
	//class GDN_TheWorld_Viewer;

	class GDN_TheWorld_MapModder : public godot::Node
	{
		GDCLASS(GDN_TheWorld_MapModder, Node)

	public:
		//enum class outImageFormat
		//{
		//};

	protected:
		static void _bind_methods();
		void _notification(int p_what);

	public:
		GDN_TheWorld_MapModder();
		~GDN_TheWorld_MapModder();

		//
		// Godot Standard Functions
		//
		virtual void _ready(void) override;
		virtual void _process(double _delta) override;
		virtual void _physics_process(double _delta) override;
		virtual void _input(const Ref<InputEvent>& event) override;

		void custom_ready(GDN_TheWorld_Viewer* viewer, GDN_TheWorld_Edit* edit, bool debug = false);
		void debug_process(double _delta);

		void framePostDraw(void);

		void initForNormals(TheWorld_Utils::MemoryBuffer* float32HeigthsBuffer, size_t tileSize, size_t viewportBorder = 0);
		void deinit(void);
		void startProcessing(void);
		bool working(void);
		TheWorld_Utils::MemoryBuffer* getOutBuffer(void);
		void startProcessRequired(bool b);
		bool startProcessRequired(void);
		void deinitRequired(bool b);
		bool deinitRequired(void);

	private:
		bool workToDo(void);

	private:
		const int32_t c_uninitialized = 0;
		const int32_t c_initializedForNormals = 1;
		int32_t m_initialized = c_uninitialized;

		bool m_startProcessingRequired = false;
		bool m_deinitRequired = false;

		GDN_TheWorld_Viewer* m_viewer = nullptr;
		GDN_TheWorld_Edit* m_edit = nullptr;
		size_t m_tileSize = 0;
		size_t m_viewportBorder = 0;
		size_t m_imageSize = 0;
		TheWorld_Utils::MemoryBuffer* m_float32HeigthsBuffer = nullptr;

		godot::SubViewport* m_viewport = nullptr;
		godot::Sprite2D* m_map = nullptr;

		const int16_t c_tileStatusWaiting = 0;
		const int16_t c_tileStatusProcessing = 1;
		std::map<godot::Vector2i, int16_t> m_tiles;
		std::queue<godot::Vector2i> m_tilesQueue;
		godot::Vector2i m_currentTile;
		bool m_processingTile = false;

		godot::Ref<godot::Image> m_outImage;
		std::unique_ptr<TheWorld_Utils::MemoryBuffer> m_outBuffer;
		bool m_frameAvailable = false;

		godot::Sprite2D* m_debugSprite = nullptr;
	};

	class GDN_TheWorld_Edit : public godot::MarginContainer, public TheWorld_Utils::ThreadInitDeinit, public TheWorld_ClientServer::ClientCallback
	{
		GDCLASS(GDN_TheWorld_Edit, MarginContainer)

	public:

		class InnerData
		{
		public:
			class SelTexturePanel
			{
			public:
				class TextureItemPanel
				{
				public:
					TextureItemPanel()
					{
						m_panel = nullptr;
						m_idx = -1;
						m_selected = false;
					}
					~TextureItemPanel()
					{
					}

				public:
					godot::PanelContainer* m_panel;
					int32_t m_idx;
					bool m_selected;
				};

			public:
				SelTexturePanel()
				{
					m_openSelTexturesRequired = false;
					m_closeSelTexturesRequired = false;
					m_changeTextureRequired = false;
					m_wnd = nullptr;
				}
				~SelTexturePanel()
				{
				}

			public:
				bool m_openSelTexturesRequired;
				bool m_closeSelTexturesRequired;
				bool m_changeTextureRequired;
				std::string m_selectedTexSlotName;
				std::string m_selectedTexName;
				std::string m_newTexSlotName;
				std::string m_newTexName;
				godot::Window* m_wnd;
				map<std::string, std::unique_ptr<TextureItemPanel>> m_textureItems;
			};

		public:
			InnerData()
			{
				m_selTexturePanel = std::make_unique<SelTexturePanel>();
				m_mainPanelContainer = nullptr;
				m_mainTabContainer = nullptr;
				m_mainTerrainScrollContainer = nullptr;
				m_mainVBoxContainer = nullptr;
				m_noiseVBoxContainer = nullptr;
				m_noiseButton = nullptr;
				m_infoVBoxContainer = nullptr;
				m_infoButton = nullptr;
				m_infoLabel = nullptr;
				m_terrEditVBoxContainer = nullptr;
				m_terrEditButton = nullptr;
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
				m_elapsedLabel = nullptr;
				m_elapsed1Label = nullptr;
				m_numQuadrantToSaveLabel = nullptr;
				m_numQuadrantToUploadLabel = nullptr;
				m_message = nullptr;
				m_allCheckBox = nullptr;
				m_terrTypeOptionButton = nullptr;
				m_lookDevOptionButton = nullptr;
				m_lowElevationTexPanel = nullptr;
				m_highElevationTexPanel = nullptr;
				m_dirtTexPanel = nullptr;
				m_rocksTexPanel = nullptr;
				m_lowElevationTex = nullptr;
				m_highElevationTex = nullptr;
				m_dirtTex = nullptr;
				m_rocksTex = nullptr;
				m_lowElevationTexName = nullptr;
				m_highElevationTexName = nullptr;
				m_dirtTexName = nullptr;
				m_rocksTexName = nullptr;
				m_lowElevationTexSelected = false;
				m_highElevationTexSelected = false;
				m_dirtTexSelected = false;
				m_rocksTexSelected = false;
				m_onGoing = false;
				m_mouseInsideMainPanel = false;
				m_slopeVerticalFactor = nullptr;
				m_slopeFlatFactor = nullptr;
				m_dirtOnRocksFactor = nullptr;
				m_highElevationFactor = nullptr;
				m_lowElevationFactor = nullptr;
				m_splatMapMode = nullptr;

			}
			~InnerData()
			{
			}

		public:
			godot::Ref<godot::StyleBoxFlat> m_styleBoxSelectedFrame;
			godot::Ref<godot::StyleBoxFlat> m_styleBoxHighlightedFrame;

			godot::Control* m_mainPanelContainer;
			godot::Control* m_mainTabContainer;
			godot::ScrollContainer* m_mainTerrainScrollContainer;
			godot::Control* m_mainVBoxContainer;

			godot::Label* m_elapsedLabel;
			godot::Label* m_elapsed1Label;

			godot::Label* m_numQuadrantToSaveLabel;
			godot::Label* m_numQuadrantToUploadLabel;

			godot::Label* m_message;
			godot::String m_lastMessage;

			godot::CheckBox* m_allCheckBox;
			godot::OptionButton* m_terrTypeOptionButton;
			godot::OptionButton* m_lookDevOptionButton;

			godot::Button* m_infoButton;
			godot::Control* m_infoVBoxContainer;
			godot::CheckBox* m_showGizmo = nullptr;
			godot::CheckBox* m_showNormal = nullptr;
			godot::Label* m_infoLabel;

			godot::Button* m_noiseButton;
			godot::Control* m_noiseVBoxContainer;
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

			godot::Control* m_terrEditVBoxContainer;
			godot::Button* m_terrEditButton;

			godot::LineEdit* m_slopeVerticalFactor;
			godot::LineEdit* m_slopeFlatFactor;
			godot::LineEdit* m_dirtOnRocksFactor;
			godot::LineEdit* m_highElevationFactor;
			godot::LineEdit* m_lowElevationFactor;
			godot::LineEdit* m_splatMapMode;
			godot::PanelContainer* m_lowElevationTexPanel;
			godot::PanelContainer* m_highElevationTexPanel;
			godot::PanelContainer* m_dirtTexPanel;
			godot::PanelContainer* m_rocksTexPanel;
			godot::TextureRect* m_lowElevationTex;
			godot::TextureRect* m_highElevationTex;
			godot::TextureRect* m_dirtTex;
			godot::TextureRect* m_rocksTex;
			godot::Label* m_lowElevationTexName;
			godot::Label* m_highElevationTexName;
			godot::Label* m_dirtTexName;
			godot::Label* m_rocksTexName;
			bool m_lowElevationTexSelected;
			bool m_highElevationTexSelected;
			bool m_dirtTexSelected;
			bool m_rocksTexSelected;

			std::string m_elapsed;
			bool m_onGoing;
			std::string m_minHeightStr;
			std::string m_maxHeightStr;
			std::string m_counterStr;
			std::string m_note1Str;
			std::string m_mouseHitStr;
			std::string m_mouseQuadHitStr;
			std::string m_mouseQuadHitPosStr;
			std::string m_mouseQuadSelStr;
			std::string m_mouseQuadSelPosStr;

			std::unique_ptr<SelTexturePanel> m_selTexturePanel;

			bool m_mouseInsideMainPanel;
		};

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

		template <class T> T* createControl(godot::Node* parent, std::string name = "", godot::String text = "", std::string signal = "", godot::Object* callableObject = nullptr, std::string callableMethod = "", godot::Variant custom1 = "", godot::Variant custom2 = "", godot::Variant custom3 = "", Color selfModulateColor = Color(0.0f, 0.0f, 0.0f, 0.0f));
			
		virtual void threadInit(void) {}
		virtual void threadDeinit(void) {}

		//
		// Godot Standard Functions
		//
		virtual void _ready(void) override;
		virtual void _process(double _delta) override;
		virtual void _input(const Ref<InputEvent>& event) override;
		virtual void _unhandled_input(const Ref<InputEvent>& event) override;

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
		godot::Window* initSelTexturePanel(void);
		void voidSignalManager(godot::Object* obj, godot::String signal, godot::String className, int instanceId, godot::String objectName, godot::Variant custom1, godot::Variant custom2, godot::Variant custom3);
		void guiInputEvent(const godot::Ref<godot::InputEvent>& event, godot::Object* obj, godot::String signal, godot::String className, int instanceId, godot::String objectName, godot::Variant custom1, godot::Variant custom2, godot::Variant custom3);
		void toggledSignalManager(const bool pressed, godot::Object* obj, godot::String signal, godot::String className, int instanceId, godot::String objectName, godot::Variant custom1, godot::Variant custom2, godot::Variant custom3);
		void controlNeedResize(void);
		//void editModeNoisePanel(void);
		//void editModeInfoPanel(void);
		//void editModeTerrEditPanel(void);
		void setSizeUI(void);
		void changeTextureAction(void);
		void editModeGenerateAction(void);
		void editModeBlendAction(void);
		void editModeGenNormalsAction(void);
		void editModeApplyTexturesAction(void);
		void editModeSaveAction(void);
		void editModeUploadAction(void);
		void editModeStopAction(void);
		void editModeReloadShader(void);
		void editModeSelectTerrainTypeAction(int32_t index);
		void editModeSelectLookDevAction(int32_t index);

		void editModeGenerate(void);
		void editModeBlend(void);
		void editModeGenNormals(void);
		void editModeGenNormals_1(bool forceGenSelectedQuad, bool evaluateSelectedQuadOnly);
		void editModeApplyTextures(void);
		void editModeApplyTextures_1(bool forceApplySelectedQuad, bool evaluateSelectedQuadOnly);
		void editModeSave(void);
		void editModeUpload(void);
		void generateNormals(size_t numVerticesPerSize, float gridStepInWU,
			float x, float z, float distance,
			TheWorld_Utils::MemoryBuffer* float32HeigthsBuffer, TheWorld_Utils::MemoryBuffer* normalsBuffer,
			TheWorld_Utils::MemoryBuffer* west_float32HeigthsBuffer, TheWorld_Utils::MemoryBuffer* west_normalsBuffer,
			TheWorld_Utils::MemoryBuffer* east_float32HeigthsBuffer, TheWorld_Utils::MemoryBuffer* east_normalsBuffer,
			TheWorld_Utils::MemoryBuffer* north_float32HeigthsBuffer, TheWorld_Utils::MemoryBuffer* north_normalsBuffer,
			TheWorld_Utils::MemoryBuffer* south_float32HeigthsBuffer, TheWorld_Utils::MemoryBuffer* south_normalsBuffer);
		void generateSplatmapForBlendedQuadrants(size_t numVerticesPerSize, float gridStepInWU,
			float x, float z, float distance,
			TheWorld_Utils::MemoryBuffer* float32HeigthsBuffer, TheWorld_Utils::MemoryBuffer* normalsBuffer, TheWorld_Utils::MemoryBuffer* splatmapBuffer,
			TheWorld_Utils::MemoryBuffer* west_float32HeigthsBuffer, TheWorld_Utils::MemoryBuffer* west_normalsBuffer, TheWorld_Utils::MemoryBuffer* west_splatmapBuffer,
			TheWorld_Utils::MemoryBuffer* east_float32HeigthsBuffer, TheWorld_Utils::MemoryBuffer* east_normalsBuffer, TheWorld_Utils::MemoryBuffer* east_splatmapBuffer,
			TheWorld_Utils::MemoryBuffer* north_float32HeigthsBuffer, TheWorld_Utils::MemoryBuffer* north_normalsBuffer, TheWorld_Utils::MemoryBuffer* north_splatmapBuffer,
			TheWorld_Utils::MemoryBuffer* south_float32HeigthsBuffer, TheWorld_Utils::MemoryBuffer* south_normalsBuffer, TheWorld_Utils::MemoryBuffer* south_splatmapBuffer,
			float slopeVerticalFactor,	// if higher slope will have higher maximum and so rocks will be more diffuse on slopes
			float slopeFlatFactor,		// if higher slope will have lesser mininum and maximum so rocks will be less diffuse on slopes
			float dirtOnRocksFactor,	// if higher dirt will be more diffuse on rocks
			float highElevationFactor,	// if higher high elevation amount will be higher at lower altitude
			float lowElevationFactor,	// if higher low elevation amount will be higher at higher altitude
			size_t splatMapMode);

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
		void setSlopeVerticalFactor(float value);
		float slopeVerticalFactor(void);
		void setSlopeFlatFactor(float value);
		float slopeFlatFactor(void);
		void setDirtOnRocksFactor(float value);
		float dirtOnRocksFactor(void);
		void setHighElevationFactor(float value);
		float highElevationFactor(void);
		void setLowEleveationFactor(float value);
		float lowEleveationFactor(void);
		void setSplatMapMode(size_t value);
		size_t splatMapMode(void);

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

		GDN_TheWorld_Globals* Globals(bool useCache = true)
		{
			if (m_viewer == nullptr)
				return nullptr;
			return m_viewer->Globals();
		}

		godot::Size2 getTerrainScrollPanelSize(void);
		size_t getNumWorkingThread(void)
		{
			size_t maxThreads;
			return m_tp.getNumWorkingThreads(maxThreads);
		}
		void requestedQuit(void)
		{
			m_requestedThreadExit = true;
		}

	private:
		bool m_initialized;
		bool m_quitting;
		bool m_ready;
		GDN_TheWorld_Viewer* m_viewer;
		std::recursive_mutex m_mtxUI;

		map<TheWorld_Utils::WorldModifierPos, std::unique_ptr<TheWorld_Utils::WorldModifier>> m_wms;
		
		bool m_actionInProgress;
		bool m_actionStopRequested;
		TheWorld_Utils::TimerMs m_actionClock;
		TheWorld_Utils::TimerMs m_refreshUI;
		bool m_onGoingElapsedLabel;
		//Color m_elapsedLabelNormalColor;
		size_t m_completedItems;
		size_t m_elapsedCompleted;
		size_t m_allItems;
		size_t m_lastElapsed;
		bool m_requiredUIAcceptFocus;
		bool m_UIAcceptingFocus;

		std::unique_ptr<InnerData> m_innerData;

		bool m_controlNeedResize;
		bool m_scrollPanelsNeedResize;
		
		bool m_infoLabelTextChanged;
		// filler
		//char filler[72];	// 72 OK (73 no) oppure 32 + 1 std::string (sembra che occupi 40 byte)
		// filler
		
		bool m_lastMessageChanged;

		std::map<QuadrantPos, std::string> m_mapQuadToSave;

		TheWorld_Utils::ThreadPool m_tp;
		bool m_requestedThreadExit = false;

		GDN_TheWorld_MapModder* m_mapModder = nullptr;
	};
}

