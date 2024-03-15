#version 430

// pipeline-ból bejövő per-fragment attribútumok
in vec3 vs_out_pos;
in vec3 vs_out_norm;
in vec2 vs_out_tex;

// kimenő érték - a fragment színe
out vec4 fs_out_col;

// textúra mintavételező objektum
uniform sampler2D texImage;

// kamera pozíció
uniform vec3 cameraPos;

// fenyforras tulajdonsagok
 uniform vec4 lightPos;

 uniform vec3 La = vec3(1.0, 1.0, 1.0 );
 uniform vec3 Ld = vec3(1.0, 1.0, 1.0 );
 uniform vec3 Ls = vec3(1.0, 1.0, 1.0 );

uniform float lightConstantAttenuation    = 1.0;
uniform float lightLinearAttenuation      = 2.0;
uniform float lightQuadraticAttenuation   = 0.0;

// anyag tulajdonsagok

 uniform vec3 Ka = vec3( 1.0 );
 uniform vec3 Kd = vec3( 1.0 );
 uniform vec3 Ks = vec3( 1.0 );

 uniform float Shininess = 80.0;


/* segítség:
	    - normalizálás: http://www.opengl.org/sdk/docs/manglsl/xhtml/normalize.xml
	    - skaláris szorzat: http://www.opengl.org/sdk/docs/manglsl/xhtml/dot.xml
	    - clamp: http://www.opengl.org/sdk/docs/manglsl/xhtml/clamp.xml
		- reflect: http://www.opengl.org/sdk/docs/manglsl/xhtml/reflect.xml
				reflect(beérkező_vektor, normálvektor);
		- pow: http://www.opengl.org/sdk/docs/manglsl/xhtml/pow.xml
				pow(alap, kitevő);
*/

void main()
{
	// A fragment normálvektora
	// MINDIG normalizáljuk!
	vec3 normal = normalize( vs_out_norm );

	vec3 ambient = Ka*La;

	vec3 toLight = normalize(lightPos.xyz - vs_out_pos);
	vec3 diffuse = clamp(dot(toLight,normal),0.f,1.f) * Ld * Kd;

	vec3 toEye = normalize(cameraPos - vs_out_pos);
	vec3 r = reflect(normalize(-toLight),normal);
	vec3 specular = pow(clamp(dot(normalize(r),toEye),0,1),Shininess) * Ks * Ls;

	// normal vector debug:
	// fs_out_col = vec4( normal * 0.5 + 0.5, 1.0 );

	fs_out_col = vec4((ambient + diffuse + specular),1) + texture(texImage, vs_out_tex);
}