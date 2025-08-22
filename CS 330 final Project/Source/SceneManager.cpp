///////////////////////////////////////////////////////////////////////////////
// scenemanager.cpp
// ================
// This file contains the implementation of the `SceneManager` class, which is 
// responsible for managing the preparation and rendering of 3D scenes. It 
// handles textures, materials, lighting configurations, and object rendering.
//
// AUTHOR: Brian Battersby
// INSTITUTION: Southern New Hampshire University (SNHU)
// COURSE: CS-330 Computational Graphics and Visualization
//
// INITIAL VERSION: November 1, 2023
// LAST REVISED: December 1, 2024
//
// RESPONSIBILITIES:
// - Load, bind, and manage textures in OpenGL.
// - Define materials and lighting properties for 3D objects.
// - Manage transformations and shader configurations.
// - Render complex 3D scenes using basic meshes.
//
// NOTE: This implementation leverages external libraries like `stb_image` for 
// texture loading and GLM for matrix and vector operations.
///////////////////////////////////////////////////////////////////////////////

#include "SceneManager.h"

#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#endif

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/transform.hpp>

// declaration of global variables
namespace
{
	const char* g_ModelName = "model";
	const char* g_ColorValueName = "objectColor";
	const char* g_TextureValueName = "objectTexture";
	const char* g_UseTextureName = "bUseTexture";
	const char* g_UseLightingName = "bUseLighting";
}

/***********************************************************
 *  SceneManager()
 *
 *  The constructor for the class
 ***********************************************************/
SceneManager::SceneManager(ShaderManager *pShaderManager)
{
	m_pShaderManager = pShaderManager;
	m_basicMeshes = new ShapeMeshes();
}

/***********************************************************
 *  ~SceneManager()
 *
 *  The destructor for the class
 ***********************************************************/
SceneManager::~SceneManager()
{
	m_pShaderManager = NULL;
	delete m_basicMeshes;
	m_basicMeshes = NULL;
}

/***********************************************************
 *  CreateGLTexture()
 *
 *  This method is used for loading textures from image files,
 *  configuring the texture mapping parameters in OpenGL,
 *  generating the mipmaps, and loading the read texture into
 *  the next available texture slot in memory.
 ***********************************************************/
bool SceneManager::CreateGLTexture(const char* filename, std::string tag)
{
	int width = 0;
	int height = 0;
	int colorChannels = 0;
	GLuint textureID = 0;

	// indicate to always flip images vertically when loaded
	stbi_set_flip_vertically_on_load(true);

	// try to parse the image data from the specified image file
	unsigned char* image = stbi_load(
		filename,
		&width,
		&height,
		&colorChannels,
		0);

	// if the image was successfully read from the image file
	if (image)
	{
		std::cout << "Successfully loaded image:" << filename << ", width:" << width << ", height:" << height << ", channels:" << colorChannels << std::endl;

		glGenTextures(1, &textureID);
		glBindTexture(GL_TEXTURE_2D, textureID);

		// set the texture wrapping parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		// set texture filtering parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		// if the loaded image is in RGB format
		if (colorChannels == 3)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
		// if the loaded image is in RGBA format - it supports transparency
		else if (colorChannels == 4)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
		else
		{
			std::cout << "Not implemented to handle image with " << colorChannels << " channels" << std::endl;
			return false;
		}

		// generate the texture mipmaps for mapping textures to lower resolutions
		glGenerateMipmap(GL_TEXTURE_2D);

		// free the image data from local memory
		stbi_image_free(image);
		glBindTexture(GL_TEXTURE_2D, 0); // Unbind the texture

		// register the loaded texture and associate it with the special tag string
		m_textureIDs[m_loadedTextures].ID = textureID;
		m_textureIDs[m_loadedTextures].tag = tag;
		m_loadedTextures++;

		return true;
	}

	std::cout << "Could not load image:" << filename << std::endl;

	// Error loading the image
	return false;
}

/***********************************************************
 *  BindGLTextures()
 *
 *  This method is used for binding the loaded textures to
 *  OpenGL texture memory slots.  There are up to 16 slots.
 ***********************************************************/
void SceneManager::BindGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		// bind textures on corresponding texture units
		glActiveTexture(GL_TEXTURE0 + i);
		glBindTexture(GL_TEXTURE_2D, m_textureIDs[i].ID);
	}
}

/***********************************************************
 *  DestroyGLTextures()
 *
 *  This method is used for freeing the memory in all the
 *  used texture memory slots.
 ***********************************************************/
void SceneManager::DestroyGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		glGenTextures(1, &m_textureIDs[i].ID);
	}
}

/***********************************************************
 *  FindTextureID()
 *
 *  This method is used for getting an ID for the previously
 *  loaded texture bitmap associated with the passed in tag.
 ***********************************************************/
int SceneManager::FindTextureID(std::string tag)
{
	int textureID = -1;
	int index = 0;
	bool bFound = false;

	while ((index < m_loadedTextures) && (bFound == false))
	{
		if (m_textureIDs[index].tag.compare(tag) == 0)
		{
			textureID = m_textureIDs[index].ID;
			bFound = true;
		}
		else
			index++;
	}

	return(textureID);
}

/***********************************************************
 *  FindTextureSlot()
 *
 *  This method is used for getting a slot index for the previously
 *  loaded texture bitmap associated with the passed in tag.
 ***********************************************************/
int SceneManager::FindTextureSlot(std::string tag)
{
	int textureSlot = -1;
	int index = 0;
	bool bFound = false;

	while ((index < m_loadedTextures) && (bFound == false))
	{
		if (m_textureIDs[index].tag.compare(tag) == 0)
		{
			textureSlot = index;
			bFound = true;
		}
		else
			index++;
	}

	return(textureSlot);
}

/***********************************************************
 *  FindMaterial()
 *
 *  This method is used for getting a material from the previously
 *  defined materials list that is associated with the passed in tag.
 ***********************************************************/
bool SceneManager::FindMaterial(std::string tag, OBJECT_MATERIAL& material)
{
	if (m_objectMaterials.size() == 0)
	{
		return(false);
	}

	int index = 0;
	bool bFound = false;
	while ((index < m_objectMaterials.size()) && (bFound == false))
	{
		if (m_objectMaterials[index].tag.compare(tag) == 0)
		{
			bFound = true;
			material.diffuseColor = m_objectMaterials[index].diffuseColor;
			material.specularColor = m_objectMaterials[index].specularColor;
			material.shininess = m_objectMaterials[index].shininess;
		}
		else
		{
			index++;
		}
	}

	return(true);
}

/***********************************************************
 *  SetTransformations()
 *
 *  This method is used for setting the transform buffer
 *  using the passed in transformation values.
 ***********************************************************/
void SceneManager::SetTransformations(
	glm::vec3 scaleXYZ,
	float XrotationDegrees,
	float YrotationDegrees,
	float ZrotationDegrees,
	glm::vec3 positionXYZ)
{
	// variables for this method
	glm::mat4 modelView;
	glm::mat4 scale;
	glm::mat4 rotationX;
	glm::mat4 rotationY;
	glm::mat4 rotationZ;
	glm::mat4 translation;

	// set the scale value in the transform buffer
	scale = glm::scale(scaleXYZ);
	// set the rotation values in the transform buffer
	rotationX = glm::rotate(glm::radians(XrotationDegrees), glm::vec3(1.0f, 0.0f, 0.0f));
	rotationY = glm::rotate(glm::radians(YrotationDegrees), glm::vec3(0.0f, 1.0f, 0.0f));
	rotationZ = glm::rotate(glm::radians(ZrotationDegrees), glm::vec3(0.0f, 0.0f, 1.0f));
	// set the translation value in the transform buffer
	translation = glm::translate(positionXYZ);

	modelView = translation * rotationZ * rotationY * rotationX * scale;

	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setMat4Value(g_ModelName, modelView);
	}
}

/***********************************************************
 *  SetShaderColor()
 *
 *  This method is used for setting the passed in color
 *  into the shader for the next draw command
 ***********************************************************/
void SceneManager::SetShaderColor(
	float redColorValue,
	float greenColorValue,
	float blueColorValue,
	float alphaValue)
{
	// variables for this method
	glm::vec4 currentColor;

	currentColor.r = redColorValue;
	currentColor.g = greenColorValue;
	currentColor.b = blueColorValue;
	currentColor.a = alphaValue;

	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, false);
		m_pShaderManager->setVec4Value(g_ColorValueName, currentColor);
	}
}

/***********************************************************
 *  SetShaderTexture()
 *
 *  This method is used for setting the texture data
 *  associated with the passed in ID into the shader.
 ***********************************************************/
void SceneManager::SetShaderTexture(
	std::string textureTag)
{
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, true);

		int textureID = -1;
		textureID = FindTextureSlot(textureTag);
		m_pShaderManager->setSampler2DValue(g_TextureValueName, textureID);
	}
}

/***********************************************************
 *  SetTextureUVScale()
 *
 *  This method is used for setting the texture UV scale
 *  values into the shader.
 ***********************************************************/
void SceneManager::SetTextureUVScale(float u, float v)
{
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setVec2Value("UVscale", glm::vec2(u, v));
	}
}
/***********************************************************
 *  LoadSceneTextures()
 *
 *  This method is used for preparing the 3D scene by loading
 *  the shapes, textures in memory to support the 3D scene
 *  rendering
 * ADDED FROM 5-2 ASSIGNMENT
 ***********************************************************/
void SceneManager::LoadSceneTextures()
{
	/*** STUDENTS - add the code BELOW for loading the textures that ***/
	/*** will be used for mapping to objects in the 3D scene. Up to  ***/
	/*** 16 textures can be loaded per scene. Refer to the code in   ***/
	/*** the OpenGL Sample for help.                                 ***/

	//5-2 assignment
	bool bReturn = false;

	bReturn = CreateGLTexture(
		"textures/Party_hat.jpg",
		"Party");
	bReturn = CreateGLTexture(
		"textures/blue_party.jpg",
		"Blue");
	bReturn = CreateGLTexture(
		"textures/Check_floor.jpg",
		"Floor");
	bReturn = CreateGLTexture(
		"textures/table.jpg",
		"Table");
	bReturn = CreateGLTexture(
		"textures/Plate.jpg",
		"Plate");
	bReturn = CreateGLTexture(
		"textures/top_frosting.png",
		"Frost");
	bReturn = CreateGLTexture(
		"textures/frosting_sides.png",
		"Frost_sides");
	bReturn = CreateGLTexture(
		"textures/Purple_balloon.png",
		"balloon");
	bReturn = CreateGLTexture(
		"textures/red_present.jpg",
		"present");
	// after the texture image data is loaded into memory, the
	// loaded textures need to be bound to texture slots - there
	// are a total of 16 available slots for scene textures
	BindGLTextures();
}

/***********************************************************
 *  DefineObjectMaterials()
 *
 *  This method is used for configuring the various material
 *  settings for all of the objects within the 3D scene.
 * 
 * ADDED FOR 6-3
 ***********************************************************/
void SceneManager::DefineObjectMaterials()
{

	OBJECT_MATERIAL candleMaterial;
	candleMaterial.tag = "Candle";
	candleMaterial.diffuseColor = glm::vec3(1.0f, 0.85f, 0.5f);     // Warm yellow wax
	candleMaterial.specularColor = glm::vec3(0.2f, 0.2f, 0.2f);     // Subtle soft specular
	candleMaterial.shininess = 4.0f;                                // Low shininess for waxy surface
	m_objectMaterials.push_back(candleMaterial);

	OBJECT_MATERIAL balloonMaterial;
	balloonMaterial.tag = "Balloon";
	balloonMaterial.diffuseColor = glm::vec3(0.4f, 0.1f, 0.6f);     // Deep purple
	balloonMaterial.specularColor = glm::vec3(0.3f, 0.2f, 0.5f);    // Soft, rubbery sheen
	balloonMaterial.shininess = 16.0f;                              // Semi-gloss finish
	m_objectMaterials.push_back(balloonMaterial);

	OBJECT_MATERIAL wrapMaterial;
	wrapMaterial.tag = "WrappingPaper";
	wrapMaterial.diffuseColor = glm::vec3(0.7f, 0.0f, 0.0f);      // Bold red
	wrapMaterial.specularColor = glm::vec3(1.0f, 0.9f, 0.3f);     // Gold-like highlights
	wrapMaterial.shininess = 64.0f;                               // High-gloss
	m_objectMaterials.push_back(wrapMaterial);

	OBJECT_MATERIAL woodMaterial;
	woodMaterial.tag = "Wood";
	woodMaterial.diffuseColor = glm::vec3(0.4f, 0.25f, 0.1f);     // Rich brown
	woodMaterial.specularColor = glm::vec3(0.05f, 0.05f, 0.05f);  // Almost no shine
	woodMaterial.shininess = 4.0f;                                // Rough surface
	m_objectMaterials.push_back(woodMaterial);

	OBJECT_MATERIAL hatMaterial;
	hatMaterial.tag = "PaperHat";
	hatMaterial.diffuseColor = glm::vec3(0.8f, 0.4f, 0.6f);       // Fun pink-purple
	hatMaterial.specularColor = glm::vec3(0.1f, 0.1f, 0.1f);      // Dull paper
	hatMaterial.shininess = 2.0f;                                 // Very flat
	m_objectMaterials.push_back(hatMaterial);

	OBJECT_MATERIAL cakeMaterial;
	cakeMaterial.tag = "Cake";
	cakeMaterial.diffuseColor = glm::vec3(0.95f, 0.8f, 0.7f);     // Light pink frosting
	cakeMaterial.specularColor = glm::vec3(0.2f, 0.15f, 0.1f);    // Slight gloss
	cakeMaterial.shininess = 8.0f;                                // Soft finish
	m_objectMaterials.push_back(cakeMaterial);


	OBJECT_MATERIAL ceramicMaterial;
	ceramicMaterial.tag = "Ceramic";
	ceramicMaterial.diffuseColor = glm::vec3(0.9f, 0.9f, 0.95f);  // Slightly blue white
	ceramicMaterial.specularColor = glm::vec3(0.9f, 0.9f, 0.9f);  // Strong reflection
	ceramicMaterial.shininess = 48.0f;                            // Smooth glazed surface
	m_objectMaterials.push_back(ceramicMaterial);


}

/***********************************************************
 *  SetShaderMaterial()
 *
 *  This method is used for passing the material values
 *  into the shader.
 ***********************************************************/
void SceneManager::SetShaderMaterial(
	std::string materialTag)
{
	if (m_objectMaterials.size() > 0)
	{
		OBJECT_MATERIAL material;
		bool bReturn = false;

		bReturn = FindMaterial(materialTag, material);
		if (bReturn == true)
		{
			m_pShaderManager->setVec3Value("material.diffuseColor", material.diffuseColor);
			m_pShaderManager->setVec3Value("material.specularColor", material.specularColor);
			m_pShaderManager->setFloatValue("material.shininess", material.shininess);
		}
	}
}

/**************************************************************/
/*** STUDENTS CAN MODIFY the code in the methods BELOW for  ***/
/*** preparing and rendering their own 3D replicated scenes.***/
/*** Please refer to the code in the OpenGL sample project  ***/
/*** for assistance.                                        ***/
/**************************************************************/


/***********************************************************
 *  PrepareScene()
 *
 *  This method is used for preparing the 3D scene by loading
 *  the shapes, textures in memory to support the 3D scene 
 *  rendering
 ***********************************************************/

void SceneManager::SetupSceneLights()
{
	// Enable lighting in the shader
	m_pShaderManager->setBoolValue(g_UseLightingName, true);
	// Point Light - Candle Flame (Main Focus)
	m_pShaderManager->setVec3Value("pointLights[0].position", 0.0f, 8.3f, 0.0f);  // above candle
	m_pShaderManager->setVec3Value("pointLights[0].ambient", 0.3f, 0.15f, 0.05f); // warm soft base
	m_pShaderManager->setVec3Value("pointLights[0].diffuse", 1.0f, 0.6f, 0.2f);   // flame orange
	m_pShaderManager->setVec3Value("pointLights[0].specular", 1.0f, 0.8f, 0.5f);  // warm spark
	m_pShaderManager->setFloatValue("pointLights[0].constant", 1.0f);
	m_pShaderManager->setFloatValue("pointLights[0].linear", 0.09f);
	m_pShaderManager->setFloatValue("pointLights[0].quadratic", 0.032f);
	m_pShaderManager->setBoolValue("pointLights[0].bActive", true);

	// Directional Light - Soft Room Fill
	m_pShaderManager->setVec3Value("directionalLight.direction", -0.2f, -1.0f, -0.3f);  // Top-left angle
	m_pShaderManager->setVec3Value("directionalLight.ambient", 0.1f, 0.1f, 0.15f);      // bluish gray tone
	m_pShaderManager->setVec3Value("directionalLight.diffuse", 0.2f, 0.2f, 0.3f);       // low intensity
	m_pShaderManager->setVec3Value("directionalLight.specular", 0.1f, 0.1f, 0.15f);     // very soft highlight
	m_pShaderManager->setBoolValue("directionalLight.bActive", true);

	// Point Light 2 - Purple Backlight Accent
	m_pShaderManager->setVec3Value("pointLights[1].position", 2.5f, 6.0f, -2.0f);      // near balloon
	m_pShaderManager->setVec3Value("pointLights[1].ambient", 0.05f, 0.02f, 0.08f);     // subtle purple
	m_pShaderManager->setVec3Value("pointLights[1].diffuse", 0.1f, 0.05f, 0.2f);
	m_pShaderManager->setVec3Value("pointLights[1].specular", 0.1f, 0.1f, 0.2f);
	m_pShaderManager->setFloatValue("pointLights[1].constant", 1.0f);
	m_pShaderManager->setFloatValue("pointLights[1].linear", 0.14f);
	m_pShaderManager->setFloatValue("pointLights[1].quadratic", 0.044f);
	m_pShaderManager->setBoolValue("pointLights[1].bActive", true);



}



void SceneManager::PrepareScene()

{
	//5-3 assignment
	LoadSceneTextures();

	//6-3 assignment
	SetupSceneLights();

	// 6-3 Assignment
	DefineObjectMaterials();

	// only one instance of a particular mesh needs to be
	// loaded in memory no matter how many times it is drawn
	// in the rendered 3D scene

	m_basicMeshes->LoadPlaneMesh();
	m_basicMeshes->LoadSphereMesh();
	m_basicMeshes->LoadConeMesh();
	m_basicMeshes->LoadBoxMesh();
	m_basicMeshes->LoadCylinderMesh();
	m_basicMeshes->LoadPyramid4Mesh();
}

/***********************************************************
 *  RenderScene()
 *
 *  This method is used for rendering the 3D scene by 
 *  transforming and drawing the basic 3D shapes
 ***********************************************************/
void SceneManager::RenderScene()
{
	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	/*** Set needed transformations before drawing the basic mesh.  ***/
	/*** This same ordering of code should be used for transforming ***/
	/*** and drawing all the basic 3D shapes.						***/
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(20.0f, 1.0f, 10.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 0.0f, 0.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(1, 1, 1, 1);
	SetShaderTexture("Floor");
	SetTextureUVScale(2.50f, 2.50f);


	// draw the mesh with transformation values
	SetShaderMaterial("Ceramic");
	m_basicMeshes->DrawPlaneMesh();
	/****************************************************************/
	//cone shape (party hat)
	scaleXYZ = glm::vec3(1.0f, 2.25f, 1.0f);               // size of  cone
	positionXYZ = glm::vec3(5.0f, 4.36, -1.5f);            //moved to the side so its not on top of the cube
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	//SetShaderColor(0.9f, 0.9f, 0.4f, 1.0f);               //yellow base
	SetShaderTexture("Party");
	SetTextureUVScale(1.0f, 1.0f);

	SetShaderMaterial("PaperHat");
	m_basicMeshes->DrawConeMesh();

	//sphere for the top of the party hat
	scaleXYZ = glm::vec3(0.25f, 0.25f, 0.25f);               // size of  cone
	positionXYZ = glm::vec3(5.0f, 6.80f, -1.5f);            //moved to the side so its not on top of the cube
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	//SetShaderColor(0.25f, 0.35f, 0.6f, 1.0f);  // dark blue
	SetShaderTexture("Blue");
	SetTextureUVScale(1.0f, 1.0f);

	SetShaderMaterial("PaperHat");
	m_basicMeshes->DrawSphereMesh();

	//Flat cube for napkin
	scaleXYZ = glm::vec3(5.0f, 0.01f, 5.0);               //size of the napkin
	positionXYZ = glm::vec3(5.0f, 4.33f, -1.5f);            //moved to the bottom be under the party hat
	XrotationDegrees = 0.0f;
	YrotationDegrees = 35.0f;
	ZrotationDegrees = 0.0f;
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	//SetShaderColor(0.25f, 0.35f, 0.6f, 1.0f); //dark blue

	SetShaderTexture("Blue");
	SetTextureUVScale(1.0f, 1.0f);
	m_basicMeshes->DrawBoxMesh();
	

	//milestone 4-3: table top 

	// Box for table top
	scaleXYZ = glm::vec3(19.0f, 0.50f, 10.0f);            // size of the table top
	positionXYZ = glm::vec3(0.0f, 4.0f, 0.0f);         // height above the legs

	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	
	// SetShaderColor(0.4f, 0.2f, 0.1f, 1.0f);            //dark brown 

	SetShaderTexture("Table");            
	SetTextureUVScale(3.0f, 3.0f);         

	SetShaderMaterial("Wood");
	m_basicMeshes->DrawBoxMesh();

	// 4 boxes to act as table legs: i combined all 4 shapes in order to keep code neat and readible 
	scaleXYZ = glm::vec3(0.3f, 4.0f, 0.3f);  // leg size
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	float legHeight = scaleXYZ.y;                 // uses the same height as the table
	float legCenterY = legHeight / 2.0f;        // this will ensure that the legs dont go under the plane

	// leg Postitions
	std::vector<glm::vec3> legPositions = {
		glm::vec3(-9.2f, legCenterY, -4.7f),
		glm::vec3(9.2f, legCenterY, -4.7f),  
		glm::vec3(-9.2f, legCenterY,  4.7f), 
		glm::vec3(9.2f, legCenterY,  4.7f)   
	};

	for (const auto& pos : legPositions) {
		positionXYZ = pos;
		SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
		//SetShaderColor(0.4f, 0.2f, 0.1f, 1.0f);
		SetShaderTexture("Table");
		SetTextureUVScale(1.0f, 1.0f);

		SetShaderMaterial("Wood");
		m_basicMeshes->DrawBoxMesh();
	}
		//The rest of the Scene
		
		// cube shape (present)
		scaleXYZ = glm::vec3(3.0f, 3.0f, 3.0f);            // size of cube
		positionXYZ = glm::vec3(-6.0f, 5.76f, -2.0f);         // this is to ensure there is no clipping on the plane
		XrotationDegrees = 0.0f;
		YrotationDegrees = -35.0f;
		ZrotationDegrees = 0.0f;

		SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
		SetShaderColor(0.6f, 0.1f, 0.1f, 1.0f); //dark red
		SetShaderTexture("present");
		SetTextureUVScale(0.20f, 0.50f);

		SetShaderMaterial("WrappingPaper");
		m_basicMeshes->DrawBoxMesh();

		// Sphere shape (ballon)
		scaleXYZ = glm::vec3(2.0f, 2.50f, 2.0f);                // it is taller on y to be able to create a ballon like shape
		positionXYZ = glm::vec3(4.0f, 12.0f, -4.0f);            //this will ensure that it looks like its floating
		XrotationDegrees = 0.0f;
		YrotationDegrees = 0.0f;
		ZrotationDegrees = 0.0f;

		SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
		SetShaderMaterial("Balloon"); 
		SetShaderTexture("balloon");
		SetTextureUVScale(1.0f, 1.0f);
		// SetShaderColor(0.25f, 0.1f, 0.4f, 1.0f);                // dark purple
		m_basicMeshes->DrawSphereMesh();



		// Pyramid as balloon knot
		scaleXYZ = glm::vec3(0.3f, 0.3f, 0.3f);                    //small to appear as a knot.
		positionXYZ = glm::vec3(4.0f, 9.45f, -4.0f);                // put it on the base of the balloon
		XrotationDegrees = 0.0f;
		YrotationDegrees = 0.0f;
		ZrotationDegrees = 0.0f;

		SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
		//SetShaderColor(0.2f, 0.01f, 0.5f, 1.0f);                  // slightly darker shade to contrast balloon
		SetShaderTexture("balloon");
		SetTextureUVScale(1.0f, 1.0f);
		SetShaderMaterial("Balloon");
		m_basicMeshes->DrawPyramid4Mesh();

		// Balloon String (thin cylinder)
		scaleXYZ = glm::vec3(0.025f, 10.0f, 0.05f);               // this will the illusion of string
		positionXYZ = glm::vec3(4.0f, 4.2f, -4.0f);             // in the balloon a bit but it looks like its attached to the "knott"
		XrotationDegrees = 0.0f;
		YrotationDegrees = 0.0f;
		ZrotationDegrees = 0.0f;

		SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
		SetShaderColor(0.3f, 0.3f, 0.3f, 1.0f);                 // dark gray string

		m_basicMeshes->DrawCylinderMesh();

		// Cylinder (cake)
		scaleXYZ = glm::vec3(3.0f, 2.0f, 3.0f);                // thin and tall
		positionXYZ = glm::vec3(0.0f, 4.33f, 0.0f);             // standing up
		XrotationDegrees = 0.0f;
		YrotationDegrees = 0.0f;
		ZrotationDegrees = 0.0f;

		SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
		//SetShaderColor(1.0f, 0.7f, 0.8f, 1.0f); //light pink

		SetShaderTexture("Frost_sides");
		SetShaderTexture("Cake");
		SetTextureUVScale(1.50f, 1.50f);

		m_basicMeshes->DrawCylinderMesh();
		
		// Cylinder top (icing texture)
		scaleXYZ = glm::vec3(3.01f, 0.1f, 3.01f);                
		positionXYZ = glm::vec3(0.0f, 6.18f, 0.0f);             
		SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
		SetShaderTexture("Frost");    
		SetShaderTexture("Cake");
		SetTextureUVScale(1.0f, 1.0f);                          
		m_basicMeshes->DrawCylinderMesh();

		// Cylinder plate 
		scaleXYZ = glm::vec3(3.5f, 0.1f, 3.5f);               // flatter cylinder to appear as a cake
		positionXYZ = glm::vec3(0.0f, 4.33f, 0.0f);           //this will make it under the cake
		XrotationDegrees = 0.0f;
		YrotationDegrees = 0.0f;
		ZrotationDegrees = 0.0f;

		SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
		//SetShaderColor(0.85f, 0.85f, 0.85f, 1.0f);
		SetShaderTexture("Plate");
		SetTextureUVScale(1.0f, 1.0f);

		SetShaderMaterial("Ceramic");
		m_basicMeshes->DrawCylinderMesh();

		// Cylinder (Candle)
		scaleXYZ = glm::vec3(0.1f, 2.0f, 0.1f);               // thinner and shorter candle
		positionXYZ = glm::vec3(0.0f, 6.33f, 0.0f);
		XrotationDegrees = 0.0f;
		YrotationDegrees = 0.0f;
		ZrotationDegrees = 0.0f;

		SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
		SetShaderColor(0.9f, 0.9f, 0.4f, 1.0f);               // yellow candle
		SetShaderMaterial("Candle");
		m_basicMeshes->DrawCylinderMesh();

		

		
	}
