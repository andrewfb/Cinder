#pragma once

#include "tessellate.glsl.exports.h"

namespace rive {
namespace gpu {
namespace glsl {
const char tessellate[] = R"===(#define Hf 10
#ifdef BB
w1(c0)l0(0,g,XC);l0(1,g,YC);l0(2,g,OC);
#ifdef j9
l0(3,uint,ND);l0(4,uint,OD);l0(5,uint,PD);l0(6,uint,QD);
#else
l0(3,M,QB);
#endif
x1
#endif
V1 B0 X(0,g,X5);B0 X(1,g,Y5);B0 X(2,g,p4);B0 X(3,U,h5);x4 X(4,uint,g7);N1
#ifdef BB
n3 N5(r2,H6,RC);o3 h4(H6,o9)U3 g4(Kb,Ge,LB);g4(Lb,He,VC);V3 y1(KF,c0,F,B,O){n0(O,F,XC,g);n0(O,F,YC,g);n0(O,F,OC,g);
#ifdef j9
n0(O,F,ND,uint);n0(O,F,OD,uint);n0(O,F,PD,uint);n0(O,F,QD,uint);M QB=M(ND,OD,PD,QD);
#else
n0(O,F,QB,M);
#endif
T(X5,g);T(Y5,g);T(p4,g);T(h5,U);T(g7,uint);c j0=XC.xy;c k0=XC.zw;c y0=YC.xy;c z0=YC.zw;bool Dc=B<4;float y=Dc?OC.z:OC.w;int ea=int(Dc?QB.x:QB.y);
#ifdef nb
int Ec=ea<<16;if(QB.z==0xffffffffu){--Ec;}float r8=float(Ec>>16);
#else
float r8=float(ea<<16>>16);
#endif
float v8=float(ea>>16);c J1=c((B&1)==0?r8:v8,(B&2)==0?y+1.:y);if((v8-r8)*A.sb<.0){J1.y=2.*y+1.-J1.y;}uint C2=QB.z&0x3ffu;uint Fc=(QB.z>>10)&0x3ffu;uint R1=QB.z>>20;uint a0=QB.w;uint a8=a0&Gb;uint o0=a8>0u?E0(VC,max(a8,1u)-1u).z:0u;M k4=o0!=0u?E0(LB,o0*4u+1u):M(0u,0u,0u,0u);float v2=uintBitsToFloat(k4.z);float w2=uintBitsToFloat(k4.w);if(w2!=.0&&v2==.0){float Gc;float If=Rd(j0,k0,y0,z0,Gc);float fa=w2*(1./B9);float Jf=Md(j0,k0,y0,z0,Gc,fa);float h7=1.-Jf*(1./a3);float Kf=dot(z0-j0,z0-j0)/(fa*fa);float Lf=(Kf-1.)*.5;h7=min(h7,Lf);h7=min(h7,.99);float Mf=.5*h7;float x=G5(Mf)*-2.+1.;float Hc=H7(x*w2,If);g Ic=mix(j0.xyxy,z0.xyxy,g(1./3.,1./3.,2./3.,2./3.));k0=mix(k0,Ic.xy,Hc);y0=mix(y0,Ic.zw,Hc);}if((a0&me)!=0u){W Jc=X1(uintBitsToFloat(E0(LB,o0*4u)));c Kc=P0(Jc,-2.*k0+y0+j0);c Lc=P0(Jc,-2.*y0+z0+k0);float h1=max(dot(Kc,Kc),dot(Lc,Lc));float B4=max(ceil(sqrt(.75*4.*sqrt(h1))),1.);C2=min(uint(B4),C2);}uint w8=C2+Fc+R1-1u;W q2=f9(j0,k0,y0,z0);float X0=acos(e9(q2[0],q2[1]));float C3=X0/float(Fc);float ga=determinant(W(y0-j0,z0-k0));if(ga==.0)ga=determinant(q2);if(ga<.0)C3=-C3;X5=g(j0,k0);Y5=g(y0,z0);p4=g(float(w8)-abs(v8-J1.x),float(w8),(R1<<10)|C2,C3);if(R1>1u){W ha=W(q2[1],OC.xy);float Nf=acos(e9(ha[0],ha[1]));float Mc=float(R1);if((a0&(q3|T7))==(R7|T7)){Mc-=2.;}float ia=Nf/Mc;if(determinant(ha)<.0)ia=-ia;h5.xy=OC.xy;h5.z=ia;}if(v8<r8){a0|=e3;}g7=a0;g P=L7(J1,2./je,A.sb);
#ifdef IC
P.y=-P.y;
#endif
g0(X5);g0(Y5);g0(p4);g0(h5);g0(g7);z1(P);}
#endif
#ifdef GB
c3 d3 d2(Y3,LF){H(X5,g);H(Y5,g);H(p4,g);H(h5,U);H(g7,uint);c j0=X5.xy;c k0=X5.zw;c y0=Y5.xy;c z0=Y5.zw;W q2=f9(j0,k0,y0,z0);float Of=max(floor(p4.x),.0);float w8=p4.y;uint Nc=uint(p4.z);float C2=float(Nc&0x3ffu);float R1=float(Nc>>10);float C3=p4.w;uint a0=g7;float q4=w8-R1;float j2=Of;if(j2<=q4){a0&=~q3;}else{j0=k0=y0=z0;q2=W(q2[1],h5.xy);C2=1.;j2-=q4;q4=R1;C3=h5.z;if((a0&q3)>R7){if(j2<2.5)a0|=C9;if(j2>1.5&&j2<3.5)a0|=Eb;}else if((a0&T7)!=0u||(a0&q3)==S7){q4-=2.;--j2;}a0|=C3<.0?U7:Fb;}c x8;float X0=.0;if(j2==.0||j2==q4||(a0&q3)>R7){bool e8=j2<q4*.5;x8=e8?j0:z0;X0=pb(e8?q2[0]:q2[1]);}else if((a0&Db)!=0u){x8=k0;}else{float m1,i5;if(C2==q4){m1=j2/C2;i5=.0;}else{c r,D,P1=k0-j0;c m6=z0-j0;c E7=y0-k0;D=E7-P1;r=-3.*E7+m6;c Pf=D*(C2*2.);c o6=P1*(C2*C2);float y8=.0;float Qf=min(C2-1.,j2);c ja=normalize(q2[0]);float Rf=-abs(C3);float Sf=(1.+j2)*abs(C3);for(int ka=Hf-1;ka>=0;--ka){float i7=y8+exp2(float(ka));if(i7<=Qf){c la=i7*r+Pf;la=i7*la+o6;float Tf=dot(normalize(la),ja);float ma=i7*Rf+Sf;ma=min(ma,a3);if(Tf>=cos(ma))y8=i7;}}float Uf=y8/C2;float Oc=j2-y8;float z8=acos(clamp(ja.x,-1.,1.));z8=ja.y>=.0?z8:-z8;X0=Oc*C3+z8;c J2=c(sin(X0),-cos(X0));float m=dot(J2,r),A8=dot(J2,D),v0=dot(J2,P1);float Vf=max(A8*A8-m*v0,.0);float c2=sqrt(Vf);if(A8>.0)c2=-c2;c2-=A8;float Pc=-.5*c2*m;c na=(abs(c2*c2+Pc)<abs(m*v0+Pc))?c(c2,m):c(v0,c2);i5=(na.y!=.0)?na.x/na.y:.0;i5=clamp(i5,.0,1.);if(Oc==.0)i5=.0;m1=max(Uf,i5);}c Wf=I5(j0,k0,m1);c Qc=I5(k0,y0,m1);c Xf=I5(y0,z0,m1);c Rc=I5(Wf,Qc,m1);c Sc=I5(Qc,Xf,m1);x8=I5(Rc,Sc,m1);if(m1!=i5)X0=pb(Sc-Rc);}Y3 j7;j7.xy=m9(x8);if((a0&q3)==S7){j7.z=n9((uint(q4)<<16)|uint(j2));}else{j7.z=m9(mod(X0,M7));}j7.w=n9(a0);e2(j7);}
#endif
)===";
} // namespace glsl
} // namespace gpu
} // namespace rive