//Just a simple handler for simple initialization stuffs
#include "Utilities/BackendHandler.h"

#include <filesystem>
#include <json.hpp>
#include <fstream>

#include <Texture2D.h>
#include <Texture2DData.h>
#include <MeshBuilder.h>
#include <MeshFactory.h>
#include <NotObjLoader.h>
#include <ObjLoader.h>
#include <VertexTypes.h>
#include <ShaderMaterial.h>
#include <RendererComponent.h>
#include <TextureCubeMap.h>
#include <TextureCubeMapData.h>

#include <Timing.h>
#include <GameObjectTag.h>
#include <InputHelpers.h>

#include <IBehaviour.h>
#include <CameraControlBehaviour.h>
#include <FollowPathBehaviour.h>
#include <SimpleMoveBehaviour.h>

int main() {
	int frameIx = 0;
	float fpsBuffer[128];
	float minFps, maxFps, avgFps;
	int selectedVao = 0; // select cube by default
	std::vector<GameObject> controllables;

	BackendHandler::InitAll();

	// Let OpenGL know that we want debug output, and route it to our handler function
	glEnable(GL_DEBUG_OUTPUT);
	glDebugMessageCallback(BackendHandler::GlDebugMessage, nullptr);

	// Enable texturing
	glEnable(GL_TEXTURE_2D);

	// Push another scope so most memory should be freed *before* we exit the app
	{
		#pragma region Shader and ImGui
		Shader::sptr passthroughShader = Shader::Create();
		passthroughShader->LoadShaderPartFromFile("shaders/passthrough_vert.glsl", GL_VERTEX_SHADER);
		passthroughShader->LoadShaderPartFromFile("shaders/passthrough_frag.glsl", GL_FRAGMENT_SHADER);
		passthroughShader->Link();

		// Load our shaders
		Shader::sptr shader = Shader::Create();
		shader->LoadShaderPartFromFile("shaders/vertex_shader.glsl", GL_VERTEX_SHADER);
		shader->LoadShaderPartFromFile("shaders/frag_blinn_phong_textured.glsl", GL_FRAGMENT_SHADER);
		shader->Link();

		glm::vec3 lightPos = glm::vec3(0.0f, 0.0f, 5.0f);
		glm::vec3 lightCol = glm::vec3(0.9f, 0.85f, 0.5f);
		float     lightAmbientPow = 1.5f;
		float     lightSpecularPow = 1.0f;
		glm::vec3 ambientCol = glm::vec3(1.0f);
		float     ambientPow = 0.1f;
		float     lightLinearFalloff = 0.09f;
		float     lightQuadraticFalloff = 0.032f;
	

		//Bool variables that act as toggles for changing the lighting
		bool ambientToggle = false;
		bool specularToggle = false;
		bool noLightingToggle = false;
		bool ambient_And_Specular_Toggle = false;
		bool TextureToggle = false;
		//bool custom_Shader_Toggle = false;
		
		// These are our application / scene level uniforms that don't necessarily update
		// every frame
		shader->SetUniform("u_LightPos", lightPos);
		shader->SetUniform("u_LightCol", lightCol);
		shader->SetUniform("u_AmbientLightStrength", lightAmbientPow);
		shader->SetUniform("u_SpecularLightStrength", lightSpecularPow);
		shader->SetUniform("u_AmbientCol", ambientCol);
		shader->SetUniform("u_AmbientStrength", ambientPow);
		shader->SetUniform("u_LightAttenuationConstant", 1.0f);
		shader->SetUniform("u_LightAttenuationLinear", lightLinearFalloff);
		shader->SetUniform("u_LightAttenuationQuadratic", lightQuadraticFalloff);
	

		//set More uniform variables for lighting toggles
		shader->SetUniform("u_AmbientToggle", (int)ambientToggle);
		shader->SetUniform("u_SpecularToggle", (int)specularToggle);
		shader->SetUniform("u_LightingOff", (int)noLightingToggle);
		shader->SetUniform("u_AmbientAndSpecToggle", (int)ambient_And_Specular_Toggle);
		//shader->SetUniform("u_CustomShaderToggle", (int)custom_Shader_Toggle);

		//Create post effect objects
		PostEffect* basicEffect;

		int activeEffect = 0;
		std::vector<PostEffect*> effects;
		
		BloomEffect* bloomEffect;

		
		//SepiaEffect* sepiaEffect;
		//GreyscaleEffect* greyscaleEffect;
		//ColorCorrectEffect* colorCorrectEffect;

		

		// We'll add some ImGui controls to control our shader
		BackendHandler::imGuiCallbacks.push_back([&]() {
			
			if (ImGui::Checkbox("No Lighting", &noLightingToggle))
			{
				noLightingToggle = true;
				ambientToggle = false;
				specularToggle = false;
				ambient_And_Specular_Toggle = false;
				//custom_Shader_Toggle = false;

			}

			if (ImGui::Checkbox("Ambient", &ambientToggle))
			{
				noLightingToggle = false;
				ambientToggle = true;
				specularToggle = false;
				ambient_And_Specular_Toggle = false;
				//custom_Shader_Toggle = false;

			}

			if (ImGui::Checkbox("Specular", &specularToggle))
			{
				noLightingToggle = false;
				ambientToggle = false;
				specularToggle = true;
				ambient_And_Specular_Toggle = false;
				//custom_Shader_Toggle = false;

			}

			if (ImGui::Checkbox("Ambient + Specular + Diffuse", &ambient_And_Specular_Toggle))
			{
				noLightingToggle = false;
				ambientToggle = false;
				specularToggle = false;
				ambient_And_Specular_Toggle = true;
				//custom_Shader_Toggle = false;
			}

			if (ImGui::Button("Textures On"))
			{
				// Enable texturing
				TextureToggle = false;


				//glEnable(GL_TEXTURE_2D);
			}

			if (ImGui::Button("Textures Off"))
			{
				// Enable texturing
				//glDisable(GL_TEXTURE_2D);

				TextureToggle = true;	
		
			}

			//Re-set the unifrom variables in the shader after Imgui toggles
			shader->SetUniform("u_AmbientToggle", (int)ambientToggle);
			shader->SetUniform("u_SpecularToggle", (int)specularToggle);
			shader->SetUniform("u_LightingOff", (int)noLightingToggle);
			shader->SetUniform("u_AmbientAndSpecToggle", (int)ambient_And_Specular_Toggle);
			shader->SetUniform("u_TextureToggle", (int)TextureToggle);
		
			//Imgui controls to change the value fo the threshold and the blur value for the bloom effect
			if (ImGui::CollapsingHeader("Effect controls"))
			{
				ImGui::SliderInt("Chosen Effect", &activeEffect, 0, effects.size() - 1);

				if (activeEffect == 0)
				{
					ImGui::Text("Active Effect: Bloom Effect");

					BloomEffect* temp = (BloomEffect*)effects[activeEffect];
					float threshold = temp->GetThreshold();
					float passes = temp->GetPasses();

					if (ImGui::SliderFloat("Threshold", &threshold, 0.0f, 1.0f))
					{
						temp->SetThreshold(threshold);
					}

					if (ImGui::SliderFloat("Blur Values", &passes, 0.0f, 10.0f))
					{
						temp->SetPasses(passes);
					}
				}
				/*if (activeEffect == 1)
				{
					ImGui::Text("Active Effect: Greyscale Effect");
					
					GreyscaleEffect* temp = (GreyscaleEffect*)effects[activeEffect];
					float intensity = temp->GetIntensity();

					if (ImGui::SliderFloat("Intensity", &intensity, 0.0f, 1.0f))
					{
						temp->SetIntensity(intensity);
					}
				}
				if (activeEffect == 2)
				{
					ImGui::Text("Active Effect: Color Correct Effect");

					ColorCorrectEffect* temp = (ColorCorrectEffect*)effects[activeEffect];
					static char input[BUFSIZ];
					ImGui::InputText("Lut File to Use", input, BUFSIZ);

					if (ImGui::Button("SetLUT", ImVec2(200.0f, 40.0f)))
					{
						temp->SetLUT(LUT3D(std::string(input)));
					}
				}*/
			}
			/*if (ImGui::CollapsingHeader("Environment generation"))
			{
				if (ImGui::Button("Regenerate Environment", ImVec2(200.0f, 40.0f)))
				{
					EnvironmentGenerator::RegenerateEnvironment();
				}
			}*/
			if (ImGui::CollapsingHeader("Scene Level Lighting Settings"))
			{
				if (ImGui::ColorPicker3("Ambient Color", glm::value_ptr(ambientCol))) {
					shader->SetUniform("u_AmbientCol", ambientCol);
				}
				if (ImGui::SliderFloat("Fixed Ambient Power", &ambientPow, 0.01f, 1.0f)) {
					shader->SetUniform("u_AmbientStrength", ambientPow);
				}
			}
			if (ImGui::CollapsingHeader("Light Level Lighting Settings"))
			{
				if (ImGui::DragFloat3("Light Pos", glm::value_ptr(lightPos), 0.01f, -10.0f, 10.0f)) {
					shader->SetUniform("u_LightPos", lightPos);
				}
				if (ImGui::ColorPicker3("Light Col", glm::value_ptr(lightCol))) {
					shader->SetUniform("u_LightCol", lightCol);
				}
				if (ImGui::SliderFloat("Light Ambient Power", &lightAmbientPow, 0.0f, 1.0f)) {
					shader->SetUniform("u_AmbientLightStrength", lightAmbientPow);
				}
				if (ImGui::SliderFloat("Light Specular Power", &lightSpecularPow, 0.0f, 1.0f)) {
					shader->SetUniform("u_SpecularLightStrength", lightSpecularPow);
				}
				if (ImGui::DragFloat("Light Linear Falloff", &lightLinearFalloff, 0.01f, 0.0f, 1.0f)) {
					shader->SetUniform("u_LightAttenuationLinear", lightLinearFalloff);
				}
				if (ImGui::DragFloat("Light Quadratic Falloff", &lightQuadraticFalloff, 0.01f, 0.0f, 1.0f)) {
					shader->SetUniform("u_LightAttenuationQuadratic", lightQuadraticFalloff);
				}
			}
			
			//auto name = controllables[selectedVao].get<GameObjectTag>().Name;
			//ImGui::Text(name.c_str());
			//auto behaviour = BehaviourBinding::Get<SimpleMoveBehaviour>(controllables[selectedVao]);
			//ImGui::Checkbox("Relative Rotation", &behaviour->Relative);

			ImGui::Text("Q/E -> Yaw\nLeft/Right -> Roll\nUp/Down -> Pitch\nY -> Toggle Mode");
		
			minFps = FLT_MAX;
			maxFps = 0;
			avgFps = 0;
			for (int ix = 0; ix < 128; ix++) {
				if (fpsBuffer[ix] < minFps) { minFps = fpsBuffer[ix]; }
				if (fpsBuffer[ix] > maxFps) { maxFps = fpsBuffer[ix]; }
				avgFps += fpsBuffer[ix];
			}
			ImGui::PlotLines("FPS", fpsBuffer, 128);
			ImGui::Text("MIN: %f MAX: %f AVG: %f", minFps, maxFps, avgFps / 128.0f);
			});

		#pragma endregion 

		// GL states
		glEnable(GL_DEPTH_TEST);
		//glEnable(GL_CULL_FACE);
		glDepthFunc(GL_LEQUAL); // New 

		#pragma region TEXTURE LOADING

		// Load some textures from files
		Texture2D::sptr stone = Texture2D::LoadFromFile("images/Stone_001_Diffuse.png");
		Texture2D::sptr stoneSpec = Texture2D::LoadFromFile("images/Stone_001_Specular.png");
		Texture2D::sptr grass = Texture2D::LoadFromFile("images/grass.jpg");
		Texture2D::sptr noSpec = Texture2D::LoadFromFile("images/grassSpec.png");
		Texture2D::sptr box = Texture2D::LoadFromFile("images/box.bmp");
		Texture2D::sptr boxSpec = Texture2D::LoadFromFile("images/box-reflections.bmp");
		Texture2D::sptr simpleFlora = Texture2D::LoadFromFile("images/SimpleFlora.png");
		LUT3D testCube("cubes/BrightenedCorrection.cube");

		// Load the cube map
		//TextureCubeMap::sptr environmentMap = TextureCubeMap::LoadFromImages("images/cubemaps/skybox/sample.jpg");
		TextureCubeMap::sptr environmentMap = TextureCubeMap::LoadFromImages("images/cubemaps/skybox/ToonSky.jpg"); 

		// Creating an empty texture
		Texture2DDescription desc = Texture2DDescription();  
		desc.Width = 1;
		desc.Height = 1;
		desc.Format = InternalFormat::RGB8;
		Texture2D::sptr texture2 = Texture2D::Create(desc);
		// Clear it with a white colour
		texture2->Clear();

		#pragma endregion

#pragma region Police Car texture
		
		//Load in textures for Police Car Obj
		Texture2DData::sptr policeCarDiffuseMap = Texture2DData::LoadFromFile("images/Police_Car_Texture_FIXED.png");
		//Create New texture to be applied to Police Car.obj
		Texture2D::sptr policeCarDiffuse = Texture2D::Create();
		policeCarDiffuse->LoadData(policeCarDiffuseMap);
	
		//Create empty texture
		Texture2DDescription policeCarDesc = Texture2DDescription();
		policeCarDesc.Width = 1;
		policeCarDesc.Height = 1;
		policeCarDesc.Format = InternalFormat::RGB8;
		Texture2D::sptr policeCarTexture = Texture2D::Create(policeCarDesc);

#pragma endregion

#pragma region Muscle Car Texture

		//Load in textures for Muscle Car Obj
		Texture2DData::sptr muscleCarDiffuseMap = Texture2DData::LoadFromFile("images/Muscle_Car_Texture.png");
		//Create New texture to be applied to Police Car.obj
		Texture2D::sptr muscleCarDiffuse = Texture2D::Create();
		muscleCarDiffuse->LoadData(muscleCarDiffuseMap);

		//Create empty texture
		Texture2DDescription muscleCarDesc = Texture2DDescription();
		muscleCarDesc.Width = 1;
		muscleCarDesc.Height = 1;
		muscleCarDesc.Format = InternalFormat::RGB8;
		Texture2D::sptr muscleCarTexture = Texture2D::Create(muscleCarDesc);

#pragma endregion

#pragma region Buildings Textures
		//Load in textures for Building Obj
		Texture2DData::sptr buildingDiffuseMap = Texture2DData::LoadFromFile("images/Hotel_Texture.png");
		//Create New texture to be applied to Building.obj
		Texture2D::sptr buildingDiffuse = Texture2D::Create();
		buildingDiffuse->LoadData(buildingDiffuseMap);

		//Create empty texture
		Texture2DDescription buildingDesc = Texture2DDescription();
		buildingDesc.Width = 1;
		buildingDesc.Height = 1;
		buildingDesc.Format = InternalFormat::RGB8;
		Texture2D::sptr buildingTexture = Texture2D::Create(buildingDesc);


#pragma endregion

#pragma region House Textures
		//Load in textures for Building Obj
		Texture2DData::sptr houseDiffuseMap = Texture2DData::LoadFromFile("images/House_Texture4.png");
		//Create New texture to be applied to Building.obj
		Texture2D::sptr houseDiffuse = Texture2D::Create();
		houseDiffuse->LoadData(houseDiffuseMap);

		//Create empty texture
		Texture2DDescription houseDesc = Texture2DDescription();
		houseDesc.Width = 1;
		houseDesc.Height = 1;
		houseDesc.Format = InternalFormat::RGB8;
		Texture2D::sptr houseTexture = Texture2D::Create(houseDesc);


#pragma endregion

#pragma region House 2 Texture
		//Load in textures for Building Obj
		Texture2DData::sptr house2DiffuseMap = Texture2DData::LoadFromFile("images/House_Texture2.png");
		//Create New texture to be applied to Building.obj
		Texture2D::sptr house2Diffuse = Texture2D::Create();
		house2Diffuse->LoadData(house2DiffuseMap);

		//Create empty texture
		Texture2DDescription house2Desc = Texture2DDescription();
		house2Desc.Width = 1;
		house2Desc.Height = 1;
		house2Desc.Format = InternalFormat::RGB8;
		Texture2D::sptr house2Texture = Texture2D::Create(house2Desc);


#pragma endregion

#pragma region Plane Texture
		//Load in textures for Building Obj
		Texture2DData::sptr planeDiffuseMap = Texture2DData::LoadFromFile("images/Plane UV's Fixed.png");
		//Create New texture to be applied to Building.obj
		Texture2D::sptr planeDiffuse = Texture2D::Create();
		planeDiffuse->LoadData(planeDiffuseMap);

		//Create empty texture
		Texture2DDescription planeDesc = Texture2DDescription();
		planeDesc.Width = 1;
		planeDesc.Height = 1;
		planeDesc.Format = InternalFormat::RGB8;
		Texture2D::sptr planeTexture = Texture2D::Create(planeDesc);


#pragma endregion



		///////////////////////////////////// Scene Generation //////////////////////////////////////////////////
		#pragma region Scene Generation
		
		// We need to tell our scene system what extra component types we want to support
		GameScene::RegisterComponentType<RendererComponent>();
		GameScene::RegisterComponentType<BehaviourBinding>();
		GameScene::RegisterComponentType<Camera>();

		// Create a scene, and set it to be the active scene in the application
		GameScene::sptr scene = GameScene::Create("test");
		Application::Instance().ActiveScene = scene;

		// We can create a group ahead of time to make iterating on the group faster
		entt::basic_group<entt::entity, entt::exclude_t<>, entt::get_t<Transform>, RendererComponent> renderGroup =
			scene->Registry().group<RendererComponent>(entt::get_t<Transform>());

		// Create a material and set some properties for it
		ShaderMaterial::sptr stoneMat = ShaderMaterial::Create();  
		stoneMat->Shader = shader;
		stoneMat->Set("s_Diffuse", stone);
		stoneMat->Set("s_Specular", stoneSpec);
		stoneMat->Set("u_Shininess", 2.0f);
		stoneMat->Set("u_TextureMix", 0.0f); 

		ShaderMaterial::sptr grassMat = ShaderMaterial::Create();
		grassMat->Shader = shader;
		grassMat->Set("s_Diffuse", grass);
		grassMat->Set("s_Specular", noSpec);
		grassMat->Set("u_Shininess", 2.0f);
		grassMat->Set("u_TextureMix", 0.0f);

		ShaderMaterial::sptr boxMat = ShaderMaterial::Create();
		boxMat->Shader = shader;
		boxMat->Set("s_Diffuse", box);
		boxMat->Set("s_Specular", boxSpec);
		boxMat->Set("u_Shininess", 8.0f);
		boxMat->Set("u_TextureMix", 0.0f);

		ShaderMaterial::sptr simpleFloraMat = ShaderMaterial::Create();
		simpleFloraMat->Shader = shader;
		simpleFloraMat->Set("s_Diffuse", simpleFlora);
		simpleFloraMat->Set("s_Specular", noSpec);
		simpleFloraMat->Set("u_Shininess", 8.0f);
		simpleFloraMat->Set("u_TextureMix", 0.0f);

		//New Materials being Created for Midterm Obj's
		ShaderMaterial::sptr policeCarMat = ShaderMaterial::Create();
		policeCarMat->Shader = shader;
		policeCarMat->Set("s_Diffuse", policeCarDiffuse);
		policeCarMat->Set("s_Specular", policeCarDiffuse);
		policeCarMat->Set("u_Shininess", 8.0f);
		policeCarMat->Set("u_TextureMix", 0.0f);

		ShaderMaterial::sptr muscleCarMat = ShaderMaterial::Create();
		muscleCarMat->Shader = shader;
		muscleCarMat->Set("s_Diffuse", muscleCarDiffuse);
		muscleCarMat->Set("s_Specular",muscleCarDiffuse);
		muscleCarMat->Set("u_Shininess", 8.0f);
		muscleCarMat->Set("u_TextureMix", 0.0f);

		ShaderMaterial::sptr buildingMat = ShaderMaterial::Create();
		buildingMat->Shader = shader;
		buildingMat->Set("s_Diffuse", buildingDiffuse);
		buildingMat->Set("s_Specular", buildingDiffuse);
		buildingMat->Set("u_Shininess", 8.0f);
		buildingMat->Set("u_TextureMix", 0.0f);

		ShaderMaterial::sptr houseMat = ShaderMaterial::Create();
		houseMat->Shader = shader;
		houseMat->Set("s_Diffuse", houseDiffuse);
		houseMat->Set("s_Specular", houseDiffuse);
		houseMat->Set("u_Shininess", 8.0f);
		houseMat->Set("u_TextureMix", 0.0f);

		ShaderMaterial::sptr house2Mat = ShaderMaterial::Create();
		house2Mat->Shader = shader;
		house2Mat->Set("s_Diffuse", house2Diffuse);
		house2Mat->Set("s_Specular", house2Diffuse);
		house2Mat->Set("u_Shininess", 8.0f);
		house2Mat->Set("u_TextureMix", 0.0f);


		ShaderMaterial::sptr planeMat = ShaderMaterial::Create();
		planeMat->Shader = shader;
		planeMat->Set("s_Diffuse", planeDiffuse);
		planeMat->Set("s_Specular", planeDiffuse);
		planeMat->Set("u_Shininess", 8.0f);
		planeMat->Set("u_TextureMix", 0.0f);

		GameObject obj1 = scene->CreateEntity("Ground"); 
		{
			VertexArrayObject::sptr vao = ObjLoader::LoadFromFile("models/plane.obj");
			obj1.emplace<RendererComponent>().SetMesh(vao).SetMaterial(planeMat);
			obj1.get<Transform>().SetLocalRotation(00.0f, 0.0f, 180.0f);
		}

		//Load in New OBJ's
		GameObject obj2 = scene->CreateEntity("Police Car");
		{
			VertexArrayObject::sptr vao = ObjLoader::LoadFromFile("models/Police_Car_UPDATED.obj");
			obj2.emplace<RendererComponent>().SetMesh(vao).SetMaterial(policeCarMat);
			obj2.get<Transform>().SetLocalPosition(-3.5f, 1.0f, 0.0f);
			obj2.get<Transform>().SetLocalRotation(90.0f, 0.0f, 180.0f);
			obj2.get<Transform>().SetLocalScale(1.0f, 1.0f, 1.0f);
	
			//Bind returns a smart pointer to the behaviour that was added
			auto pathing = BehaviourBinding::Bind<FollowPathBehaviour>(obj2);
			//Set up path for the object to follow
			pathing->Points.push_back({ 17.0f, 1.0f, 0.0f });
			pathing->Points.push_back({ 17.0f, 18.0f, 0.0f });
			pathing->Points.push_back({ -17.0f, 18.0f, 0.0f });
			pathing->Points.push_back({ -17.0f, 1.1f, 0.0f });
			pathing->Speed = 5.0f;
		}

		GameObject obj3 = scene->CreateEntity("Muscle Car");
		{
			VertexArrayObject::sptr vao = ObjLoader::LoadFromFile("models/Muscle_Car.obj");
			obj3.emplace<RendererComponent>().SetMesh(vao).SetMaterial(muscleCarMat);
			obj3.get<Transform>().SetLocalPosition(2.0f, 1.0f, 0.0f);
			obj3.get<Transform>().SetLocalRotation(90.0f, 0.0f, 180.0f);
			obj3.get<Transform>().SetLocalScale(1.0f, 1.0f, 1.0f);
			

			//Bind returns a smart pointer to the behaviour that was added
			auto pathing = BehaviourBinding::Bind<FollowPathBehaviour>(obj3);
			//Set up path for the object to follow
			pathing->Points.push_back({17.0f, 1.0f, 0.0f });
			pathing->Points.push_back({ 17.0f, 18.0f, 0.0f });
			pathing->Points.push_back({ -17.0f, 18.0f, 0.0f });
			pathing->Points.push_back({ -17.0f, 1.1f, 0.0f });
			pathing->Speed = 5.0f;
		}

		GameObject obj4 = scene->CreateEntity("Building");
		{
			VertexArrayObject::sptr vao = ObjLoader::LoadFromFile("models/Hotel.obj");
			obj4.emplace<RendererComponent>().SetMesh(vao).SetMaterial(buildingMat);
			obj4.get<Transform>().SetLocalPosition(7.0f, -3.5f, 0.0f);
			obj4.get<Transform>().SetLocalRotation(90.0f, 0.0f, 180.0f);
			obj4.get<Transform>().SetLocalScale(2.5f, 2.5f, 2.5f);
			BehaviourBinding::BindDisabled<SimpleMoveBehaviour>(obj4);
		}


		GameObject obj5 = scene->CreateEntity("Building");
		{
			VertexArrayObject::sptr vao = ObjLoader::LoadFromFile("models/Hotel.obj");
			obj5.emplace<RendererComponent>().SetMesh(vao).SetMaterial(buildingMat);
			obj5.get<Transform>().SetLocalPosition(12.5f, -3.5f, 0.0f);
			obj5.get<Transform>().SetLocalRotation(90.0f, 0.0f, 180.0f);
			obj5.get<Transform>().SetLocalScale(2.5f, 2.5f, 2.5f);
			BehaviourBinding::BindDisabled<SimpleMoveBehaviour>(obj5);
		}

		GameObject obj6 = scene->CreateEntity("Building");
		{
			VertexArrayObject::sptr vao = ObjLoader::LoadFromFile("models/Hotel.obj");
			obj6.emplace<RendererComponent>().SetMesh(vao).SetMaterial(buildingMat);
			obj6.get<Transform>().SetLocalPosition(-7.0f, -3.5f, 0.0f);
			obj6.get<Transform>().SetLocalRotation(90.0f, 0.0f, 180.0f);
			obj6.get<Transform>().SetLocalScale(2.5f, 2.5f, 2.5f);
			BehaviourBinding::BindDisabled<SimpleMoveBehaviour>(obj6);
		}

		GameObject obj7 = scene->CreateEntity("Building");
		{
			VertexArrayObject::sptr vao = ObjLoader::LoadFromFile("models/Hotel.obj");
			obj7.emplace<RendererComponent>().SetMesh(vao).SetMaterial(buildingMat);
			obj7.get<Transform>().SetLocalPosition(-12.5f, -3.5f, 0.0f);
			obj7.get<Transform>().SetLocalRotation(90.0f, 0.0f, 180.0f);
			obj7.get<Transform>().SetLocalScale(2.5f, 2.5f, 2.5f);
			BehaviourBinding::BindDisabled<SimpleMoveBehaviour>(obj7);
		}

		GameObject obj8 = scene->CreateEntity("Building");
		{
			VertexArrayObject::sptr vao = ObjLoader::LoadFromFile("models/Building.obj");
			obj8.emplace<RendererComponent>().SetMesh(vao).SetMaterial(buildingMat);
			obj8.get<Transform>().SetLocalPosition(7.0f, -9.5f, 0.0f);
			obj8.get<Transform>().SetLocalRotation(90.0f, 0.0f, 270.0f);
			obj8.get<Transform>().SetLocalScale(2.5f, 2.5f, 2.5f);
			BehaviourBinding::BindDisabled<SimpleMoveBehaviour>(obj8);
		}

		GameObject obj9 = scene->CreateEntity("Building");
		{
			VertexArrayObject::sptr vao = ObjLoader::LoadFromFile("models/Building.obj");
			obj9.emplace<RendererComponent>().SetMesh(vao).SetMaterial(buildingMat);
			obj9.get<Transform>().SetLocalPosition(-7.0f, -9.5f, 0.0f);
			obj9.get<Transform>().SetLocalRotation(90.0f, 0.0f, 90.0f);
			obj9.get<Transform>().SetLocalScale(2.5f, 2.5f, 2.5f);
			BehaviourBinding::BindDisabled<SimpleMoveBehaviour>(obj9);
		}

		GameObject obj10 = scene->CreateEntity("Building");
		{
			VertexArrayObject::sptr vao = ObjLoader::LoadFromFile("models/Building.obj");
			obj10.emplace<RendererComponent>().SetMesh(vao).SetMaterial(buildingMat);
			obj10.get<Transform>().SetLocalPosition(12.5f, -9.5f, 0.0f);
			obj10.get<Transform>().SetLocalRotation(90.0f, 0.0f, 90.0f);
			obj10.get<Transform>().SetLocalScale(2.5f, 2.5f, 2.5f);
			BehaviourBinding::BindDisabled<SimpleMoveBehaviour>(obj10);
		}

		GameObject obj11 = scene->CreateEntity("Building");
		{
			VertexArrayObject::sptr vao = ObjLoader::LoadFromFile("models/Building.obj");
			obj11.emplace<RendererComponent>().SetMesh(vao).SetMaterial(buildingMat);
			obj11.get<Transform>().SetLocalPosition(-12.5f, -9.5f, 0.0f);
			obj11.get<Transform>().SetLocalRotation(90.0f, 0.0f, 270.0f);
			obj11.get<Transform>().SetLocalScale(2.5f, 2.5f, 2.5f);
			BehaviourBinding::BindDisabled<SimpleMoveBehaviour>(obj11);
		}

		GameObject obj12 = scene->CreateEntity("House");
		{
			VertexArrayObject::sptr vao = ObjLoader::LoadFromFile("models/House.obj");
			obj12.emplace<RendererComponent>().SetMesh(vao).SetMaterial(houseMat);
			obj12.get<Transform>().SetLocalPosition(-12.5f, 9.5f, 0.0f);
			obj12.get<Transform>().SetLocalRotation(90.0f, 0.0f, 270.0f);
			obj12.get<Transform>().SetLocalScale(2.5f, 2.5f, 2.5f);
			BehaviourBinding::BindDisabled<SimpleMoveBehaviour>(obj12);
		}

		GameObject obj13 = scene->CreateEntity("House");
		{
			VertexArrayObject::sptr vao = ObjLoader::LoadFromFile("models/House.obj");
			obj13.emplace<RendererComponent>().SetMesh(vao).SetMaterial(houseMat);
			obj13.get<Transform>().SetLocalPosition(-7.0f, 9.5f, 0.0f);
			obj13.get<Transform>().SetLocalRotation(90.0f, 0.0f, 90.0f);
			obj13.get<Transform>().SetLocalScale(2.5f, 2.5f, 2.5f);
			BehaviourBinding::BindDisabled<SimpleMoveBehaviour>(obj13);
		}


		GameObject obj14 = scene->CreateEntity("House");
		{
			VertexArrayObject::sptr vao = ObjLoader::LoadFromFile("models/House.obj");
			obj14.emplace<RendererComponent>().SetMesh(vao).SetMaterial(houseMat);
			obj14.get<Transform>().SetLocalPosition(7.0f, 9.5f, 0.0f);
			obj14.get<Transform>().SetLocalRotation(90.0f, 0.0f, 270.0f);
			obj14.get<Transform>().SetLocalScale(2.5f, 2.5f, 2.5f);
			BehaviourBinding::BindDisabled<SimpleMoveBehaviour>(obj14);
		}

		GameObject obj15 = scene->CreateEntity("House_With_Garage");
		{
			VertexArrayObject::sptr vao = ObjLoader::LoadFromFile("models/House.obj");
			obj15.emplace<RendererComponent>().SetMesh(vao).SetMaterial(houseMat);
			obj15.get<Transform>().SetLocalPosition(12.5f, 9.5f, 0.0f);
			obj15.get<Transform>().SetLocalRotation(90.0f, 0.0f, 90.0f);
			obj15.get<Transform>().SetLocalScale(2.5f, 2.5f, 2.5f);
			BehaviourBinding::BindDisabled<SimpleMoveBehaviour>(obj15);
		}

		GameObject obj16 = scene->CreateEntity("House");
		{
			VertexArrayObject::sptr vao = ObjLoader::LoadFromFile("models/House2.obj");
			obj16.emplace<RendererComponent>().SetMesh(vao).SetMaterial(house2Mat);
			obj16.get<Transform>().SetLocalPosition(10.0f, 14.5f, 0.0f);
			obj16.get<Transform>().SetLocalRotation(90.0f, 0.0f, 90.0f);
			obj16.get<Transform>().SetLocalScale(2.5f, 2.5f, 2.5f);
			BehaviourBinding::BindDisabled<SimpleMoveBehaviour>(obj16);
		}

		GameObject obj17 = scene->CreateEntity("House");
		{
			VertexArrayObject::sptr vao = ObjLoader::LoadFromFile("models/House2.obj");
			obj17.emplace<RendererComponent>().SetMesh(vao).SetMaterial(house2Mat);
			obj17.get<Transform>().SetLocalPosition(-10.0f, 14.5f, 0.0f);
			obj17.get<Transform>().SetLocalRotation(90.0f, 0.0f, 90.0f);
			obj17.get<Transform>().SetLocalScale(2.5f, 2.5f, 2.5f);
			BehaviourBinding::BindDisabled<SimpleMoveBehaviour>(obj17);
		}

		/*GameObject obj2 = scene->CreateEntity("monkey_quads");
		{
			VertexArrayObject::sptr vao = ObjLoader::LoadFromFile("models/monkey_quads.obj");
			obj2.emplace<RendererComponent>().SetMesh(vao).SetMaterial(stoneMat);
			obj2.get<Transform>().SetLocalPosition(0.0f, 0.0f, 2.0f);
			obj2.get<Transform>().SetLocalRotation(0.0f, 0.0f, -90.0f);
			BehaviourBinding::BindDisabled<SimpleMoveBehaviour>(obj2);
		}*/

	/*	std::vector<glm::vec2> allAvoidAreasFrom = { glm::vec2(-4.0f, -4.0f) };
		std::vector<glm::vec2> allAvoidAreasTo = { glm::vec2(4.0f, 4.0f) };

		std::vector<glm::vec2> rockAvoidAreasFrom = { glm::vec2(-3.0f, -3.0f), glm::vec2(-19.0f, -19.0f), glm::vec2(5.0f, -19.0f),
														glm::vec2(-19.0f, 5.0f), glm::vec2(-19.0f, -19.0f) };
		std::vector<glm::vec2> rockAvoidAreasTo = { glm::vec2(3.0f, 3.0f), glm::vec2(19.0f, -5.0f), glm::vec2(19.0f, 19.0f),
														glm::vec2(19.0f, 19.0f), glm::vec2(-5.0f, 19.0f) };
		glm::vec2 spawnFromHere = glm::vec2(-19.0f, -19.0f);
		glm::vec2 spawnToHere = glm::vec2(19.0f, 19.0f);*/

	/*	EnvironmentGenerator::AddObjectToGeneration("models/simplePine.obj", simpleFloraMat, 150,
			spawnFromHere, spawnToHere, allAvoidAreasFrom, allAvoidAreasTo);
		EnvironmentGenerator::AddObjectToGeneration("models/simpleTree.obj", simpleFloraMat, 150,
			spawnFromHere, spawnToHere, allAvoidAreasFrom, allAvoidAreasTo);
		EnvironmentGenerator::AddObjectToGeneration("models/simpleRock.obj", simpleFloraMat, 40,
			spawnFromHere, spawnToHere, rockAvoidAreasFrom, rockAvoidAreasTo);
		EnvironmentGenerator::GenerateEnvironment();*/

		// Create an object to be our camera
		GameObject cameraObject = scene->CreateEntity("Camera");
		{
			cameraObject.get<Transform>().SetLocalPosition(0, 3, 3).LookAt(glm::vec3(0, 0, 0));

			// We'll make our camera a component of the camera object
			Camera& camera = cameraObject.emplace<Camera>();// Camera::Create();
			camera.SetPosition(glm::vec3(0, 3, 3));
			camera.SetUp(glm::vec3(0, 0, 1));
			camera.LookAt(glm::vec3(0));
			camera.SetFovDegrees(90.0f); // Set an initial FOV
			camera.SetOrthoHeight(3.0f);
			BehaviourBinding::Bind<CameraControlBehaviour>(cameraObject);
		}
		
		int width, height;
		glfwGetWindowSize(BackendHandler::window, &width, &height);

		GameObject framebufferObject = scene->CreateEntity("Basic Effect");
		{
			basicEffect = &framebufferObject.emplace<PostEffect>();
			basicEffect->Init(width, height);
		}

		GameObject bloomEffectObject = scene->CreateEntity("Bloom Effect");
		{
			bloomEffect = &bloomEffectObject.emplace<BloomEffect>();
			bloomEffect->Init(width, height);

		}
		effects.push_back(bloomEffect);

		/*GameObject sepiaEffectObject = scene->CreateEntity("Sepia Effect");
		{
			sepiaEffect = &sepiaEffectObject.emplace<SepiaEffect>();
			sepiaEffect->Init(width, height);
		}
		effects.push_back(sepiaEffect);

		GameObject greyscaleEffectObject = scene->CreateEntity("Greyscale Effect");
		{
			greyscaleEffect = &greyscaleEffectObject.emplace<GreyscaleEffect>();
			greyscaleEffect->Init(width, height);
		}
		effects.push_back(greyscaleEffect);
		
		GameObject colorCorrectEffectObject = scene->CreateEntity("Greyscale Effect");
		{
			colorCorrectEffect = &colorCorrectEffectObject.emplace<ColorCorrectEffect>();
			colorCorrectEffect->Init(width, height);
		}
		effects.push_back(colorCorrectEffect);*/

		#pragma endregion 
		//////////////////////////////////////////////////////////////////////////////////////////

		/////////////////////////////////// SKYBOX ///////////////////////////////////////////////
		{
			// Load our shaders
			Shader::sptr skybox = std::make_shared<Shader>();
			skybox->LoadShaderPartFromFile("shaders/skybox-shader.vert.glsl", GL_VERTEX_SHADER);
			skybox->LoadShaderPartFromFile("shaders/skybox-shader.frag.glsl", GL_FRAGMENT_SHADER);
			skybox->Link();

			ShaderMaterial::sptr skyboxMat = ShaderMaterial::Create();
			skyboxMat->Shader = skybox;  
			skyboxMat->Set("s_Environment", environmentMap);
			skyboxMat->Set("u_EnvironmentRotation", glm::mat3(glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(1, 0, 0))));
			skyboxMat->RenderLayer = 100;

			MeshBuilder<VertexPosNormTexCol> mesh;
			MeshFactory::AddIcoSphere(mesh, glm::vec3(0.0f), 1.0f);
			MeshFactory::InvertFaces(mesh);
			VertexArrayObject::sptr meshVao = mesh.Bake();
			
			GameObject skyboxObj = scene->CreateEntity("skybox");  
			skyboxObj.get<Transform>().SetLocalPosition(0.0f, 0.0f, 0.0f);
			skyboxObj.get_or_emplace<RendererComponent>().SetMesh(meshVao).SetMaterial(skyboxMat);
		}
		////////////////////////////////////////////////////////////////////////////////////////


		// We'll use a vector to store all our key press events for now (this should probably be a behaviour eventually)
		std::vector<KeyPressWatcher> keyToggles;
		{
			// This is an example of a key press handling helper. Look at InputHelpers.h an .cpp to see
			// how this is implemented. Note that the ampersand here is capturing the variables within
			// the scope. If you wanted to do some method on the class, your best bet would be to give it a method and
			// use std::bind
			keyToggles.emplace_back(GLFW_KEY_T, [&]() { cameraObject.get<Camera>().ToggleOrtho(); });

			//controllables.push_back(obj2);

			keyToggles.emplace_back(GLFW_KEY_KP_ADD, [&]() {
				BehaviourBinding::Get<SimpleMoveBehaviour>(controllables[selectedVao])->Enabled = false;
				selectedVao++;
				if (selectedVao >= controllables.size())
					selectedVao = 0;
				BehaviourBinding::Get<SimpleMoveBehaviour>(controllables[selectedVao])->Enabled = true;
				});
			keyToggles.emplace_back(GLFW_KEY_KP_SUBTRACT, [&]() {
				BehaviourBinding::Get<SimpleMoveBehaviour>(controllables[selectedVao])->Enabled = false;
				selectedVao--;
				if (selectedVao < 0)
					selectedVao = controllables.size() - 1;
				BehaviourBinding::Get<SimpleMoveBehaviour>(controllables[selectedVao])->Enabled = true;
				});

			keyToggles.emplace_back(GLFW_KEY_Y, [&]() {
				auto behaviour = BehaviourBinding::Get<SimpleMoveBehaviour>(controllables[selectedVao]);
				behaviour->Relative = !behaviour->Relative;
				});
		}

		// Initialize our timing instance and grab a reference for our use
		Timing& time = Timing::Instance();
		time.LastFrame = glfwGetTime();

		///// Game loop /////
		while (!glfwWindowShouldClose(BackendHandler::window)) {
			glfwPollEvents();

			// Update the timing
			time.CurrentFrame = glfwGetTime();
			time.DeltaTime = static_cast<float>(time.CurrentFrame - time.LastFrame);

			time.DeltaTime = time.DeltaTime > 1.0f ? 1.0f : time.DeltaTime;

			// Update our FPS tracker data
			fpsBuffer[frameIx] = 1.0f / time.DeltaTime;
			frameIx++;
			if (frameIx >= 128)
				frameIx = 0;

			//Change theMuscle Car Rotation when it reaches reach a certain point
			if (obj3.get<Transform>().GetLocalPosition().x >= 16.8)
			{
				obj3.get<Transform>().SetLocalRotation(90.0f, 0.0f, 270.0f);

			}

			if (obj3.get<Transform>().GetLocalPosition().y >= 17.8)
			{
				obj3.get<Transform>().SetLocalRotation(90.0f, 0.0f, 0.0f);

			}

		/*	if (obj3.get<Transform>().GetLocalPosition().x <= -16.8)
			{
				obj3.get<Transform>().SetLocalRotation(90.0f, 0.0f, 90.0f);

			}*/

			if (obj3.get<Transform>().GetLocalPosition().y >= 0.9 && obj3.get<Transform>().GetLocalPosition().x <= -16.8)
			{
				obj3.get<Transform>().SetLocalRotation(90.0f, 0.0f, 180.0f);

			}

			//Change the Police Car Rotation when it reaches a certain point
			if (obj2.get<Transform>().GetLocalPosition().x >= 16.8)
			{
				obj2.get<Transform>().SetLocalRotation(90.0f, 0.0f, 270.0f);

			}

			if (obj2.get<Transform>().GetLocalPosition().y >= 17.8)
			{
				obj2.get<Transform>().SetLocalRotation(90.0f, 0.0f, 0.0f);

			}

		/*	if (obj2.get<Transform>().GetLocalPosition().x <= -16.8)
			{
				obj2.get<Transform>().SetLocalRotation(90.0f, 0.0f, 90.0f);

			}*/

			if (obj2.get<Transform>().GetLocalPosition().y >= 0.9 && obj2.get<Transform>().GetLocalPosition().x <= -16.8)
			{
				obj2.get<Transform>().SetLocalRotation(90.0f, 0.0f, 180.0f);

			}

			// We'll make sure our UI isn't focused before we start handling input for our game
			if (!ImGui::IsAnyWindowFocused()) {
				// We need to poll our key watchers so they can do their logic with the GLFW state
				// Note that since we want to make sure we don't copy our key handlers, we need a const
				// reference!
				for (const KeyPressWatcher& watcher : keyToggles) {
					watcher.Poll(BackendHandler::window);
				}
			}

			// Iterate over all the behaviour binding components
			scene->Registry().view<BehaviourBinding>().each([&](entt::entity entity, BehaviourBinding& binding) {
				// Iterate over all the behaviour scripts attached to the entity, and update them in sequence (if enabled)
				for (const auto& behaviour : binding.Behaviours) {
					if (behaviour->Enabled) {
						behaviour->Update(entt::handle(scene->Registry(), entity));
					}
				}
			});

			// Clear the screen
			basicEffect->Clear();
			//bloomEffect->Clear();
			/////*greyscaleEffect->Clear();
			////sepiaEffect->Clear();*/
			for (int i = 0; i < effects.size(); i++)
			{
				effects[i]->Clear();
			}


			glClearColor(0.08f, 0.17f, 0.31f, 1.0f);
			glEnable(GL_DEPTH_TEST);
			glClearDepth(1.0f);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			// Update all world matrices for this frame
			scene->Registry().view<Transform>().each([](entt::entity entity, Transform& t) {
				t.UpdateWorldMatrix();
			});
			
			// Grab out camera info from the camera object
			Transform& camTransform = cameraObject.get<Transform>();
			glm::mat4 view = glm::inverse(camTransform.LocalTransform());
			glm::mat4 projection = cameraObject.get<Camera>().GetProjection();
			glm::mat4 viewProjection = projection * view;
						
			// Sort the renderers by shader and material, we will go for a minimizing context switches approach here,
			// but you could for instance sort front to back to optimize for fill rate if you have intensive fragment shaders
			renderGroup.sort<RendererComponent>([](const RendererComponent& l, const RendererComponent& r) {
				// Sort by render layer first, higher numbers get drawn last
				if (l.Material->RenderLayer < r.Material->RenderLayer) return true;
				if (l.Material->RenderLayer > r.Material->RenderLayer) return false;

				// Sort by shader pointer next (so materials using the same shader run sequentially where possible)
				if (l.Material->Shader < r.Material->Shader) return true;
				if (l.Material->Shader > r.Material->Shader) return false;

				// Sort by material pointer last (so we can minimize switching between materials)
				if (l.Material < r.Material) return true;
				if (l.Material > r.Material) return false;
				
				return false;
			});

			// Start by assuming no shader or material is applied
			Shader::sptr current = nullptr;
			ShaderMaterial::sptr currentMat = nullptr;

			basicEffect->BindBuffer(0);

			// Iterate over the render group components and draw them
			renderGroup.each( [&](entt::entity e, RendererComponent& renderer, Transform& transform) {
				// If the shader has changed, set up it's uniforms
				if (current != renderer.Material->Shader) {
					current = renderer.Material->Shader;
					current->Bind();
					BackendHandler::SetupShaderForFrame(current, view, projection);
				}
				// If the material has changed, apply it
				if (currentMat != renderer.Material) {
					currentMat = renderer.Material;
					currentMat->Apply();
				}
				// Render the mesh
				BackendHandler::RenderVAO(renderer.Material->Shader, renderer.Mesh, viewProjection, transform);
			});

			basicEffect->UnbindBuffer();

			effects[activeEffect]->ApplyEffect(basicEffect);
			
			effects[activeEffect]->DrawToScreen();
			
			// Draw our ImGui content
			BackendHandler::RenderImGui();

			scene->Poll();
			glfwSwapBuffers(BackendHandler::window);
			time.LastFrame = time.CurrentFrame;
		}

		// Nullify scene so that we can release references
		Application::Instance().ActiveScene = nullptr;
		//Clean up the environment generator so we can release references
		EnvironmentGenerator::CleanUpPointers();
		BackendHandler::ShutdownImGui();
	}	

	// Clean up the toolkit logger so we don't leak memory
	Logger::Uninitialize();
	return 0;
}