#pragma once

#include "tessellate.glsl.exports.h"

namespace rive {
namespace gpu {
namespace glsl {
const char tessellate[] = R"===(#define Qf 10
#ifdef BB
g1(c0)i0(0,g,ZC);i0(1,g,AD);i0(2,g,PC);
#ifdef r9
i0(3,uint,ND);i0(4,uint,OD);i0(5,uint,PD);i0(6,uint,QD);
#else
i0(3,P,VB);
#endif
h1
#endif
D1 q0 N(0,g,a6);q0 N(1,g,c6);q0 N(2,g,E4);q0 N(3,X,l5);e3 N(4,uint,k7);z1
#ifdef BB
j3 Q5(A2,K6,RC);k3 w4(K6,z9)g4 v4(Lb,Re,LB);v4(Mb,Se,XC);h4 p1(JF,c0,D,p,O){j0(O,D,ZC,g);j0(O,D,AD,g);j0(O,D,PC,g);
#ifdef r9
j0(O,D,ND,uint);j0(O,D,OD,uint);j0(O,D,PD,uint);j0(O,D,QD,uint);P VB=P(ND,OD,PD,QD);
#else
j0(O,D,VB,P);
#endif
L(a6,g);L(c6,g);L(E4,g);L(l5,X);L(k7,uint);c o0=ZC.xy;c p0=ZC.zw;c B0=AD.xy;c C0=AD.zw;bool Jc=p<4;float y=Jc?PC.z:PC.w;int oa=int(Jc?VB.x:VB.y);
#ifdef ob
int Kc=oa<<16;if(VB.z==0xffffffffu){--Kc;}float A8=float(Kc>>16);
#else
float A8=float(oa<<16>>16);
#endif
float B8=float(oa>>16);c O1=c((p&1)==0?A8:B8,(p&2)==0?y+1.:y);if((B8-A8)*q.tb<.0){O1.y=2.*y+1.-O1.y;}uint I2=VB.z&0x3ffu;uint Lc=(VB.z>>10)&0x3ffu;uint Y1=VB.z>>20;uint f0=VB.w;uint h8=f0&Hb;uint Z=h8>0u?A0(XC,max(h8,1u)-1u).z:0u;P z4=Z!=0u?A0(LB,Z*4u+1u):P(0u,0u,0u,0u);float B2=uintBitsToFloat(z4.z);float C2=uintBitsToFloat(z4.w);if(C2!=.0&&B2==.0){float Mc;float Rf=Rd(o0,p0,B0,C0,Mc);float pa=C2*(1./J9);float Sf=Md(o0,p0,B0,C0,Mc,pa);float l7=1.-Sf*(1./i3);float Tf=dot(C0-o0,C0-o0)/(pa*pa);float Uf=(Tf-1.)*.5;l7=min(l7,Uf);l7=min(l7,.99);float Vf=.5*l7;float x=I5(Vf)*-2.+1.;float Nc=N7(x*C2,Rf);g Oc=mix(o0.xyxy,C0.xyxy,g(1./3.,1./3.,2./3.,2./3.));p0=mix(p0,Oc.xy,Nc);B0=mix(B0,Oc.zw,Nc);}if((f0&me)!=0u){d0 Pc=N1(uintBitsToFloat(A0(LB,Z*4u)));c Qc=H0(Pc,-2.*p0+B0+o0);c Rc=H0(Pc,-2.*B0+C0+p0);float n1=max(dot(Qc,Qc),dot(Rc,Rc));float K4=max(ceil(sqrt(.75*4.*sqrt(n1))),1.);I2=min(uint(K4),I2);}uint C8=I2+Lc+Y1-1u;d0 y2=n9(o0,p0,B0,C0);float e1=acos(m9(y2[0],y2[1]));float O3=e1/float(Lc);float qa=determinant(d0(B0-o0,C0-p0));if(qa==.0)qa=determinant(y2);if(qa<.0)O3=-O3;a6=g(o0,p0);c6=g(B0,C0);E4=g(float(C8)-abs(B8-O1.x),float(C8),(Y1<<10)|I2,O3);if(Y1>1u){d0 ra=d0(y2[1],PC.xy);float Wf=acos(m9(ra[0],ra[1]));float Sc=float(Y1);if((f0&(C3|X7))==(V7|X7)){Sc-=2.;}float sa=Wf/Sc;if(determinant(ra)<.0)sa=-sa;l5.xy=PC.xy;l5.z=sa;}if(B8<A8){f0|=m3;}k7=f0;g J=P7(O1,2./je,q.tb);
#ifdef JC
J.y=-J.y;
#endif
W(a6);W(c6);W(E4);W(l5);W(k7);l1(J);}
#endif
#ifdef GB
W2 X2 U1(k4,KF){I(a6,g);I(c6,g);I(E4,g);I(l5,X);I(k7,uint);c o0=a6.xy;c p0=a6.zw;c B0=c6.xy;c C0=c6.zw;d0 y2=n9(o0,p0,B0,C0);float Xf=max(floor(E4.x),.0);float C8=E4.y;uint Tc=uint(E4.z);float I2=float(Tc&0x3ffu);float Y1=float(Tc>>10);float O3=E4.w;uint f0=k7;float F4=C8-Y1;float l2=Xf;if(l2<=F4){f0&=~C3;}else{o0=p0=B0=C0;y2=d0(y2[1],l5.xy);I2=1.;l2-=F4;F4=Y1;O3=l5.z;if((f0&C3)>V7){if(l2<2.5)f0|=K9;if(l2>1.5&&l2<3.5)f0|=Fb;}else if((f0&X7)!=0u||(f0&C3)==W7){F4-=2.;--l2;}f0|=O3<.0?Y7:Gb;}c D8;float e1=.0;if(l2==.0||l2==F4||(f0&C3)>V7){bool k8=l2<F4*.5;D8=k8?o0:C0;e1=qb(k8?y2[0]:y2[1]);}else if((f0&Eb)!=0u){D8=p0;}else{float v1,m5;if(I2==F4){v1=l2/I2;m5=.0;}else{c A,F,T1=p0-o0;c n6=C0-o0;c K7=B0-p0;F=K7-T1;A=-3.*K7+n6;c Yf=F*(I2*2.);c p6=T1*(I2*I2);float E8=.0;float Zf=min(I2-1.,l2);c ta=normalize(y2[0]);float ag=-abs(O3);float bg=(1.+l2)*abs(O3);for(int ua=Qf-1;ua>=0;--ua){float m7=E8+exp2(float(ua));if(m7<=Zf){c va=m7*A+Yf;va=m7*va+p6;float cg=dot(normalize(va),ta);float wa=m7*ag+bg;wa=min(wa,i3);if(cg>=cos(wa))E8=m7;}}float dg=E8/I2;float Uc=l2-E8;float F8=acos(clamp(ta.x,-1.,1.));F8=ta.y>=.0?F8:-F8;e1=Uc*O3+F8;c O2=c(sin(e1),-cos(e1));float l=dot(O2,A),G8=dot(O2,F),x0=dot(O2,T1);float eg=max(G8*G8-l*x0,.0);float g2=sqrt(eg);if(G8>.0)g2=-g2;g2-=G8;float Vc=-.5*g2*l;c xa=(abs(g2*g2+Vc)<abs(l*x0+Vc))?c(g2,l):c(x0,g2);m5=(xa.y!=.0)?xa.x/xa.y:.0;m5=clamp(m5,.0,1.);if(Uc==.0)m5=.0;v1=max(dg,m5);}c fg=K5(o0,p0,v1);c Wc=K5(p0,B0,v1);c gg=K5(B0,C0,v1);c Xc=K5(fg,Wc,v1);c Yc=K5(Wc,gg,v1);D8=K5(Xc,Yc,v1);if(v1!=m5)e1=qb(Yc-Xc);}k4 n7;n7.xy=x9(D8);if((f0&C3)==W7){n7.z=y9((uint(F4)<<16)|uint(l2));}else{n7.z=x9(mod(e1,Q7));}n7.w=y9(f0);V1(n7);}
#endif
)===";
} // namespace glsl
} // namespace gpu
} // namespace rive