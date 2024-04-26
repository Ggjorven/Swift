#pragma once

#include <stdint.h>

#include <map>
#include <mutex>
#include <queue>
#include <future>
#include <memory>
#include <vector>
#include <chrono>
#include <thread>
#include <algorithm>
#include <filesystem>
#include <type_traits>
#include <unordered_map>

#include "Swift/Core/Logging.hpp"

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

namespace Swift::Utils
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

    // A threadsafe class to be used for function queues
    template<typename Func>
    class Queue
    {
    public:
        enum class ExecutionStyle
        {
            InOrder = 0, Parallel
        };
    public:
        Queue() = default;
        virtual ~Queue() = default;

        inline void Add(Func func) 
        { 
            std::scoped_lock<std::mutex> lock(m_Mutex);
            m_Queue.push(func); 
        }

        inline Func Front()
        {
            std::scoped_lock<std::mutex> lock(m_Mutex);
            Func func = std::move(m_Queue.front());
            m_Queue.pop();
            return func;
        }

        // Note(Jorben): Executing simultaneously clears the queue.
        template<typename ...Args>
        inline void Execute(ExecutionStyle style = ExecutionStyle::InOrder, Args&& ...args)
        {
            std::scoped_lock<std::mutex> lock(m_Mutex);

            switch (style)
            {
            case ExecutionStyle::InOrder:
            {
                while (!m_Queue.empty())
                {
                    Func& func = m_Queue.front();
                    func(std::forward<Args>(args)...);
                    m_Queue.pop();
                }
                break;
            }
            case ExecutionStyle::Parallel:
            {
                std::vector<std::future<void>> futures = { };

                while (!m_Queue.empty()) 
                {
                    Func func = std::move(m_Queue.front());
                    m_Queue.pop();
                    futures.emplace_back(std::async(std::launch::async, std::move(func), std::forward<Args>(args)...));
                }

                // Wait
                for (auto& future : futures)
                    future.get();

                break;
            }

            default:
                APP_LOG_ERROR("Invalid ExecutionStyle selected.");
                break;
            }
        }

        inline size_t Size() const
        { 
            std::scoped_lock<std::mutex> lock(m_Mutex);
            return m_Queue.size(); 
        }

        inline void Clear()
        {
            std::scoped_lock<std::mutex> lock(m_Mutex);

            while (!m_Queue.empty())
                m_Queue.pop();
        }

        inline std::vector<Func> AsVector() const
        {
            std::vector<Func> funcs = { };

            std::scoped_lock<std::mutex> lock(m_Mutex);
            funcs.reserve(m_Queue.size());
            for (const auto& func : m_Queue)
            {
                funcs.push_back(func);
            }

            return funcs;
        }


    private:
        std::mutex m_Mutex = {};
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

namespace Swift
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

        inline Value& operator [] (const Key& key)
        {
            auto it = m_Indices.find(key);
            if (it == m_Indices.end())
            {
                m_Items.push_back(std::make_pair(key, Value()));
                m_Indices[key] = m_Items.size() - 1;
            }
            return m_Items[m_Indices[key]].second;
        }

        inline void insert(const Key& key, const Value& value)
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

        inline void erase(const Key& key)
        {
            if (m_Indices.find(key) != m_Indices.end())
            {
                m_Items.erase(m_Items.begin() + m_Indices[key]);
                m_Indices.erase(key);
            }
        }

        inline auto begin() { return m_Items.begin(); }
        inline const auto begin() const { return m_Items.begin(); }
        inline auto end() { return m_Items.end(); }
        inline const auto end() const { return m_Items.end(); }

    public:
        std::unordered_map<Key, size_t> m_Indices = { };
        std::vector<std::pair<Key, Value>> m_Items = { };
    };


    // A threadsafe linked list class
    template <typename T>
    class LinkedList 
    {
    private:
        struct Node;
    public:
        LinkedList() 
            : Head(nullptr) 
        {
        }

        virtual ~LinkedList() 
        {
            Clear();
        }

        inline void Push(const T& newData) 
        {
            std::scoped_lock<std::mutex> lock(m_Mutex);

            Node* newNode = new Node(newData);
            if (!Head) 
            {
                Head = newNode;
                return;
            }

            Node* current = Head;
            while (current->Next)
                current = current->Next;

            current->Next = newNode;
        }

        inline void Pop() 
        {
            std::scoped_lock<std::mutex> lock(m_Mutex);

            if (Head) 
            {
                Node* temp = Head;
                Head = Head->Next;
                delete temp;
            }
        }

        inline T& Front() 
        {
            std::scoped_lock<std::mutex> lock(m_Mutex);

            if (Head) 
                return Head->Data;

            APP_ASSERT(false, "LinkedList is empty");
            
            static T empty = {};
            return empty;
        }

        inline std::vector<T>& AsVector() const 
        {
            std::scoped_lock<std::mutex> lock(m_Mutex);

            static std::vector<T> result = { };
            result.clear();

            Node* current = Head;
            while (current) 
            {
                result.push_back(current->Data);
                current = current->Next;
            }

            return result;
        }

        inline void Remove(const T& value) 
        {
            std::scoped_lock<std::mutex> lock(m_Mutex);

            Node* current = Head;
            Node* prev = nullptr;

            while (current) 
            {
                if (current->Data == value) 
                {
                    if (prev)
                        prev->Next = current->Next;
                    else
                        Head = current->Next;

                    delete current;
                    return;
                }
                prev = current;
                current = current->Next;
            }

            APP_ASSERT(false, "Failed to find item by T value and remove it from LinkedList.");
        }

        inline bool HasItem(const T& value) const
        {
            Node* current = Head;
            while (current)
            {
                if (current->Data == value)
                    return true;

                current = current->Next;
            }

            return false;
        }

        inline void Clear() 
        {
            while (Head) 
            {
                Node* temp = Head;
                Head = Head->Next;
                delete temp;
            }
        }

        inline bool Empty() const { return Head == nullptr; }

    private:
        struct Node
        {
        public:
            T Data = {};
            Node* Next = nullptr;

        public:
            Node(const T& newData) 
                : Data(newData), Next(nullptr) 
            {
            }
        };
    private:
        mutable std::mutex m_Mutex = {};

        Node* Head = nullptr;
    };


}