#include "VulkanRenderer.h"
#include "ModelLoader.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#define VMA_IMPLEMENTATION
#include "vk_mem_alloc.h"

#include <iostream>
#include <chrono>

using namespace Puffin::Rendering;

void VulkanRenderer::Init(UI::UIManager* UIManager, Input::InputManager* InputManager)
{
	InitWindow(InputManager);
	InitVulkan(UIManager);
}

bool VulkanRenderer::Update(UI::UIManager* UIManager, Input::InputManager* InputManager, float dt)
{
	glfwPollEvents();

	InputManager->UpdateInput(window);
	UpdateCamera(camera, InputManager, dt);

	float prevFov = camera.fov;

	bool running = UIManager->DrawUI(dt, InputManager);
	DrawFrame(UIManager, dt);

	// Recalculate Perspective if FOV changes
	if (camera.fov != prevFov)
	{
		UpdatePerspective(camera, camera.fov, camera.aspect, camera.zNear, camera.zFar);
	}

	// Pass Viewport Texture to Viewport Window
	UIManager->GetWindowViewport()->SetSceneTexture(offscreenTexture);

	if (glfwWindowShouldClose(window))
	{
		running = false;
	}

	return running;
}

void VulkanRenderer::InitMesh(ECS::Entity entity, std::string model_path, std::string texture_path)
{
	MeshComponent& comp = world->GetComponent<MeshComponent>(entity);

	comp.model_path = model_path;
	comp.texture_path = texture_path;

	InitTexture(comp.texture, comp.texture_path);
	IO::LoadMesh(comp, comp.model_path);

	CreateVertexBuffers(comp);
	CreateIndexBuffers(comp);
	CreateUniformBuffer(comp);
}

void VulkanRenderer::InitMeshCube(ECS::Entity entity, glm::vec3 color)
{
	MeshComponent& comp = world->GetComponent<MeshComponent>(entity);

	InitTexture(comp.texture, "textures\\cube.png");
	comp.vertices = cube_vertices;
	comp.indices = cube_indices;

	for (int i = 0; i < comp.vertices.size(); i++)
	{
		comp.vertices[i].color = color;
	}

	CreateVertexBuffers(comp);
	CreateIndexBuffers(comp);
	CreateUniformBuffer(comp);
}

void VulkanRenderer::InitLight(LightComponent& light, glm::vec3 position, glm::vec3 ambient, glm::vec3 diffuse, float specular, int shininess)
{
	light.uniformBuffer.position = position;
	light.uniformBuffer.ambientColor = ambient;
	light.uniformBuffer.diffuseColor = diffuse;
	light.uniformBuffer.specularStrength = specular;
	light.uniformBuffer.shininess = shininess;
}

void VulkanRenderer::InitCamera(CameraComponent& camera, glm::vec3 position_, glm::vec3 direction_, glm::vec3 up_, float fov_, float aspect_, float near_, float far_)
{
	camera.direction = direction_;

	// Set Position, LookAt and Up Vectors
	camera.position = position_;
	camera.lookat = camera.position + camera.direction;
	camera.up = up_;

	// Calculate Perspective Projection
	UpdatePerspective(camera, fov_, aspect_, near_, far_);

	// Calculate Right and Up vectors
	camera.right = glm::normalize(glm::cross(camera.up, camera.direction));
	camera.up = glm::cross(camera.direction, camera.right);

	camera.yaw = -90.0f;

	camera.speed = 5.0f;

	UpdateViewMatrix(camera);
}

VulkanRenderer::~VulkanRenderer()
{
	Cleanup();
}

//-------------------------------------------------------------------------------------

void VulkanRenderer::InitWindow(Input::InputManager* InputManager)
{
	glfwInit();

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

	window = glfwCreateWindow(WIDTH, HEIGHT, "Puffin Engine", monitor, nullptr);

	glfwMaximizeWindow(window);
	glfwSetWindowUserPointer(window, this);
	glfwSetFramebufferSizeCallback(window, FramebufferResizeCallback);

	InputManager->SetupInput(window);
}

//-------------------------------------------------------------------------------------

void VulkanRenderer::InitVulkan(UI::UIManager* UIManager)
{
	CreateInstance();
	SetupDebugMessenger();
	CreateSurface();
	PickPhysicalDevice();
	CreateLogicalDevice();
	CreateAllocator(); // Create Memory Allocator
	CreateSwapChain();
	CreateImageViews();
	InitOffscreen();
	CreateRenderPass();
	CreateDescriptorSetLayout();
	CreateGraphicsPipeline();
	CreateCommandPool(commandPool, 0);
	CreateDepthResources();
	CreateFrameBuffers();

	// Create Variables for Offscreen Rendering
	CreateOffscreenVariables();

	// Create Sampler
	CreateTextureSampler();
	UIManager->GetWindowViewport()->SetTextureSampler(textureSampler);

	InitTexture(offscreenTexture, "textures\\texture.jpg");
	UIManager->GetWindowViewport()->SetSceneTexture(offscreenTexture);

	// Initialize Camera
	InitCamera(camera, glm::vec3(0.0f, 0.0f, 10.0f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, 1.0f, 0.0f), 60.0f,
		(float)offscreenExtent.width / (float)offscreenExtent.height, 0.1f, 100.0f);

	UIManager->GetWindowSettings()->SetCamera(&camera);

	// Initialize Lights
	InitLight(light, glm::vec3(-2.0f, 0.0f, 2.0f), glm::vec3(0.1f, 0.1f, 0.1f), glm::vec3(0.6f, 0.6f, 1.0f), 0.5f, 16);

	InitMesh(1, "models\\chalet.obj", "textures\\chalet.jpg");
	InitMesh(2, "models\\space_engineer.obj", "textures\\space_engineer.jpg");
	InitMeshCube(3, glm::vec3(1.0f, 0.0f, 0.0f)); //Initialize Components with default cube mesh
	InitMeshCube(4); //Initialize Components with default cube mesh
	InitMeshCube(5);

	//CreateUniformBuffers();
	CreateLightBuffers();
	CreateViewBuffers();
	CreateDescriptorPool();

	CreateDescriptorSets();

	CreateMainCommandBuffers();
	CreateSyncObjects();
	SetupImGui();
}

void VulkanRenderer::InitTexture(Texture& texture, std::string texture_path)
{
	CreateTextureImage(texture, texture_path);
	CreateTextureImageView(texture);
}

void VulkanRenderer::SetupImGui()
{
	SetupImGuiWindow();

	// Setup Dear ImGui Context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;

	ImGui::StyleColorsDark();

	ImGui_ImplGlfw_InitForVulkan(window, true);

	ImGui_ImplVulkan_InitInfo init_info = {};
	init_info.Instance = instance;
	init_info.PhysicalDevice = physicalDevice;
	init_info.Device = device;
	init_info.QueueFamily = FindQueueFamilies(physicalDevice).graphicsFamily.value();
	init_info.Queue = presentQueue;
	init_info.PipelineCache = pipelineCache;
	init_info.DescriptorPool = imguiDescriptorPool;
	init_info.Allocator = nullptr;
	init_info.MinImageCount = minImageCount;
	init_info.ImageCount = minImageCount;
	//init_info.CheckVkResultFn = check_vk_result;

	ImGui_ImplVulkan_Init(&init_info, imguiRenderPass);

	imguiCommandBuffers[currentFrame] = BeginSingleTimeCommands();
	ImGui_ImplVulkan_CreateFontsTexture(imguiCommandBuffers[currentFrame]);
	EndSingleTimeCommands(imguiCommandBuffers[currentFrame]);
}

void VulkanRenderer::SetupImGuiWindow()
{
	VkBool32 res;
	vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, FindQueueFamilies(physicalDevice).graphicsFamily.value(), surface, &res);
	if (res != VK_TRUE)
	{
		fprintf(stderr, "Error no WSI support on physical device 0\n");
		exit(-1);
	}

	// Select Surface Format
	const VkFormat requestSurfaceImageFormat[] = { VK_FORMAT_B8G8R8A8_UNORM, VK_FORMAT_R8G8B8A8_UNORM, VK_FORMAT_B8G8R8_UNORM, VK_FORMAT_R8G8B8_UNORM };
	const VkColorSpaceKHR requestSurfaceColorSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR;

	//imgui_window->SurfaceFormat = SelectSurfaceFormats(requestSurfaceImageFormat, (size_t)IM_ARRAYSIZE(requestSurfaceImageFormat), requestSurfaceColorSpace);

	// Select Present Mode
#ifdef IMGUI_UNLIMITED_FRAME_RATE
	VkPresentModeKHR present_modes[] = { VK_PRESENT_MODE_MAILBOX_KHR, VK_PRESENT_MODE_IMMEDIATE_KHR, VK_PRESENT_MODE_FIFO_KHR };
#else
	VkPresentModeKHR present_modes[] = { VK_PRESENT_MODE_FIFO_KHR };
#endif
	//imgui_window->PresentMode = SelectPresentMode(present_modes, IM_ARRAYSIZE(present_modes));

	IM_ASSERT(swapChainImages.size() >= 2);
	CreateImGuiVariables();
}

VkSurfaceFormatKHR VulkanRenderer::SelectSurfaceFormats(const VkFormat* request_formats, int request_formats_count, VkColorSpaceKHR request_color_space)
{
	uint32_t format_count;
	vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &format_count, NULL);

	std::vector<VkSurfaceFormatKHR> formats;
	formats.resize(format_count);

	vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &format_count, formats.data());

	if (format_count == 1)
	{
		if (formats[0].format == VK_FORMAT_UNDEFINED)
		{
			VkSurfaceFormatKHR ret;
			ret.format = request_formats[0];
			ret.colorSpace = request_color_space;
			return ret;
		}
		else
		{
			return formats[0];
		}
	}
	else
	{
		for (int request_i = 0; request_i < request_formats_count; request_i++)
		{
			for (uint32_t format_i = 0; format_i < format_count; format_i++)
			{
				if (formats[format_i].format == request_formats[request_i] && formats[format_i].colorSpace == request_color_space)
					return formats[format_i];
			}
		}

		return formats[0];
	}
}

VkPresentModeKHR VulkanRenderer::SelectPresentMode(const VkPresentModeKHR* request_modes, int request_modes_count)
{
	uint32_t mode_count;
	vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &mode_count, NULL);

	std::vector<VkPresentModeKHR> modes;
	modes.resize(mode_count);

	vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &mode_count, modes.data());

	for (int request_i = 0; request_i < request_modes_count; request_i++)
	{
		for (uint32_t mode_i = 0; mode_i < mode_count; mode_i++)
		{
			if (request_modes[request_i] == modes[mode_i])
				return request_modes[request_i];
		}
	}

	return VK_PRESENT_MODE_FIFO_KHR;
}

void VulkanRenderer::RecreateSwapChain(UI::UIManager* UIManager)
{
	int width = 0, height = 0;
	glfwGetFramebufferSize(window, &width, &height);

	while (width == 0 || height == 0)
	{
		glfwGetFramebufferSize(window, &width, &height);
		glfwWaitEvents();
	}

	ImGui_ImplVulkan_SetMinImageCount(minImageCount);

	vkDeviceWaitIdle(device);

	CleanupSwapChain();
	CleanupOffscreen();

	CreateSwapChain();
	CreateImageViews();
	CreateRenderPass();
	CreateGraphicsPipeline();
	CreateDepthResources();
	CreateFrameBuffers();

	// Update offscreen extent to match viewport size
	ImVec2 viewportSize = UIManager->GetWindowViewport()->GetViewportSize();
	offscreenExtent.width = static_cast<uint32_t>(viewportSize.x);
	offscreenExtent.height = static_cast<uint32_t>(viewportSize.y);

	CreateOffscreenVariables();

	// Recalculate Camera Perspective if window size changed
	UpdatePerspective(camera, camera.fov, (float)offscreenExtent.width / (float)offscreenExtent.height, 0.1f, 100.0f);

	CreateUniformBuffers();
	CreateLightBuffers();
	CreateViewBuffers();
	CreateDescriptorPool();
	CreateDescriptorSets();
	CreateMainCommandBuffers();

	CreateImGuiVariables();
}

void VulkanRenderer::InitOffscreen()
{
	// Initialize Variables needed for Offscreen Framebuffer/Attachment Creation
	offscreenExtent.width = 1024;
	offscreenExtent.height = 1024;
	offscreenFormat = VK_FORMAT_R8G8B8A8_SRGB;
}

void VulkanRenderer::CreateOffscreenVariables()
{
	CreateOffscreenAttachments();
	CreateOffscreenDepthAttachment();
	CreateOffscreenFramebuffers();
}

void VulkanRenderer::CreateImGuiVariables()
{
	CreateImGuiRenderPass();
	CreateImGuiFramebuffers();
	CreateImGuiDescriptorPool();
	CreateImGuiCommandBuffers();
}

void VulkanRenderer::CreateInstance()
{
	if (enableValidationLayers && !CheckValidationLayerSupport())
	{
		throw std::runtime_error("validation layers requested, but not available!");
	}

	VkApplicationInfo appInfo = {};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = "Hello Triangle";
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.pEngineName = "No Engine";
	appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.apiVersion = VK_API_VERSION_1_2;

	VkInstanceCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pApplicationInfo = &appInfo;

	auto extensions = GetRequiredExtensions();
	createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
	createInfo.ppEnabledExtensionNames = extensions.data();

	if (enableValidationLayers)
	{
		createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
		createInfo.ppEnabledLayerNames = validationLayers.data();
	}
	else
	{
		createInfo.enabledLayerCount = 0;
	}

	VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo;
	if (enableValidationLayers)
	{
		createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
		createInfo.ppEnabledLayerNames = validationLayers.data();

		PopulateDebugMessengerCreateInfo(debugCreateInfo);
		createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
	}
	else {
		createInfo.enabledLayerCount = 0;

		createInfo.pNext = nullptr;
	}

	if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create instance!");
	}

	uint32_t vulkanExtensionCount = 0;
	vkEnumerateInstanceExtensionProperties(nullptr, &vulkanExtensionCount, nullptr);

	std::vector<VkExtensionProperties> vulkanExtensions(vulkanExtensionCount);
	vkEnumerateInstanceExtensionProperties(nullptr, &vulkanExtensionCount, vulkanExtensions.data());

	std::cout << "available extensions:" << std::endl;

	for (const auto& vulkanExtension : vulkanExtensions)
	{
		std::cout << "\t" << vulkanExtension.extensionName << std::endl;
	}
}

bool VulkanRenderer::CheckValidationLayerSupport()
{
	uint32_t layerCount;
	vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

	std::vector<VkLayerProperties> availableLayers(layerCount);
	vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

	for (const char* layerName : validationLayers)
	{
		bool layerFound = false;

		for (const auto& layerProperties : availableLayers)
		{
			if (strcmp(layerName, layerProperties.layerName) == 0)
			{
				layerFound = true;
				break;
			}
		}

		if (!layerFound)
		{
			return false;
		}
	}

	return true;
}

std::vector<const char*> VulkanRenderer::GetRequiredExtensions()
{
	uint32_t glfwExtensionCount = 0;
	const char** glfwExtensions;
	glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

	std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

	if (enableValidationLayers)
	{
		extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
	}

	return extensions;
}

void VulkanRenderer::SetupDebugMessenger()
{
	if (!enableValidationLayers) return;

	VkDebugUtilsMessengerCreateInfoEXT createInfo{};
	PopulateDebugMessengerCreateInfo(createInfo);

	/*if (CreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr, &debugMessenger) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to set up debug messenger!");
	}*/
}

void VulkanRenderer::CreateSurface()
{
	if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create window surface!");
	}
}

void VulkanRenderer::PickPhysicalDevice()
{
	uint32_t deviceCount = 0;
	vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

	if (deviceCount == 0)
	{
		throw std::runtime_error("failed to find GPUs with Vulkan support!");
	}

	std::vector<VkPhysicalDevice> availableDevices(deviceCount);
	vkEnumeratePhysicalDevices(instance, &deviceCount, availableDevices.data());

	std::cout << "available devices:" << std::endl;

	for (const auto& availableDevice : availableDevices)
	{
		VkPhysicalDeviceProperties deviceProperties;
		vkGetPhysicalDeviceProperties(availableDevice, &deviceProperties);
		std::cout << "\t" << deviceProperties.deviceName << std::endl;
	}

	std::vector<VkPhysicalDevice> suitableDevices;

	// Iterate through available devices
	for (const auto& device : availableDevices)
	{
		if (IsDeviceSuitable(device))
		{
			// Add devices which meet requirements to suitable devices vector
			suitableDevices.push_back(device);
		}
	}

	VkPhysicalDevice bestDevice = {};
	int bestRating = 0;

	// If there is only one suitable device, don't bother checking it's rating
	if (suitableDevices.size() > 1)
	{
		std::cout << "suitable device:" << std::endl;

		// Iterate through devices which meet requirements
		for (const auto& device : suitableDevices)
		{
			// Get Device Rating
			int rating = RateDeviceSuitability(device);

			// Print device Name and Score
			VkPhysicalDeviceProperties deviceProperties;
			vkGetPhysicalDeviceProperties(device, &deviceProperties);
			std::cout << "\t" << deviceProperties.deviceName << " - " << std::to_string(rating) << std::endl;

			// If Device rating is better than best rating, device is new best device
			if (rating > bestRating)
			{
				bestRating = rating;
				bestDevice = device;
			}
		}
	}
	else
	{
		bestDevice = suitableDevices[0];
	}

	// Set physical device to best device
	physicalDevice = bestDevice;

	VkPhysicalDeviceProperties deviceProperties;
	vkGetPhysicalDeviceProperties(physicalDevice, &deviceProperties);
	std::cout << "selected device:" << std::endl;
	std::cout << "\t" << deviceProperties.deviceName << std::endl;

	if (physicalDevice == VK_NULL_HANDLE)
	{
		throw std::runtime_error("failed to find a suitable GPU!");
	}
}

int VulkanRenderer::RateDeviceSuitability(VkPhysicalDevice device)
{
	VkPhysicalDeviceProperties deviceProperties;
	VkPhysicalDeviceFeatures deviceFeatures;

	vkGetPhysicalDeviceProperties(device, &deviceProperties);
	vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

	int score = 0;

	// Discrete GPUs have a significant performance advantage
	if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
		score += 1000;
	}

	// Maximum possible size of textures affects graphics quality
	score += deviceProperties.limits.maxImageDimension2D;

	// Application can't function without geometry shaders
	if (!deviceFeatures.geometryShader) {
		return 0;
	}

	return score;
}

bool VulkanRenderer::IsDeviceSuitable(VkPhysicalDevice device)
{
	QueueFamilyIndices indices = FindQueueFamilies(device);

	bool extensionsSupported = CheckDeviceExtensionsSupport(device);

	bool swapChainAdequate = false;
	if (extensionsSupported)
	{
		SwapChainSupportDetails swapChainSupport = QuerySwapChainSupport(device);
		swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
	}

	VkPhysicalDeviceFeatures supportedFeatures;
	vkGetPhysicalDeviceFeatures(device, &supportedFeatures);

	return indices.isComplete() && extensionsSupported && swapChainAdequate && supportedFeatures.samplerAnisotropy;
}

QueueFamilyIndices VulkanRenderer::FindQueueFamilies(VkPhysicalDevice device)
{
	QueueFamilyIndices indices;

	uint32_t queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

	std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

	int i = 0;
	for (const auto& queueFamily : queueFamilies)
	{
		if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
		{
			indices.graphicsFamily = i;
		}

		VkBool32 presentSupport = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);

		if (presentSupport)
		{
			indices.presentFamily = i;
		}

		if (indices.isComplete())
		{
			break;
		}

		i++;
	}

	return indices;
}

bool VulkanRenderer::CheckDeviceExtensionsSupport(VkPhysicalDevice device)
{
	uint32_t extensionCount;
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

	std::vector<VkExtensionProperties> availableExtensions(extensionCount);
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

	std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

	for (const auto& extension : availableExtensions)
	{
		requiredExtensions.erase(extension.extensionName);
	}

	return requiredExtensions.empty();
}

SwapChainSupportDetails VulkanRenderer::QuerySwapChainSupport(VkPhysicalDevice device)
{
	// Query Capabilities
	SwapChainSupportDetails details;

	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

	// Query Formats
	uint32_t formatCount;
	vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);

	if (formatCount != 0)
	{
		details.formats.resize(formatCount);
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
	}

	// Query Present Modes
	uint32_t presentModeCount;
	vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);

	if (presentModeCount != 0) {
		details.presentModes.resize(presentModeCount);
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.presentModes.data());
	}

	return details;
}

void VulkanRenderer::CreateLogicalDevice()
{
	QueueFamilyIndices indices = FindQueueFamilies(physicalDevice);

	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
	std::set<uint32_t> uniqueQueueFamilies = { indices.graphicsFamily.value(), indices.presentFamily.value() };

	float queuePriority = 1.0f;
	for (uint32_t queueFamily : uniqueQueueFamilies) {
		VkDeviceQueueCreateInfo queueCreateInfo = {};
		queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfo.queueFamilyIndex = queueFamily;
		queueCreateInfo.queueCount = 1;
		queueCreateInfo.pQueuePriorities = &queuePriority;
		queueCreateInfos.push_back(queueCreateInfo);
	}

	// Enabled GPU Features
	VkPhysicalDeviceFeatures deviceFeatures = {};
	deviceFeatures.samplerAnisotropy = VK_TRUE;

	VkDeviceCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	createInfo.pQueueCreateInfos = queueCreateInfos.data();
	createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
	createInfo.pEnabledFeatures = &deviceFeatures;
	createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
	createInfo.ppEnabledExtensionNames = deviceExtensions.data();

	if (enableValidationLayers)
	{
		createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
		createInfo.ppEnabledLayerNames = validationLayers.data();
	}
	else
	{
		createInfo.enabledLayerCount = 0;
	}

	VkResult result = vkCreateDevice(physicalDevice, &createInfo, nullptr, &device);

	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create logical device!");
	}

	vkGetDeviceQueue(device, indices.graphicsFamily.value(), 0, &graphicsQueue);
	vkGetDeviceQueue(device, indices.presentFamily.value(), 0, &presentQueue);
}

void VulkanRenderer::CreateAllocator()
{
	// Initialize Allocator Info
	VmaAllocatorCreateInfo allocatorInfo = {};
	allocatorInfo.physicalDevice = physicalDevice;
	allocatorInfo.device = device;
	allocatorInfo.instance = instance;

	// Create Allocator
	vmaCreateAllocator(&allocatorInfo, &allocator);
}

void VulkanRenderer::CreateSwapChain()
{
	SwapChainSupportDetails swapChainSupport = QuerySwapChainSupport(physicalDevice);

	VkSurfaceFormatKHR surfaceFormat = ChooseSwapSurfaceFormat(swapChainSupport.formats);
	VkPresentModeKHR presentMode = ChooseSwapPresentMode(swapChainSupport.presentModes);
	VkExtent2D extent = ChooseSwapExtent(swapChainSupport.capabilities);

	uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
	minImageCount = imageCount;
	//imgui_window->ImageCount = imageCount;

	if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount)
	{
		imageCount = swapChainSupport.capabilities.maxImageCount;
	}

	VkSwapchainCreateInfoKHR createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	createInfo.surface = surface;
	createInfo.minImageCount = imageCount;
	createInfo.imageFormat = surfaceFormat.format;
	createInfo.imageColorSpace = surfaceFormat.colorSpace;
	createInfo.imageExtent = extent;
	createInfo.imageArrayLayers = 1;
	createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;

	QueueFamilyIndices indices = FindQueueFamilies(physicalDevice);
	uint32_t queueFamilyIndices[] = { indices.graphicsFamily.value(), indices.presentFamily.value() };

	if (indices.graphicsFamily != indices.presentFamily) {
		createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		createInfo.queueFamilyIndexCount = 2;
		createInfo.pQueueFamilyIndices = queueFamilyIndices;
	}
	else {
		createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		createInfo.queueFamilyIndexCount = 0; // Optional
		createInfo.pQueueFamilyIndices = nullptr; // Optional
	}

	createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
	createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	createInfo.presentMode = presentMode;
	createInfo.clipped = VK_TRUE;
	createInfo.oldSwapchain = VK_NULL_HANDLE;

	if (vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapChain) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create swap chain!");
	}

	vkGetSwapchainImagesKHR(device, swapChain, &imageCount, nullptr);
	swapChainImages.resize(imageCount);
	vkGetSwapchainImagesKHR(device, swapChain, &imageCount, swapChainImages.data());

	swapChainImageFormat = surfaceFormat.format;
	swapChainExtent = extent;
}

VkSurfaceFormatKHR VulkanRenderer::ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats)
{
	for (const auto& availableFormat : availableFormats)
	{
		if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
		{
			return availableFormat;
		}
	}

	return availableFormats[0];
}

VkPresentModeKHR VulkanRenderer::ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes)
{
	for (const auto& availablePresentMode : availablePresentModes)
	{
		if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR)
		{
			return availablePresentMode;
		}
	}

	return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D VulkanRenderer::ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities)
{
	if (capabilities.currentExtent.width != UINT32_MAX)
	{
		return capabilities.currentExtent;
	}
	else
	{
		int width, height;
		glfwGetFramebufferSize(window, &width, &height);

		VkExtent2D actualExtent = {
		static_cast<uint32_t>(width),
		static_cast<uint32_t>(height)
		};

		actualExtent.width = std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, actualExtent.width));
		actualExtent.height = std::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, actualExtent.height));

		return actualExtent;
	}
}

void VulkanRenderer::CreateImageViews()
{
	swapChainImageViews.resize(swapChainImages.size());

	for (size_t i = 0; i < swapChainImages.size(); i++)
	{
		swapChainImageViews[i] = CreateImageView(swapChainImages[i], swapChainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT);
	}
}

void VulkanRenderer::CreateOffscreenAttachments()
{
	offscreenAttachments.resize(swapChainImages.size());

	// Create Image and Image View to be attached to framebuffer
	for (size_t i = 0; i < offscreenAttachments.size(); i++)
	{
		CreateImage(offscreenExtent.width, offscreenExtent.height,
			offscreenFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, offscreenAttachments[i].image, VMA_MEMORY_USAGE_GPU_ONLY, offscreenAttachments[i].allocation);

		offscreenAttachments[i].imageView = CreateImageView(offscreenAttachments[i].image, offscreenFormat, VK_IMAGE_ASPECT_COLOR_BIT);
	}
}

VkImageView VulkanRenderer::CreateImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags)
{
	VkImageViewCreateInfo viewInfo = {};
	viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewInfo.image = image;
	viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	viewInfo.format = format;
	viewInfo.subresourceRange.aspectMask = aspectFlags;
	viewInfo.subresourceRange.baseMipLevel = 0;
	viewInfo.subresourceRange.levelCount = 1;
	viewInfo.subresourceRange.baseArrayLayer = 0;
	viewInfo.subresourceRange.layerCount = 1;

	VkImageView imageView;
	if (vkCreateImageView(device, &viewInfo, nullptr, &imageView) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create texture image view!");
	}

	return imageView;
}

void VulkanRenderer::CreateRenderPass()
{
	VkAttachmentDescription colorAttachment = {};
	colorAttachment.format = offscreenFormat;
	colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

	VkAttachmentReference colorAttachmentRef = {};
	colorAttachmentRef.attachment = 0;
	colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentDescription depthAttachment = {};
	depthAttachment.format = FindDepthFormat();
	depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkAttachmentReference depthAttachmentRef = {};
	depthAttachmentRef.attachment = 1;
	depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass = {};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorAttachmentRef;
	subpass.pDepthStencilAttachment = &depthAttachmentRef;

	VkSubpassDependency dependency = {};
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;
	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.srcAccessMask = 0;
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	std::array<VkAttachmentDescription, 2> attachments = { colorAttachment, depthAttachment };
	VkRenderPassCreateInfo renderPassInfo = {};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
	renderPassInfo.pAttachments = attachments.data();
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpass;
	renderPassInfo.dependencyCount = 1;
	renderPassInfo.pDependencies = &dependency;

	if (vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create render pass!");
	}
}

void VulkanRenderer::CreateImGuiRenderPass()
{
	VkAttachmentDescription colorAttachment = {};
	colorAttachment.format = swapChainImageFormat;
	colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	VkAttachmentReference colorAttachmentRef = {};
	colorAttachmentRef.attachment = 0;
	colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass = {};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorAttachmentRef;

	VkSubpassDependency dependency = {};
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;
	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	VkRenderPassCreateInfo renderPassInfo = {};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.attachmentCount = 1;
	renderPassInfo.pAttachments = &colorAttachment;
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpass;
	renderPassInfo.dependencyCount = 1;
	renderPassInfo.pDependencies = &dependency;

	if (vkCreateRenderPass(device, &renderPassInfo, nullptr, &imguiRenderPass) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create Imgui render pass!");
	}
}

VkFormat VulkanRenderer::FindDepthFormat()
{
	return FindSupportedFormat({ VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
		VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
}

VkFormat VulkanRenderer::FindSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features)
{
	for (VkFormat format : candidates)
	{
		VkFormatProperties props;
		vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &props);

		if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features)
		{
			return format;
		}
		else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features)
		{
			return format;
		}
	}

	throw std::runtime_error("failed to find supported format!");
}

void VulkanRenderer::CreateDescriptorSetLayout()
{
	VkDescriptorSetLayoutBinding uboLayoutBinding = {};
	uboLayoutBinding.binding = 0;
	uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	uboLayoutBinding.descriptorCount = 1;
	uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	uboLayoutBinding.pImmutableSamplers = nullptr; // Optional

	VkDescriptorSetLayoutBinding lightLayoutBinding = {};
	lightLayoutBinding.binding = 1;
	lightLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	lightLayoutBinding.descriptorCount = 1;
	lightLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	lightLayoutBinding.pImmutableSamplers = nullptr; // Optional

	VkDescriptorSetLayoutBinding viewLayoutBinding = {};
	viewLayoutBinding.binding = 2;
	viewLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	viewLayoutBinding.descriptorCount = 1;
	viewLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	viewLayoutBinding.pImmutableSamplers = nullptr;

	VkDescriptorSetLayoutBinding samplerLayoutBinding = {};
	samplerLayoutBinding.binding = 3;
	samplerLayoutBinding.descriptorCount = 1;
	samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	samplerLayoutBinding.pImmutableSamplers = nullptr;
	samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	std::array<VkDescriptorSetLayoutBinding, 4> bindings = { uboLayoutBinding, lightLayoutBinding, viewLayoutBinding, samplerLayoutBinding };

	VkDescriptorSetLayoutCreateInfo layoutInfo = {};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
	layoutInfo.pBindings = bindings.data();

	if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create descriptor set layout!");
	}
}

void VulkanRenderer::CreateGraphicsPipeline()
{
	// Read Shader Code from files
	auto vertShaderCode = ReadFile("shaders/vert.spv");
	auto fragShaderCode = ReadFile("shaders/frag.spv");

	// Create Shader Modules from code
	VkShaderModule vertShaderModule = CreateShaderModule(vertShaderCode);
	VkShaderModule fragShaderModule = CreateShaderModule(fragShaderCode);

	// Create Shader Stage Info
	VkPipelineShaderStageCreateInfo vertShaderStageInfo = {};
	vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vertShaderStageInfo.module = vertShaderModule;
	vertShaderStageInfo.pName = "main";

	VkPipelineShaderStageCreateInfo fragShaderStageInfo = {};
	fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	fragShaderStageInfo.module = fragShaderModule;
	fragShaderStageInfo.pName = "main";

	// Create Array for Shader Stage Info
	VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

	auto bindingDescription = Vertex::getBindingDescription();
	auto attributeDescriptions = Vertex::getAttributeDescriptions();

	// Create Vertex Input Info
	VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
	vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInputInfo.vertexBindingDescriptionCount = 1;
	vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
	vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
	vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

	// Create Input Assembly Info
	VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
	inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	inputAssembly.primitiveRestartEnable = VK_FALSE;

	// Define Viewport Size
	VkViewport viewport = {};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = (float)swapChainExtent.width;
	viewport.height = (float)swapChainExtent.height;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	// Define Scissor Extent (Pixel Outside Scissor Rectangle will be discarded
	VkRect2D scissor = {};
	scissor.offset = { 0, 0 };
	scissor.extent = swapChainExtent;

	// Create Viewport State from Viewport and Scissor Rectangle
	VkPipelineViewportStateCreateInfo viewportState = {};
	viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState.viewportCount = 1;
	viewportState.pViewports = &viewport;
	viewportState.scissorCount = 1;
	viewportState.pScissors = &scissor;

	// Rasterization Stage Creation
	VkPipelineRasterizationStateCreateInfo rasterizer = {};
	rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizer.depthClampEnable = VK_FALSE;
	rasterizer.rasterizerDiscardEnable = VK_FALSE;
	rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizer.lineWidth = 1.0f;
	rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
	rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rasterizer.depthBiasEnable = VK_FALSE;
	rasterizer.depthBiasConstantFactor = 0.0f; // Optional
	rasterizer.depthBiasClamp = 0.0f; // Optional
	rasterizer.depthBiasSlopeFactor = 0.0f; // Optional

	// Multisampling
	VkPipelineMultisampleStateCreateInfo multisampling = {};
	multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampling.sampleShadingEnable = VK_FALSE;
	multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	multisampling.minSampleShading = 1.0f; // Optional
	multisampling.pSampleMask = nullptr; // Optional
	multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
	multisampling.alphaToOneEnable = VK_FALSE; // Optional

	VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
	colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	colorBlendAttachment.blendEnable = VK_FALSE;
	colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
	colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
	colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD; // Optional
	colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
	colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
	colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD; // Optional

	VkPipelineColorBlendStateCreateInfo colorBlending = {};
	colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlending.logicOpEnable = VK_FALSE;
	colorBlending.logicOp = VK_LOGIC_OP_COPY; // Optional
	colorBlending.attachmentCount = 1;
	colorBlending.pAttachments = &colorBlendAttachment;
	colorBlending.blendConstants[0] = 0.0f; // Optional
	colorBlending.blendConstants[1] = 0.0f; // Optional
	colorBlending.blendConstants[2] = 0.0f; // Optional
	colorBlending.blendConstants[3] = 0.0f; // Optional

	VkDynamicState dynamicStates[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR, VK_DYNAMIC_STATE_LINE_WIDTH };

	VkPipelineDynamicStateCreateInfo dynamicState = {};
	dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicState.dynamicStateCount = 2;
	dynamicState.pDynamicStates = dynamicStates;

	VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = 1;
	pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout;
	pipelineLayoutInfo.pushConstantRangeCount = 0; // Optional
	pipelineLayoutInfo.pPushConstantRanges = nullptr; // Optional

	if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create pipeline layout!");
	}

	VkPipelineDepthStencilStateCreateInfo depthStencil = {};
	depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depthStencil.depthTestEnable = VK_TRUE;
	depthStencil.depthWriteEnable = VK_TRUE;
	depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
	depthStencil.depthBoundsTestEnable = VK_FALSE;
	depthStencil.minDepthBounds = 0.0f; // Optional
	depthStencil.maxDepthBounds = 1.0f; // Optional
	depthStencil.stencilTestEnable = VK_FALSE;
	depthStencil.front = {}; // Optional
	depthStencil.back = {}; // Optional

	// Create Graphics Pipeline
	VkGraphicsPipelineCreateInfo pipelineInfo = {};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineInfo.stageCount = 2;
	pipelineInfo.pStages = shaderStages;

	pipelineInfo.pVertexInputState = &vertexInputInfo;
	pipelineInfo.pInputAssemblyState = &inputAssembly;
	pipelineInfo.pViewportState = &viewportState;
	pipelineInfo.pRasterizationState = &rasterizer;
	pipelineInfo.pMultisampleState = &multisampling;
	pipelineInfo.pDepthStencilState = &depthStencil;
	pipelineInfo.pColorBlendState = &colorBlending;
	//pipelineInfo.pDynamicState = nullptr; // Optional
	pipelineInfo.pDynamicState = &dynamicState;

	pipelineInfo.layout = pipelineLayout;

	pipelineInfo.renderPass = renderPass;
	pipelineInfo.subpass = 0;

	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
	pipelineInfo.basePipelineIndex = -1; // Optional

	if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create graphics pipeline!");
	}

	// Destroy Shader Modules
	vkDestroyShaderModule(device, fragShaderModule, nullptr);
	vkDestroyShaderModule(device, vertShaderModule, nullptr);
}

VkShaderModule VulkanRenderer::CreateShaderModule(const std::vector<char>& code)
{
	VkShaderModuleCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.codeSize = code.size();
	createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

	VkShaderModule shaderModule;
	if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create shader module!");
	}

	return shaderModule;
}

void VulkanRenderer::CreateCommandPool(VkCommandPool& command_pool, VkCommandPoolCreateFlags flags)
{
	QueueFamilyIndices queueFamilyIndices = FindQueueFamilies(physicalDevice);

	VkCommandPoolCreateInfo poolInfo = {};
	poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();
	poolInfo.flags = flags;

	if (vkCreateCommandPool(device, &poolInfo, nullptr, &command_pool) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create command pool!");
	}
}

void VulkanRenderer::CreateDepthResources()
{
	VkFormat depthFormat = FindDepthFormat();

	// Create Image with VMA
	CreateImage(swapChainExtent.width, swapChainExtent.height, depthFormat, VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 
		depthAttachment.image, VMA_MEMORY_USAGE_GPU_ONLY,  depthAttachment.allocation);
	depthAttachment.imageView = CreateImageView(depthAttachment.image, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT);
}

void VulkanRenderer::CreateOffscreenDepthAttachment()
{
	VkFormat depthFormat = FindDepthFormat();

	// Create Image with VMA
	CreateImage(offscreenExtent.width, offscreenExtent.height, depthFormat, VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		offscreenDepthAttachment.image, VMA_MEMORY_USAGE_GPU_ONLY, offscreenDepthAttachment.allocation);
	offscreenDepthAttachment.imageView = CreateImageView(offscreenDepthAttachment.image, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT);
}

void VulkanRenderer::CreateImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VmaMemoryUsage allocationUsage, VmaAllocation& allocation)
{
	VkImageCreateInfo imageInfo = {};
	imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageInfo.imageType = VK_IMAGE_TYPE_2D;
	imageInfo.extent.width = width;
	imageInfo.extent.height = height;
	imageInfo.extent.depth = 1;
	imageInfo.mipLevels = 1;
	imageInfo.arrayLayers = 1;
	imageInfo.format = format;
	imageInfo.tiling = tiling;
	imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageInfo.usage = usage;
	imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	VmaAllocationCreateInfo allocInfo = {};
	allocInfo.usage = allocationUsage;

	if (vmaCreateImage(allocator, &imageInfo, &allocInfo, &image, &allocation, nullptr))
	{
		throw std::runtime_error("failed to create image!");
	}
}

uint32_t VulkanRenderer::FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties)
{
	VkPhysicalDeviceMemoryProperties memProperties;
	vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

	for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
	{
		if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
		{
			return i;
		}
	}

	throw std::runtime_error("failed to find suitable memory type!");
}

void VulkanRenderer::CreateFrameBuffers()
{
	swapChainFramebuffers.resize(swapChainImageViews.size());

	for (size_t i = 0; i < swapChainImageViews.size(); i++)
	{
		std::array<VkImageView, 2> attachments = {
			swapChainImageViews[i],
			depthAttachment.imageView
		};

		VkFramebufferCreateInfo framebufferInfo = {};
		framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.renderPass = renderPass;
		framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
		framebufferInfo.pAttachments = attachments.data();
		framebufferInfo.width = swapChainExtent.width;
		framebufferInfo.height = swapChainExtent.height;
		framebufferInfo.layers = 1;

		if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &swapChainFramebuffers[i]) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create framebuffer!");
		}
	}
}

void VulkanRenderer::CreateOffscreenFramebuffers()
{
	offscreenFramebuffers.resize(offscreenAttachments.size());

	for (int i = 0; i < offscreenFramebuffers.size(); i++)
	{
		std::array<VkImageView, 2> attachments = {
			offscreenAttachments[i].imageView,
			offscreenDepthAttachment.imageView
		};

		VkFramebufferCreateInfo framebufferInfo = {};
		framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.renderPass = renderPass;
		framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
		framebufferInfo.pAttachments = attachments.data();
		framebufferInfo.width = offscreenExtent.width;
		framebufferInfo.height = offscreenExtent.height;
		framebufferInfo.layers = 1;

		if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &offscreenFramebuffers[i]) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create offscreen framebuffer!");
		}
	}
}

void VulkanRenderer::CreateImGuiFramebuffers()
{
	imguiFramebuffers.resize(swapChainImageViews.size());

	for (int i = 0; i < swapChainImageViews.size(); i++)
	{
		VkImageView attachment[1] = { swapChainImageViews[i] };

		VkFramebufferCreateInfo framebufferInfo = {};
		framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.renderPass = imguiRenderPass;
		framebufferInfo.attachmentCount = 1;
		framebufferInfo.pAttachments = attachment;
		framebufferInfo.width = swapChainExtent.width;
		framebufferInfo.height = swapChainExtent.height;
		framebufferInfo.layers = 1;

		if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &imguiFramebuffers[i]) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create Imgui framebuffer!");
		}
	}
}

void VulkanRenderer::CreateTextureImage(Texture& texture, std::string texture_path)
{
	int texWidth, texHeight, texChannels;

	// Load Texture from file
	stbi_uc* pixels = stbi_load(texture_path.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);

	// Get required image size for texture
	VkDeviceSize imageSize = texWidth * texHeight * 4;

	if (!pixels)
	{
		throw std::runtime_error("failed to load texture image!");
	}

	// Create Staging Buffer/Memory for image
	VkBuffer stagingBuffer;
	VmaAllocation stagingAllocation;

	// VMA Buffer Creation
	CreateBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		stagingBuffer, VMA_MEMORY_USAGE_CPU_TO_GPU, stagingAllocation);

	// Copy Texture data to staging buffer
	void* data;
	vmaMapMemory(allocator, stagingAllocation, &data);
	memcpy(data, pixels, static_cast<size_t>(imageSize));
	vmaUnmapMemory(allocator, stagingAllocation);

	// Free Loaded texture
	stbi_image_free(pixels);

	CreateImage(texWidth, texHeight, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		texture.image, VMA_MEMORY_USAGE_GPU_ONLY, texture.allocation);

	TransitionImageLayout(texture.image, VK_FORMAT_R8G8B8A8_SRGB,
		VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

	CopyBufferToImage(stagingBuffer, texture.image, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight));

	TransitionImageLayout(texture.image, VK_FORMAT_R8G8B8A8_SRGB,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	vkDestroyBuffer(device, stagingBuffer, nullptr);
	vmaFreeMemory(allocator, stagingAllocation);
}

void VulkanRenderer::CreateTextureImageView(Texture& texture)
{
	texture.imageView = CreateImageView(texture.image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT);
}

void VulkanRenderer::CreateVertexBuffers(MeshComponent& mesh_component)
{
	VkDeviceSize bufferSize = sizeof(mesh_component.vertices[0]) * mesh_component.vertices.size();

	// Create CPU staging buffer for Vertex data
	VkBuffer stagingBuffer;
	VmaAllocation stagingAllocation;

	CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		stagingBuffer, VMA_MEMORY_USAGE_CPU_TO_GPU, stagingAllocation);

	// Map vertex data to Staging Buffer
	void* data;
	vmaMapMemory(allocator, stagingAllocation, &data);
	memcpy(data, mesh_component.vertices.data(), (size_t)bufferSize);
	vmaUnmapMemory(allocator, stagingAllocation);

	// Create GPU vertex buffer
	CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, mesh_component.vertexBuffer,
		VMA_MEMORY_USAGE_CPU_TO_GPU, mesh_component.vertexAllocation);

	CopyBuffer(stagingBuffer, mesh_component.vertexBuffer, bufferSize);

	vkDestroyBuffer(device, stagingBuffer, nullptr);
	vmaFreeMemory(allocator, stagingAllocation);
}

void VulkanRenderer::CreateIndexBuffers(MeshComponent& mesh_component)
{
	VkDeviceSize bufferSize = sizeof(mesh_component.indices[0]) * mesh_component.indices.size();

	// Create CPU staging buffer for Indices data
	VkBuffer stagingBuffer;
	VmaAllocation stagingAllocation;

	CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		stagingBuffer, VMA_MEMORY_USAGE_CPU_TO_GPU, stagingAllocation);

	// Map indices data to Staging Buffer
	void* data;
	vmaMapMemory(allocator, stagingAllocation, &data);
	memcpy(data, mesh_component.indices.data(), (size_t)bufferSize);
	vmaUnmapMemory(allocator, stagingAllocation);

	// Create GPU indices buffer
	CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT |
		VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		mesh_component.indexBuffer, VMA_MEMORY_USAGE_CPU_TO_GPU, mesh_component.indexAllocation);

	CopyBuffer(stagingBuffer, mesh_component.indexBuffer, bufferSize);

	vkDestroyBuffer(device, stagingBuffer, nullptr);
	vmaFreeMemory(allocator, stagingAllocation);
}

void VulkanRenderer::CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size)
{
	VkCommandBuffer commandBuffer = BeginSingleTimeCommands();

	VkBufferCopy copyRegion = {};
	copyRegion.size = size;
	vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

	EndSingleTimeCommands(commandBuffer);
}

void VulkanRenderer::CreateTextureSampler()
{
	VkSamplerCreateInfo samplerInfo = {};
	samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerInfo.magFilter = VK_FILTER_LINEAR;
	samplerInfo.minFilter = VK_FILTER_LINEAR;

	samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;

	samplerInfo.anisotropyEnable = VK_TRUE;
	samplerInfo.maxAnisotropy = 16;

	samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;

	samplerInfo.unnormalizedCoordinates = VK_FALSE;

	samplerInfo.compareEnable = VK_FALSE;
	samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;

	samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	samplerInfo.mipLodBias = 0.0f;
	samplerInfo.minLod = 0.0f;
	samplerInfo.maxLod = 0.0f;

	if (vkCreateSampler(device, &samplerInfo, nullptr, &textureSampler) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create texture sampler!");
	}
}

void VulkanRenderer::CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VmaMemoryUsage allocationUsage, VmaAllocation& allocation)
{
	// Define Buffer Info and Create Buffer
	VkBufferCreateInfo bufferInfo = {};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = size;
	bufferInfo.usage = usage;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	VmaAllocationCreateInfo allocInfo = {};
	allocInfo.usage = allocationUsage;
	allocInfo.requiredFlags = properties;

	if (vmaCreateBuffer(allocator, &bufferInfo, &allocInfo, &buffer, &allocation, nullptr))
	{
		throw std::runtime_error("failed to allocate buffer memory!");
	}
}

void VulkanRenderer::TransitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout)
{
	VkCommandBuffer commandBuffer = BeginSingleTimeCommands();

	VkImageMemoryBarrier barrier = {};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.oldLayout = oldLayout;
	barrier.newLayout = newLayout;

	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

	barrier.image = image;
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = 1;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;

	VkPipelineStageFlags sourceStage;
	VkPipelineStageFlags destinationStage;

	if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

		sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	}
	else {
		throw std::invalid_argument("unsupported layout transition!");
	}

	vkCmdPipelineBarrier(
		commandBuffer,
		sourceStage, destinationStage,
		0,
		0, nullptr,
		0, nullptr,
		1, &barrier
	);

	EndSingleTimeCommands(commandBuffer);
}

void VulkanRenderer::CopyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height)
{
	VkCommandBuffer commandBuffer = BeginSingleTimeCommands();

	VkBufferImageCopy region = {};
	region.bufferOffset = 0;
	region.bufferRowLength = 0;
	region.bufferImageHeight = 0;

	region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	region.imageSubresource.mipLevel = 0;
	region.imageSubresource.baseArrayLayer = 0;
	region.imageSubresource.layerCount = 1;

	region.imageOffset = { 0, 0, 0 };
	region.imageExtent = {
		width,
		height,
		1
	};

	vkCmdCopyBufferToImage(
		commandBuffer,
		buffer,
		image,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		1,
		&region
	);

	EndSingleTimeCommands(commandBuffer);
}

VkCommandBuffer VulkanRenderer::BeginSingleTimeCommands()
{
	VkCommandBufferAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandPool = commandPool;
	allocInfo.commandBufferCount = 1;

	VkCommandBuffer commandBuffer;
	vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);

	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	vkBeginCommandBuffer(commandBuffer, &beginInfo);

	return commandBuffer;
}

void VulkanRenderer::EndSingleTimeCommands(VkCommandBuffer commandBuffer)
{
	vkEndCommandBuffer(commandBuffer);

	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;

	vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(graphicsQueue);

	vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
}

void VulkanRenderer::CreateUniformBuffer(MeshComponent& mesh_component)
{
	VkDeviceSize bufferSize = sizeof(mesh_component.matrices);

	mesh_component.uniformBuffers.resize(swapChainImages.size());
	mesh_component.uniformAllocations.resize(swapChainImages.size());

	for (size_t i = 0; i < swapChainImages.size(); i++)
	{
		CreateBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			mesh_component.uniformBuffers[i], VMA_MEMORY_USAGE_CPU_TO_GPU, mesh_component.uniformAllocations[i]);
	}
}

void VulkanRenderer::CreateUniformBuffers()
{
	for (ECS::Entity entity : entities)
	{
		MeshComponent& comp = world->GetComponent<MeshComponent>(entity);

		CreateUniformBuffer(comp);
	}
}

void VulkanRenderer::CreateLightBuffers()
{
	VkDeviceSize bufferSize = sizeof(light.buffers);

	light.buffers.resize(swapChainImages.size());
	light.allocations.resize(swapChainImages.size());

	for (int i = 0; i < swapChainImages.size(); i++)
	{
		CreateBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			light.buffers[i], VMA_MEMORY_USAGE_GPU_ONLY, light.allocations[i]);
	}
}

void VulkanRenderer::CreateViewBuffers()
{
	VkDeviceSize bufferSize = sizeof(ViewBufferObject);

	camera.viewBuffers.resize(swapChainImages.size());
	camera.viewAllocations.resize(swapChainImages.size());

	for (int i = 0; i < swapChainImages.size(); i++)
	{
		CreateBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			camera.viewBuffers[i], VMA_MEMORY_USAGE_GPU_ONLY, camera.viewAllocations[i]);
	}
}

void VulkanRenderer::CreateDescriptorPool()
{
	std::array<VkDescriptorPoolSize, 4> poolSizes = {};
	poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolSizes[0].descriptorCount = static_cast<uint32_t>(swapChainImages.size() * entities.size());
	poolSizes[1].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolSizes[1].descriptorCount = static_cast<uint32_t>(swapChainImages.size() * entities.size());
	poolSizes[2].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolSizes[2].descriptorCount = static_cast<uint32_t>(swapChainImages.size() * entities.size());
	poolSizes[3].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	poolSizes[3].descriptorCount = static_cast<uint32_t>(swapChainImages.size() * entities.size());

	VkDescriptorPoolCreateInfo poolInfo = {};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
	poolInfo.pPoolSizes = poolSizes.data();
	poolInfo.maxSets = static_cast<uint32_t>(swapChainImages.size() * entities.size());

	if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create descriptor pool!");
	}
}

void VulkanRenderer::CreateImGuiDescriptorPool()
{
	VkDescriptorPoolSize poolSizes[] =
	{
			{ VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
			{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
			{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
			{ VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
			{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
			{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
			{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
	};

	VkDescriptorPoolCreateInfo poolInfo = {};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
	poolInfo.maxSets = 1000 * IM_ARRAYSIZE(poolSizes);
	poolInfo.poolSizeCount = (uint32_t)IM_ARRAYSIZE(poolSizes);
	poolInfo.pPoolSizes = poolSizes;

	if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &imguiDescriptorPool) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create Imgui descriptor pool!");
	}
}

void VulkanRenderer::CreateDescriptorSets()
{
	for (ECS::Entity entity : entities)
	{
		MeshComponent& comp = world->GetComponent<MeshComponent>(entity);

		std::vector<VkDescriptorSetLayout> layouts(swapChainImages.size(), descriptorSetLayout);
		VkDescriptorSetAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.descriptorPool = descriptorPool;
		allocInfo.descriptorSetCount = static_cast<uint32_t>(swapChainImages.size());
		allocInfo.pSetLayouts = layouts.data();

		comp.descriptorSets.resize(swapChainImages.size());
		if (vkAllocateDescriptorSets(device, &allocInfo, comp.descriptorSets.data()) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to allocate descriptor sets!");
		}

		for (size_t i = 0; i < swapChainImages.size(); i++)
		{
			VkDescriptorBufferInfo meshBufferInfo = {};
			meshBufferInfo.buffer = comp.uniformBuffers[i];
			meshBufferInfo.offset = 0;
			meshBufferInfo.range = sizeof(comp.matrices);

			VkDescriptorBufferInfo lightBufferInfo = {};
			lightBufferInfo.buffer = light.buffers[i];
			lightBufferInfo.offset = 0;
			lightBufferInfo.range = sizeof(LightBufferObject);

			VkDescriptorBufferInfo viewBufferInfo = {};
			viewBufferInfo.buffer = camera.viewBuffers[i];
			viewBufferInfo.offset = 0;
			viewBufferInfo.range = sizeof(ViewBufferObject);

			VkDescriptorImageInfo imageInfo = {};
			imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			imageInfo.imageView = comp.texture.imageView;
			imageInfo.sampler = textureSampler;

			std::array<VkWriteDescriptorSet, 4> descriptorWrites = {};

			descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrites[0].dstSet =comp.descriptorSets[i];
			descriptorWrites[0].dstBinding = 0;
			descriptorWrites[0].dstArrayElement = 0;
			descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			descriptorWrites[0].descriptorCount = 1;
			descriptorWrites[0].pBufferInfo = &meshBufferInfo;

			descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrites[1].dstSet = comp.descriptorSets[i];
			descriptorWrites[1].dstBinding = 1;
			descriptorWrites[1].dstArrayElement = 0;
			descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			descriptorWrites[1].descriptorCount = 1;
			descriptorWrites[1].pBufferInfo = &lightBufferInfo;

			descriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrites[2].dstSet = comp.descriptorSets[i];
			descriptorWrites[2].dstBinding = 2;
			descriptorWrites[2].dstArrayElement = 0;
			descriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			descriptorWrites[2].descriptorCount = 1;
			descriptorWrites[2].pBufferInfo = &viewBufferInfo;

			descriptorWrites[3].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrites[3].dstSet = comp.descriptorSets[i];
			descriptorWrites[3].dstBinding = 3;
			descriptorWrites[3].dstArrayElement = 0;
			descriptorWrites[3].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			descriptorWrites[3].descriptorCount = 1;
			descriptorWrites[3].pImageInfo = &imageInfo;

			vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
		}
	}
}

void VulkanRenderer::CreateMainCommandBuffers()
{
	commandBuffers.resize(offscreenFramebuffers.size());

	CreateCommandBuffers(commandBuffers.data(), (uint32_t)commandBuffers.size(), commandPool);

	for (size_t i = 0; i < commandBuffers.size(); i++)
	{
		VkCommandBufferBeginInfo beginInfo = {};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = 0; // Optional
		beginInfo.pInheritanceInfo = nullptr; // Optional

		if (vkBeginCommandBuffer(commandBuffers[i], &beginInfo) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to begin recording command buffer!");
		}

		VkRenderPassBeginInfo renderPassInfo = {};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass = renderPass;
		renderPassInfo.framebuffer = offscreenFramebuffers[i];
		renderPassInfo.renderArea.offset = { 0, 0 };
		renderPassInfo.renderArea.extent = offscreenExtent;

		std::array<VkClearValue, 2> clearValues = {};
		clearValues[0].color = { 0.0f, 0.0f, 0.0f, 1.0f };
		clearValues[1].depthStencil = { 1.0f, 0 };

		renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
		renderPassInfo.pClearValues = clearValues.data();

		vkCmdBeginRenderPass(commandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

		// Define Viewport Size
		VkViewport viewport = {};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = (float)offscreenExtent.width;
		viewport.height = (float)offscreenExtent.height;
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;

		vkCmdSetViewport(commandBuffers[i], 0, 1, &viewport);

		// Define Scissor Extent (Pixel Outside Scissor Rectangle will be discarded)
		VkRect2D scissor = {};
		scissor.offset = { 0, 0 };
		scissor.extent = offscreenExtent;

		vkCmdSetScissor(commandBuffers[i], 0, 1, &scissor);

		vkCmdBindPipeline(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

		for (ECS::Entity entity : entities)
		{
			MeshComponent& comp = world->GetComponent<MeshComponent>(entity);

			VkBuffer vertexBuffers[] = { comp.vertexBuffer };
			VkDeviceSize offsets[] = { 0 };
			vkCmdBindVertexBuffers(commandBuffers[i], 0, 1, vertexBuffers, offsets);

			vkCmdBindIndexBuffer(commandBuffers[i], comp.indexBuffer, 0, VK_INDEX_TYPE_UINT32);

			vkCmdBindDescriptorSets(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS,
				pipelineLayout, 0, 1, &comp.descriptorSets[i], 0, nullptr);

			vkCmdDrawIndexed(commandBuffers[i], static_cast<uint32_t>(comp.indices.size()), 1, 0, 0, 0); // Draw with Index
		}

		vkCmdEndRenderPass(commandBuffers[i]);

		if (vkEndCommandBuffer(commandBuffers[i]) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to record command buffer!");
		}
	}
}

void VulkanRenderer::CreateImGuiCommandBuffers()
{
	// Create Command Pool/Buffers
	CreateCommandPool(imguiCommandPool, VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
	imguiCommandBuffers.resize(swapChainFramebuffers.size());
	CreateCommandBuffers(imguiCommandBuffers.data(), static_cast<uint32_t>(imguiCommandBuffers.size()), imguiCommandPool);
}

void VulkanRenderer::CreateCommandBuffers(VkCommandBuffer* command_buffer, uint32_t command_buffer_count, VkCommandPool& command_pool)
{
	VkCommandBufferAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandPool = command_pool;
	allocInfo.commandBufferCount = command_buffer_count;

	if (vkAllocateCommandBuffers(device, &allocInfo, command_buffer) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to allocate command buffers!");
	}
}

void VulkanRenderer::CreateSyncObjects()
{
	imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
	renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
	inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
	imagesInFlight.resize(swapChainImages.size(), VK_NULL_HANDLE);

	VkSemaphoreCreateInfo semaphoreInfo = {};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	VkFenceCreateInfo fenceInfo = {};
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]) != VK_SUCCESS ||
			vkCreateSemaphore(device, &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]) != VK_SUCCESS ||
			vkCreateFence(device, &fenceInfo, nullptr, &inFlightFences[i]) != VK_SUCCESS) {

			throw std::runtime_error("failed to create synchronization objects for a frame!");
		}
	}
}

//-------------------------------------------------------------------------------------

void VulkanRenderer::DrawFrame(UI::UIManager* UIManager, float delta_time)
{
	vkWaitForFences(device, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);

	uint32_t imageIndex;
	VkResult result = vkAcquireNextImageKHR(device, swapChain, UINT64_MAX, imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);

	if (result == VK_ERROR_OUT_OF_DATE_KHR)
	{
		RecreateSwapChain(UIManager);
		return;
	}
	else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
	{
		throw std::runtime_error("failed to acquire swap chain image!");
	}

	if (UIManager->GetWindowViewport()->GetViewportSize().x != (float)offscreenExtent.width 
		|| UIManager->GetWindowViewport()->GetViewportSize().y != (float)offscreenExtent.height
		|| newMeshAdded)
	{
		RecreateSwapChain(UIManager);
		newMeshAdded = false;
	}

	// Check if a previous frame is using this image (i.e. there is its fence to wait on)
	if (imagesInFlight[imageIndex] != VK_NULL_HANDLE) {
		vkWaitForFences(device, 1, &imagesInFlight[imageIndex], VK_TRUE, UINT64_MAX);
	}

	// Mark the image as now being in use by this frame
	imagesInFlight[imageIndex] = inFlightFences[currentFrame];

	UpdateUniformBuffers(imageIndex, delta_time);
	UpdateImguiCommandBuffers(imageIndex);

	std::array<VkCommandBuffer, 2> submitCommandBuffers = {  commandBuffers[imageIndex], imguiCommandBuffers[imageIndex] };

	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

	VkSemaphore waitSemaphores[] = { imageAvailableSemaphores[currentFrame] };
	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = waitSemaphores;
	submitInfo.pWaitDstStageMask = waitStages;
	submitInfo.commandBufferCount = static_cast<uint32_t>(submitCommandBuffers.size());
	submitInfo.pCommandBuffers = submitCommandBuffers.data();

	VkSemaphore signalSemaphores[] = { renderFinishedSemaphores[currentFrame] };
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = signalSemaphores;

	vkResetFences(device, 1, &inFlightFences[currentFrame]);

	result = vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFences[currentFrame]);

	if (result == VK_ERROR_OUT_OF_HOST_MEMORY)
	{
		throw std::runtime_error("VkQueueSubmit: Out of Host Memory");
	}
	else if (result == VK_ERROR_OUT_OF_DEVICE_MEMORY)
	{
		throw std::runtime_error("VkQueueSubmit: Out of Device Memory");
	}
	else if (result == VK_ERROR_DEVICE_LOST)
	{
		throw std::runtime_error("VkQueueSubmit: Device Lost");
	}

	VkPresentInfoKHR presentInfo = {};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = signalSemaphores;

	VkSwapchainKHR swapChains[] = { swapChain };
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = swapChains;
	presentInfo.pImageIndices = &imageIndex;
	presentInfo.pResults = nullptr; // Optional

	result = vkQueuePresentKHR(presentQueue, &presentInfo);

	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || framebufferResized)
	{
		framebufferResized = false;
		RecreateSwapChain(UIManager);
	}
	else if (result != VK_SUCCESS)
	{
		throw std::runtime_error("failed to present swap chain image!");
	}

	vkQueueWaitIdle(presentQueue);

	currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;

	offscreenTexture = offscreenAttachments[imageIndex];
	UIManager->GetWindowViewport()->SetSceneTexture(offscreenTexture);
}

void VulkanRenderer::UpdateUniformBuffers(uint32_t currentImage, float delta_time)
{
	static auto startTime = std::chrono::high_resolution_clock::now(); // Time Count Started

	auto currentTime = std::chrono::high_resolution_clock::now();
	float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

	for (ECS::Entity entity : entities)
	{
		MeshComponent& comp = world->GetComponent<MeshComponent>(entity);

		// Init any new components
		if (comp.vertices.empty())
		{
			InitMeshCube(entity);
			newMeshAdded = true;
		}

		MeshMatrices matrice = {};

		matrice.model = BuildMeshTransform(world->GetComponent<TransformComponent>(entity));

		matrice.inv_model = glm::inverse(matrice.model);
		matrice.view = camera.matrices.view;
		matrice.proj = camera.matrices.perspective;
		matrice.proj[1][1] *= -1;

		comp.matrices = matrice;

		void* data;
		vmaMapMemory(allocator, comp.uniformAllocations[currentImage], &data);
		memcpy(data, &matrice, sizeof(matrice));
		vmaUnmapMemory(allocator, comp.uniformAllocations[currentImage]);

		vmaMapMemory(allocator, light.allocations[currentImage], &data);
		memcpy(data, &light.uniformBuffer, sizeof(LightBufferObject));
		vmaUnmapMemory(allocator, light.allocations[currentImage]);

		vmaMapMemory(allocator, camera.viewAllocations[currentImage], &data);
		memcpy(data, &camera.viewBufferObject, sizeof(ViewBufferObject));
		vmaUnmapMemory(allocator, camera.viewAllocations[currentImage]);
	}
}

void VulkanRenderer::UpdateCamera(CameraComponent& camera, Puffin::Input::InputManager* inputManager, float delta_time)
{
	if (inputManager->IsCursorLocked())
	{
		// Camera Movement
		if (inputManager->GetAction("CamMoveLeft").state == Puffin::Input::HELD)
		{
			camera.position += camera.speed * camera.right * delta_time;
		}
		else if (inputManager->GetAction("CamMoveRight").state == Puffin::Input::HELD)
		{
			camera.position -= camera.speed * camera.right * delta_time;
		}

		if (inputManager->GetAction("CamMoveForward").state == Puffin::Input::HELD)
		{
			camera.position += camera.speed * camera.direction * delta_time;
		}
		else if (inputManager->GetAction("CamMoveBackward").state == Puffin::Input::HELD)
		{
			camera.position -= camera.speed * camera.direction * delta_time;
		}

		if (inputManager->GetAction("CamMoveUp").state == Puffin::Input::HELD)
		{
			camera.position += camera.speed * camera.up * delta_time;
		}
		else if (inputManager->GetAction("CamMoveDown").state == Puffin::Input::HELD)
		{
			camera.position -= camera.speed * camera.up * delta_time;
		}

		// Mouse Rotation
		camera.yaw += inputManager->GetMouseXOffset();
		camera.pitch -= inputManager->GetMouseYOffset();

		if (camera.pitch > 89.0f)
			camera.pitch = 89.0f;

		if (camera.pitch < -89.0f)
			camera.pitch = -89.0f;

		// Calculate Direction vector from yaw and pitch of camera
		camera.direction.x = cos(glm::radians(camera.yaw)) * cos(glm::radians(camera.pitch));
		camera.direction.y = sin(glm::radians(camera.pitch));
		camera.direction.z = sin(glm::radians(camera.yaw)) * cos(glm::radians(camera.pitch));
		camera.direction = glm::normalize(camera.direction);
	}

	// Calculate Right, Up and LookAt vectors
	camera.right = glm::normalize(glm::cross(camera.up, camera.direction));
	camera.lookat = camera.position + camera.direction;

	UpdateViewMatrix(camera);
}

glm::mat4 VulkanRenderer::BuildMeshTransform(glm::vec3 position, glm::vec3 rotation, glm::vec3 scale)
{
	// Set Translation
	glm::mat4 model_transform = glm::translate(glm::mat4(1.0f), position);

	// Set Rotation
	model_transform = glm::rotate(model_transform, glm::radians(rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
	model_transform = glm::rotate(model_transform, glm::radians(rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
	model_transform = glm::rotate(model_transform, glm::radians(rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));

	// Set Scale
	model_transform = glm::scale(model_transform, scale);

	return model_transform;
}

glm::mat4 VulkanRenderer::BuildMeshTransform(TransformComponent transform)
{
	// Set Translation
	glm::mat4 model_transform = glm::translate(glm::mat4(1.0f), (glm::vec3)transform.position);

	// Set Rotation
	model_transform = glm::rotate(model_transform, glm::radians(transform.rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
	model_transform = glm::rotate(model_transform, glm::radians(transform.rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
	model_transform = glm::rotate(model_transform, glm::radians(transform.rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));

	// Set Scale
	model_transform = glm::scale(model_transform, (glm::vec3)transform.scale);

	return model_transform;
}

void VulkanRenderer::UpdateImguiCommandBuffers(uint32_t currentImage)
{
	vkResetCommandBuffer(imguiCommandBuffers[currentImage], 0);

	// Fill ImGui Command Buffers with Draw Commands
	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	beginInfo.pInheritanceInfo = nullptr;

	if (vkBeginCommandBuffer(imguiCommandBuffers[currentImage], &beginInfo) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to begin recording Imgui command buffer!");
	}

	VkRenderPassBeginInfo renderPassInfo = {};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.renderPass = imguiRenderPass;
	renderPassInfo.framebuffer = imguiFramebuffers[currentImage];
	renderPassInfo.renderArea.extent = swapChainExtent;
	renderPassInfo.clearValueCount = 1;

	VkClearValue clearValue = {};
	clearValue.color = { 0.0f, 0.0f, 0.0f, 1.0f };

	renderPassInfo.pClearValues = &clearValue;

	vkCmdBeginRenderPass(imguiCommandBuffers[currentImage], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

	// Record Imgui Draw Data and draw functions into command buffer
	ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), imguiCommandBuffers[currentImage]);

	vkCmdEndRenderPass(imguiCommandBuffers[currentImage]);

	if (vkEndCommandBuffer(imguiCommandBuffers[currentImage]) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to record Imgui command buffer!");
	}
}

//-------------------------------------------------------------------------------------

void VulkanRenderer::Cleanup()
{
	CleanupSwapChain();
	CleanupOffscreen();
	CleanupImGui();

	vkDestroySampler(device, textureSampler, nullptr);

	vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);

	// Cleanup Meshes
	for (ECS::Entity entity : entities)
	{
		MeshComponent& comp = world->GetComponent<MeshComponent>(entity);

		CleanupMeshComponent(comp);
	}

	//entities.clear();
	//world.reset();

	// Cleanup Textures
	CleanupFrameBufferAttachment(cube_texture);

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		vkDestroySemaphore(device, renderFinishedSemaphores[i], nullptr);
		vkDestroySemaphore(device, imageAvailableSemaphores[i], nullptr);
		vkDestroyFence(device, inFlightFences[i], nullptr);
	}

	vkDestroyCommandPool(device, commandPool, nullptr);

	// Destroy Memory Allocator
	vmaDestroyAllocator(allocator);

	vkDestroyDevice(device, nullptr);

	vkDestroySurfaceKHR(instance, surface, nullptr);

	if (enableValidationLayers)
	{
		DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
	}

	vkDestroyInstance(instance, nullptr);

	glfwDestroyWindow(window);

	glfwTerminate();
}

void VulkanRenderer::CleanupSwapChain()
{
	CleanupFrameBufferAttachment(depthAttachment);

	for (size_t i = 0; i < swapChainFramebuffers.size(); i++)
	{
		vkDestroyFramebuffer(device, swapChainFramebuffers[i], nullptr);
	}

	vkFreeCommandBuffers(device, commandPool, static_cast<uint32_t>(commandBuffers.size()), commandBuffers.data());

	vkDestroyPipeline(device, graphicsPipeline, nullptr);
	vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
	vkDestroyRenderPass(device, renderPass, nullptr);
	

	for (size_t i = 0; i < swapChainImageViews.size(); i++)
	{
		vkDestroyImageView(device, swapChainImageViews[i], nullptr);
	}

	vkDestroySwapchainKHR(device, swapChain, nullptr);

	vkDestroyDescriptorPool(device, descriptorPool, nullptr);
}

void VulkanRenderer::CleanupOffscreen()
{
	CleanupFrameBufferAttachment(offscreenDepthAttachment);

	for (size_t i = 0; i < offscreenFramebuffers.size(); i++)
	{
		vkDestroyFramebuffer(device, offscreenFramebuffers[i], nullptr);
	}

	for (size_t i = 0; i < offscreenAttachments.size(); i++)
	{
		CleanupFrameBufferAttachment(offscreenAttachments[i]);
	}
}

void VulkanRenderer::CleanupImGui()
{
	for (size_t i = 0; i < imguiFramebuffers.size(); i++)
	{
		vkDestroyFramebuffer(device, imguiFramebuffers[i], nullptr);
	}

	vkDestroyRenderPass(device, imguiRenderPass, nullptr);

	vkFreeCommandBuffers(device, imguiCommandPool, static_cast<uint32_t>(imguiCommandBuffers.size()), imguiCommandBuffers.data());
	vkDestroyCommandPool(device, imguiCommandPool, nullptr);

	ImGui_ImplVulkan_Shutdown();
	ImGui_ImplGlfw_Shutdown();

	vkDestroyDescriptorPool(device, imguiDescriptorPool, nullptr);
}

void VulkanRenderer::CleanupMeshComponent(MeshComponent& mesh_component)
{
	vkDestroyBuffer(device, mesh_component.vertexBuffer, nullptr);
	vmaFreeMemory(allocator, mesh_component.vertexAllocation);

	vkDestroyBuffer(device, mesh_component.indexBuffer, nullptr);
	vmaFreeMemory(allocator, mesh_component.indexAllocation);

	for (int i = 0; i < mesh_component.uniformBuffers.size(); i++)
	{
		vkDestroyBuffer(device, mesh_component.uniformBuffers[i], nullptr);
		vmaFreeMemory(allocator, mesh_component.uniformAllocations[i]);
	}
}

void VulkanRenderer::CleanupFrameBufferAttachment(FrameBufferAttachment& attachment)
{
	// Destroy Attachment Image View
	vkDestroyImageView(device, attachment.imageView, nullptr);

	// Destroy Attachment Image and Memory Allocation
	vmaDestroyImage(allocator, attachment.image, attachment.allocation);
}