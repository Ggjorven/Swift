#pragma once

#include <stdint.h>

#include <map>
#include <memory>
#include <queue>
#include <vector>
#include <chrono>
#include <thread>
#include <algorithm>
#include <filesystem>
#include <type_traits>
#include <unordered_map>

// TODO: Include logging

#define BIT(x) (1 << x)
#define BIT_X(x, y) (x << y)

#define DEFINE_BITWISE_OPS(Type) \
constexpr Type operator | (Type lhs, Type rhs) \
{ \
    return static_cast<Type>(static_cast<int>(lhs) | static_cast<int>(rhs)); \
} \
constexpr bool operator & (Type lhs, Type rhs) \
{ \
    return static_cast<int>(lhs) & static_cast<int>(rhs); \
} \
constexpr Type operator ^ (Type lhs, Type rhs) \
{ \
    return static_cast<Type>(static_cast<int>(lhs) ^ static_cast<int>(rhs)); \
} \

#define MAX_UINT8 255
#define MAX_UINT16 65535 
#define MAX_UINT32 4294967295 
#define MAX_UINT64 18446744073709551615ULL 
#define MAX_INT8 127 
#define MAX_INT16 32767
#define MAX_INT32 2147483647
#define MAX_INT64 9223372036854775807LL
#define MAX_FLOAT 3.402823466e+38F 
#define MAX_DOUBLE 1.7976931348623158e+308

#define PUBLIC_PADDING(index, size) \
private: \
    char Padding##index[size] = {}; \
public:

#define PRIVATE_PADDING(index, size) \
char Padding##index[size] = {}; 

namespace VkOutline::Utils
{

    // Platform specific
    class ToolKit
    {
    public:
        static std::string OpenFile(const std::string& filter, const std::string& dir = "") { return s_Instance ? s_Instance->OpenFileImpl(filter, dir) : ""; }
        static std::string SaveFile(const std::string& filter, const std::string& dir = "") { return s_Instance ? s_Instance->SaveFileImpl(filter, dir) : ""; }

        static std::string OpenDirectory(const std::string& dir = "") { return s_Instance ? s_Instance->OpenDirectoryImpl(dir) : ""; }

        static double GetTime() { return s_Instance ? s_Instance->GetTimeImpl() : 0.0f; }

        // Non-Platform specific
        inline static void Sleep(uint32_t miliseconds)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(miliseconds));
        }

        inline static void Replace(std::string& str, char replace, char with)
        {
            std::replace(str.begin(), str.end(), replace, with);
        }

    private:
        virtual std::string OpenFileImpl(const std::string& filter, const std::string& dir) const = 0;
        virtual std::string SaveFileImpl(const std::string& filter, const std::string& dir) const = 0;
        
        virtual std::string OpenDirectoryImpl(const std::string& dir) const = 0;

        virtual double GetTimeImpl() const = 0;

    private:
        static std::unique_ptr<ToolKit> s_Instance;
    };

    // A class to be used for function queues
    template<typename Func>
    class Queue
    {
    public:
        Queue() = default;
        virtual ~Queue() = default;

        inline void Add(Func func) { m_Queue.push(func); }

        // Note(Jorben): Executing simultaneously clears the queue.
        template<typename ...Args>
        void Execute(Args&& ...args)
        {
            while (!m_Queue.empty())
            {
                Func& func = m_Queue.front();
                func(std::forward<Args>(args)...);
                m_Queue.pop();
            }
        }

        inline size_t Size() { return m_Queue.size(); }

    private:
        std::queue<Func> m_Queue = { };
    };

    class Timer
    {
    public:
        Timer()
            : m_Start(ToolKit::GetTime())
        {
        }

        inline double GetPassedTime() const 
        { 
            double end = ToolKit::GetTime();
            double result = end - m_Start;

            return result; 
        }

    private:
        double m_Start = 0.0f;
    };

}

namespace VkOutline
{

    template<typename Key, typename Value>
    using Dict = std::unordered_map<Key, Value>;

    template<typename Key, typename Value>
    class SortedDict
    {
    public:
        SortedDict() = default;
        SortedDict(const SortedDict<Key, Value>& other) = default;
        virtual ~SortedDict() = default;

        Value& operator [] (const Key& key)
        {
            auto it = m_Indices.find(key);
            if (it == m_Indices.end())
            {
                m_Items.push_back(std::make_pair(key, Value()));
                m_Indices[key] = m_Items.size() - 1;
            }
            return m_Items[m_Indices[key]].second;
        }

        void insert(const Key& key, const Value& value)
        {
            if (m_Indices.find(key) == m_Indices.end())
            {
                m_Items.push_back(std::make_pair(key, value));
                m_Indices[key] = m_Items.size() - 1;
            }
            else
            {
                m_Items[m_Indices[key]].second = value;
            }
        }

        void erase(const Key& key)
        {
            if (m_Indices.find(key) != m_Indices.end())
            {
                m_Items.erase(m_Items.begin() + m_Indices[key]);
                m_Indices.erase(key);
            }
        }

        auto begin() { return m_Items.begin(); }
        const auto begin() const { return m_Items.begin(); }
        auto end() { return m_Items.end(); }
        const auto end() const { return m_Items.end(); }

    public:
        std::unordered_map<Key, size_t> m_Indices = { };
        std::vector<std::pair<Key, Value>> m_Items = { };
    };

}