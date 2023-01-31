// Grid / GridVertex are in WUs

//class GridVertex
//{
//public:
//	GridVertex(void) : x(0.0f), y(0.0f), z(0.0f), level(0) {}
//	GridVertex(float _x, float _y, float _z, int _level)
//	{
//		this->x = _x;
//		this->y = _y;
//		this->z = _z;
//		this->level = _level;
//	}
//	GridVertex(const GridVertex& p)
//	{
//		*this = p;
//	}
//	//GridVertex(std::string serializedBuffer)
//	//{
//	//	sscanf_s(serializedBuffer.c_str(), "%f-%f-%f-%d", &x, &y, &z, &level);
//	//}
//	//GridVertex(const char* serializedBuffer)
//	//{
//	//	sscanf_s(serializedBuffer, "%f-%f-%f-%d", &x, &y, &z, &level);
//	//}
//	GridVertex(BYTE* stream, size_t& size)
//	{
//		// optimize method
//		//*this = deserializeFromByteStream<GridVertex>(stream, size);

//		// alternative method
//		size_t _size;
//		size = 0;
//		x = deserializeFromByteStream<float>(stream + size, _size);
//		size += _size;
//		y = deserializeFromByteStream<float>(stream + size, _size);
//		size += _size;
//		z = deserializeFromByteStream<float>(stream + size, _size);
//		size += _size;
//		level = deserializeFromByteStream<int>(stream + size, _size);
//		size += _size;
//	}

//	~GridVertex()
//	{
//	}

//	// needed to use an istance of gridPoint as a key in a map (to keep the map sorted by z and by x for equal z)
//	// first row, second row, ... etc
//	bool operator<(const GridVertex& p) const
//	{
//		if (z < p.z)
//			return true;
//		if (z > p.z)
//			return false;
//		else
//			return x < p.x;
//	}

//	void operator=(const GridVertex& p)
//	{
//		x = p.x;
//		y = p.y;
//		z = p.z;
//		level = p.level;
//	}
//		
//	bool operator==(const GridVertex& p) const
//	{
//		return exactlyEqual(p);
//		//return equalApartFromAltitude(p);
//	}

//	bool equalApartFromAltitude(const GridVertex& p) const
//	{
//		if (x == p.x && z == p.z && level == p.level)
//			return true;
//		else
//			return false;
//	}

//	bool exactlyEqual(const GridVertex& p) const
//	{
//		if (x == p.x && y == p.y && z == p.z && level == p.level)
//			return true;
//		else
//			return false;
//	}

//	//std::string serialize(void)
//	//{
//	//	char buffer[256];
//	//	sprintf_s(buffer, "%f-%f-%f-%d", x, y, z, level);
//	//	return buffer;
//	//}

//	void serialize(BYTE* stream, size_t& size)
//	{
//		// optimize method
//		//serializeToByteStream<GridVertex>(*this, stream, size);

//		// alternative method
//		size_t sz;
//		serializeToByteStream<float>(x, stream, sz);
//		size = sz;
//		serializeToByteStream<float>(y, stream + size, sz);
//		size += sz;
//		serializeToByteStream<float>(z, stream + size, sz);
//		size += sz;
//		serializeToByteStream<int>(level, stream + size, sz);
//		size += sz;
//	}

//	std::string toString()
//	{
//		return "Level=" + std::to_string(level) + "-X=" + std::to_string(x) + "-Z=" + std::to_string(z) + "-Altitude=" + std::to_string(y);
//	}

//	float altitude(void) 
//	{
//		return y; 
//	}
//	float posX(void) 
//	{
//		return x; 
//	}
//	float posZ(void)
//	{
//		return z; 
//	}
//	int lvl(void) 
//	{
//		return level; 
//	}
//	void setAltitude(float a)
//	{
//		y = a; 
//	}

//private:
//	float x;
//	float y;
//	float z;
//	int level;
//};

		//void MapManagerCalcNextCoordOnTheGridInWUs(std::vector<float>& inCoords, std::vector<float>& outCoords);

		//float MapManagerGridStepInWU(void);

		//void resizeEditModeUI(void);

		//Chunk* getTrackedChunk(void);
		//String getTrackedChunkStr(void);
