#include "Context.h"
#include "../ECS/ECSBase.h"
#include "../VulkanContext/VulkanContext.h"

namespace vm
{
	Context::Context()
	{
		vulkanContext = make_ref(VulkanContext());
	}

	Context::~Context()
	{
	}

	void Context::InitSystems()
	{
		for (auto& system : m_systems)
		{
			if (system.second->IsEnabled())
				system.second->Init();
		}
	}

	void Context::UpdateSystems(double delta)
	{
		for (auto& system : m_systems)
		{
			if (system.second->IsEnabled())
				system.second->Update(delta);
		}
	}

	Entity* Context::CreateEntity()
	{
		Entity* entity = new Entity();
		entity->SetContext(this);
		entity->SetEnabled(true);

		size_t key = entity->GetID();
		m_entities[key] = std::shared_ptr<Entity>(entity);

		return m_entities[key].get();
	}

	Entity* Context::GetEntity(size_t id)
	{
		if (m_entities.find(id) != m_entities.end())
			return m_entities[id].get();
		else
			return nullptr;
	}

	void Context::RemoveEntity(size_t id)
	{
		if (m_entities.find(id) != m_entities.end())
			m_entities.erase(id);
	}

	VulkanContext* Context::GetVKContext()
	{
		return VulkanContext::get();
	}
}
