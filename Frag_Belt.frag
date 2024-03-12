#version 430

// pipeline-b�l bej�v� per-fragment attrib�tumok
in vec3 vs_out_pos;
in vec3 vs_out_norm;
in vec2 vs_out_tex;

// kimen� �rt�k - a fragment sz�ne
out vec4 fs_out_col;

// text�ra mintav�telez� objektum
uniform sampler2D texImage;

/* seg�ts�g:
	    - normaliz�l�s: http://www.opengl.org/sdk/docs/manglsl/xhtml/normalize.xml
	    - skal�ris szorzat: http://www.opengl.org/sdk/docs/manglsl/xhtml/dot.xml
	    - clamp: http://www.opengl.org/sdk/docs/manglsl/xhtml/clamp.xml
		- reflect: http://www.opengl.org/sdk/docs/manglsl/xhtml/reflect.xml
				reflect(be�rkez�_vektor, norm�lvektor);
		- pow: http://www.opengl.org/sdk/docs/manglsl/xhtml/pow.xml
				pow(alap, kitev�);
*/

void main()
{
	// A fragment norm�lvektora
	// MINDIG normaliz�ljuk!
	vec3 normal = normalize( vs_out_norm );

	if (!gl_FrontFacing){
		normal *= -1;
	}
	
	// normal vector debug:
	// fs_out_col = vec4( normal * 0.5 + 0.5, 1.0 );
	fs_out_col =  texture(texImage, vs_out_tex);
}