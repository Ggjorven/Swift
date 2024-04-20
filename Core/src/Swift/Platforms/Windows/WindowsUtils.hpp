#pragma once

#include "Swift/Utils/Utils.hpp"

namespace Swift::Utils
{
	
	class WindowsToolKit : public ToolKit
	{
	public:
		WindowsToolKit() = default;

	private:
		std::string OpenFileImpl(const std::string& filter, const std::string& dir) const override;
		std::string SaveFileImpl(const std::string& filter, const std::string& dir) const override;

		std::string OpenDirectoryImpl(const std::string& dir) const override;

		double GetTimeImpl() const override;
	};

}