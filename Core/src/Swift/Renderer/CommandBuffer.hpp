#pragma once

#include "Swift/Core/Core.hpp"
#include "Swift/Utils/Utils.hpp"

namespace Swift
{

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Specification 
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	enum class CommandBufferUsage
	{
		None = 0, Sequence = BIT(0), Parallel = BIT(1)
	};
	DEFINE_BITWISE_OPS(CommandBufferUsage)

	enum class Queue
	{
		None = 0, Graphics, Compute
	};

	struct CommandBufferSpecification
	{
	public:
		CommandBufferUsage Usage = CommandBufferUsage::Sequence;
	};

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// CommandBuffer 
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	class CommandBuffer
	{
	public:
		CommandBuffer() = default;
		virtual ~CommandBuffer() = default;

		virtual void Begin() = 0;
		virtual void End() = 0;
		virtual void Submit(Queue queue = Queue::Graphics, const std::vector<Ref<CommandBuffer>>& waitOn = { }) = 0;

		virtual void WaitOnFinish() = 0;

		static Ref<CommandBuffer> Create(CommandBufferSpecification specs = {});
	};

}