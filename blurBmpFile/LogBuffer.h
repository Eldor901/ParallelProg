#pragma once

#include <windows.h>
#include <stdexcept>

template<typename T>
class LogBuffer
{
	class Node
	{
	public:
		Node(Node* prev, Node* next)
			: prev(prev)
			, next(next)
		{
		}
		virtual ~Node() = default;
		virtual T& GetData()
		{
			throw std::runtime_error("Cannot dereference end iterator");
		}

		Node* prev;
		Node* next;
	};

public:
	LogBuffer()
		: m_size(0)
	{
		m_first = new Node(nullptr, nullptr);
		m_last = m_first;
	}

	LogBuffer(const LogBuffer& other)
	{
		LogBuffer tmp;
		for (T data : other)
		{
			tmp.Log(data);
		}
		m_first = new Node(nullptr, nullptr);
		m_last = m_first;
		*this = move(tmp);
	}

	LogBuffer(LogBuffer&& other)
	{
		Node* endNode = new Node(nullptr, nullptr);
		m_first = other.m_first;
		m_last = other.m_last;
		m_size = other.m_size;
		other.m_first = endNode;
		other.m_last = endNode;
		other.m_size = 0;
	}

	~LogBuffer()
	{
		Clear();
		delete m_last;
	}

	LogBuffer& operator=(const LogBuffer& other)
	{
		if (this != &other)
		{
			LogBuffer tmp(other);
			*this = move(tmp);
		}
		return *this;
	}

	LogBuffer& operator=(LogBuffer&& other)
	{
		if (this != &other)
		{
			Clear();
			std::swap(m_first, other.m_first);
			std::swap(m_last, other.m_last);
			std::swap(m_size, other.m_size);
		}
		return *this;
	}

	void AddCriticalSection(CRITICAL_SECTION* criticalSection)
	{
		m_criticalSection = criticalSection;
	}

	void Log(const T& data)
	{
		if (m_criticalSection != nullptr)
		{
			EnterCriticalSection(m_criticalSection);
		}

		Node* newNode = new NodeWithData(data, m_last->prev, m_last);
		if (IsEmpty())
		{
			m_first = newNode;
		}
		else
		{
			m_last->prev->next = newNode;
		}
		m_last->prev = newNode;
		m_size++;

		if (m_criticalSection != nullptr)
		{
			LeaveCriticalSection(m_criticalSection);
		}
	}

	size_t GetSize() const
	{
		return m_size;
	}

	bool IsEmpty() const
	{
		return m_size == 0;
	}

	void Clear()
	{
		Node* curNode = m_first;
		while (curNode->next != nullptr)
		{
			curNode = curNode->next;
			delete curNode->prev;
		}
		m_first = m_last;
		m_size = 0;
	}

	class CIterator
	{
	public:
		CIterator()
			: _node(nullptr)
			, _isReverse(false)
		{
		}

		CIterator& operator++()
		{
			!_isReverse ? DoIncrement() : DoDecrement();
			return *this;
		}

		const CIterator operator++(int)
		{
			CIterator tmp(*this);
			++(*this);
			return tmp;
		}

		CIterator& operator--()
		{
			!_isReverse ? DoDecrement() : DoIncrement();
			return *this;
		}

		const CIterator operator--(int)
		{
			CIterator tmp(*this);
			--(*this);
			return tmp;
		}

		T& operator*() const
		{
			if (_node == nullptr)
			{
				throw std::runtime_error("Cannot dereference empty iterator");
			}
			return _node->GetData();
		}

		bool operator==(const CIterator& other) const
		{
			return _node == other._node;
		}

		bool operator!=(const CIterator& other) const
		{
			return _node != other._node;
		}

	private:
		friend LogBuffer;

		Node* _node;
		bool _isReverse;

		CIterator(Node* node, bool isReverse)
			: _node(node)
			, _isReverse(isReverse)
		{
		}

		void DoIncrement()
		{
			if (_node == nullptr)
			{
				throw std::runtime_error("Cannot increment empty iterator");
			}
			if (_node->next == nullptr)
			{
				throw std::runtime_error("Cannot increment end iterator");
			}
			_node = _node->next;
		}

		void DoDecrement()
		{
			if (_node == nullptr)
			{
				throw std::runtime_error("Cannot decrement empty iterator");
			}
			if (_node->prev == nullptr)
			{
				throw std::runtime_error("Cannot decrement begin iterator");
			}
			_node = _node->prev;
		}
	};

	CIterator begin() const
	{
		return CIterator(m_first, false);
	}

	CIterator end() const
	{
		return CIterator(m_last, false);
	}

	const CIterator cbegin() const
	{
		return CIterator(m_first, false);
	}

	const CIterator cend() const
	{
		return CIterator(m_last, false);
	}

	CIterator rbegin() const
	{
		return CIterator(m_first, true);
	}

	CIterator rend() const
	{
		return CIterator(m_last, true);
	}

	const CIterator crbegin() const
	{
		return CIterator(m_first, true);
	}

	const CIterator crend() const
	{
		return CIterator(m_last, true);
	}

private:
	class NodeWithData : public Node
	{
	public:
		NodeWithData(const T& data, Node* prev, Node* next)
			: _data(data)
			, Node(prev, next)
		{
		}

		T& GetData() override
		{
			return _data;
		}

	private:
		T _data;
	};

	Node* m_first;
	Node* m_last;
	size_t m_size;
	CRITICAL_SECTION* m_criticalSection = nullptr;
};
