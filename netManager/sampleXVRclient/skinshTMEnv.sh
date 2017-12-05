[VERTEX SHADER]
#define MATRIXINTER
#define SHADOWS


uniform mat4 transforms[40];

attribute vec2 texturecoord;
attribute vec3 normal;
attribute vec4 weight;
attribute vec4 index;

varying vec2 texcoor1;
varying vec3 mnormal;
varying vec3 lightDir;
varying vec3 reflectDir;

//varying vec4 shadowTexCoord;



void main(void)
{
  
    ivec4 id=ivec4(index);
    mat4 weightmat;

/*
    if (dot1<0.0){ qy.x=-qy.x; qy.y=-qy.y; qy.z=-qy.z;qy.w=-qy.w; ty.x=-ty.x;ty.y=-ty.y;ty.z=-ty.z;ty.w=-ty.w;} //
    if (dot2<0.0){ qz.x=-qz.x; qz.y=-qz.y; qz.z=-qz.z;qz.w=-qz.w; tz.x=-tz.x;tz.y=-tz.y;tz.z=-tz.z;tz.w=-tz.w;} //
    if (dot3<0.0){ qw.x=-qw.x; qw.y=-qw.y; qw.z=-qw.z;qw.w=-qw.w; tw.x=-tw.x;tw.y=-tw.y;tw.z=-tw.z;tw.w=-tw.w;} //
*/
    weightmat=transforms[id.x]*weight.x+transforms[id.y]*weight.y+transforms[id.z]*weight.z+transforms[id.w]*weight.w;
    mat3 rotmat;
    rotmat[0]=vec3(weightmat[0]);rotmat[1]=vec3(weightmat[1]);rotmat[2]=vec3(weightmat[2]);

    vec4 pos= gl_ModelViewMatrix*weightmat*gl_Vertex;
    vec4 lpos = gl_ModelViewMatrix*gl_LightSource[0].position;
    lightDir=normalize(lpos.xyz-pos.xyz);
 
//    vec4 tmpNorm=vec4(normal.x,normal.y,normal.z,0.0);
    gl_Position = gl_ProjectionMatrix*pos;
    mnormal=normalize(gl_NormalMatrix*rotmat*normal);
    texcoor1=texturecoord;  
    reflectDir=reflect(pos.xyz,mnormal);
//    shadowTexCoord = gl_TextureMatrix[0] * gl_ModelViewMatrix * pos;

}

[FRAGMENT SHADER]
//#define SHADOWS
/*
// skinning.frag: applies textures and does lighting to the skinning deformed mesh 
//
// author: Bernhard Spanlang
// Technical University of Catalunya (UPC)
// 
//
// Copyright (c) July 2006: Bernhard Spanlang
// http://www.lsi.upc.edu/~bspanlang/
//
*/
//varying vec4 Color;

const vec3 Xunitvec = vec3(1.0, 0.0, 0.0);
const vec3 Yunitvec = vec3(0.0, 1.0, 0.0);



uniform sampler2D tex;	
uniform sampler2D envMap;
uniform float dofragshad;

//uniform samplerCube envMap;	
uniform sampler2DShadow shadowMap;
	
varying vec2 texcoor1;
varying vec3 lightDir;
varying vec3 mnormal;
varying vec3 reflectDir;

//varying vec4 shadowTexCoord;

	
void main()
{
	vec4 color = texture2D(tex,texcoor1.xy);
	
	
	vec2 index;
	vec3 reflDir=reflectDir;

    index.t = dot(normalize(reflDir), Yunitvec);
    reflDir.y = 0.0;
    index.s = dot(normalize(reflDir), Xunitvec) * 0.5;

    // Translate index values into proper range

    if (reflectDir.z >= 0.0)
        index = (index + 1.0) * 0.5;
    else
    {
        index.t = (index.t + 1.0) * 0.5;
        index.s = (-index.s) * 0.5 + 1.0;
    }

	vec3 envCol = texture2D(envMap,index).rgb;
	float intens = dot(lightDir,mnormal);
//	vec4 resColor=vec4(color.rgb*intens, 1.0);
	vec4 resColor=vec4(color.rgb*1.0, 1.0);
	resColor.rgb=mix(resColor.rgb,envCol,0.5);
	gl_FragColor = resColor;
}


