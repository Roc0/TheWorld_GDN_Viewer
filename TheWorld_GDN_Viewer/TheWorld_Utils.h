#pragma once

#ifdef _THEWORLD_CLIENT
	#include <Godot.hpp>
	#include <ImageTexture.hpp>
	#include <PoolArrays.hpp>
#endif

#include "framework.h"
#include <cfloat>
#include <assert.h> 
#include <guiddef.h>
#include <string> 
#include <chrono>
#include <vector>
#include <queue>
#include <thread>
#include <functional>
#include <exception>
#include <plog/Log.h>
#include "gsl\assert"

//#include <Godot.hpp>
//#include <Vector3.hpp>
//#include <boost/algorithm/string.hpp>

#define Vector3Zero Vector3(0, 0, 0)
#define Vector3UP Vector3(0, 1, 0)

#define Vector3X Vector3(1, 0, 1)
#define Vector3Y Vector3(0, 1, 0)
#define Vector3Z Vector3(0, 0, 1)

static constexpr float kPi = 3.14159265358979323846f;
static constexpr float kPi2 = 6.28318530717958647692f;
static constexpr float kEpsilon = 0.0001f;
static constexpr float kAreaEpsilon = FLT_EPSILON;
static constexpr float kNormalEpsilon = 0.001f;

namespace TheWorld_Utils
{
	template <class TimeT = std::chrono::milliseconds, class ClockT = std::chrono::steady_clock> class Timer
	{
		using timep_t = typename ClockT::time_point;
		timep_t _start = ClockT::now(), _end = {};

	public:
		Timer(std::string method = "", std::string headerMsg = "", bool tickMsg = false, bool tockMsg = false)
		{
			_counterStarted = false;
			_headerMsg = headerMsg;
			_save_headerMsg = headerMsg;
			_method = method;
			_tickMsg = tickMsg;
			_tockMsg = tockMsg;
		}

		~Timer()
		{
			if (_tockMsg)
				tock();
		}

		void tick()
		{
			_end = timep_t{};
			_start = ClockT::now();
			_counterStarted = true;

			if (_tickMsg)
			{
				std::string method = _method;
				if (method.length() > 0)
					method += " ";
				std::string headerMsg = _headerMsg;
				if (headerMsg.length() > 0)
					headerMsg = "- " + headerMsg;
				else
					headerMsg = "-";
				PLOG_DEBUG << std::string("START ") + method + headerMsg;
			}
		}

		void tock()
		{
			if (_counterStarted)
			{
				_end = ClockT::now();
				_counterStarted = false;

				if (_tockMsg)
				{
					std::string method = _method;
					if (method.length() > 0)
						method += " ";
					std::string headerMsg = _headerMsg;
					if (headerMsg.length() > 0)
						headerMsg = "- " + headerMsg;
					else
						headerMsg = "-";
					PLOG_DEBUG << std::string("ELAPSED ") + method + headerMsg + " ==> " + std::to_string(duration().count()).c_str() + " ms";
				}
			}
		}

		void headerMsg(std::string headerMsg)
		{
			_headerMsg = headerMsg;
		}

		void headerMsgSuffix(std::string _headerMsgSuffix)
		{
			_headerMsg = _save_headerMsg + _headerMsgSuffix;
		}

		void method(std::string method)
		{
			_method = method;
		}

		template <class TT = TimeT>	TT duration() const
		{
			Expects(_end != timep_t{} && "toc before reporting");
			//assert(_end != timep_t{} && "toc before reporting");
			return std::chrono::duration_cast<TT>(_end - _start);
		}
	private:
		bool _counterStarted;
		std::string _headerMsg;
		std::string _save_headerMsg;
		std::string _method;
		bool _tickMsg;
		bool _tockMsg;

	};

#define TimerMs Timer<std::chrono::milliseconds, std::chrono::steady_clock>
#define TimerMcs Timer<std::chrono::microseconds, std::chrono::steady_clock>

	class GridVertex;

	class MeshCacheBuffer
	{
	public:
		MeshCacheBuffer(void);
		MeshCacheBuffer(std::string cacheDir, float gridStepInWU, size_t numVerticesPerSize, int level, float lowerXGridVertex, float lowerZGridVertex);

		std::string getMeshIdFromMeshCache(void);
		void refreshVerticesFromBuffer(std::string buffer, std::string& meshIdFromBuffer, std::vector<TheWorld_Utils::GridVertex>& vectGridVertices, void* heigths, float& minY, float& maxY);
		void readBufferFromMeshCache(std::string meshId, std::string& buffer, size_t& vectSizeFromCache);
		void readVerticesFromMeshCache(std::string meshId, std::vector<TheWorld_Utils::GridVertex>& vectGridVertices, void* heigths, float& minY, float& maxY);
		void writeBufferToMeshCache(std::string buffer);
		void setBufferForMeshCache(std::string meshId, size_t numVerticesPerSize, std::vector<TheWorld_Utils::GridVertex>& vectGridVertices, std::string& buffer);
		//std::string getCacheDir()
		//{
		//	return m_cacheDir;
		//}

		std::string getMeshId()
		{
			return m_meshId;
		}

#ifdef _THEWORLD_CLIENT
		//void writeHeightmap(godot::Ref<godot::Image> heightMapImage);
		//void writeNormalmap(godot::Ref<godot::Image> normalMapImage);
		//godot::Ref<godot::Image> readHeigthmap(bool& ok);
		//godot::Ref<godot::Image> readNormalmap(bool& ok);
		enum class ImageType
		{
			heightmap = 0,
			normalmap = 1
		};

		void writeImage(godot::Ref<godot::Image> image, enum class ImageType type);
		godot::Ref<godot::Image> readImage(bool& ok, enum class ImageType type);
#endif

	private:
		std::string m_meshFilePath;
		std::string m_cacheDir;
		std::string m_meshId;

#ifdef _THEWORLD_CLIENT
		std::string m_heightmapFilePath;
		std::string m_normalmapFilePath;
#endif
	};
	
	class ThreadPool
	{
	public:
		void Start(size_t num_threads = 0);
		void QueueJob(const std::function<void()>& job);
		void Stop();
		bool busy();

	private:
		void ThreadLoop();

		bool should_terminate = false;           // Tells threads to stop looking for jobs
		std::mutex queue_mutex;                  // Prevents data races to the job queue
		std::condition_variable mutex_condition; // Allows threads to wait on new jobs or termination 
		std::vector<std::thread> threads;
		std::queue<std::function<void()>> jobs;
	};

	std::string ToString(GUID* guid);

	class Utils
	{
	public:
		static std::string ReplaceString(std::string subject, const std::string& search, const std::string& replace);

		static void ReplaceStringInPlace(std::string& subject, const std::string& search, const std::string& replace);

		static bool isEqualWithLimitedPrecision(float num1, float num2, int precision)
		{
			float diff = abs(num1 - num2);

			float value = 0.0F;
			if (precision == 7)
				value = 0.0000001F;
			else if (precision == 6)
				value = 0.000001F;
			else if (precision == 5)
				value = 0.00001F;
			else if (precision == 4)
				value = 0.0001F;
			else if (precision == 3)
				value = 0.001F;
			else if (precision == 2)
				value = 0.01F;
			else if (precision == 1)
				value = 0.1F;

			if (diff < value)
				return true;

			return false;
		}

		static int align(int x, int a) {
			return (x + a - 1) & ~(a - 1);
		}

		/*
		 * Case Insensitive String Comparision
		 */
		//static bool caseInSensStringEqual(std::string& str1, std::string& str2)
		//{
		//	return boost::iequals(str1, str2);
		//}

		/*
		 * Case Insensitive String Comparision
		 */
		//static bool caseInSensWStringEqual(std::wstring& str1, std::wstring& str2)
		//{
		//	return boost::iequals(str1, str2);
		//}

		static float square(float f) {
			return f * f;
		}

		/** Return the next power of two.
		* @see http://graphics.stanford.edu/~seander/bithacks.html
		* @warning Behaviour for 0 is undefined.
		* @note isPowerOfTwo(x) == true -> nextPowerOfTwo(x) == x
		* @note nextPowerOfTwo(x) = 2 << log2(x-1)
		*/
		static uint32_t nextPowerOfTwo(uint32_t x) {
			assert(x != 0);
			// On modern CPUs this is supposed to be as fast as using the bsr instruction.
			x--;
			x |= x >> 1;
			x |= x >> 2;
			x |= x >> 4;
			x |= x >> 8;
			x |= x >> 16;
			return x + 1;
		}

		template <typename T>
		static T max2(const T& a, const T& b) {
			return a > b ? a : b;
		}

		template <typename T>
		static T min2(const T& a, const T& b) {
			return a < b ? a : b;
		}

		template <typename T>
		static T max3(const T& a, const T& b, const T& c) {
			return max2(a, max2(b, c));
		}

		/// Return the maximum of the three arguments.
		template <typename T>
		static T min3(const T& a, const T& b, const T& c) {
			return min2(a, min2(b, c));
		}

		/// Clamp between two values.
		template <typename T>
		static T clamp(const T& x, const T& a, const T& b) {
			return Utils::min2(Utils::max2(x, a), b);
		}

		template <typename T>
		static void swap(T& a, T& b) {
			T temp = a;
			a = b;
			b = temp;
		}

		union FloatUint32 {
			float f;
			uint32_t u;
		};

		static bool isFinite(float f) {
			FloatUint32 fu;
			fu.f = f;
			return fu.u != 0x7F800000u && fu.u != 0x7F800001u;
		}

		static bool isNan(float f) {
			return f != f;
		}

		static int ftoi_ceil(float val) {
			return (int)ceilf(val);
		}

		static bool isZero(const float f, const float epsilon) {
			return fabs(f) <= epsilon;
		}

		// Robust floating point comparisons:
		// http://realtimecollisiondetection.net/blog/?p=89
		//static bool equal(const float f0, const float f1, const float epsilon = 0.00001);
	};

	class GDN_TheWorld_Exception : public std::exception
	{
	public:
		_declspec(dllexport) GDN_TheWorld_Exception(const char* function, const char* message = NULL, const char* message2 = NULL, int rc = 0)
		{
			m_exceptionName = "MapManagerException";
			if (message == NULL || strlen(message) == 0)
				m_message = "MapManager Generic Exception - C++ Exception";
			else
			{
				if (message2 == NULL || strlen(message2) == 0)
				{
					m_message = message;
				}
				else
				{
					m_message = message;
					m_message += " - ";
					m_message += message2;
					m_message += " - rc=";
					m_message += std::to_string(rc);
				}
			}
		};
		_declspec(dllexport) ~GDN_TheWorld_Exception() {};

		const char* what() const throw ()
		{
			return m_message.c_str();
		}

		const char* exceptionName()
		{
			return m_exceptionName.c_str();
		}
	private:
		std::string m_message;

	protected:
		std::string m_exceptionName;
	};


	template <typename F>
	struct FinalAction {
		FinalAction(F f) : clean_{ f } {}
		~FinalAction() { if (enabled_) clean_(); }
		void disable() { enabled_ = false; };
	private:
		F clean_;
		bool enabled_{ true };
	};

	template <typename F> FinalAction<F> finally(F f) { return FinalAction<F>(f); }

	typedef std::chrono::time_point<std::chrono::system_clock, std::chrono::milliseconds> MsTimePoint;

	template <typename T>
	void serializeToByteStream(T in, BYTE* stream, size_t& size)
	{
		T* pIn = &in;
		BYTE* pc = reinterpret_cast<BYTE*>(pIn);
		for (size_t i = 0; i < sizeof(T); i++)
		{
			stream[i] = *pc;
			pc++;
		}
		size = sizeof(T);
	}

	template <typename T>
	T deserializeFromByteStream(BYTE* stream, size_t& size)
	{
		T* pOut = reinterpret_cast<T*>(stream);
		size = sizeof(T);
		return *pOut;
	}

	// Grid / GridVertex are in WUs
	class GridVertex
	{
	public:
		GridVertex(void) : x(0.0f), y(0.0f), z(0.0f), level(0) {}
		GridVertex(float _x, float _y, float _z, int _level)
		{
			this->x = _x;
			this->y = _y;
			this->z = _z;
			this->level = _level;
		}
		GridVertex(const GridVertex& p)
		{
			*this = p;
		}
		//GridVertex(std::string serializedBuffer)
		//{
		//	sscanf_s(serializedBuffer.c_str(), "%f-%f-%f-%d", &x, &y, &z, &level);
		//}
		//GridVertex(const char* serializedBuffer)
		//{
		//	sscanf_s(serializedBuffer, "%f-%f-%f-%d", &x, &y, &z, &level);
		//}
		GridVertex(BYTE* stream, size_t& size)
		{
			// optimize method
			//*this = deserializeFromByteStream<GridVertex>(stream, size);

			// alternative method
			size_t _size;
			size = 0;
			x = deserializeFromByteStream<float>(stream + size, _size);
			size += _size;
			y = deserializeFromByteStream<float>(stream + size, _size);
			size += _size;
			z = deserializeFromByteStream<float>(stream + size, _size);
			size += _size;
			level = deserializeFromByteStream<int>(stream + size, _size);
			size += _size;
		}

		~GridVertex()
		{
		}

		// needed to use an istance of gridPoint as a key in a map (to keep the map sorted by z and by x for equal z)
		// first row, second row, ... etc
		bool operator<(const GridVertex& p) const
		{
			if (z < p.z)
				return true;
			if (z > p.z)
				return false;
			else
				return x < p.x;
		}

		void operator=(const GridVertex& p)
		{
			x = p.x;
			y = p.y;
			z = p.z;
			level = p.level;
		}
			
		bool operator==(const GridVertex& p) const
		{
			return exactlyEqual(p);
			//return equalApartFromAltitude(p);
		}

		bool equalApartFromAltitude(const GridVertex& p) const
		{
			if (x == p.x && z == p.z && level == p.level)
				return true;
			else
				return false;
		}

		bool exactlyEqual(const GridVertex& p) const
		{
			if (x == p.x && y == p.y && z == p.z && level == p.level)
				return true;
			else
				return false;
		}

		//std::string serialize(void)
		//{
		//	char buffer[256];
		//	sprintf_s(buffer, "%f-%f-%f-%d", x, y, z, level);
		//	return buffer;
		//}

		void serialize(BYTE* stream, size_t& size)
		{
			// optimize method
			//serializeToByteStream<GridVertex>(*this, stream, size);

			// alternative method
			size_t sz;
			serializeToByteStream<float>(x, stream, sz);
			size = sz;
			serializeToByteStream<float>(y, stream + size, sz);
			size += sz;
			serializeToByteStream<float>(z, stream + size, sz);
			size += sz;
			serializeToByteStream<int>(level, stream + size, sz);
			size += sz;
		}

		std::string toString()
		{
			return "Level=" + std::to_string(level) + "-X=" + std::to_string(x) + "-Z=" + std::to_string(z) + "-Altitude=" + std::to_string(y);
		}

		float altitude(void) { return y; }
		float posX(void) { return x; }
		float posZ(void) { return z; }
		int lvl(void) { return level; }
		void setAltitude(float a) { y = a; }

	private:
		float x;
		float y;
		float z;
		int level;
	};

	/*static float roundToDigit(float num, int digit)
	{
		//float value = (int)(num * pow(10.0, digit) + .5);
		//float v = (float)value / (int)pow(10.0, digit);
		//return v;

		char str[40];
		sprintf(str, "%.6f", num);
		sscanf(str, "%f", &num);
		return num;
	}*/

	/*static bool isEqualVectorWithLimitedPrecision(godot::Vector3 v1, godot::Vector3 v2, int precision)
	{
		if (isEqualWithLimitedPrecision(v1.x, v2.x, precision) && isEqualWithLimitedPrecision(v1.y, v2.y, precision) && isEqualWithLimitedPrecision(v1.z, v2.z, precision))
			return true;
		else
			return false;
	}*/
}

static bool equal(const float f0, const float f1, const float epsilon = 0.00001)
{
	//return fabs(f0-f1) <= epsilon;
	return fabs(f0 - f1) <= epsilon * TheWorld_Utils::Utils::max3(1.0f, fabsf(f0), fabsf(f1));
}

