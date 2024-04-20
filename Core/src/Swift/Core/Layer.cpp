#include "swpch.h"
#include "Layer.hpp"

namespace Swift
{

	Layer::Layer(const std::string& LayerName)
		: m_DebugName(LayerName)
	{
	}

}

namespace Swift
{

	LayerStack::LayerStack()
		: m_InsertIndex(0)
	{
	}

	LayerStack::~LayerStack()
	{
	}

	void LayerStack::AddLayer(Layer* layer)
	{
		m_Layers.emplace(m_Layers.begin() + m_InsertIndex++, layer);

		layer->OnAttach();
	}

	void LayerStack::PopLayer(Layer* layer)
	{
		auto layerIndex = std::find(m_Layers.begin(), m_Layers.begin() + m_InsertIndex, layer);
		if (layerIndex != m_Layers.begin() + m_InsertIndex)
		{
			layer->OnDetach();
			m_Layers.erase(layerIndex);
			m_InsertIndex--;
		}
	}

	void LayerStack::AddOverlay(Layer* overlay)
	{
		m_Layers.emplace_back(overlay);

		overlay->OnAttach();
	}

	void LayerStack::PopOverlay(Layer* overlay)
	{
		auto layerIndex = std::find(m_Layers.begin() + m_InsertIndex, m_Layers.end(), overlay);
		if (layerIndex != m_Layers.end())
		{
			overlay->OnDetach();
			m_Layers.erase(layerIndex);
		}
	}

}