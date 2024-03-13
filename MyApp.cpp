#include "MyApp.h"
#include "SDL_GLDebugMessageCallback.h"
#include "ObjParser.h"
#include "ParametricSurfaceMesh.hpp"

#include <imgui.h>

CMyApp::CMyApp()
{
}

CMyApp::~CMyApp()
{
}

void CMyApp::SetupDebugCallback()
{
	// engedélyezzük és állítsuk be a debug callback függvényt ha debug context-ben vagyunk 
	GLint context_flags;
	glGetIntegerv(GL_CONTEXT_FLAGS, &context_flags);
	if (context_flags & GL_CONTEXT_FLAG_DEBUG_BIT) {
		glEnable(GL_DEBUG_OUTPUT);
		glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
		glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_NOTIFICATION, 0, nullptr, GL_FALSE);
		glDebugMessageCallback(SDL_GLDebugMessageCallback, nullptr);
	}
}

void CMyApp::InitShaders()
{
	m_programID = glCreateProgram();
	AssembleProgram( m_programID, "Vert_PosNormTex.vert", "Frag_ZH.frag" );
	InitSkyboxShaders();
	m_beltProgramID = glCreateProgram();
	AssembleProgram(m_beltProgramID, "Vert_Belt.vert", "Frag_Belt.frag");
}

void CMyApp::InitSkyboxShaders()
{
	m_programSkyboxID = glCreateProgram();
	AssembleProgram( m_programSkyboxID, "Vert_skybox.vert", "Frag_skybox.frag" );
}

void CMyApp::CleanShaders()
{
	glDeleteProgram( m_programID );
	CleanSkyboxShaders();
	glDeleteProgram(m_beltProgramID);
}

void CMyApp::CleanSkyboxShaders()
{
	glDeleteProgram( m_programSkyboxID );
}

// Nyers parameterek
struct Param
{
	glm::vec3 GetPos( float u, float v ) const noexcept
	{
		return glm::vec3( u, v, 0.0f );
	}
	glm::vec3 GetNorm( float u, float v ) const noexcept
	{
		return glm::vec3( 0.0,0.0,1.0 );
	}
	glm::vec2 GetTex( float u, float v ) const noexcept
	{
		return glm::vec2( u, v );
	}
};

class Orb
{	
public:
	Orb(float const& distance, float const& radius, float const& axisAngle,
		float const& periodTimeAroundSun,  float const& periodTimeOwn)
		: distance(distance), radius(radius), axisAngle(axisAngle),
		periodTimeAroundSun(periodTimeAroundSun), periodTimeOwn(periodTimeOwn)
	{}

	glm::mat4 GenTransformMatrix(float m_ElapsedTimeInSec) {

		glm::mat4 matWorld = glm::rotate<float>(glm::radians(m_ElapsedTimeInSec * (periodTimeAroundSun != 0 ? 360.f / periodTimeAroundSun : 0)), glm::vec3(0.0, 1.0, 0.0))
			* glm::translate<float>(glm::vec3(distance, 0.0f, 0.0f))
			* glm::rotate<float>(glm::radians(-1 * axisAngle), glm::vec3(0.0f, 0.0f, 1.0f))
			* glm::rotate<float>(glm::radians(m_ElapsedTimeInSec * (periodTimeOwn != 0 ? 360.f / periodTimeOwn : 0)), glm::vec3(0.0, 1.0, 0.0))
			* glm::scale<float>(glm::vec3(radius, radius, radius)) * glm::identity<glm::mat4>();

		return matWorld;
	}

private:
	float distance;
	float radius;
	float axisAngle;
	float periodTimeAroundSun;
	float periodTimeOwn;
};

void CMyApp::InitGeometry()
{

	const std::initializer_list<VertexAttributeDescriptor> vertexAttribList =
	{
		{ 0, offsetof( Vertex, position ), 3, GL_FLOAT },
		{ 1, offsetof( Vertex, normal   ), 3, GL_FLOAT },
		{ 2, offsetof( Vertex, texcoord ), 2, GL_FLOAT },
	};

	// Ez egy sík, a gömb objektum koordinátáit, normál vektorait
	// és textúra koordinátáit a vertex-shaderben számoljuk 
	MeshObject<Vertex> surfaceMeshCPU = GetParamSurfMesh(Param());
	m_surfaceGPU = CreateGLObjectFromMesh(surfaceMeshCPU, vertexAttribList);

	// aszteroida

	MeshObject<Vertex> asteroidMeshCPU = ObjParser::parse("Assets/asteroid.obj");
	m_asteroidGPU = CreateGLObjectFromMesh(asteroidMeshCPU, vertexAttribList);

	// Skybox
	InitSkyboxGeometry();

	// övek
	MeshObject<Vertex> beltMeshCPU;

	std::vector<Vertex>vertices;
								 
	vertices.push_back({ glm::vec3(-0.5,  0.0, -0.5), glm::vec3(0.0, +1.0, 0.0), glm::vec2(0, 0) });
	vertices.push_back({ glm::vec3(-0.5,  0.0, +0.5), glm::vec3(0.0, +1.0, 0.0), glm::vec2(1, 0) });
	vertices.push_back({ glm::vec3(+0.5,  0.0, -0.5), glm::vec3(0.0, +1.0, 0.0), glm::vec2(0, 1) });
	vertices.push_back({ glm::vec3(+0.5,  0.0, +0.5), glm::vec3(0.0, +1.0, 0.0), glm::vec2(1, 1) });

	beltMeshCPU.indexArray =
	{
		0, 1, 2,
		1, 3, 2
	};

	beltMeshCPU.vertexArray = vertices;
	m_beltGPU = CreateGLObjectFromMesh(beltMeshCPU, vertexAttribList);
}

void CMyApp::CleanGeometry()
{
	CleanOGLObject( m_surfaceGPU );
	CleanOGLObject( m_asteroidGPU );
	CleanOGLObject( m_beltGPU );
	CleanSkyboxGeometry();
}

void CMyApp::InitSkyboxGeometry()
{
	// skybox geo
	MeshObject<glm::vec3> skyboxCPU =
	{
		std::vector<glm::vec3>
		{
			// hátsó lap
			glm::vec3(-1, -1, -1),
			glm::vec3( 1, -1, -1),
			glm::vec3( 1,  1, -1),
			glm::vec3(-1,  1, -1),
			// elülső lap
			glm::vec3(-1, -1, 1),
			glm::vec3( 1, -1, 1),
			glm::vec3( 1,  1, 1),
			glm::vec3(-1,  1, 1),
		},

		std::vector<GLuint>
		{
			// hátsó lap
			0, 1, 2,
			2, 3, 0,
			// elülső lap
			4, 6, 5,
			6, 4, 7,
			// bal
			0, 3, 4,
			4, 3, 7,
			// jobb
			1, 5, 2,
			5, 6, 2,
			// alsó
			1, 0, 4,
			1, 4, 5,
			// felső
			3, 2, 6,
			3, 6, 7,
		}
	};

	m_SkyboxGPU = CreateGLObjectFromMesh( skyboxCPU, { { 0, offsetof( glm::vec3,x ), 3, GL_FLOAT } } );
}

void CMyApp::RenderPlanet(glm:: mat4 matWorld, GLuint TextureID) {

	glBindTexture(GL_TEXTURE_2D, TextureID);

	glBindVertexArray(m_surfaceGPU.vaoID);

	glUniformMatrix4fv(ul("world"), 1, GL_FALSE, glm::value_ptr(matWorld));
	glUniformMatrix4fv(ul("worldIT"), 1, GL_FALSE, glm::value_ptr(glm::transpose(glm::inverse(matWorld))));

	glDrawElements(GL_TRIANGLES,
		m_surfaceGPU.count,
		GL_UNSIGNED_INT,
		nullptr);

	glBindTexture(GL_TEXTURE_2D, 0);

}

void CMyApp::CleanSkyboxGeometry()
{
	CleanOGLObject( m_SkyboxGPU );
}

void CMyApp::InitTextures()
{
	// diffuse textures

/*	glGenTextures(1, &m_SuzanneTextureID);
	TextureFromFile( m_SuzanneTextureID, "Assets/wood.jpg" );
	SetupTextureSampling( GL_TEXTURE_2D, m_SuzanneTextureID );

	glGenTextures( 1, &m_surfaceTextureID );
	TextureFromFile( m_surfaceTextureID, "Assets/color_checkerboard.png" );
	SetupTextureSampling( GL_TEXTURE_2D, m_surfaceTextureID );*/

	// skybox texture

	InitSkyboxTextures();

	// bolygók textúrái
	//sun.jpg – Nap textúrája
	glGenTextures(1, &m_sunTextureID);
	TextureFromFile(m_sunTextureID, "Assets/sun.jpg");
	SetupTextureSampling(GL_TEXTURE_2D, m_sunTextureID);

	//mercury.jpg – Merkúr textúrája
	glGenTextures(1, &m_mercuryTextureID);
	TextureFromFile(m_mercuryTextureID, "Assets/mercury.jpg");
	SetupTextureSampling(GL_TEXTURE_2D, m_mercuryTextureID);

	//venus.jpg – Vénusz textúrája
	glGenTextures(1, &m_venusTextureID);
	TextureFromFile(m_venusTextureID, "Assets/venus.jpg");
	SetupTextureSampling(GL_TEXTURE_2D, m_venusTextureID);

	//earth.png – A Föld nappali oldalának textúrája
	glGenTextures(1, &m_earthTextureID);
	TextureFromFile(m_earthTextureID, "Assets/earth.png");
	SetupTextureSampling(GL_TEXTURE_2D, m_earthTextureID);

	//moon.jpg – Hold textúrája
	glGenTextures(1, &m_moonTextureID);
	TextureFromFile(m_moonTextureID, "Assets/moon.jpg");
	SetupTextureSampling(GL_TEXTURE_2D, m_moonTextureID);

	//mars.jpg – Mars textúrája
	glGenTextures(1, &m_marsTextureID);
	TextureFromFile(m_marsTextureID, "Assets/mars.jpg");
	SetupTextureSampling(GL_TEXTURE_2D, m_marsTextureID);

	//jupiter.jpg – Jupiter textúrája
	glGenTextures(1, &m_jupiterTextureID);
	TextureFromFile(m_jupiterTextureID, "Assets/jupiter.jpg");
	SetupTextureSampling(GL_TEXTURE_2D, m_jupiterTextureID);

	//saturn.jpg – Szaturnusz textúrája
	glGenTextures(1, &m_saturnTextureID);
	TextureFromFile(m_saturnTextureID, "Assets/saturn.jpg");
	SetupTextureSampling(GL_TEXTURE_2D, m_saturnTextureID);

	//uranus.jpg – Uránusz textúrája
	glGenTextures(1, &m_uranusTextureID);
	TextureFromFile(m_uranusTextureID, "Assets/uranus.jpg");
	SetupTextureSampling(GL_TEXTURE_2D, m_uranusTextureID);

	//neptune.jpg – Neptunusz textúrája
	glGenTextures(1, &m_neptuneTextureID);
	TextureFromFile(m_neptuneTextureID, "Assets/neptune.jpg");
	SetupTextureSampling(GL_TEXTURE_2D, m_neptuneTextureID);

	//pluto.jpg – Pluto textúrája
	glGenTextures(1, &m_plutoTextureID);
	TextureFromFile(m_plutoTextureID, "Assets/pluto.jpg");
	SetupTextureSampling(GL_TEXTURE_2D, m_plutoTextureID);

	//kuiper.png - Kuiper-öv textúrája
	glGenTextures(1, &m_kuiperTextureID);
	TextureFromFile(m_kuiperTextureID, "Assets/kuiper.png");
	SetupTextureSampling(GL_TEXTURE_2D, m_kuiperTextureID);

	//saturn-ring.png - Szaturnusz-öv textúrája
	glGenTextures(1, &m_saturn_ringTextureID);
	TextureFromFile(m_saturn_ringTextureID, "Assets/saturn-ring.png");
	SetupTextureSampling(GL_TEXTURE_2D, m_saturn_ringTextureID);

	//uranus-ring.png - Uránusz-öv textúrája
	glGenTextures(1, &m_uranus_ringTextureID);
	TextureFromFile(m_uranus_ringTextureID, "Assets/uranus-ring.png");
	SetupTextureSampling(GL_TEXTURE_2D, m_uranus_ringTextureID);

	//neptune-ring.pngg - Neptunusz-öv textúrája
	glGenTextures(1, &m_neptune_ringTextureID);
	TextureFromFile(m_neptune_ringTextureID, "Assets/neptune-ring.png");
	SetupTextureSampling(GL_TEXTURE_2D, m_neptune_ringTextureID);

	//rock.jpg - aszteroida textúrája
	glGenTextures(1, &m_asteroidTextureID);
	TextureFromFile(m_asteroidTextureID, "Assets/rock.jpg");
	SetupTextureSampling(GL_TEXTURE_2D, m_asteroidTextureID);
}

void CMyApp::CleanTextures()
{
	// diffuse textures

	glDeleteTextures(1, &m_sunTextureID);
	glDeleteTextures(1, &m_mercuryTextureID);
	glDeleteTextures(1, &m_venusTextureID);
	glDeleteTextures(1, &m_earthTextureID);
	glDeleteTextures(1, &m_moonTextureID);
	glDeleteTextures(1, &m_marsTextureID);
	glDeleteTextures(1, &m_jupiterTextureID);
	glDeleteTextures(1, &m_saturnTextureID);
	glDeleteTextures(1, &m_uranusTextureID);
	glDeleteTextures(1, &m_neptuneTextureID);
	glDeleteTextures(1, &m_plutoTextureID);

	glDeleteTextures(1, &m_kuiperTextureID);
	glDeleteTextures(1, &m_saturn_ringTextureID);
	glDeleteTextures(1, &m_uranus_ringTextureID);
	glDeleteTextures(1, &m_neptune_ringTextureID);

	glDeleteTextures(1, &m_asteroidTextureID);

	// skybox texture

	CleanSkyboxTextures();
}

void CMyApp::InitSkyboxTextures()
{
	// skybox texture

	glGenTextures( 1, &m_skyboxTextureID );
	TextureFromFile(m_skyboxTextureID, "Assets/space_xpos.png", GL_TEXTURE_CUBE_MAP, GL_TEXTURE_CUBE_MAP_POSITIVE_X);
	TextureFromFile(m_skyboxTextureID, "Assets/space_xneg.png", GL_TEXTURE_CUBE_MAP, GL_TEXTURE_CUBE_MAP_NEGATIVE_X);
	TextureFromFile(m_skyboxTextureID, "Assets/space_ypos.png", GL_TEXTURE_CUBE_MAP, GL_TEXTURE_CUBE_MAP_POSITIVE_Y);
	TextureFromFile(m_skyboxTextureID, "Assets/space_yneg.png", GL_TEXTURE_CUBE_MAP, GL_TEXTURE_CUBE_MAP_NEGATIVE_Y);
	TextureFromFile(m_skyboxTextureID, "Assets/space_zpos.png", GL_TEXTURE_CUBE_MAP, GL_TEXTURE_CUBE_MAP_POSITIVE_Z);
	TextureFromFile(m_skyboxTextureID, "Assets/space_zneg.png", GL_TEXTURE_CUBE_MAP, GL_TEXTURE_CUBE_MAP_NEGATIVE_Z);
	SetupTextureSampling( GL_TEXTURE_CUBE_MAP, m_skyboxTextureID, false );

	glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
}

void CMyApp::CleanSkyboxTextures()
{
	glDeleteTextures( 1, &m_skyboxTextureID );
}

bool CMyApp::Init()
{
	SetupDebugCallback();

	// törlési szín legyen kékes
	glClearColor(0.125f, 0.25f, 0.5f, 1.0f);

	InitShaders();
	InitGeometry();
	InitTextures();

	//
	// egyéb inicializálás
	//

	glEnable(GL_CULL_FACE); // kapcsoljuk be a hátrafelé néző lapok eldobását
	glCullFace(GL_BACK);    // GL_BACK: a kamerától "elfelé" néző lapok, GL_FRONT: a kamera felé néző lapok

	glEnable(GL_DEPTH_TEST); // mélységi teszt bekapcsolása (takarás)

	// kamera
	m_camera.SetView(
		glm::vec3(0.0, 5.0, 35.0),// honnan nézzük a színteret	   - eye
		glm::vec3(0.0, 0.0, 0.0),   // a színtér melyik pontját nézzük - at
		glm::vec3(0.0, 1.0, 0.0));  // felfelé mutató irány a világban - up


	return true;
}

void CMyApp::Clean()
{
	CleanShaders();
	CleanGeometry();
	CleanTextures();
}

void CMyApp::Update( const SUpdateInfo& updateInfo )
{
	m_ElapsedTimeInSec = updateInfo.ElapsedTimeInSec;

	m_camera.Update( updateInfo.DeltaTimeInSec );
}

void CMyApp::Render()
{
	// töröljük a frampuffert (GL_COLOR_BUFFER_BIT)...
	// ... és a mélységi Z puffert (GL_DEPTH_BUFFER_BIT)
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	//
	// égitestek
	//
	
	glUseProgram( m_programID );

	glm::mat4 matWorld = glm::identity<glm::mat4>();

/*	glUniformMatrix4fv(ul("world"), 1, GL_FALSE, glm::value_ptr(matWorld));
	glUniformMatrix4fv( ul( "worldIT" ),  1, GL_FALSE, glm::value_ptr( glm::transpose( glm::inverse( matWorld ) ) ) );*/

	glUniformMatrix4fv( ul( "viewProj" ), 1, GL_FALSE, glm::value_ptr( m_camera.GetViewProj() ) );

	// - Fényforrások beállítása
	glUniform3fv( ul( "cameraPos" ), 1, glm::value_ptr( m_camera.GetEye() ) );
	glUniform4fv( ul( "lightPos" ),  1, glm::value_ptr( m_lightPos ) );

	glUniform3fv( ul( "La" ),		 1, glm::value_ptr( m_La ) );
	glUniform3fv( ul( "Ld" ),		 1, glm::value_ptr( m_Ld ) );
	glUniform3fv( ul( "Ls" ),		 1, glm::value_ptr( m_Ls ) );

	glUniform1f( ul( "lightConstantAttenuation"	 ), m_lightConstantAttenuation );
	glUniform1f( ul( "lightLinearAttenuation"	 ), m_lightLinearAttenuation   );
	glUniform1f( ul( "lightQuadraticAttenuation" ), m_lightQuadraticAttenuation);

	// - Anyagjellemzők beállítása
	glUniform3fv( ul( "Ka" ),		 1, glm::value_ptr( m_Ka ) );
	glUniform3fv( ul( "Kd" ),		 1, glm::value_ptr( m_Kd ) );
	glUniform3fv( ul( "Ks" ),		 1, glm::value_ptr( m_Ks ) );

	glUniform1f( ul( "Shininess" ),	m_Shininess );

	// Nap
	// középpont: (0.0, 0.0, 0.0); sugár: 1.0;
	// forgástengely: 7.25 deg

	glActiveTexture(GL_TEXTURE0);

	matWorld = glm::rotate<float>(glm::radians(-7.25f), glm::vec3(0.0f, 0.0f, 1.0f))
		* glm::rotate<float>(glm::radians(m_ElapsedTimeInSec * (360.f / 27.f)), glm::vec3(0.0, 1.0, 0.0))
		* glm::identity<glm::mat4>();
	RenderPlanet(matWorld, m_sunTextureID);

	// Merkúr
	// Felszíne legyen 1 egységre a Nap felszínétől; surgár: 0.15;
	// középpont: (2.15, 0.0, 0.0)  (1 + 1 + 0.15 = 2.15)
	// forgástengely: 0.01 deg
	Orb mercury(2.15f, 0.15f, 0.01f, 89.f, 58.7f);	
	matWorld = mercury.GenTransformMatrix(m_ElapsedTimeInSec);
	RenderPlanet(matWorld, m_mercuryTextureID);

	// Vénusz
	// Felszíne legyen 2 egységre a Nap felszínétől; surgár:  0.13;
	// 2 + 1 + 0.13 = 3.13
	// forgástengely: 177.4 deg
	Orb venus(3.13f, 0.13f, 177.4f, 243.f, 255.f);
	matWorld = venus.GenTransformMatrix(m_ElapsedTimeInSec);
	RenderPlanet(matWorld, m_venusTextureID);

	// Föld
	// Felszíne legyen 3 egységre a Nap felszínétől; surgár: 0.2;
	// 3 + 1 + 0.2 = 4.2
	// forgástengely: 23.44 fok
	Orb earth(4.2f, 0.2f, 23.44f, 365.f, 1.f);
	matWorld = earth.GenTransformMatrix(m_ElapsedTimeInSec);
	RenderPlanet(matWorld, m_earthTextureID);

	// Hold
	// Felszíne legyen 0.2 egységre a Fökld felszínétől; surgár: Föld méretének 1 / 3 része
	// 4.2 + 0.2 + 0.2 + 0.06667 = 4.66667
	// forgástengely: 1.54
	// egy földi év alatt 12-szer kerülje meg a Földet
	matWorld = glm::rotate<float>(glm::radians(m_ElapsedTimeInSec * (360.f / 365.f)), glm::vec3(0.0, 1.0, 0.0))
			* glm::translate<float>(glm::vec3(4.2f, 0.0f, 0.0f))
			* glm::rotate<float>(glm::radians(m_ElapsedTimeInSec * (360.f / 30.f)), glm::vec3(0.0, 1.0, 0.0))
			* glm::translate<float>(glm::vec3(0.46667f, 0.0f, 0.0f))
			* glm::rotate<float>(glm::radians(-1.54f), glm::vec3(0.0f, 0.0f, 1.0f))
			* glm::scale<float>(glm::vec3(0.06667f, 0.06667f, 0.06667f));
	RenderPlanet(matWorld, m_moonTextureID);

	// Mars
	// Felszíne legyen 4 egységre a Nap felszínétől; surgár: 0.19;
	// 4 + 1 + 0.19 = 5.19
	// forgástengely: 25.19 fok
	Orb mars(5.19f, 0.19f, 25.19f, 687.f, 1.04f);
	matWorld = mars.GenTransformMatrix(m_ElapsedTimeInSec);
	RenderPlanet(matWorld, m_marsTextureID);

	// Jupiter
	// Felszíne legyen 5 egységre a Nap felszínétől; surgár: 0.4;
	//  5 + 1 + 0.4 = 6.4
	// forgástengely: 3.13 fok	
	Orb jupiter(6.4f, 0.4f, 3.13f, 4329.f, 0.42f);
	matWorld = jupiter.GenTransformMatrix(m_ElapsedTimeInSec);
	RenderPlanet(matWorld, m_jupiterTextureID);

	// Szaturnusz
	// Felszíne legyen 6 egységre a Nap felszínétől; surgár: 0.35;
	// 6 + 1 + 0.35 = 7.35
	// forgástengely: 26.73 fok
	Orb saturn(7.35f, 0.35f, 26.73f, 10753.f, 0.46f);
	matWorld = saturn.GenTransformMatrix(m_ElapsedTimeInSec);
	RenderPlanet(matWorld, m_saturnTextureID);

	// Uránusz
	// Felszíne legyen 7 egységre a Nap felszínétől; surgár: 0.25;
	// 7 + 1 + 0.25 = 8.25
	// forgástengely: 97.77 fok
	Orb uranus(8.25f, 0.25f, 97.77f, 30664.f, 0.71f);
	matWorld = uranus.GenTransformMatrix(m_ElapsedTimeInSec);
	RenderPlanet(matWorld, m_uranusTextureID);

	// Neptunusz
	// Felszíne legyen 8 egységre a Nap felszínétől; surgár: 0.26;
	// 8 + 1 + 0.26 = 9.26
	// forgástengely: 28.32 fok
	Orb neptune(9.26f, 0.26f, 28.32f, 60148.f, 0.66f);
	matWorld = neptune.GenTransformMatrix(m_ElapsedTimeInSec);
	RenderPlanet(matWorld, m_neptuneTextureID);

	// Pluto
	// Felszíne legyen 9 egységre a Nap felszínétől; surgár: 0.1;
	// 9 + 1 + 0.1 = 10.1
	// forgástengely: 119.61 fok
	Orb pluto(10.1f, 0.1f, 119.61f, 90520.f, 6.37f);
	matWorld = pluto.GenTransformMatrix(m_ElapsedTimeInSec);
	RenderPlanet(matWorld, m_plutoTextureID);
	
	
	//
	// skybox
	//

	// - VAO
	glBindVertexArray( m_SkyboxGPU.vaoID );

	// - Textura
	glActiveTexture( GL_TEXTURE1 );
	glBindTexture( GL_TEXTURE_CUBE_MAP, m_skyboxTextureID );

	// - Program
	glUseProgram( m_programSkyboxID );

	// - uniform parameterek
	glUniformMatrix4fv( ul("world"),    1, GL_FALSE, glm::value_ptr( glm::translate( m_camera.GetEye() ) ) );
	glUniformMatrix4fv( ul("viewProj"), 1, GL_FALSE, glm::value_ptr( m_camera.GetViewProj() ) );

	// - textúraegységek beállítása
	glUniform1i( ul( "skyboxTexture" ), 1 );

	// mentsük el az előző Z-test eredményt, azaz azt a relációt, ami alapján update-eljük a pixelt.
	GLint prevDepthFnc;
	glGetIntegerv(GL_DEPTH_FUNC, &prevDepthFnc);

	// most kisebb-egyenlőt használjunk, mert mindent kitolunk a távoli vágósíkokra
	glDepthFunc(GL_LEQUAL);

	// - Rajzolas
	glDrawElements( GL_TRIANGLES, m_SkyboxGPU.count, GL_UNSIGNED_INT, nullptr );

	glDepthFunc(prevDepthFnc);


	//
	// gyűrűk
	//

	glUseProgram(m_beltProgramID);

	matWorld = glm::identity<glm::mat4>();

	/*	glUniformMatrix4fv(ul("world"), 1, GL_FALSE, glm::value_ptr(matWorld));
		glUniformMatrix4fv( ul( "worldIT" ),  1, GL_FALSE, glm::value_ptr( glm::transpose( glm::inverse( matWorld ) ) ) );*/

	glUniformMatrix4fv(ul("viewProj"), 1, GL_FALSE, glm::value_ptr(m_camera.GetViewProj()));

	// - Fényforrások beállítása
	glUniform3fv(ul("cameraPos"), 1, glm::value_ptr(m_camera.GetEye()));
	glUniform4fv(ul("lightPos"), 1, glm::value_ptr(m_lightPos));

	glUniform3fv(ul("La"), 1, glm::value_ptr(m_La));
	glUniform3fv(ul("Ld"), 1, glm::value_ptr(m_Ld));
	glUniform3fv(ul("Ls"), 1, glm::value_ptr(m_Ls));

	glUniform1f(ul("lightConstantAttenuation"), m_lightConstantAttenuation);
	glUniform1f(ul("lightLinearAttenuation"), m_lightLinearAttenuation);
	glUniform1f(ul("lightQuadraticAttenuation"), m_lightQuadraticAttenuation);

	// - Anyagjellemzők beállítása
	glUniform3fv(ul("Ka"), 1, glm::value_ptr(m_Ka));
	glUniform3fv(ul("Kd"), 1, glm::value_ptr(m_Kd));
	glUniform3fv(ul("Ks"), 1, glm::value_ptr(m_Ks));

	glUniform1f(ul("Shininess"), m_Shininess);
	glUniform1f(ul("Shininess"), m_Shininess);

	// aszteroida
	glBindVertexArray(m_asteroidGPU.vaoID);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, m_asteroidTextureID);

	Orb asteroid(4.5f, 0.05f, 0.f, 110.f, 0.0f);
	matWorld = asteroid.GenTransformMatrix(m_ElapsedTimeInSec)
		* glm::rotate<float>(glm::radians(m_ElapsedTimeInSec* (30.f)), glm::vec3(1.0, 0.0, 0.0))
		* glm::rotate<float>(glm::radians(m_ElapsedTimeInSec* (30.f)), glm::vec3(0.0, 1.0, 0.0))
		* glm::rotate<float>(glm::radians(m_ElapsedTimeInSec* (30.f)), glm::vec3(0.0, 0.0, 1.0));

	glUniformMatrix4fv(ul("world"), 1, GL_FALSE, glm::value_ptr(matWorld));
	glUniformMatrix4fv(ul("worldIT"), 1, GL_FALSE, glm::value_ptr(glm::transpose(glm::inverse(matWorld))));

	glDrawElements(GL_TRIANGLES,
		m_asteroidGPU.count,
		GL_UNSIGNED_INT,
		nullptr);

	glBindVertexArray(m_beltGPU.vaoID);

	glEnable(GL_BLEND);
	glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ZERO);

	glDisable(GL_CULL_FACE);
	
	// Szaturnusz-öv
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, m_saturn_ringTextureID);

	Orb saturnRing(7.35f, 1.5f, 26.73f, 10753.f, 0.f);
	matWorld = saturnRing.GenTransformMatrix(m_ElapsedTimeInSec);

	glUniformMatrix4fv(ul("world"), 1, GL_FALSE, glm::value_ptr(matWorld));
	glUniformMatrix4fv(ul("worldIT"), 1, GL_FALSE, glm::value_ptr(glm::transpose(glm::inverse(matWorld))));

	glDrawElements(GL_TRIANGLES,
		m_beltGPU.count,
		GL_UNSIGNED_INT,
		nullptr);

	// Uránusz-öv
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, m_uranus_ringTextureID);

	Orb uranusRing(8.25f, 0.85f, 97.77f, 30664.f, 0.f);
	matWorld = uranusRing.GenTransformMatrix(m_ElapsedTimeInSec);

	glUniformMatrix4fv(ul("world"), 1, GL_FALSE, glm::value_ptr(matWorld));
	glUniformMatrix4fv(ul("worldIT"), 1, GL_FALSE, glm::value_ptr(glm::transpose(glm::inverse(matWorld))));

	glDrawElements(GL_TRIANGLES,
		m_beltGPU.count,
		GL_UNSIGNED_INT,
		nullptr);

	// Neptunusz-öv
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, m_neptune_ringTextureID);

	Orb neptuneRing(9.26f, 1.0f, 28.32f, 60148.f, 0.66f);
	matWorld = neptuneRing.GenTransformMatrix(m_ElapsedTimeInSec);

	glUniformMatrix4fv(ul("world"), 1, GL_FALSE, glm::value_ptr(matWorld));
	glUniformMatrix4fv(ul("worldIT"), 1, GL_FALSE, glm::value_ptr(glm::transpose(glm::inverse(matWorld))));

	glDrawElements(GL_TRIANGLES,
		m_beltGPU.count,
		GL_UNSIGNED_INT,
		nullptr);

	// Kuiper-öv

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, m_kuiperTextureID);

	matWorld = glm::translate<float>(glm::vec3(0.0f, -0.05f, 0.0f))
		* glm::scale<float>(glm::vec3(25.0f, 1.0f, 25.0f))
		* glm::identity<glm::mat4>();

	glUniformMatrix4fv(ul("world"), 1, GL_FALSE, glm::value_ptr(matWorld));
	glUniformMatrix4fv(ul("worldIT"), 1, GL_FALSE, glm::value_ptr(glm::transpose(glm::inverse(matWorld))));

	glDrawElements(GL_TRIANGLES,
		m_beltGPU.count,
		GL_UNSIGNED_INT,
		nullptr);

	glEnable(GL_CULL_FACE);
	glDisable(GL_BLEND);

	// shader kikapcsolasa
	glUseProgram( 0 );

	// - Textúrák kikapcsolása, minden egységre külön
	glActiveTexture( GL_TEXTURE0 );
	glBindTexture( GL_TEXTURE_2D, 0 );
	glActiveTexture( GL_TEXTURE1 );
	glBindTexture( GL_TEXTURE_CUBE_MAP, 0 );


	// VAO kikapcsolása
	glBindVertexArray( 0 );
}

void CMyApp::RenderGUI()
{
	// ImGui::ShowDemoWindow();
	
}

GLint CMyApp::ul( const char* uniformName ) noexcept
{
	GLuint programID = 0;

	// Kérdezzük le az aktuális programot!
	// https://registry.khronos.org/OpenGL-Refpages/gl4/html/glGet.xhtml
	glGetIntegerv( GL_CURRENT_PROGRAM, reinterpret_cast<GLint*>( &programID ) );
	// A program és a uniform név ismeretében kérdezzük le a location-t!
	// https://registry.khronos.org/OpenGL-Refpages/gl4/html/glGetUniformLocation.xhtml
	return glGetUniformLocation( programID, uniformName );
}

// https://wiki.libsdl.org/SDL2/SDL_KeyboardEvent
// https://wiki.libsdl.org/SDL2/SDL_Keysym
// https://wiki.libsdl.org/SDL2/SDL_Keycode
// https://wiki.libsdl.org/SDL2/SDL_Keymod

void CMyApp::KeyboardDown(const SDL_KeyboardEvent& key)
{	
	if ( key.repeat == 0 ) // Először lett megnyomva
	{
		if ( key.keysym.sym == SDLK_F5 && key.keysym.mod & KMOD_CTRL )
		{
			CleanShaders();
			InitShaders();
		}
		if ( key.keysym.sym == SDLK_F1 )
		{
			GLint polygonModeFrontAndBack[ 2 ] = {};
			// https://registry.khronos.org/OpenGL-Refpages/gl4/html/glGet.xhtml
			glGetIntegerv( GL_POLYGON_MODE, polygonModeFrontAndBack ); // Kérdezzük le a jelenlegi polygon módot! Külön adja a front és back módokat.
			GLenum polygonMode = ( polygonModeFrontAndBack[ 0 ] != GL_FILL ? GL_FILL : GL_LINE ); // Váltogassuk FILL és LINE között!
			// https://registry.khronos.org/OpenGL-Refpages/gl4/html/glPolygonMode.xhtml
			glPolygonMode( GL_FRONT_AND_BACK, polygonMode ); // Állítsuk be az újat!
		}
	}
	m_camera.KeyboardDown( key );
}

void CMyApp::KeyboardUp(const SDL_KeyboardEvent& key)
{
	m_camera.KeyboardUp( key );
}

// https://wiki.libsdl.org/SDL2/SDL_MouseMotionEvent

void CMyApp::MouseMove(const SDL_MouseMotionEvent& mouse)
{
	m_camera.MouseMove( mouse );
}

// https://wiki.libsdl.org/SDL2/SDL_MouseButtonEvent

void CMyApp::MouseDown(const SDL_MouseButtonEvent& mouse)
{
}

void CMyApp::MouseUp(const SDL_MouseButtonEvent& mouse)
{
}

// https://wiki.libsdl.org/SDL2/SDL_MouseWheelEvent

void CMyApp::MouseWheel(const SDL_MouseWheelEvent& wheel)
{
	m_camera.MouseWheel( wheel );
}


// a két paraméterben az új ablakméret szélessége (_w) és magassága (_h) található
void CMyApp::Resize(int _w, int _h)
{
	glViewport(0, 0, _w, _h);
	m_camera.Resize( _w, _h );
}

