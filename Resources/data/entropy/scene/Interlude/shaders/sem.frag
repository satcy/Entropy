#version 450

uniform sampler2D tMatCap;
uniform float alpha;
in vec3 e;
in vec3 n;
out vec4 fragColor;

void main() {
	
	vec3 r = reflect( e, n );
	r = e - 2. * dot( n, e ) * n;
	float m = 2. * sqrt( pow( r.x, 2. ) + pow( r.y, 2. ) + pow( r.z + 1., 2. ) );
	vec2 vN = r.xy / m + .5;

	vec3 base = texture2D( tMatCap, vN ).rgb;

	fragColor = vec4( base, alpha );

}