[VERTEX SHADER]
uniform vec4 quats[30];
uniform vec4 trans[30];

attribute vec2 texturecoord;
attribute vec3 normal;
attribute vec4 weight;
attribute vec4 index;

varying vec2 texcoor1;
varying vec3 mnormal;
varying vec3 lightDir;

#ifdef MATCONV

mat4 DQToMatrix(in vec4 Qn, in vec4 Qd){

	float len2 = dot(Qn, Qn);
/*        float len=sqrt(len2);
        Qn/=len;
        Qd/=len;
*/	float w = Qn.x, x = Qn.y, y = Qn.z, z = Qn.w;
        float w2=w*w, x2=x*x, y2=y*y, z2=z*z;
	float t0 = Qd.x, t1 = Qd.y, t2 = Qd.z, t3 = Qd.w;

        float tr0= 2.0*(-t0*x+t1*w-t2*z+t3*y);
        float tr1= 2.0*(-t0*y+t1*z-t3*x+t2*w);
        float tr2= 2.0*(-t0*z+t2*x+t3*w-t1*y);


/*	mat4 m=mat4(1.0-2.0*(y2 + z2),   2.0*(x*y - w*z),          2.0*(x*z + w*y),   0.0,
	            2.0*(x*y + w*z),     1.0-2.0*(x2+z2),          2.0*(y*z - w*x),   0.0,
	            2.0*(x*z - w*y),     2.0*(y*z + w*x),         1.0-2.0*( x2+y2),   0.0,
	            tr0,tr1,tr2, 1.0);
*/

	mat4 m=mat4(w2+x2-y2-z2,   2.0*(x*y - w*z),          2.0*(x*z + w*y),   0.0,
	            2.0*(x*y + w*z),     w2+y2-x2-z2,        2.0*(y*z - w*x),   0.0,
	            2.0*(x*z - w*y),     2.0*(y*z + w*x),         w2+z2-x2-y2,   0.0,
	            tr0,tr1,tr2, 1.0);
	           
/*	mat4 m=mat4(w2+x2-y2-z2,   2.0*(x*y - w*z),          2.0*(x*z + w*y),   tr0,
	            2.0*(x*y + w*z),     w2+y2-x2-z2,        2.0*(y*z - w*x),   tr1,
	            2.0*(x*z - w*y),     2.0*(y*z + w*x),         w2+z2-x2-y2,   tr2,
	            0.0,0.0,0.0, 1.0);
*/
/*
	mat4 m=mat4(w2+x2-y2-z2,   2.0*(x*y + w*z),          2.0*(x*z - w*y),   tr0,
	            2.0*(x*y - w*z),     w2+y2-x2-z2,        2.0*(y*z + w*x),   tr1,
	            2.0*(x*z + w*y),     2.0*(y*z + w*x),         w2+z2-x2-y2,   tr2,
	            0.0,0.0,0.0, 1.0);
*/	
/*	mat4 m=mat4(1.0-2.0*(y2 + z2),   2.0*(x*y + w*z),          2.0*(x*z - w*y),   0.0,
	            2.0*(x*y - w*z),     1.0-2.0*(x2+z2),          2.0*(y*z + w*x),   0.0,
	            2.0*(x*z + w*y),     2.0*(y*z - w*x),         1.0-2.0*( x2+y2),   0.0,
	            tr0,tr1,tr2, 1.0);
*/
        m/=len2;
	return(m);

}
#endif
/*
inline void operator*=(const CalQuaternion& q)
	{
		float qx, qy, qz, qw;
		qx = x;
		qy = y;
		qz = z;
		qw = w;
		
		x = qw * q.x + qx * q.w + qy * q.z - qz * q.y;
		y = qw * q.y - qx * q.z + qy * q.w + qz * q.x;
		z = qw * q.z + qx * q.y - qy * q.x + qz * q.w;
		w = qw * q.w - qx * q.x - qy * q.y - qz * q.z;
	}
*/
/*	
vec4 mulQuat(in vec4 q1, in vec4 q2){
	vec4 rq;
	rq.x=q1.w*q2.x+q1.x*q2.w+q1.y*q2.z-q1.z*q2.y;
	rq.y=q1.w*q2.y-q1.x*q2.z+q1.y*q2.w+q1.z*q2.x;
	rq.z=q1.w*q2.z+q1.x*q2.y-q1.y*q2.x+q1.z*q2.w;
	rq.w=q1.w*q2.w-q1.x*q2.x-q1.y*q2.y-q1.z*q2.z;
	return(rq);
}
*/

vec4 mulQuat(in vec4 q1, in vec4 q2){
	return(vec4(cross(q1.xyz,q2.xyz)+q1.w*q2.xyz+q2.w*q1.xyz,q1.w*q2.w-dot(q1.xyz,q2.xyz)));
}

vec4 transVec(in vec4 d,in vec4 nd,in vec4 vert){
	vec4 ndc=vec4(-1.0*nd.xyz,nd.w);//conjugate of nondual
	return(mulQuat(ndc,d)*2.0+mulQuat(mulQuat(ndc,vert),nd));
}

void main(void)
{
  
    ivec4 id=ivec4(index);

    vec4 qx=quats[id.x],qy=quats[id.y],qz=quats[id.z],qw=quats[id.w];
    vec4 tx=trans[id.x],ty=trans[id.y],tz=trans[id.z],tw=trans[id.w];
    float dot1=dot(qx,qy), dot2=dot(qx,qz),dot3=dot(qx,qw);
    if (dot1<0.0){ qy*=-1.0;ty*=-1.0;} //
    if (dot2<0.0){ qz*=-1.0;tz*=-1.0;} //
    if (dot3<0.0){ qw*=-1.0;tw*=-1.0;} //

    vec4 nQuat=qx*weight.x+qy*weight.y+qz*weight.z+qw*weight.w;
    vec4 dQuat=tx*weight.x+ty*weight.y+tz*weight.z+tw*weight.w;
    float rlen=1.0/sqrt(dot(nQuat,nQuat));
    nQuat*=rlen;
    dQuat*=rlen;
    dQuat+=nQuat*-1.0*dot(nQuat,dQuat);
     

    vec4 pos= gl_ModelViewMatrix*transVec(dQuat,nQuat,gl_Vertex);;
    vec4 lpos = gl_LightSource[0].position;
    lightDir=normalize(lpos.xyz-pos.xyz);
 
//    vec3 anorm=normalize(transVec(vec4(0.0,1.0,0.0,0.0),nQuat,vec4(normal,0.0)).xyz);
//    vec3 anorm=transVec(dQuat,nQuat,vec4(normal,0.0)).xyz;
    
    gl_Position = gl_ProjectionMatrix*pos;
    vec3 anorm=mulQuat(    mulQuat(nQuat,vec4(normal,0.0)), vec4(-1.0*nQuat.xyz,nQuat.w)   ).xyz;
    mnormal=gl_NormalMatrix*anorm.xyz;
    texcoor1=texturecoord;  
}


[FRAGMENT SHADER]


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
//uniform samplerCube envMap;	
//uniform sampler2D envMap;	
varying vec2 texcoor1;
varying vec3 lightDir;
varying vec3 mnormal;

	
void main()
{
	vec4 color = texture2D(tex,texcoor1.xy);
//	color.rgb = gl_Color.rgb;//color.rgb;
//        vec4 envCol=textureCube(envMap,mnormal);
//    vec4 envCol=texture2D(envMap,mnormal.xy);
//	color.rgb=mix(color.rgb,envCol.rgb,0.5);
//        color = texture2D(tex,texcoor1.xy);

//    float intens = dot(lightDir,mnormal);
//    gl_FragColor = vec4(color.rgb*intens,color.a);
	gl_FragColor = vec4(color.rgb,color.a);



//        gl_FragColor=color;
//        gl_FragColor=vec4(mnormal.r,mnormal.g,mnormal.b,1.0);
}
