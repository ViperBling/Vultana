#pragma once

#include <EASTL/string.h>
#include <EASTL/vector.h>

#include <cstdint>
#include <cstring>
#include <type_traits>

namespace Assets
{
    bool ReadFileToBuffer(const eastl::string& path, eastl::vector<uint8_t>& outBuffer);
    bool ReadFileHeadToBuffer(const eastl::string& path, uint32_t nBytes, eastl::vector<uint8_t>& outBuffer);
    bool WriteFileFromBuffer(const eastl::string& path, const eastl::vector<uint8_t>& data);

    // Little-endian, x86, memcpy field-wise serialization. Format changes touch only these two classes + the payload read/write functions.
    class FBinaryWriter
    {
    public:
        void Write(const void* pData, uint32_t size)
        {
            const uint8_t* p = reinterpret_cast<const uint8_t*>(pData);
            m_Data.insert(m_Data.end(), p, p + size);
        }
        void WriteU32(uint32_t v) { Write(&v, sizeof(v)); }
        void WriteI32(int32_t v) { Write(&v, sizeof(v)); }
        void WriteU64(uint64_t v) { Write(&v, sizeof(v)); }
        void WriteF32(float v) { Write(&v, sizeof(v)); }
        void WriteU8(uint8_t v) { Write(&v, sizeof(v)); }
        void WriteBool(bool v) { WriteU8(v ? 1u : 0u); }
        void WriteString(const eastl::string& str)
        {
            uint32_t length = static_cast<uint32_t>(str.size());
            WriteU32(length);
            if (!str.empty())
            {
                Write(str.c_str(), (uint32_t)str.size());
            }
        }

        // blob write: [u32 len][bytes] (symmetric with the reader's ReadBlob)
        void WriteBlob(const eastl::vector<uint8_t>& data)
        {
            uint32_t length = static_cast<uint32_t>(data.size());
            WriteU32(length);
            if (!data.empty())
            {
                Write(data.data(), length);
            }
        }
        void Align16()
        {
            const size_t padding = (16 - (m_Data.size() & 15)) & 15;
            for (size_t i = 0; i < padding; ++i)
                m_Data.push_back(0);
        }

        template <typename T>
        void WritePod(const T& v)
        {
            static_assert(std::is_trivially_copyable_v<T>, "WritePod requires trivially copyable");
            Write(&v, sizeof(T));
        }

        template <typename T>
        void WriteVector(const eastl::vector<T>& v)
        {
            static_assert(std::is_trivially_copyable_v<T>, "WriteVector requires trivially copyable elements");
            uint32_t length = static_cast<uint32_t>(v.size());
            WriteU32(length);
            if (!v.empty())
            {
                Write(v.data(), (uint32_t)(sizeof(T) * v.size()));
            }
        }

        size_t GetSize() const { return m_Data.size(); }
        const eastl::vector<uint8_t>& GetData() const { return m_Data; }

    private:
        eastl::vector<uint8_t> m_Data;
    };

    class FBinaryReader
    {
    public:
        FBinaryReader(const uint8_t* pData, size_t size) : m_pData(pData), m_Size(size) {}

        bool Read(void* pOut, uint32_t size)
        {
            if (m_Offset + size > m_Size)
            {
                return false;
            }
            memcpy(pOut, m_pData + m_Offset, size);
            m_Offset += size;
            return true;
        }

        bool ReadU32(uint32_t& v) { return Read(&v, sizeof(v)); }
        bool ReadI32(int32_t& v) { return Read(&v, sizeof(v)); }
        bool ReadU64(uint64_t& v) { return Read(&v, sizeof(v)); }
        bool ReadF32(float& v) { return Read(&v, sizeof(v)); }
        bool ReadU8(uint8_t& v) { return Read(&v, sizeof(v)); }
        bool ReadBool(bool& v) { uint8_t b = 0; if (!ReadU8(b)) return false; v = (b != 0); return true; }

        bool ReadString(eastl::string& s)
        {
            uint32_t length = 0;
            if (!ReadU32(length))
            {
                return false;
            }
            if (m_Offset + length > m_Size)
            {
                return false;
            }
            s.assign(reinterpret_cast<const char*>(m_pData + m_Offset), length);
            m_Offset += length;
            return true;
        }

        bool ReadBlob(eastl::vector<uint8_t>& out)
        {
            uint32_t length = 0;
            if (!ReadU32(length))
            {
                return false;
            }
            if (length > (m_Size - m_Offset))
            {
                return false;
            }
            out.resize(length);
            if (length > 0)
            {
                memcpy(out.data(), m_pData + m_Offset, length);
                m_Offset += length;
            }
            return true;
        }

        template <typename T>
        bool ReadPod(T& v)
        {
            static_assert(std::is_trivially_copyable_v<T>, "ReadPod requires trivially copyable");
            return Read(&v, sizeof(T));
        }

        template <typename T>
        bool ReadVector(eastl::vector<T>& v)
        {
            static_assert(std::is_trivially_copyable_v<T>, "ReadVector requires trivially copyable elements");
            uint32_t count = 0;
            if (!ReadU32(count))
            {
                return false;
            }
            if (count > (m_Size - m_Offset) / sizeof(T))
            {
                return false;
            }
            v.resize(count);
            if (count > 0)
            {
                memcpy(v.data(), m_pData + m_Offset, sizeof(T) * count);
                m_Offset += sizeof(T) * count;
            }
            return true;
        }

        bool Seek(size_t offset)
        {
            if (offset > m_Size)
            {
                return false;
            }
            m_Offset = offset;
            return true;
        }
        size_t GetOffset() const { return m_Offset; }
        size_t GetSize() const { return m_Size; } // for remaining-bytes checks (ReadHeaderRegion's refCount, etc.)

    private:
        const uint8_t* m_pData = nullptr;
        size_t m_Size = 0;
        size_t m_Offset = 0;
    };
}