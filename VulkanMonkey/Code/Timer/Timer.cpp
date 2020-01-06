#include "Timer.h"
#include "../VulkanContext/VulkanContext.h"
#include <thread>

using namespace vm;

Timer::Timer() noexcept
{
	m_start = {};
}

void Timer::Start() noexcept
{
	m_start = std::chrono::high_resolution_clock::now();
}

double Timer::Count() noexcept
{
	const std::chrono::duration<double> t_duration = std::chrono::high_resolution_clock::now() - m_start;
	return t_duration.count();
}

FrameTimer::FrameTimer() : Timer()
{
	m_duration = {};
	delta = 0.0f;
	time = 0.0f;
	measures.resize(1);
}

void FrameTimer::Delay(double seconds)
{
	static size_t system_delay = 0;
	static Timer timer;

	const std::chrono::nanoseconds delay(static_cast<size_t>(SECONDS_TO_NANOSECONDS(seconds)) - system_delay);

	timer.Start();
	std::this_thread::sleep_for(delay);
	system_delay = static_cast<size_t>(SECONDS_TO_NANOSECONDS(timer.Count())) - delay.count();
}


void FrameTimer::Tick() noexcept
{
	m_duration = std::chrono::high_resolution_clock::now() - m_start;
	delta = m_duration.count();
	time += delta;
}

void GPUTimer::start(const vk::CommandBuffer* cmd) noexcept
{
	_cmd = cmd;
	_cmd->resetQueryPool(*queryPool, 0, 2);
	_cmd->writeTimestamp(vk::PipelineStageFlagBits::eTopOfPipe, *queryPool, 0);
}

void GPUTimer::end(float* res)
{
	_cmd->writeTimestamp(vk::PipelineStageFlagBits::eBottomOfPipe, *queryPool, 1);
	if (res)
		*res = getTime();
}

void GPUTimer::initQueryPool()
{
	const auto gpuProps = VulkanContext::get()->gpu.getProperties();
	if (!gpuProps.limits.timestampComputeAndGraphics)
		throw std::runtime_error("Timestamps not supported");

	timestampPeriod = gpuProps.limits.timestampPeriod;

	vk::QueryPoolCreateInfo qpci;
	qpci.queryType = vk::QueryType::eTimestamp;
	qpci.queryCount = 2;

	queryPool = std::make_unique<vk::QueryPool>(VulkanContext::get()->device.createQueryPool(qpci));

	queryTimes.resize(2, 0);
}

float GPUTimer::getTime()
{
	const auto res = VulkanContext::get()->device.getQueryPoolResults<uint64_t>(*queryPool, 0, 2, queryTimes, sizeof(uint64_t), vk::QueryResultFlagBits::e64);
	if (res != vk::Result::eSuccess)
		return 0.0f;
	return static_cast<float>(queryTimes[1] - queryTimes[0]) * timestampPeriod * 1e-6f;
}

void GPUTimer::destroy() const noexcept
{
	VulkanContext::get()->device.destroyQueryPool(*queryPool);
}
