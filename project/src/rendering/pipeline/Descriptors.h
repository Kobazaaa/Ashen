#ifndef ASHEN_DESCRIPTORS_H
#define ASHEN_DESCRIPTORS_H

// -- Vulkan Includes --
#include <vulkan/vulkan.h>

// -- Standard Library --
#include <string>
#include <vector>
#include <unordered_map>

// -- Forward Declares --
namespace ashen
{
	class Buffer;
	class DescriptorSetAllocator;
	class VulkanContext;
}

namespace ashen
{
    //? ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    //? ~~    DescriptorPool
    //? ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    class DescriptorPool final
    {
    public:
        //--------------------------------------------------
        //    Constructor & Destructor
        //--------------------------------------------------
        explicit DescriptorPool() = default;
        ~DescriptorPool();

		DescriptorPool(const DescriptorPool& other)					= delete;
		DescriptorPool(DescriptorPool&& other) noexcept				= delete;
		DescriptorPool& operator=(const DescriptorPool& other)		= delete;
		DescriptorPool& operator=(DescriptorPool&& other) noexcept	= delete;

		//--------------------------------------------------
		//    Accessors & Mutators
		//--------------------------------------------------
		const VkDescriptorPool& GetHandle() const;

    private:
		VulkanContext* m_pContext{};
		VkDescriptorPool m_Pool { VK_NULL_HANDLE };

		friend class DescriptorPoolBuilder;
	};

	//? ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	//? ~~    DescriptorPoolBuilder
	//? ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	class DescriptorPoolBuilder final
	{
	public:
		//--------------------------------------------------
		//    Constructor & Destructor
		//--------------------------------------------------
		explicit DescriptorPoolBuilder(VulkanContext& context);

		//--------------------------------------------------
		//    Builder
		//--------------------------------------------------
		DescriptorPoolBuilder& SetMaxSets(uint32_t count);
		DescriptorPoolBuilder& AddPoolSize(VkDescriptorType type, uint32_t count);
		DescriptorPoolBuilder& AddFlags(VkDescriptorPoolCreateFlags flags);
		DescriptorPoolBuilder& SetFlags(VkDescriptorPoolCreateFlags flags);
		void Build(DescriptorPool& dp) const;

	private:
		VulkanContext* m_pContext{};

		uint32_t m_MaxSets{};
		std::vector<VkDescriptorPoolSize> m_vPoolSizes{};
		VkDescriptorPoolCreateFlags m_CreateFlags{};
	};





	//? ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	//? ~~	  DescriptorLayoutBinding
	//? ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	class DescriptorLayoutBinding final
	{
	public:
		//--------------------------------------------------
		//    Constructor & Destructor
		//--------------------------------------------------
		explicit DescriptorLayoutBinding(DescriptorSetAllocator& alloc, uint32_t binding);

		//--------------------------------------------------
		//    Builder
		//--------------------------------------------------
		DescriptorLayoutBinding& SetType(VkDescriptorType type);
		DescriptorLayoutBinding& SetCount(uint32_t count);
		DescriptorLayoutBinding& SetShaderStages(VkShaderStageFlags flags);
		DescriptorLayoutBinding& AddBindingFlags(VkDescriptorBindingFlags flags);
		DescriptorLayoutBinding& SetBindingFlags(VkDescriptorBindingFlags flags);
		DescriptorSetAllocator& EndLayoutBinding() const;

	private:
		DescriptorSetAllocator* m_pDescriptorSetAllocator{};
		VkDescriptorSetLayoutBinding m_LayoutBindings{};
		VkDescriptorBindingFlags m_BindingFlags{};

		friend class DescriptorSetAllocator;
	};


	//? ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	//? ~~	  DescriptorSet
	//? ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	class DescriptorSet final
	{
	public:
		//--------------------------------------------------
		//    Constructor & Destructor
		//--------------------------------------------------
		explicit DescriptorSet() = default;
		~DescriptorSet();

		//--------------------------------------------------
		//    Accessors & Mutators
		//--------------------------------------------------
		const VkDescriptorSet& GetHandle() const;
		const VkDescriptorSetLayout& GetLayout() const;
		const std::vector<VkDescriptorSetLayoutBinding>& GetBindings() const;

	private:
		VkDescriptorSet m_DescriptorSet{};
		VkDescriptorSetLayout m_Layout{};
		std::vector<VkDescriptorSetLayoutBinding> m_vLayoutBinding{};

		VulkanContext* m_pContext{};

		inline static std::unordered_map<std::string, VkDescriptorSetLayout> vCreatedLayouts{};
		friend class DescriptorSetAllocator;
	};

	//? ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	//? ~~	  DescriptorSetAllocator	
	//? ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	class DescriptorSetAllocator final
	{
	public:
		//--------------------------------------------------
		//    Constructor & Destructor
		//--------------------------------------------------
		explicit DescriptorSetAllocator(VulkanContext& context);

		//--------------------------------------------------
		//    Builder
		//--------------------------------------------------
		DescriptorLayoutBinding& NewLayoutBinding();
		void Allocate(const DescriptorPool& pool, DescriptorSet& ds);

	private:
		VulkanContext* m_pContext{};

		std::vector<DescriptorLayoutBinding> m_vLayoutBindings{};
	};

	//? ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	//? ~~	  DescriptorSetWriter	
	//? ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	class DescriptorSetWriter final
	{
	public:
		//--------------------------------------------------
		//    Constructor
		//--------------------------------------------------
		explicit DescriptorSetWriter(VulkanContext& context);

		//--------------------------------------------------
		//    Writing
		//--------------------------------------------------
		DescriptorSetWriter& AddBufferInfo(const Buffer& buffer, uint32_t offset, uint32_t range);
		DescriptorSetWriter& WriteBuffers(const DescriptorSet& set, uint32_t binding, uint32_t count = 0xFFFFFFFF);

		void Execute();

	private:
		VulkanContext* m_pContext{};

		std::vector<VkWriteDescriptorSet> m_vDescriptorWrites;
		std::vector<VkDescriptorBufferInfo> m_vBufferInfos;
	};
}

#endif // ASHEN_DESCRIPTORS_H
