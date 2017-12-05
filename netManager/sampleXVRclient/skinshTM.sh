[VERTEX SHADER]
#define MATRIXINTER
#define SHADOWS


uniform mat4 transforms[50];
uniform float morphval0;
uniform float morphval1;
uniform float morphval2;
uniform float morphval3;
uniform float morphval4;


attribute vec2 texturecoord;
attribute vec3 normal;
attribute vec4 weight;
attribute vec4 index;

attribute vec3 vertexMorph0;
attribute vec3 normalMorph0;

attribute vec3 vertexMorph1;
attribute vec3 normalMorph1;

attribute vec3 vertexMorph2;
attribute vec3 normalMorph2;

attribute vec3 vertexMorph3;
attribute vec3 normalMorph3;

attribute vec3 vertexMorph4;
attribute vec3 normalMorph4;



varying vec2 texcoor1;
varying vec3 mnormal;
varying vec3 lightDir;

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
//	vec4 pos= gl_ModelViewMatrix*weightmat*gl_Vertex;
    vec4 pos= gl_ModelViewMatrix*weightmat*vec4((vec3(gl_Vertex)+vertexMorph0*morphval0+vertexMorph1*morphval1+vertexMorph2*morphval2+vertexMorph3*morphval3+vertexMorph4*morphval4),1.0);
 //   vec4 pos= gl_ModelViewMatrix*weightmat*vec4((vec3(gl_Vertex)+vertexMorph0*morphval0+vertexMorph1*morphval1+vertexMorph2*morphval2),1.0);
//    vec4 pos= gl_ModelViewMatrix*weightmat*vec4((vec3(gl_Vertex)+vertexMorph0*morphval0),1.0);
    vec4 lpos = gl_LightSource[0].position;
    lightDir=normalize(lpos.xyz-pos.xyz);
 
//    vec4 tmpNorm=vec4(normal.x,normal.y,normal.z,0.0);
    gl_Position = gl_ProjectionMatrix*pos;
    mnormal=normalize(gl_NormalMatrix*rotmat*normal);
    texcoor1=texturecoord;  
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




uniform sampler2D tex;	
uniform float dofragshad;
uniform sampler2D bump;
uniform float normalinfl;

//uniform samplerCube envMap;	
//uniform sampler2DShadow shadowMap;
	
varying vec2 texcoor1;
varying vec3 lightDir;
varying vec3 mnormal;

//varying vec4 shadowTexCoord;

	
void main()
{
	vec4 color = texture2D(tex,texcoor1.xy);
	vec3 nrm = texture2D(bump,texcoor1.xy).rgb*2.0-vec3(1,1,1);

	vec3 up=vec3(0.0,1.0,0.0);
	vec3 right=cross(up,mnormal);
	vec3 upv=cross(right,mnormal);
	mat3 tangMat;
	tangMat[0]=right;
	tangMat[1]=upv;
	tangMat[2]=mnormal;
	vec3 adnrm=tangMat*nrm;
		
	vec3 pertnorm=normalize(mnormal+adnrm*normalinfl);
	
	float intens = dot(lightDir,pertnorm);
	vec4 resColor=vec4(color.rgb*intens, color.a);
//	vec4 resColor=vec4(nrm.rgb, 1.0);
//	vec4 resColor=vec4(vec3(1.0,1.0,1.0)*intens, 1.0);
	gl_FragColor = resColor;
}


