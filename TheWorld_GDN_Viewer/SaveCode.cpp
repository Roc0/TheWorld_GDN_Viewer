//#ifdef _THEWORLD_CLIENT
//
//	void MeshCacheBuffer::writeImage(godot::Ref<godot::Image> image, enum class ImageType type)
//	{
//		std::string _fileName;
//		if (type == ImageType::heightmap)
//			_fileName = m_heightmapFilePath;
//		else if (type == ImageType::normalmap)
//			_fileName = m_normalmapFilePath;
//		else
//			throw(std::exception((std::string(__FUNCTION__) + std::string("Unknown image type (") + std::to_string((int)type) + ")").c_str()));
//		godot::String fileName = _fileName.c_str();
//
//		int64_t w = image->get_width();
//		int64_t h = image->get_height();
//		godot::Image::Format f = image->get_format();
//		godot::PoolByteArray data = image->get_data();
//		int64_t size = data.size();
//		godot::File* file = godot::File::_new();
//		godot::Error err = file->open(fileName, godot::File::WRITE);
//		if (err != godot::Error::OK)
//			throw(std::exception((std::string(__FUNCTION__) + std::string("file->open error (") + std::to_string((int)err) + ") : " + _fileName).c_str()));
//		file->store_64(w);
//		file->store_64(h);
//		file->store_64((int64_t)f);
//		file->store_64(size);
//		file->store_buffer(data);
//		file->close();
//	}
//
//	godot::Ref<godot::Image> MeshCacheBuffer::readImage(bool& ok, enum class ImageType type)
//	{
//		std::string _fileName;
//		if (type == ImageType::heightmap)
//			_fileName = m_heightmapFilePath;
//		else if (type == ImageType::normalmap)
//			_fileName = m_normalmapFilePath;
//		else
//			throw(std::exception((std::string(__FUNCTION__) + std::string("Unknown image type (") + std::to_string((int)type) + ")").c_str()));
//		godot::String fileName = _fileName.c_str();
//
//		if (!fs::exists(_fileName))
//		{
//			ok = false;
//			return nullptr;
//		}
//
//		godot::File* file = godot::File::_new();
//		godot::Error err = file->open(fileName, godot::File::READ);
//		if (err != godot::Error::OK)
//			throw(std::exception((std::string(__FUNCTION__) + std::string("file->open error (") + std::to_string((int)err) + ") : " + _fileName).c_str()));
//		int64_t w = file->get_64();
//		int64_t h = file->get_64();
//		godot::Image::Format f = (godot::Image::Format)file->get_64();
//		int64_t size = file->get_64();
//		godot::PoolByteArray data = file->get_buffer(size);
//		file->close();
//
//		godot::Ref<godot::Image> image = godot::Image::_new();
//		image->create_from_data(w, h, false, f, data);
//
//		ok = true;
//
//		return image;
//	}
//		
//#endif

{
	//bool loadingHeighmapOK = false;
	//bool loadingNormalmapOK = false;
	//{
	//	TheWorld_Utils::GuardProfiler profiler(std::string("WorldDeploy 3.1 ") + __FUNCTION__, "Load images from FS");
	//	m_heightMapImage = m_quadTree->getQuadrant()->getMeshCacheBuffer().readImage(loadingHeighmapOK, TheWorld_Utils::MeshCacheBuffer::ImageType::heightmap);
	//	m_normalMapImage = m_quadTree->getQuadrant()->getMeshCacheBuffer().readImage(loadingNormalmapOK, TheWorld_Utils::MeshCacheBuffer::ImageType::normalmap);
	//}

	//if (!loadingHeighmapOK || !loadingNormalmapOK)
	//{
	//	TheWorld_Utils::GuardProfiler profiler(std::string("WorldDeploy 3.2 ") + __FUNCTION__, "Create Images from Vertex Array");

	//	// Creating Heightmap Map Texture
	//	{
	//		Ref<Image> image = Image::_new();
	//		image->create(_resolution, _resolution, false, Image::FORMAT_RH);
	//		m_heightMapImage = image;
	//	}

	//	// Creating Normal Map Texture
	//	{
	//		Ref<Image> image = Image::_new();
	//		image->create(_resolution, _resolution, false, Image::FORMAT_RGB8);
	//		m_normalMapImage = image;
	//	}

	//	{
	//		std::vector<TheWorld_Utils::GridVertex>& gridVertices = m_quadTree->getQuadrant()->getGridVertices();

	//		// Filling Heightmap Map Texture , Normal Map Texture
	//		assert(_resolution == m_heightMapImage->get_height());
	//		assert(_resolution == m_heightMapImage->get_width());
	//		assert(_resolution == m_quadTree->getQuadrant()->getPos().getNumVerticesPerSize());
	//		float gridStepInWUs = m_viewer->Globals()->gridStepInWU();
	//		m_heightMapImage->lock();
	//		m_normalMapImage->lock();
	//		for (int z = 0; z < _resolution; z++)			// m_heightMapImage->get_height()
	//			for (int x = 0; x < _resolution; x++)		// m_heightMapImage->get_width()
	//			{
	//				//Sleep(0);
	//				float h = gridVertices[z * _resolution + x].altitude();
	//				m_heightMapImage->set_pixel(x, z, Color(h, 0, 0));
	//				//if (h != 0)	// DEBUGRIC
	//				//	m_viewer->Globals()->debugPrint("Altitude not null.X=" + String(std::to_string(x).c_str()) + " Z=" + std::to_string(z).c_str() + " H=" + std::to_string(h).c_str());

	//				// h = height of the point for which we are computing the normal
	//				// hr = height of the point on the rigth
	//				// hl = height of the point on the left
	//				// hf = height of the forward point (z growing)
	//				// hb = height of the backward point (z lessening)
	//				// step = step in WUs between points
	//				// we compute normal normalizing the vector (h - hr, step, h - hf) or (hl - h, step, hb - h)
	//				// according to https://hterrain-plugin.readthedocs.io/en/latest/ section "Procedural generation" it should be (h - hr, step, hf - h)
	//				Vector3 normal;
	//				Vector3 P((float)x, h, (float)z);
	//				if (x < _resolution - 1 && z < _resolution - 1)
	//				{
	//					float hr = gridVertices[z * _resolution + x + 1].altitude();
	//					float hf = gridVertices[(z + 1) * _resolution + x].altitude();
	//					normal = Vector3(h - hr, gridStepInWUs, h - hf).normalized();
	//					//{		// Verify
	//					//	Vector3 PR((float)(x + gridStepInWUs), hr, (float)z);
	//					//	Vector3 PF((float)x, hf, (float)(z + gridStepInWUs));
	//					//	Vector3 normal1 = (PF - P).cross(PR - P).normalized();
	//					//	if (!equal(normal1, normal))	// DEBUGRIC
	//					//		m_viewer->Globals()->debugPrint("Normal=" + String(normal) + " - Normal1= " + String(normal1));
	//					//}
	//				}
	//				else
	//				{
	//					if (x == _resolution - 1 && z == 0)
	//					{
	//						float hf = gridVertices[(z + 1) * _resolution + x].altitude();
	//						float hl = gridVertices[z * _resolution + x - 1].altitude();
	//						normal = Vector3(hl - h, gridStepInWUs, h - hf).normalized();
	//						//{		// Verify
	//						//	Vector3 PL((float)(x - gridStepInWUs), hl, (float)z);
	//						//	Vector3 PF((float)x, hf, (float)(z + gridStepInWUs));
	//						//	Vector3 normal1 = (PL - P).cross(PF - P).normalized();
	//						//	if (!equal(normal1, normal))	// DEBUGRIC
	//						//		m_viewer->Globals()->debugPrint("Normal=" + String(normal) + " - Normal1= " + String(normal1));
	//						//}
	//					}
	//					else if (x == 0 && z == _resolution - 1)
	//					{
	//						float hr = gridVertices[z * _resolution + x + 1].altitude();
	//						float hb = gridVertices[(z - 1) * _resolution + x].altitude();
	//						normal = Vector3(h - hr, gridStepInWUs, hb - h).normalized();
	//						//{		// Verify
	//						//	Vector3 PR((float)(x + gridStepInWUs), hr, (float)z);
	//						//	Vector3 PB((float)(x), hb, (float)(z - gridStepInWUs));
	//						//	Vector3 normal1 = (PR - P).cross(PB - P).normalized();
	//						//	if (!equal(normal1, normal))	// DEBUGRIC
	//						//		m_viewer->Globals()->debugPrint("Normal=" + String(normal) + " - Normal1= " + String(normal1));
	//						//}
	//					}
	//					else
	//					{
	//						float hl = gridVertices[z * _resolution + x - 1].altitude();
	//						float hb = gridVertices[(z - 1) * _resolution + x].altitude();
	//						normal = Vector3(hl - h, gridStepInWUs, hb - h).normalized();
	//						//{		// Verify
	//						//	Vector3 PB((float)x, hb, (float)(z - gridStepInWUs));
	//						//	Vector3 PL((float)(x - gridStepInWUs), hl, (float)z);
	//						//	Vector3 normal1 = (PB - P).cross(PL - P).normalized();
	//						//	if (!equal(normal1, normal))	// DEBUGRIC
	//						//		m_viewer->Globals()->debugPrint("Normal=" + String(normal) + " - Normal1= " + String(normal1));
	//						//}
	//					}
	//				}
	//				Color c = encodeNormal(normal);
	//				m_normalMapImage->set_pixel(x, z, c);
	//			}
	//		m_normalMapImage->unlock();
	//		m_heightMapImage->unlock();

	//		m_quadTree->getQuadrant()->getMeshCacheBuffer().writeImage(m_heightMapImage, TheWorld_Utils::MeshCacheBuffer::ImageType::heightmap);
	//		m_quadTree->getQuadrant()->getMeshCacheBuffer().writeImage(m_normalMapImage, TheWorld_Utils::MeshCacheBuffer::ImageType::normalmap);

	//		//{
	//		//	
	//		//	int64_t h1 = m_heightMapImage->get_height();
	//		//	int64_t w1 = m_heightMapImage->get_width();
	//		//	Image::Format f1 = m_heightMapImage->get_format();

	//		//	PoolByteArray datas1 = m_heightMapImage->get_data();
	//		//	File* file = File::_new();
	//		//	Error err = file->open("C:\\Users\\Riccardo\\appdata\\Roaming\\Godot\\app_userdata\\Godot Proj\\TheWorld\\Cache\\ST-5.000000_SZ-1025\\L-0\\prova.res", File::WRITE);
	//		//	int64_t i64 = datas1.size();
	//		//	file->store_64(i64);
	//		//	file->store_buffer(datas1);
	//		//	file->close();
	//		//	err = file->open("C:\\Users\\Riccardo\\appdata\\Roaming\\Godot\\app_userdata\\Godot Proj\\TheWorld\\Cache\\ST-5.000000_SZ-1025\\L-0\\prova.res", File::READ);
	//		//	i64 = file->get_64();
	//		//	PoolByteArray datas2 = file->get_buffer(i64);
	//		//	file->close();
	//		//	i64 = datas2.size();
	//		//	m_heightMapImage->create_from_data(_resolution, _resolution, false, Image::FORMAT_RH, datas2);

	//		//	int64_t h2 = m_heightMapImage->get_height();
	//		//	int64_t w2 = m_heightMapImage->get_width();
	//		//	Image::Format f2 = m_heightMapImage->get_format();
	//		//}
	//		
	//		
	//		//{
	//		//	int64_t h1 = m_heightMapImage->get_height();
	//		//	int64_t w1 = m_heightMapImage->get_width();
	//		//	Image::Format f1 = m_heightMapImage->get_format();
	//		//	m_heightMapImage->convert(godot::Image::FORMAT_RGB8);
	//		//	//m_heightMapImage = cache.readHeigthmap(loadingHeighmapOK);
	//		//	m_heightMapImage->convert(Image::FORMAT_RH);
	//		//	//m_normalMapImage = cache.readNormalmap(loadingNormalmapOK);
	//		//	//m_normalMapImage->convert(Image::FORMAT_RGB8);
	//		//	int64_t h = m_heightMapImage->get_height();
	//		//	int64_t w = m_heightMapImage->get_width();
	//		//	Image::Format f = m_heightMapImage->get_format();
	//		//	m_heightMapImage->lock();
	//		//	m_normalMapImage->lock();
	//		//	for (int z = 0; z < _resolution; z++)			// m_heightMapImage->get_height()
	//		//		for (int x = 0; x < _resolution; x++)		// m_heightMapImage->get_width()
	//		//		{
	//		//			Color c = m_heightMapImage->get_pixel(x, z);
	//		//			c = m_normalMapImage->get_pixel(x, z);
	//		//		}
	//		//	m_normalMapImage->unlock();
	//		//	m_heightMapImage->unlock();
	//		//}
	//	}
	//}
}

//void GDN_TheWorld_Globals_Client::MapManagerCalcNextCoordOnTheGridInWUs(std::vector<float>& inCoords, std::vector<float>& outCoords)
//{
//	std::vector<ClientServerVariant> replyParams;
//	std::vector<ClientServerVariant> inputParams;
//	for (size_t idx = 0; idx < inCoords.size(); idx++)
//		inputParams.push_back(inCoords[idx]);
//	int rc = execMethodSync(THEWORLD_CLIENTSERVER_METHOD_MAPM_CALCNEXTCOORDGETVERTICES, inputParams, replyParams);
//	if (rc != THEWORLD_CLIENTSERVER_RC_OK)
//	{
//		std::string m = std::string("ClientInterface::execMethodSync ==> MapManager::calcNextCoordOnTheGridInWUs error ") + std::to_string(rc);
//		throw(GDN_TheWorld_Exception(__FUNCTION__, m.c_str(), "", rc));
//	}
//	if (replyParams.size() != inCoords.size())
//	{
//		std::string m = std::string("execMethodSync ==> MapManager::calcNextCoordOnTheGridInWUs error (not enough params replied)") + std::to_string(rc);
//		throw(GDN_TheWorld_Exception(__FUNCTION__, m.c_str()));
//	}

//	for (size_t idx = 0; idx < inCoords.size(); idx++)
//	{
//		const auto ptr(std::get_if<float>(&replyParams[idx]));
//		if (ptr == NULL)
//		{
//			std::string m = std::string("execMethodSync ==> MapManager::calcNextCoordOnTheGridInWUs did not return a float");
//			throw(GDN_TheWorld_Exception(__FUNCTION__, m.c_str()));
//		}
//		outCoords.push_back(*ptr);
//	}
//}

			//else if (method == THEWORLD_CLIENTSERVER_METHOD_MAPM_CALCNEXTCOORDGETVERTICES)
			//{
			//	for (size_t i = 0; i < reply->m_inputParams.size(); i++)
			//	{
			//		float* coord = std::get_if<float>(&reply->m_inputParams[i]);
			//		if (coord == nullptr)
			//		{
			//			reply->replyError(THEWORLD_CLIENTSERVER_RC_INPUT_PARAM_ERROR, "First param must be a FLOAT");
			//			return;
			//		}
			//		float f = m_mapManager->calcNextCoordOnTheGridInWUs(*coord);
			//		reply->replyParam(f);
			//	}
			//	reply->replyComplete();
			//}

	//float GDN_TheWorld_Globals_Client::MapManagerGridStepInWU()
	//{
	//	std::vector<ClientServerVariant> replyParams;
	//	std::vector<ClientServerVariant> inputParams;
	//	int rc = execMethodSync("MapManager::gridStepInWU", inputParams, replyParams);
	//	if (rc != THEWORLD_CLIENTSERVER_RC_OK)
	//	{
	//		std::string m = std::string("execMethodSync ==> MapManager::gridStepInWU error ") + std::to_string(rc);
	//		throw(GDN_TheWorld_Exception(__FUNCTION__, m.c_str(), "", rc));
	//	}
	//	if (replyParams.size() == 0)
	//	{
	//		std::string m = std::string("execMethodSync ==> MapManager::gridStepInWU error ") + std::to_string(rc);
	//		throw(GDN_TheWorld_Exception(__FUNCTION__, m.c_str(), "No param replied", rc));
	//	}

	//	const auto ptr(std::get_if<float>(&replyParams[0]));
	//	if (ptr == NULL)
	//	{
	//		std::string m = std::string("execMethodSync ==> MapManager::gridStepInWU did not return a float");
	//		throw(GDN_TheWorld_Exception(__FUNCTION__, m.c_str()));
	//	}
	//	
	//	return *ptr;
	//}


//String GDN_TheWorld_Viewer::getTrackedChunkStr(void)
//{
//	std::string strTrackedChunk;
//
//	Chunk* trackedChunk = getTrackedChunk();
//	if (trackedChunk != nullptr)
//		strTrackedChunk = trackedChunk->getQuadTree()->getQuadrant()->getPos().getName() + "-" + trackedChunk->getPos().getIdStr();
//
//	return strTrackedChunk.c_str();
//}

//Chunk* GDN_TheWorld_Viewer::getTrackedChunk(void)
//{
//	GDN_TheWorld_Camera* activeCamera = WorldCamera()->getActiveCamera();
//	if (!activeCamera)
//		return nullptr;
//
//	Vector2 mousePos = get_viewport()->get_mouse_position();
//
//	Chunk* chunk = nullptr;
//
//	std::lock_guard<std::recursive_mutex> lock(m_mtxQuadTree);
//	for (MapQuadTree::iterator itQuadTree = m_mapQuadTree.begin(); itQuadTree != m_mapQuadTree.end(); itQuadTree++)
//	{
//		if (!itQuadTree->second->isValid())
//			continue;
//
//		if (!itQuadTree->second->isVisible())
//			continue;
//
//		std::vector<TheWorld_Viewer_Utils::GridVertex>& vertices = itQuadTree->second->getQuadrant()->getGridVertices();
//
//		Point2 p1 = activeCamera->unproject_position(Vector3(vertices[0].posX(), vertices[0].altitude(), vertices[0].posZ()));
//		Point2 p2 = activeCamera->unproject_position(Vector3(vertices[vertices.size() - 1].posX(), vertices[vertices.size() - 1].altitude(), vertices[vertices.size() - 1].posZ()));
//		
//		//Rect2 rect(p1, p2 - p1);
//		//if (rect.has_point(mousePos))
//		float greaterX = (p1.x > p2.x ? p1.x : p2.x);
//		float greaterY = (p1.y > p2.y ? p1.y : p2.y);
//		float lowerX = (p1.x < p2.x ? p1.x : p2.x);
//		float lowerY = (p1.y < p2.y ? p1.y : p2.y);
//		if (mousePos.x >= lowerX && mousePos.x < greaterX && mousePos.y >= lowerY && mousePos.y < greaterY)
//		{
//			Chunk::MapChunk& mapChunk = itQuadTree->second->getMapChunk();
//
//			Chunk::MapChunk::iterator itMapChunk;
//			for (Chunk::MapChunk::iterator itMapChunk = mapChunk.begin(); itMapChunk != mapChunk.end(); itMapChunk++)
//				for (Chunk::MapChunkPerLod::iterator itMapChunkPerLod = itMapChunk->second.begin(); itMapChunkPerLod != itMapChunk->second.end(); itMapChunkPerLod++)
//					if (itMapChunkPerLod->second->isActive())
//					{
//						int numWorldVerticesPerSize = itQuadTree->second->getQuadrant()->getPos().getNumVerticesPerSize();
//
//						size_t idxFirstVertexOfChunk = itMapChunkPerLod->second->getFirstWorldVertRow() * numWorldVerticesPerSize + itMapChunkPerLod->second->getFirstWorldVertCol();
//						Point2 p1 = activeCamera->unproject_position(Vector3(vertices[idxFirstVertexOfChunk].posX(), vertices[idxFirstVertexOfChunk].altitude(), vertices[idxFirstVertexOfChunk].posZ()));
//						size_t idxLastVertexOfChunk = itMapChunkPerLod->second->getLastWorldVertRow() * numWorldVerticesPerSize + itMapChunkPerLod->second->getLastWorldVertCol();
//						Point2 p2 = activeCamera->unproject_position(Vector3(vertices[idxLastVertexOfChunk].posX(), vertices[idxLastVertexOfChunk].altitude(), vertices[idxLastVertexOfChunk].posZ()));
//
//						//Rect2 rect(p1, p2 - p1);
//						//if (rect.has_point(mousePos))
//						greaterX = (p1.x > p2.x ? p1.x : p2.x);
//						greaterY = (p1.y > p2.y ? p1.y : p2.y);
//						lowerX = (p1.x < p2.x ? p1.x : p2.x);
//						lowerY = (p1.y < p2.y ? p1.y : p2.y);
//						if (mousePos.x >= lowerX && mousePos.x < greaterX && mousePos.y >= lowerY && mousePos.y < greaterY)
//						{
//							chunk = itMapChunkPerLod->second;
//							break;
//						}
//					}
//
//			break;
//		}
//	}
//
//	return chunk;
//}

//void GDN_TheWorld_Viewer::viewportResizing(void)
//{
//	resizeEditModeUI();
//}

	//Control* _editModeUIControl = EditModeUIControl();
	//if (_editModeUIControl != nullptr)
	//	_editModeUIControl->queue_free();

	//_editModeUIControl = godot::MarginContainer::_new();
	//if (_editModeUIControl == nullptr)
	//	throw(GDN_TheWorld_Exception(__FUNCTION__, std::string("Create Control error!").c_str()));
	//get_parent()->add_child(_editModeUIControl);
	//_editModeUIControl->set_name(THEWORLD_EDIT_MODE_UI_CONTROL_NAME);
	////_editModeUIControl->set_size(godot::Vector2(150, 0));
	//resizeEditModeUI();

	//	Control* panelContainer = godot::PanelContainer::_new();
	//	if (panelContainer == nullptr)
	//		throw(GDN_TheWorld_Exception(__FUNCTION__, std::string("Create Control error!").c_str()));
	//	_editModeUIControl->add_child(panelContainer);

	//		Control* vboxContainer = godot::VBoxContainer::_new();
	//		if (vboxContainer == nullptr)
	//			throw(GDN_TheWorld_Exception(__FUNCTION__, std::string("Create Control error!").c_str()));
	//		panelContainer->add_child(vboxContainer);

	//			Control* marginContainer = godot::MarginContainer::_new();
	//			if (marginContainer == nullptr)
	//				throw(GDN_TheWorld_Exception(__FUNCTION__, std::string("Create Control error!").c_str()));
	//			vboxContainer->add_child(marginContainer);
	//			
	//				godot::Label* label = godot::Label::_new();
	//				if (label == nullptr)
	//					throw(GDN_TheWorld_Exception(__FUNCTION__, std::string("Create Control error!").c_str()));
	//				marginContainer->add_child(label);
	//				label->set_text("Edit Mode");
	//				label->set_align(godot::Label::Align::ALIGN_CENTER);

	//			marginContainer = godot::MarginContainer::_new();
	//			if (marginContainer == nullptr)
	//				throw(GDN_TheWorld_Exception(__FUNCTION__, std::string("Create Control error!").c_str()));
	//			vboxContainer->add_child(marginContainer);
	//			//marginContainer->set_margin(GDN_TheWorld_Viewer::Margin::MARGIN_RIGHT, 50.0);
	//			//marginContainer->set_margin(GDN_TheWorld_Viewer::Margin::MARGIN_TOP, 5.0);
	//			//marginContainer->set_margin(GDN_TheWorld_Viewer::Margin::MARGIN_LEFT, 50.0);
	//			//marginContainer->set_margin(GDN_TheWorld_Viewer::Margin::MARGIN_BOTTOM, 5.0);

	//				godot::Button* button = godot::Button::_new();
	//				if (button == nullptr)
	//					throw(GDN_TheWorld_Exception(__FUNCTION__, std::string("Create Control error!").c_str()));
	//				marginContainer->add_child(button);
	//				button->set_text("Generate");
	//				button->connect("pressed", this, "edit_mode_generate");

//void GDN_TheWorld_Viewer::resizeEditModeUI(void)
//{
//	Control* editModeUIControl = EditModeUIControl();
//	if (editModeUIControl != nullptr)
//	{
//		editModeUIControl->set_anchor(GDN_TheWorld_Viewer::Margin::MARGIN_RIGHT, 1.0);
//		editModeUIControl->set_margin(GDN_TheWorld_Viewer::Margin::MARGIN_RIGHT, 0.0);
//		editModeUIControl->set_margin(GDN_TheWorld_Viewer::Margin::MARGIN_TOP, 0.0);
//		editModeUIControl->set_margin(GDN_TheWorld_Viewer::Margin::MARGIN_LEFT, get_viewport()->get_size().x - 150);
//		editModeUIControl->set_margin(GDN_TheWorld_Viewer::Margin::MARGIN_BOTTOM, 0.0);
//	}
//}

//void GDN_TheWorld_Viewer::editModeGenerate(void)
//{
//	resizeEditModeUI();
//}

	//register_method("viewport_resizing", &GDN_TheWorld_Viewer::viewportResizing);
	//register_method("edit_mode_generate", &GDN_TheWorld_Viewer::editModeGenerate);
