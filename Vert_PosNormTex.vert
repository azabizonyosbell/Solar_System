#version 430

float M_PI = 3.14;

// VBO-ból érkező változók
layout( location = 0 ) in vec3 vs_in_pos;
layout( location = 1 ) in vec3 vs_in_norm;
layout( location = 2 ) in vec2 vs_in_tex;

// a pipeline-ban tovább adandó értékek
out vec3 vs_out_pos;
out vec3 vs_out_norm;
out vec2 vs_out_tex;

// shader külső paraméterei - most a három transzformációs mátrixot külön-külön vesszük át
uniform mat4 world;
uniform mat4 worldIT;
uniform mat4 viewProj;

vec3 GetPos(float u, float v){
		float a = u * 2 * (M_PI + 0.005);
		float b = v * M_PI;

		float r = 1;

		float x = r * cos(a) * sin(b);
		float y = r * sin(a) * sin(b);
		float z = r * cos(b);

		return vec3(x, z, y);
	}

vec3 GetNorm(float u, float v){
		vec3 p = GetPos(u, v);
		return normalize(p);
	}

vec2 GetTex(float u, float v){
		return vec2(u, v);
	}

void main()
{
	gl_Position = viewProj * world * vec4( GetNorm(vs_in_pos.x, vs_in_pos.y), 1 );
	vs_out_pos  = (world   * vec4(GetPos(vs_in_pos.x, vs_in_pos.y),  1)).xyz;
	vs_out_norm = (worldIT * vec4(GetNorm(vs_in_pos.x, vs_in_pos.y), 0)).xyz;
	vs_out_tex = GetTex(vs_in_tex.x, vs_in_tex.y);

}