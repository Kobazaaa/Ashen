// -- Ashen Includes --
#include "Descriptors.h"
#include "VulkanContext.h"
#include "Buffer.h"

// -- Standard Library --
#include <sstream>

//? ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//? ~~    DescriptorPool
//? ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//--------------------------------------------------
//    Constructor & Destructor
//--------------------------------------------------
ashen::DescriptorPool::~DescriptorPool()
{
	if (!m_pContext) return;
	vkDestroyDescriptorPool(m_pContext->GetDevice(), m_Pool, nullptr);
}

//--------------------------------------------------
//    Accessors
//--------------------------------------------------
const VkDescriptorPool& ashen::DescriptorPool::GetHandle() const
{
	return m_Pool;
}


//? ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//? ~~    DescriptorPoolBuilder
//? ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//--------------------------------------------------
//    Constructor & Destructor
//--------------------------------------------------
ashen::DescriptorPoolBuilder::DescriptorPoolBuilder(VulkanContext& context)
	: m_pContext(&context)
{ }

//--------------------------------------------------
//    Builder
//--------------------------------------------------
ashen::DescriptorPoolBuilder& ashen::DescriptorPoolBuilder::SetMaxSets(uint32_t count)
{
	m_MaxSets = count;
	return *this;
}
ashen::DescriptorPoolBuilder& ashen::DescriptorPoolBuilder::AddPoolSize(VkDescriptorType type, uint32_t count)
{
	m_vPoolSizes.emplace_back(type, count);
	return *this;
}
ashen::DescriptorPoolBuilder& ashen::DescriptorPoolBuilder::AddFlags(VkDescriptorPoolCreateFlags flags)
{
	m_CreateFlags |= flags;
	return *this;
}
ashen::DescriptorPoolBuilder& ashen::DescriptorPoolBuilder::SetFlags(VkDescriptorPoolCreateFlags flags)
{
	m_CreateFlags = flags;
	return *this;
}

void ashen::DescriptorPoolBuilder::Build(DescriptorPool& dp) const
{
	dp.m_pContext = m_pContext;

	VkDescriptorPoolCreateInfo poolInfo{};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.poolSizeCount = static_cast<uint32_t>(m_vPoolSizes.size());
	poolInfo.pPoolSizes = m_vPoolSizes.data();
	poolInfo.maxSets = m_MaxSets;
	poolInfo.flags = m_CreateFlags;

	if (vkCreateDescriptorPool(m_pContext->GetDevice(), &poolInfo, nullptr, &dp.m_Pool) != VK_SUCCESS)
		throw std::runtime_error("Failed to create Descriptor Pool!");
}




//? ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//? ~~	  DescriptorLayoutBinding
//? ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//--------------------------------------------------
//    Constructor & Destructor
//--------------------------------------------------
ashen::DescriptorLayoutBinding::DescriptorLayoutBinding(DescriptorSetAllocator& alloc, uint32_t binding)
	: m_pDescriptorSetAllocator(&alloc)
{
	m_LayoutBindings.binding = binding;
	m_LayoutBindings.descriptorCount = 1;
	m_LayoutBindings.pImmutableSamplers = nullptr;
}

//--------------------------------------------------
//    Builder
//--------------------------------------------------
ashen::DescriptorLayoutBinding& ashen::DescriptorLayoutBinding::SetType(VkDescriptorType type)
{
	m_LayoutBindings.descriptorType = type;
	return *this;
}
ashen::DescriptorLayoutBinding& ashen::DescriptorLayoutBinding::SetCount(uint32_t count)
{
	m_LayoutBindings.descriptorCount = count;
	return *this;
}
ashen::DescriptorLayoutBinding& ashen::DescriptorLayoutBinding::SetShaderStages(VkShaderStageFlags flags)
{
	m_LayoutBindings.stageFlags = flags;
	return *this;
}
ashen::DescriptorLayoutBinding& ashen::DescriptorLayoutBinding::AddBindingFlags(VkDescriptorBindingFlags flags)
{
	m_BindingFlags |= flags;
	return *this;
}
ashen::DescriptorLayoutBinding& ashen::DescriptorLayoutBinding::SetBindingFlags(VkDescriptorBindingFlags flags)
{
	m_BindingFlags = flags;
	return *this;
}

ashen::DescriptorSetAllocator& ashen::DescriptorLayoutBinding::EndLayoutBinding() const
{
	return *m_pDescriptorSetAllocator;
}


//? ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//? ~~	  DescriptorSet
//? ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//--------------------------------------------------
//    Constructor & Destructor
//--------------------------------------------------
ashen::DescriptorSet::~DescriptorSet()
{
	const auto it = std::ranges::find_if(vCreatedLayouts, [&](const auto& el)
	{
		return m_Layout == el.second;
	});

	if (it == vCreatedLayouts.end()) return;
	if (!m_pContext) return;

	vkDestroyDescriptorSetLayout(m_pContext->GetDevice(), it->second, nullptr);
	vCreatedLayouts.erase(it);
}

//--------------------------------------------------
//    Accessors & Mutators
//--------------------------------------------------
const VkDescriptorSet& ashen::DescriptorSet::GetHandle() const
{
	return m_DescriptorSet;
}
const VkDescriptorSetLayout& ashen::DescriptorSet::GetLayout() const
{
	return m_Layout;
}
const std::vector<VkDescriptorSetLayoutBinding>& ashen::DescriptorSet::GetBindings() const
{
	return m_vLayoutBinding;
}


//? ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//? ~~	  DescriptorSetAllocator	
//? ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//--------------------------------------------------
//    Constructor & Destructor
//--------------------------------------------------
ashen::DescriptorSetAllocator::DescriptorSetAllocator(VulkanContext& context)
	: m_pContext(&context)
{ }

//--------------------------------------------------
//    Builder
//--------------------------------------------------
ashen::DescriptorLayoutBinding& ashen::DescriptorSetAllocator::NewLayoutBinding()
{
	m_vLayoutBindings.emplace_back(*this, static_cast<uint32_t>(m_vLayoutBindings.size()));
	return m_vLayoutBindings.back();
}

void ashen::DescriptorSetAllocator::Allocate(const DescriptorPool& pool, DescriptorSet& ds)
{
	std::ostringstream oss;
	for (const auto& b : m_vLayoutBindings) {
		oss << b.m_BindingFlags << ":"
			<< b.m_LayoutBindings.binding << ":"
			<< b.m_LayoutBindings.descriptorType << ":"
			<< b.m_LayoutBindings.descriptorCount << ":"
			<< b.m_LayoutBindings.stageFlags << "|";
	}
	auto str = oss.str();

	const auto layoutBindingCount = m_vLayoutBindings.size();
	std::vector<VkDescriptorBindingFlags> vBindingFlags(layoutBindingCount);
	std::vector<VkDescriptorSetLayoutBinding> vLayoutBindings(layoutBindingCount);
	for (size_t i{ 0 }; i < layoutBindingCount; ++i)
	{
		const auto& el = m_vLayoutBindings[i];
		vBindingFlags[i] = el.m_BindingFlags;
		vLayoutBindings[i] = el.m_LayoutBindings;
	}
	ds.m_vLayoutBinding = vLayoutBindings;

	if (DescriptorSet::vCreatedLayouts.contains(str))
	{
		ds.m_Layout = DescriptorSet::vCreatedLayouts.at(str);
	}
	else
	{
		VkDescriptorSetLayoutBindingFlagsCreateInfo flagsInfo{};
		flagsInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO;
		flagsInfo.bindingCount = static_cast<uint32_t>(layoutBindingCount);
		flagsInfo.pBindingFlags = vBindingFlags.data();

		VkDescriptorSetLayoutCreateInfo layoutInfo{};
		layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		layoutInfo.bindingCount = static_cast<uint32_t>(layoutBindingCount);
		layoutInfo.pBindings = vLayoutBindings.data();
		layoutInfo.flags = 0;
		layoutInfo.pNext = &flagsInfo;

		if (vkCreateDescriptorSetLayout(m_pContext->GetDevice(), &layoutInfo, nullptr, &ds.m_Layout) != VK_SUCCESS)
			throw std::runtime_error("Failed to create Descriptor Set Layout!");

		DescriptorSet::vCreatedLayouts.try_emplace(str, ds.m_Layout);
	}

	m_vLayoutBindings.clear();
	ds.m_pContext = m_pContext;

	VkDescriptorSetAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = pool.GetHandle();
	allocInfo.descriptorSetCount = 1;
	allocInfo.pSetLayouts = &ds.m_Layout;
	allocInfo.pNext = nullptr;

	if (vkAllocateDescriptorSets(m_pContext->GetDevice(), &allocInfo, &ds.m_DescriptorSet) != VK_SUCCESS)
		throw std::runtime_error("Failed to allocate Descriptor Sets!");
}


//? ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//? ~~	  DescriptorSetWriter	
//? ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//--------------------------------------------------
//    Constructor
//--------------------------------------------------
ashen::DescriptorSetWriter::DescriptorSetWriter(VulkanContext& context)
	: m_pContext(&context)
{}

//--------------------------------------------------
//    Writing
//--------------------------------------------------
ashen::DescriptorSetWriter& ashen::DescriptorSetWriter::AddBufferInfo(const Buffer& buffer, uint32_t offset, uint32_t range)
{
	VkDescriptorBufferInfo bufferInfo{};
	bufferInfo.buffer = buffer.GetHandle();
	bufferInfo.offset = offset;
	bufferInfo.range = range;
	m_vBufferInfos.push_back(bufferInfo);

	return *this;
}
ashen::DescriptorSetWriter& ashen::DescriptorSetWriter::WriteBuffers(const DescriptorSet& set, uint32_t binding, uint32_t count)
{
	VkWriteDescriptorSet write{};
	write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	write.dstSet = set.GetHandle();
	write.dstBinding = binding;
	write.dstArrayElement = 0;
	write.descriptorType = set.GetBindings()[binding].descriptorType;
	write.descriptorCount = count == 0xFFFFFFFF ? set.GetBindings()[binding].descriptorCount : count;
	write.pBufferInfo = m_vBufferInfos.data();
	write.pImageInfo = nullptr;
	write.pTexelBufferView = nullptr;
	m_vDescriptorWrites.push_back(write);

	return *this;
}

void ashen::DescriptorSetWriter::Execute()
{
	vkUpdateDescriptorSets(m_pContext->GetDevice(), static_cast<uint32_t>(m_vDescriptorWrites.size()), m_vDescriptorWrites.data(), 0, nullptr);
	m_vDescriptorWrites.clear();
	m_vBufferInfos.clear();
}

