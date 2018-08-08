#ifndef SCREEN_WIDTH
#define SCREEN_WIDTH 1920
#endif

#ifndef SCREEN_HEIGHT
#define SCREEN_HEIGHT 1080
#endif

#if (SCREEN_HEIGHT == 1080)
#define SCR_DIV .002
#define SCR_W 1.8
#define SCR_H 1
#endif

#if (SCREEN_HEIGHT == 800)
#define SCR_DIV .003
#define SCR_W 1.6
#define SCR_H 1
#endif

#if (SCREEN_HEIGHT == 720)
#define SCR_DIV .003
#define SCR_W 1.8
#define SCR_H 1
#endif

uniform sampler3D m;
vec2 v,x,y,r,z;
vec3 l=vec3(0),f=vec3(-22,3.5,22),n=l,c,s,a,g,t,d,e;
float i=gl_TexCoord[0].x/52838,o=.8,u=0,D=0,b=0,p=0,h=1,C,F,T,Z,Y,X,W,V;

float U(vec3 v)
{
return(texture3D(m,v).x*.8+texture3D(m,v*=2).x*.4+texture3D(m,v*=2).x*.2+texture3D(m,v*=2).x*.1+texture3D(m,v*=2).x*.05)*.02;
}
float S(vec2 t)
{
return texture3D(m,vec3(t,0)).x;
}
float R(vec3 v)
{
return texture3D(m,v).x;
}
float R(vec3 t,vec3 v)
{
return length(min(v-=abs(t),0))-max(min(v.x,min(v.y,v.z)),0);
}
float S(vec3 t,vec2 v)
{
return length(min(v-=abs(t.xy),0))-max(min(v.x,v.y),0);
}
float U(vec3 t,float v)
{
return length(min(v-=abs(t.x),0))-max(v,0);
}
vec3 Q(vec3 t,vec2 v)
{
return vec3(mod(t.xy,v)-v.xy*.5,t.z);
}
vec3 P(vec3 t,float v)
{
return vec3(mod(t.x,v)-v*.5,t.yz);
}
float P(vec3 t)
{
	return C=max(max(R(t,vec3(.5,.25,1)),(t.y+abs(t.x)-.6)*.7),t.y*.99+t.z*.1-.15)-.15,
		n+=max((t.z<0?vec3(1,0,0):vec3(1))*(.25-R(t+vec3(0,.1,0),vec3(.4,0,1))),0),
			l=C<.01?
				vec3(t.y*.3+.3):
				l,
		C;
}

float Q(vec3 t)
{
	return max(t.y+abs(t.x),t.y+abs(t.z))*.7;
}

float O(vec3 v)
{
	return p=C=max(min(max(max(max(Q(v*.5-vec3(0,.2,0))*2,-Q(v*.5-vec3(0,.1,0))*2),min(U(v,.5),U(v.zyx,.5))),v.y+.5),max(Q(v),v.y+.3)),-max(-Q(v+vec3(0,.1,0)),min(U(v,.05),U(v.zyx,.05)))),
	C+=C<.01?
		R(floor(v*33)*.1)*.005:0,
	t=mod(v,vec3(.1))-.05,
	C=min(C,max(p,min(U(t,.002),U(t.zyx,.002)))),
	l=C<.001?
		vec3(.15):
		l,
	C;
}
float N(vec3 t)
{
	return C=R(t,vec3(.03,4,.03)),
	n.x+=max(.2-R(t-vec3(0,4,0),vec3(.03)),0),
	C;
}
float L(vec3 v)
{
	return i<30?
		x=floor(v.xz/vec2(88,44)),
		t=Q(v.xzy,vec2(88,44)),
		t=vec3(mod(t.x+22*x.y,44)-22,t.zy),
		T=atan(t.x,t.z),
		Z=length(t.xz),
		Y=floor(S(vec2(floor(T*1.3)*.3+S(x*.1)))*30)*.4+14,
		X=clamp(floor(Z*2.5)*.4,1.2,Y)-.4,
		p=6/floor(X*6.7),p=mod(T+X*p,p)-p*.5,
		C=R(vec3(vec2(cos(p)*Z-X,t.y-.1)*mat2(.7,.7,-.7,.7),sin(p)*Z),vec3(.15,.01,.15)),
		X+=.4,p=6/floor(X*6.7),
		p=mod(T+X*p,p)-p*.5,
		C=min(C,R(vec3(vec2(cos(p)*Z-X,t.y-.1)*mat2(.7,.7,-.7,.7),sin(p)*Z),vec3(.15,.01,.15))),
		X+=.4,p=6/floor(X*6.7),
		p=mod(T+X*p,p)-p*.5,
		C=min(C,R(vec3(vec2(cos(p)*Z-X,t.y-.1)*mat2(.7,.7,-.7,.7),sin(p)*Z),vec3(.15,.01,.15))),
		l=C<.001?
			d*(1.2-abs(T)*.3):
			l,p=max(min(max(R(t-vec3(0,0,.3),vec3(.3,4,.6)),t.z*.99+t.y*.15-.5),R(t,vec3(.3,4,.2))),max(t.y+t.x,t.y-t.x)*.7-2.92),
		l=p<.001?
			d*(R(floor(t*5)*.05)*.1+.3):
			l,
		F=v.y+U(v*.1)*2,
		l=F<.001?
			d*(.2-v.y+(X-Z<.3&&X-Z>.2||abs(t.x)<.05||abs(t.z)<.05||abs(t.x-t.z)<.07||abs(t.x+t.z)<.07?.2:0)):
			l,
		min(min(min(C,p),F),P(v-vec3(0,10,i*7-50)))
	:i<70?
		l=v.y<.001?vec3(.1):l,
			min(min(min(min(
				min(min(O(v*.5-vec3(-1,1.5,0))*2,O(v*.5-vec3(2,1.5,0))*2),O(v*.05-vec3(.3,1,1.6))*20),
			(
				x=floor(v.xz/vec2(.1)),
				t=Q(v.xzy,vec2(.1)),
				C=min(min(min(min(min(min(min(min(R(t,vec3(.05,.05,S(x*.1)*.15+1)),
						(t.x+=.1,x.x-=1,R(t,vec3(.05,.05,S(x*.1)*.15+1)))),
						(t.y+=.1,x.y-=1,R(t,vec3(.05,.05,S(x*.1)*.15+1)))),
						(t.x-=.1,x.x+=1,R(t,vec3(.05,.05,S(x*.1)*.15+1)))),
						(t.x-=.1,x.x+=1,R(t,vec3(.05,.05,S(x*.1)*.15+1)))),
						(t.y-=.1,x.y+=1,R(t,vec3(.05,.05,S(x*.1)*.15+1)))),
						(t.y-=.1,x.y+=1,R(t,vec3(.05,.05,S(x*.1)*.15+1)))),
						(t.x+=.1,x.x-=1,R(t,vec3(.05,.05,S(x*.1)*.15+1)))),
						(t.x+=.1,x.x-=1,R(t,vec3(.05,.05,S(x*.1)*.15+1)))),
				t=Q(v.xzy,vec2(1,3)),
				Z=U(t.yxz,.05),
				C=max(max(C,-U(t,.025)),-Z),
				e=vec3(0,.7,1)*S(clamp(t.xz,vec2(-1.4,0),vec2(1.4,.9))*1.5+i*.1),
				T=max(max(Z,v.y-.9),-U(t,.2)),
				n+=e*max(.07-T*.4,0),
				l=T<.001?
					e:
					C<.001?
						vec3(.1)+S(x*.1)*.1+S(v.xz)*.05:
						l,
				C
			)),
			v.y-.9),
			(
				t=v*5-vec3(5,5,-20),
				C=max(U(t.yzx,2.3),min(S(t.zxy,vec2(1)),R(P(t.yxz,.3),vec3(.03,1.05,1.05)))),
				l=C<.01?vec3(R(t*.1)+1)*.08:l,
				n+=R(t-vec3(0,2.1,-1),vec3(.8,.05,.05))<.15?vec3(1,1,.8)*(R(t*.5)+.5)*(.15-p):vec3(0),
				min(C*.2,min(min(N(t),N(t+vec3(-.2,.4,0))),N(t+vec3(.2,.7,.3)))*.2)
			)
			),P((v-f).zyx*5)*.2)
	:(
		t=v+vec3(-2,-.15,0),
		T=abs(t.x),
		C=S(t-vec3(0,.2,0),vec2(1.5,.05))+U(v*.5)*.5,
		t=P(t.zxy,1.3),
		p=min(min(R(t,vec3(.05,1.45,.2)),R(t+vec3(0,1.4,0),vec3(.01,.01,1))),R(t+vec3(0,-1.4,0),vec3(.01,.01,1))),
		l=C<.01?
			d*(R(v*5)*.1+.22)+(T<1.3&&T>.2?T>1.28||T<.22||mod(T-.2,.36)<.01&&mod(v.z,.3)<.1?.5:0:.1):
			p<.01?
				d*.2+.1:
				l,
		C=min(C,p),
		t=v+vec3(6,3,0),
		t.yz*=mat2(.7,.7,-.7,.7),
		t.xy*=mat2(.7,.7,-.7,.7),
		p=max(max(max(R(t,vec3(4)),-S(t,vec2(3))),-S(t.zxy,vec2(3))),-S(t.yzx,vec2(3))),
		l=p<.01?
			d*.5+R(t*.1)*.05:l,
		min(C,p)
	);
}

float L(vec2 t,vec2 p)
{
	return length(max(vec2(0),x=max(t-v,v-p)))+min(max(x.x,x.y),0);
}

float K(vec2 v,float m,float x,float p)
{
	for(C=b=0;b<min(floor(m*15),10);b+=1)
		C+=
			(S(vec2(b*p,0))<.5?o=L(r=v+x*vec2(b*.025,0),r+x*.01),o<0?2:.001/abs(o):0)+
			(S(vec2(b*p,.1))<.5?o=L(r=v+x*vec2(b*.025,.01),r+x*.01),o<0?2:.001/abs(o):0)+
			(S(vec2(b*p,.2))<.5?o=L(r=v+x*vec2(b*.025+.01,0),r+x*.01),o<0?2:.001/abs(o):0)+
			(S(vec2(b*p,.3))<.5?o=L(r=v+x*vec2(b*.025+.01,.01),r+x*.01),o<0?2:.001/abs(o):0);
	return C+=mod(m*2,1)<.5?
		o=L(r=v+b*vec2(x*.025,0),r+x*.02),o<0?2:.001/abs(o):0;
}
void main()
{
	F=max(i-80,0)*.5*sin(i*.5);
	for(C=0;C<gl_FragCoord.y;p=p*.5+F*(S(vec2(i,C+=1)*.07)-.5)*.2);

	c=normalize(vec3(
	v=gl_FragCoord.xy*
	SCR_DIV
	-vec2(
	SCR_W
	+p,
	SCR_H
	)
	,2)),
	V=i<15?
		g=vec3(0,17,-30+i*2),d=vec3(1,1,1.5),.0002:
		i<30?
		g=vec3(-4,10,i*1.5-10),d=vec3(1,1,1.5),u=1.6,.0002
		:i<40?
		g=vec3(40+i,4.5,8.5),d=vec3(1),u=-.7,.004
		:i<42.5||i>102?
		v.x=-2
		:i<58?
		g=vec3(2,1.6,i*.2-16),d=vec3(1),o=0,u=2.5-i*.05,.004
		:i<70?
		g=vec3(1,1.5,-5),d=vec3(1),f=vec3(i*1.8-115,3,0),o=-.2,u=6.5-i*.2,.004
		:i<90?
		g=vec3(2,2,i),d=vec3(1.5,1,.8),o=1,.004:
		(g=vec3(3,.5,i-100),d=vec3(1.5,1,.8),o=-.2,u=1.6,.001),

		a=g,s=c,c.yz*=mat2(cos(o),-sin(o),sin(o),cos(o)),c.xz*=mat2(cos(u),-sin(u),sin(u),cos(u));
		for(;D<100&&h>.001;W=h,h=L(a),D+=h,a+=h*c);
		a+=c*(W*(h-.001)/(W-h)-h),W=h,h=L(a),D+=h,a+=h*c,
		f-=g,
		f.xz*=mat2(cos(u),sin(u),-sin(u),cos(u)),
		f.yz*=mat2(cos(o),sin(o),-sin(o),cos(o)),
		f/=s,
		y=f.xy/f.z*v,
		gl_FragColor.xyz=-
		SCR_W
		<v.x?mix(max(l+min((L(a+normalize(vec3(C-L(a-vec3(.01,0,0)),C-L(a-vec3(0,.01,0)),C-L(a-vec3(0,0,.01))))*.001)-h)*90-.1,0)*d,0)+n,vec3(d*.3),min(D*D*V,1))
		*(1-length(v)*.3)
		+U(vec3(v*7,i))+
		vec3(.9,.4,.04)*
		(
			i<15?
				K(vec2(-1.7,-.9),i-7,3,.3)
			:i<30?
				.001/abs(length((r=v-y)-(z=vec2(.2,-.2)*min(max(0,i-15),1))*clamp(dot(r,z)/dot(z,z),0,1)))+
				K(y+vec2(.2,-.23),i-16,1,.4)
			:i>60&&i<70?
				p=L(y+vec2(-.2,-.2),y+vec2(.2)),
				.001/abs(p)+(p<0?.08:0)+
				K(y+vec2(-.2,-.23),i-62,1,.1):0
		)
		:vec3(0);
}
