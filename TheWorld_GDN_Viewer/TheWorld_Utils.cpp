//#include "pch.h"
#ifdef _THEWORLD_CLIENT
	#include <Godot.hpp>
	#include <ResourceLoader.hpp>
	#include <File.hpp>
#endif
#include "TheWorld_Utils.h"
#include <filesystem>
#include <string>

namespace fs = std::filesystem;

namespace TheWorld_Utils
{
	MeshCacheBuffer::MeshCacheBuffer(void)
	{
		m_gridStepInWU = -1;
		m_numVerticesPerSize = -1;
		m_level = 0;
		m_lowerXGridVertex = 0;
		m_lowerZGridVertex = 0;
	}

	MeshCacheBuffer::MeshCacheBuffer(std::string cacheDir, float gridStepInWU, size_t numVerticesPerSize, int level, float lowerXGridVertex, float lowerZGridVertex)
	{
		m_gridStepInWU = gridStepInWU;
		m_numVerticesPerSize = numVerticesPerSize;
		m_level = level;
		m_lowerXGridVertex = lowerXGridVertex;
		m_lowerZGridVertex = lowerZGridVertex;

		m_cacheDir = std::string(cacheDir) + "\\" + "Cache" + "\\" + "ST-" + std::to_string(gridStepInWU) + "_SZ-" + std::to_string(numVerticesPerSize) + "\\L-" + std::to_string(level);
		if (!fs::exists(m_cacheDir))
		{
			fs::create_directories(m_cacheDir);
		}
		std::string cacheFileName = "X-" + std::to_string(lowerXGridVertex) + "_Z-" + std::to_string(lowerZGridVertex) + ".mesh";
		m_meshFilePath = m_cacheDir + "\\" + cacheFileName;

#ifdef _THEWORLD_CLIENT
		std::string heightmapFileName = "X-" + std::to_string(lowerXGridVertex) + "_Z-" + std::to_string(lowerZGridVertex) + "_heightmap.res";
		m_heightmapFilePath = m_cacheDir + "\\" + heightmapFileName;
		std::string normalmapFileName = "X-" + std::to_string(lowerXGridVertex) + "_Z-" + std::to_string(lowerZGridVertex) + "_normalmap.res";
		m_normalmapFilePath = m_cacheDir + "\\" + normalmapFileName;
#endif
	}
	
	std::string MeshCacheBuffer::getMeshIdFromMeshCache(void)
	{
		if (!fs::exists(m_meshFilePath))
			return "";
		
		BYTE shortBuffer[256 + 1];
		size_t size;

		FILE* inFile = nullptr;
		errno_t err = fopen_s(&inFile, m_meshFilePath.c_str(), "rb");
		if (err != 0)
			throw(GDN_TheWorld_Exception(__FUNCTION__, std::string("Open " + m_meshFilePath + " in errore - Err=" + std::to_string(err)).c_str()));

		if (fread(shortBuffer, 1, 1, inFile) != 1)	// "0"
		{
			fclose(inFile);
			throw(GDN_TheWorld_Exception(__FUNCTION__, std::string("Read error 1!").c_str()));
		}

		// get size of a size_t
		size_t size_tSize;
		TheWorld_Utils::serializeToByteStream<size_t>(0, shortBuffer, size_tSize);

		// read the serialized size of the mesh id
		if (fread(shortBuffer, size_tSize, 1, inFile) != 1)
		{
			fclose(inFile);
			throw(GDN_TheWorld_Exception(__FUNCTION__, std::string("Read error 2!").c_str()));
		}
		// and deserialize it
		size_t meshIdSize = TheWorld_Utils::deserializeFromByteStream<size_t>(shortBuffer, size);

		// read the mesh id
		if (fread(shortBuffer, meshIdSize, 1, inFile) != 1)
		{
			fclose(inFile);
			throw(GDN_TheWorld_Exception(__FUNCTION__, std::string("Read error 3!").c_str()));
		}
		// and deserialize it
		m_meshId = std::string((char*)shortBuffer, meshIdSize);

		fclose(inFile);

		return m_meshId;
	}

	void MeshCacheBuffer::refreshVerticesFromBuffer(std::string buffer, std::string& meshIdFromBuffer, std::vector<TheWorld_Utils::GridVertex>& vectGridVertices, void* heigths, float& minY, float& maxY)
	{
		size_t size = 0;

		const char* movingStreamBuffer = buffer.c_str();
		const char* endOfBuffer = movingStreamBuffer + buffer.size();

		movingStreamBuffer++;	// bypass "0"

		size_t meshIdSize = TheWorld_Utils::deserializeFromByteStream<size_t>((BYTE*)movingStreamBuffer, size);
		movingStreamBuffer += size;

		meshIdFromBuffer = std::string(movingStreamBuffer, meshIdSize);
		movingStreamBuffer += meshIdSize;

		m_meshId = meshIdFromBuffer;

		size_t vectSize = TheWorld_Utils::deserializeFromByteStream<size_t>((BYTE*)movingStreamBuffer, size);
		movingStreamBuffer += size;

#ifdef _THEWORLD_CLIENT
		godot::PoolRealArray* heigthsP = (godot::PoolRealArray*)heigths;
		heigthsP->resize((int)vectSize);
#endif

		vectGridVertices.clear();
		vectGridVertices.resize((int)vectSize);

		bool first = true;
		int idx = 0;
		if (vectSize > 0)
		{
			while (movingStreamBuffer < endOfBuffer)
			{
				if (idx >= vectSize)
					throw(GDN_TheWorld_Exception(__FUNCTION__, std::string("Length of buffer inconsistent, idx=" + std::to_string(idx) + " vectSize=" + std::to_string(vectSize)).c_str())); 

				TheWorld_Utils::GridVertex v = TheWorld_Utils::GridVertex((BYTE*)movingStreamBuffer, size);
				//vectGridVertices.push_back(v);
				vectGridVertices[idx] = v;
#ifdef _THEWORLD_CLIENT
				heigthsP->set(idx, v.altitude());
#endif
				if (first)
				{
					minY = maxY = v.altitude();
					first = false;
				}
				else
				{
					if (v.altitude() > maxY)
						maxY = v.altitude();
					if (v.altitude() < minY)
						minY = v.altitude();
				}
				idx++;
				movingStreamBuffer += size;
			}

#ifdef _THEWORLD_CLIENT
			{
				// SUPERDEBUGRIC
				//TheWorld_Utils::TimerMs clock("MeshCacheBuffer::refreshMeshCacheFromBuffer", m_meshId.c_str(), false, true);
								
				//clock.tick();
				//godot::ResourceLoader* resLoader = godot::ResourceLoader::get_singleton();
				//godot::Ref<godot::Image> image = resLoader->load("res://height.res");
				//int res = (int)image->get_width();
				//image->lock();
				//for (int z = 0; z < res; z++)
				//	for (int x = 0; x < res; x++)
				//	{
				//		godot::Color c = image->get_pixel(x, z);
				//		TheWorld_Utils::GridVertex& v = vectGridVertices[z * res + x];
				//		v.setAltitude(c.r * 3);
				//	}
				//image->unlock();
				//clock.headerMsg("Mock Loop to replace altitudes" + m_meshId);
				//clock.tock();

				//clock.tick();
				//setBufferForMeshCache(meshId, res, vectGridVertices, buffer);
				//clock.headerMsg("Mock setBufferForMeshCache" + m_meshId);
				//clock.tock();
				// SUPERDEBUGRIC
			}
#endif
			if (vectGridVertices.size() != vectSize)
				throw(GDN_TheWorld_Exception(__FUNCTION__, std::string("Sequence error 4!").c_str()));

			FILE* outFile = nullptr;
			errno_t err = fopen_s(&outFile, m_meshFilePath.c_str(), "wb");
			if (err != 0)
			{
				throw(GDN_TheWorld_Exception(__FUNCTION__, std::string("Open " + m_meshFilePath + " in errore - Err=" + std::to_string(err)).c_str()));
			}

			if (fwrite(buffer.c_str(), buffer.size(), 1, outFile) != 1)
			{
				fclose(outFile);
				throw(GDN_TheWorld_Exception(__FUNCTION__, std::string("Write error 3!").c_str()));
			}

			fclose(outFile);

#ifdef _THEWORLD_CLIENT
			if (fs::exists(m_heightmapFilePath))
				fs::remove(m_heightmapFilePath);
			if (fs::exists(m_normalmapFilePath))
				fs::remove(m_normalmapFilePath);
#endif
		}
	}

	void MeshCacheBuffer::readBufferFromMeshCache(std::string _meshId, std::string& buffer, size_t& vectSizeFromCache)
	{
		BYTE shortBuffer[256 + 1];
		size_t size;

		// read vertices from local cache
		if (!fs::exists(m_meshFilePath))
			throw(GDN_TheWorld_Exception(__FUNCTION__, std::string("not found current quadrant in cache").c_str()));

		FILE* inFile = nullptr;
		errno_t err = fopen_s(&inFile, m_meshFilePath.c_str(), "rb");
		if (err != 0)
			throw(GDN_TheWorld_Exception(__FUNCTION__, std::string("Open " + m_meshFilePath + " in errore - Err=" + std::to_string(err)).c_str()));

		if (fread(shortBuffer, 1, 1, inFile) != 1)	// "0"
		{
			fclose(inFile);
			throw(GDN_TheWorld_Exception(__FUNCTION__, std::string("Read error 1!").c_str()));
		}

		// get size of a size_t
		size_t size_tSize;
		TheWorld_Utils::serializeToByteStream<size_t>(0, shortBuffer, size_tSize);

		// read the serialized size of the mesh id
		if (fread(shortBuffer, size_tSize, 1, inFile) != 1)
		{
			fclose(inFile);
			throw(GDN_TheWorld_Exception(__FUNCTION__, std::string("Read error 2!").c_str()));
		}
		// and deserialize it
		size_t meshIdSize = TheWorld_Utils::deserializeFromByteStream<size_t>(shortBuffer, size);

		// read the mesh id
		if (fread(shortBuffer, meshIdSize, 1, inFile) != 1)
		{
			fclose(inFile);
			throw(GDN_TheWorld_Exception(__FUNCTION__, std::string("Read error 3!").c_str()));
		}
		// and deserialize it
		std::string meshId((char*)shortBuffer, meshIdSize);

		if (meshId != _meshId)
		{
			fclose(inFile);
			throw(GDN_TheWorld_Exception(__FUNCTION__, (std::string("meshId from cache (") + meshId + ") not equal to meshId from server (" + _meshId).c_str()));
		}

		m_meshId = meshId;

		// read the serialized size of the vector of GridVertex
		if (fread(shortBuffer, size_tSize, 1, inFile) != 1)
		{
			fclose(inFile);
			throw(GDN_TheWorld_Exception(__FUNCTION__, std::string("Read error 4!").c_str()));
		}
		// and deserialize it
		vectSizeFromCache = TheWorld_Utils::deserializeFromByteStream<size_t>(shortBuffer, size);

		// Serialize an empty GridVertex only to obtain the size of a serialized GridVertex
		size_t serializedVertexSize = 0;
		TheWorld_Utils::GridVertex v;
		v.serialize(shortBuffer, serializedVertexSize);

		// alloc buffer to contain the serialized entire vector of GridVertex
		size_t streamBufferSize = 1 /* "0" */ + size_tSize + meshId.length() + size_tSize /* the size of a size_t */ + vectSizeFromCache * serializedVertexSize;
		BYTE* streamBuffer = (BYTE*)calloc(1, streamBufferSize);
		if (streamBuffer == nullptr)
		{
			fclose(inFile);
			throw(GDN_TheWorld_Exception(__FUNCTION__, std::string("Allocation error!").c_str()));
		}
		
		// reposition to the beginning of file
		int ret = fseek(inFile, 0, SEEK_SET);
		if (ret != 0)
		{
			fclose(inFile);
			::free(streamBuffer);
			throw(GDN_TheWorld_Exception(__FUNCTION__, std::string("fseek to beginning of file error!").c_str()));
		}

		size_t s = fread(streamBuffer, streamBufferSize, 1, inFile);

		fclose(inFile);

		buffer = std::string((char*)streamBuffer, streamBufferSize);

		::free(streamBuffer);
	}
		
	void MeshCacheBuffer::readVerticesFromMeshCache(std::string _meshId, std::vector<TheWorld_Utils::GridVertex>& vectGridVertices, void* heigths, float& minY, float& maxY)
	{
		std::string buffer;
		size_t vectSizeFromCache;
		size_t size;
		
		readBufferFromMeshCache(_meshId, buffer, vectSizeFromCache);

		BYTE* movingStreamBuffer = (BYTE *)buffer.c_str();
		BYTE* endOfBuffer = (BYTE*)movingStreamBuffer + buffer.size();

		movingStreamBuffer++;	// bypass "0"

		size_t meshIdSize = TheWorld_Utils::deserializeFromByteStream<size_t>((BYTE*)movingStreamBuffer, size);
		movingStreamBuffer += size;

		std::string meshId = std::string((char*)movingStreamBuffer, meshIdSize);
		movingStreamBuffer += meshIdSize;

		if (meshId != _meshId)
		{
			throw(GDN_TheWorld_Exception(__FUNCTION__, (std::string("meshId from cache (") + meshId + ") not equal to meshId in input (" + _meshId).c_str()));
		}

		m_meshId = meshId;

		size_t vectSize = TheWorld_Utils::deserializeFromByteStream<size_t>((BYTE*)movingStreamBuffer, size);
		//size_t heightsArraySize = (m_numVerticesPerSize * m_gridStepInWU) * (m_numVerticesPerSize * m_gridStepInWU);
		movingStreamBuffer += size;
		
#ifdef _THEWORLD_CLIENT
		godot::PoolRealArray* heigthsP = (godot::PoolRealArray*)heigths;
		heigthsP->resize((int)vectSize);
#endif

		vectGridVertices.clear();
		vectGridVertices.resize((int)vectSize);

		bool first = true;
		int idx = 0;
		while (movingStreamBuffer < endOfBuffer)
		{
			if (idx >= vectSize)
				throw(GDN_TheWorld_Exception(__FUNCTION__, std::string("Length of buffer inconsistent, idx=" + std::to_string(idx) + " vectSize=" + std::to_string(vectSize)).c_str())); 

			TheWorld_Utils::GridVertex v = TheWorld_Utils::GridVertex((BYTE*)movingStreamBuffer, size);
			//vectGridVertices.push_back(v);
			vectGridVertices[idx] = v;
#ifdef _THEWORLD_CLIENT
			heigthsP->set(idx, v.altitude());
#endif
			if (first)
			{
				minY = maxY = v.altitude();
				first = false;
			}
			else
			{
				if (v.altitude() > maxY)
					maxY = v.altitude();
				if (v.altitude() < minY)
					minY = v.altitude();
			}
			idx++;
			movingStreamBuffer += size;
		}

		if (vectGridVertices.size() != vectSizeFromCache || vectGridVertices.size() != vectSize)
			throw(GDN_TheWorld_Exception(__FUNCTION__, std::string("Sequence error 4!").c_str()));
	}

	void MeshCacheBuffer::writeBufferToMeshCache(std::string buffer)
	{
		FILE* outFile = nullptr;
		errno_t err = fopen_s(&outFile, m_meshFilePath.c_str(), "wb");
		if (err != 0)
		{
			throw(GDN_TheWorld_Exception(__FUNCTION__, std::string("Open " + m_meshFilePath + " in errore - Err=" + std::to_string(err)).c_str()));
		}

		if (fwrite(buffer.c_str(), buffer.size(), 1, outFile) != 1)
		{
			fclose(outFile);
			throw(GDN_TheWorld_Exception(__FUNCTION__, std::string("Write error 3!").c_str()));
		}

		fclose(outFile);

#ifdef _THEWORLD_CLIENT
		if (fs::exists(m_heightmapFilePath))
			fs::remove(m_heightmapFilePath);
		if (fs::exists(m_normalmapFilePath))
			fs::remove(m_normalmapFilePath);
#endif
	}
		
	void MeshCacheBuffer::setBufferForMeshCache(std::string meshId, size_t numVerticesPerSize, std::vector<TheWorld_Utils::GridVertex>& vectGridVertices, std::string& buffer)
	{
		BYTE shortBuffer[256 + 1];

		// get size of a size_t
		size_t size_tSize;
		TheWorld_Utils::serializeToByteStream<size_t>(0, shortBuffer, size_tSize);

		// Serialize an empty GridVertex only to obtain the size of a serialized GridVertex
		size_t serializedVertexSize = 0;
		TheWorld_Utils::GridVertex v;
		v.serialize(shortBuffer, serializedVertexSize);

		size_t vectSize = vectGridVertices.size();
		size_t streamBufferSize = 1 /* "0" */ + size_tSize + meshId.length() + size_tSize /* the size of a size_t */ + vectSize * serializedVertexSize;

		BYTE* streamBuffer = (BYTE*)calloc(1, streamBufferSize);
		if (streamBuffer == nullptr)
			throw(std::exception((std::string(__FUNCTION__) + std::string("Allocation error")).c_str()));

		size_t streamBufferIterator = 0;
		memcpy(streamBuffer + streamBufferIterator, "0", 1);
		streamBufferIterator++;

		size_t size = 0;
		TheWorld_Utils::serializeToByteStream<size_t>(meshId.length(), streamBuffer + streamBufferIterator, size);
		streamBufferIterator += size;

		memcpy(streamBuffer + streamBufferIterator, meshId.c_str(), meshId.length());
		streamBufferIterator += meshId.length();

		size = 0;
		TheWorld_Utils::serializeToByteStream<size_t>(vectSize, streamBuffer + streamBufferIterator, size);
		streamBufferIterator += size;

		if (vectSize != 0)
			for (int z = 0; z < numVerticesPerSize; z++)			// m_heightMapImage->get_height()
				for (int x = 0; x < numVerticesPerSize; x++)		// m_heightMapImage->get_width()
				{
					TheWorld_Utils::GridVertex& v = vectGridVertices[z * numVerticesPerSize + x];
					v.serialize(streamBuffer + streamBufferIterator, size);
					streamBufferIterator += size;
				}

		buffer = std::string((char*)streamBuffer, streamBufferIterator);

		free(streamBuffer);

		m_meshId = meshId;
	}

#ifdef _THEWORLD_CLIENT

	void MeshCacheBuffer::writeImage(godot::Ref<godot::Image> image, enum class ImageType type)
	{
		std::string _fileName;
		if (type == ImageType::heightmap)
			_fileName = m_heightmapFilePath;
		else if (type == ImageType::normalmap)
			_fileName = m_normalmapFilePath;
		else
			throw(std::exception((std::string(__FUNCTION__) + std::string("Unknown image type (") + std::to_string((int)type) + ")").c_str()));
		godot::String fileName = _fileName.c_str();

		int64_t w = image->get_width();
		int64_t h = image->get_height();
		godot::Image::Format f = image->get_format();
		godot::PoolByteArray data = image->get_data();
		int64_t size = data.size();
		godot::File* file = godot::File::_new();
		godot::Error err = file->open(fileName, godot::File::WRITE);
		if (err != godot::Error::OK)
			throw(std::exception((std::string(__FUNCTION__) + std::string("file->open error (") + std::to_string((int)err) + ") : " + _fileName).c_str()));
		file->store_64(w);
		file->store_64(h);
		file->store_64((int64_t)f);
		file->store_64(size);
		file->store_buffer(data);
		file->close();
	}

	godot::Ref<godot::Image> MeshCacheBuffer::readImage(bool& ok, enum class ImageType type)
	{
		std::string _fileName;
		if (type == ImageType::heightmap)
			_fileName = m_heightmapFilePath;
		else if (type == ImageType::normalmap)
			_fileName = m_normalmapFilePath;
		else
			throw(std::exception((std::string(__FUNCTION__) + std::string("Unknown image type (") + std::to_string((int)type) + ")").c_str()));
		godot::String fileName = _fileName.c_str();

		if (!fs::exists(_fileName))
		{
			ok = false;
			return nullptr;
		}

		godot::File* file = godot::File::_new();
		godot::Error err = file->open(fileName, godot::File::READ);
		if (err != godot::Error::OK)
			throw(std::exception((std::string(__FUNCTION__) + std::string("file->open error (") + std::to_string((int)err) + ") : " + _fileName).c_str()));
		int64_t w = file->get_64();
		int64_t h = file->get_64();
		godot::Image::Format f = (godot::Image::Format)file->get_64();
		int64_t size = file->get_64();
		godot::PoolByteArray data = file->get_buffer(size);
		file->close();

		godot::Ref<godot::Image> image = godot::Image::_new();
		image->create_from_data(w, h, false, f, data);

		ok = true;

		return image;
	}
		
	//void MeshCacheBuffer::writeHeightmap(godot::Ref<godot::Image> heightMapImage)
	//{
	//	godot::Error err = heightMapImage->save_png(godot::String(m_heightmapFilePath.c_str()));
	//	if (err != godot::Error::OK)
	//		throw(std::exception((std::string(__FUNCTION__) + std::string("save_png heightmap error") + std::to_string((int)err)).c_str()));
	//}

	//void MeshCacheBuffer::writeNormalmap(godot::Ref<godot::Image> normalMapImage)
	//{
	//	godot::Error err = normalMapImage->save_png(godot::String(m_normalmapFilePath.c_str()));
	//	if (err != godot::Error::OK)
	//		throw(std::exception((std::string(__FUNCTION__) + std::string("save_png normalmap error") + std::to_string((int)err)).c_str()));
	//}

	//godot::Ref<godot::Image> MeshCacheBuffer::readHeigthmap(bool& ok)
	//{
	//	ok = true;

	//	if (!fs::exists(m_heightmapFilePath))
	//	{
	//		ok = false;
	//		return nullptr;
	//	}

	//	godot::Ref<godot::Image> heightMapImage = godot::Image::_new();
	//	godot::Error err = heightMapImage->load(godot::String(m_heightmapFilePath.c_str()));
	//	if (err != godot::Error::OK)
	//		throw(std::exception((std::string(__FUNCTION__) + std::string("load heightmap error") + std::to_string((int)err)).c_str()));

	//	return heightMapImage;
	//}

	//godot::Ref<godot::Image> MeshCacheBuffer::readNormalmap(bool& ok)
	//{
	//	ok = true;

	//	if (!fs::exists(m_normalmapFilePath))
	//	{
	//		ok = false;
	//		return nullptr;
	//	}

	//	godot::Ref<godot::Image> normalMapImage = godot::Image::_new();
	//	godot::Error err = normalMapImage->load(godot::String(m_normalmapFilePath.c_str()));
	//	if (err != godot::Error::OK)
	//		throw(std::exception((std::string(__FUNCTION__) + std::string("load normalmap error") + std::to_string((int)err)).c_str()));

	//	return normalMapImage;
	//}

#endif

	void ThreadPool::Start(size_t num_threads, /*const std::function<void()>* threadInitFunction, const std::function<void()>* threadDeinitFunction,*/ ThreadInitDeinit* threadInitDeinit)
	{
		m_threadInitDeinit = threadInitDeinit;
		//m_threadInitFunction = threadInitFunction;
		//m_threadDeinitFunction = threadDeinitFunction;
		uint32_t _num_threads = (uint32_t)num_threads;
		if (_num_threads <= 0)
			_num_threads  = std::thread::hardware_concurrency(); // Max # of threads the system supports
		//const uint32_t num_threads = 2;
		threads.resize(_num_threads);
		for (uint32_t i = 0; i < _num_threads; i++)
		{
			threads.at(i) = std::thread(&ThreadPool::ThreadLoop, this);
		}
	}
	
	void ThreadPool::ThreadLoop()
	{
		if (m_threadInitDeinit != nullptr)
			m_threadInitDeinit->threadInit();
		//if (m_threadInitFunction != nullptr)
		//{
		//	(*m_threadInitFunction)();
		//}

		while (true)
		{
			std::function<void()> job;
			{
				std::unique_lock<std::mutex> lock(queue_mutex);
				mutex_condition.wait(lock, [this] { return !jobs.empty() || should_terminate; });
				if (should_terminate)
				{
					break;
				}
				job = jobs.front();
				jobs.pop();
			}
			job();
		}

		if (m_threadInitDeinit != nullptr)
			m_threadInitDeinit->threadDeinit();
		//if (m_threadDeinitFunction != nullptr)
		//{
		//	(*m_threadDeinitFunction)();
		//}
	}

	void ThreadPool::QueueJob(const std::function<void()>& job)
	{
		{
			std::unique_lock<std::mutex> lock(queue_mutex);
			jobs.push(job);
		}
		mutex_condition.notify_one();
	}
	
	bool ThreadPool::busy()
	{
		bool poolbusy;
		{
			std::unique_lock<std::mutex> lock(queue_mutex);
			poolbusy = !jobs.empty();
		}
		return poolbusy;
	}

	void ThreadPool::Stop()
	{
		{
			std::unique_lock<std::mutex> lock(queue_mutex);
			should_terminate = true;
		}
		mutex_condition.notify_all();
		for (std::thread& active_thread : threads) {
			active_thread.join();
		}
		threads.clear();
	}
	
	std::string Utils::ReplaceString(std::string subject, const std::string& search, const std::string& replace)
	{
		size_t pos = 0;
		while ((pos = subject.find(search, pos)) != std::string::npos)
		{
			subject.replace(pos, search.length(), replace);
			pos += replace.length();
		}
		return subject;
	}

	void Utils::ReplaceStringInPlace(std::string& subject, const std::string& search, const std::string& replace)
	{
		size_t pos = 0;
		while ((pos = subject.find(search, pos)) != std::string::npos)
		{
			subject.replace(pos, search.length(), replace);
			pos += replace.length();
		}
	}

	std::string ToString(GUID* guid)
	{
		char guid_string[37]; // 32 hex chars + 4 hyphens + null terminator
		snprintf(
			guid_string, sizeof(guid_string),
			"%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x",
			guid->Data1, guid->Data2, guid->Data3,
			guid->Data4[0], guid->Data4[1], guid->Data4[2],
			guid->Data4[3], guid->Data4[4], guid->Data4[5],
			guid->Data4[6], guid->Data4[7]);
		return guid_string;
	}
}

