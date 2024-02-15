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

struct Sphere
{
	glm::vec3 GetPos(float u, float v) const noexcept
	{

		float a = u * 2 * M_PI;
		float b = v * M_PI;

		float r = 1;

		float x = r * cos(a) * sin(b);
		float y = r * sin(a) * sin(b);
		float z = r * cos(b);

		return glm::vec3(x, z, y);

	}
	glm::vec3 GetNorm(float u, float v) const noexcept
	{
		glm::vec3 p = GetPos(u, v);

		float e = 0.01;

		glm::vec3 u1 = GetPos(glm::clamp(u - e, 0.f, 1.f), v);
		glm::vec3 u2 = GetPos(glm::clamp(u + e, 0.f, 1.f), v);

		glm::vec3 t1 = u2 - u1;

		glm::vec3 v1 = GetPos(u, glm::clamp(v - e, 0.f, 1.f));
		glm::vec3 v2 = GetPos(u, glm::clamp(v + e, 0.f, 1.f));

		glm::vec3 t2 = v2 - v1;

		return normalize(glm::cross(t1, t2));
	}
	glm::vec2 GetTex(float u, float v) const noexcept
	{
		return glm::vec2(u, v);
	}
};

void CMyApp::InitGeometry()
{

	const std::initializer_list<VertexAttributeDescriptor> vertexAttribList =
	{
		{ 0, offsetof( Vertex, position ), 3, GL_FLOAT },
		{ 1, offsetof( Vertex, normal   ), 3, GL_FLOAT },
		{ 2, offsetof( Vertex, texcoord ), 2, GL_FLOAT },
	};

	// Suzanne

	/*MeshObject<Vertex> suzanneMeshCPU = ObjParser::parse("Assets/Suzanne.obj");
	m_SuzanneGPU = CreateGLObjectFromMesh( suzanneMeshCPU, vertexAttribList );*/

	// Parametrikus felület
	/*MeshObject<Vertex> surfaceMeshCPU = GetParamSurfMesh(Param());
	m_surfaceGPU = CreateGLObjectFromMesh( surfaceMeshCPU, vertexAttribList );*/

	// Bolygó (gömb)
	MeshObject<Vertex> planetMeshCPU = GetParamSurfMesh(Sphere());
	m_planetGPU = CreateGLObjectFromMesh(planetMeshCPU, vertexAttribList);

	// Skybox
	InitSkyboxGeometry();
}

void CMyApp::CleanGeometry()
{
//	CleanOGLObject( m_SuzanneGPU );
//	CleanOGLObject( m_surfaceGPU );
	CleanOGLObject( m_planetGPU );
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

void CMyApp::RenderPlanet(glm::mat4 matWorld) {

	glBindVertexArray(m_planetGPU.vaoID);

	glUniformMatrix4fv(ul("world"), 1, GL_FALSE, glm::value_ptr(matWorld));
	glUniformMatrix4fv(ul("worldIT"), 1, GL_FALSE, glm::value_ptr(glm::transpose(glm::inverse(matWorld))));

	glDrawElements(GL_TRIANGLES,
		m_planetGPU.count,
		GL_UNSIGNED_INT,
		nullptr);
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
}

void CMyApp::CleanTextures()
{
	// diffuse textures

//	glDeleteTextures( 1, &m_SuzanneTextureID );
//	glDeleteTextures( 1, &m_surfaceTextureID );
	glDeleteTextures(1, &m_sunTextureID);

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
		glm::vec3(0.0, 7.0, 7.0),// honnan nézzük a színteret	   - eye
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

	// Suzanne

	/*glBindVertexArray(m_SuzanneGPU.vaoID);

	// - Textúrák beállítása, minden egységre külön
	glActiveTexture( GL_TEXTURE0 );
	glBindTexture( GL_TEXTURE_2D, m_SuzanneTextureID );*/

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


	// - textúraegységek beállítása
	/*glUniform1i(ul("texImage"), 0);

	glDrawElements(GL_TRIANGLES,
					m_SuzanneGPU.count,			 
					GL_UNSIGNED_INT,
					nullptr );

	// Parametrikus felület

	glBindVertexArray( m_surfaceGPU.vaoID );

	glActiveTexture( GL_TEXTURE0 );
	glBindTexture( GL_TEXTURE_2D, m_surfaceTextureID );

	matWorld = glm::translate( glm::vec3(2.0f,0.0f,0.0f));

	glUniformMatrix4fv( ul( "world" ),    1, GL_FALSE, glm::value_ptr( matWorld ) );
	glUniformMatrix4fv( ul( "worldIT" ),  1, GL_FALSE, glm::value_ptr( glm::transpose( glm::inverse( matWorld ) ) ) );

	glDrawElements(GL_TRIANGLES,
					m_surfaceGPU.count,
					GL_UNSIGNED_INT,
					nullptr );*/


	// Nap
	// középpont: (0.0,0.0,0.0); sugár: 1.0; forgástengely: 7.25 deg

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, m_sunTextureID);

	matWorld = glm::rotate<float>(glm::radians(-7.25f), glm::vec3(0.0f, 0.0f, 1.0f))
		* glm::identity<glm::mat4>();
	RenderPlanet(matWorld);

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

