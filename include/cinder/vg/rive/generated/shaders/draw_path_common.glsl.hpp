#pragma once

#include "draw_path_common.glsl.exports.h"

namespace rive {
namespace gpu {
namespace glsl {
const char draw_path_common[] = R"===(#define L6 -2.
#define Pb -1.5
#define Qb .25
#define X7 1e3
#define Rb (X7*X7)
#ifdef BB
n3 jb(r2,te,AC);
#ifdef HB
N5(r2,H6,RC);
#endif
o3 U3 g4(Kb,Ge,LB);E4(S8,Fa,QC);F4(T8,Ga,PB);g4(Lb,He,VC);V3
#endif
#if defined(HB)||defined(CB)
h4(H6,o9)
#endif
#ifdef GB
c3 K2(r2,Mb,CD);
#if defined(HB)||defined(CB)
N5(r2,H6,RC);
#endif
#ifdef CB
#ifdef WD
G9(r2,I6,BC);
#elif defined(XD)
H9(r2,I6,BC);
#elif defined(YD)
K2(r2,I6,BC);
#else
S4(r2,I6,BC);
#endif
#endif
K2(W3,I7,EC);
#if defined(AB)&&defined(FB)&&!defined(EB)
M6(ID);
#endif
d3 h4(Mb,Ma)
#ifdef CB
h4(I6,I9)
#endif
N4 X3(W3,J7,B5)O4
#endif
#ifdef GB
d bool y5(g v){return v.y>=.0;}d bool y5(E v){return v.y>=.0;}
#endif
#if defined(GB)&&defined(HB)
d bool Oa(g v){return v.x<Pb;}d bool Pa(g v){return v.y<Pb;}
#endif
#ifdef BB
g Sb(float J9,c Y7,float r1){c O5=(1.-Y7*abs(r1))*.5;float r3,T4;if(abs(J9-x6)<1./X7){r3=.0;T4=.0;}else{float K9=tan(J9);r3=sign(x6-J9)/max(abs(K9),1./Rb);T4=r3>=.0?O5.y-(1.-O5.x)*K9:O5.y+O5.x*K9;}g v;v.x=max(O5.x,.0)+Qb;v.y=-O5.y+L6;v.z=r3;v.w=T4;return v;}
#endif
#ifdef HB
d e k3(g v f3){e r3=v.z;e T4=max(v.w,.0);e P5=r3>=.0?a4(T4):.0;if(abs(r3)<X7){e x=abs(v.x)-Qb;e y=-v.y+L6;e I2=(y-T4)*0.5984134206;i t=T4+I2*c1(0.20888568955,0.62665706865,1.04442844776,1.46219982687);i u=t*-r3+(y*r3+x);i Ie=c1(a4(u[0]),a4(u[1]),a4(u[2]),a4(u[3]));i Tb=t*5.09593080173+-2.54796540086;i Je=exp2(-Tb*Tb);P5+=dot(Ie,Je)*I2;}return P5*sign(v.x);}d e Q3(g v f3){float P5=1.;float Ke=(1.-L6)+v.x;P5-=a4(Ke);float Le=1.-v.y;P5-=a4(Le);return P5;}
#endif
#if defined(GB)&&defined(CB)
d e c9(c L9,c M4 f3){c Q5=round(L9);i v;
#ifdef WD
v=uintBitsToFloat(M(i4(BC,I9,Q5,M4)));
#elif defined(XD)
e4 Me=e4(i4(BC,I9,Q5,M4));v=g(Me)*(1./Ob);
#elif defined(YD)
S J1=S(Q5);d4 Ne=Zd(kb(BC,J1,.xyzw));v=c1(W7,-W7,255.,-255.)*Ne;
#else
v=c1(i4(BC,I9,Q5,M4));
#endif
v=c1(G5(v.x),G5(v.y),G5(v.z),G5(v.w));v.xw=mix(v.xw,v.yz,d1(L9.x+.5-Q5.x));v.x=mix(v.w,v.x,d1(L9.y+.5-Q5.y));return a4(v.x);}
#endif
#if defined(BB)&&defined(ZC)
d S U4(int Ub){return S(Ub&((1<<zb)-1),Ub>>zb);}d float Vb(W O0,c Oe){c Q1=P0(O0,Oe);return(abs(Q1.x)+abs(Q1.y))*(1./dot(Q1,Q1));}d bool K8(g N6,g M9,int O,q1(uint)L2,q1(c)Pe
#ifndef AB
,q1(g)E1
#else
,q1(Y)O6
#endif
R5){int Z7=int(N6.x);float r1=N6.y;float N9=N6.z;int Wb=floatBitsToInt(N6.w)>>2;int P6=floatBitsToInt(N6.w)&3;int O9=min(Z7,Wb-1);int j4=O*Wb+O9;Y3 V4=D1(AC,U4(j4));uint a0=Q4(V4.w);uint a8=max(a0&Gb,1u);M P9=E0(VC,a8-1u);c Xb=uintBitsToFloat(P9.xy);L2=P9.z&0xffffu;uint Yb=P9.w;W O0=X1(uintBitsToFloat(E0(LB,L2*4u)));M k4=E0(LB,L2*4u+1u);c O1=uintBitsToFloat(k4.xy);float v2=uintBitsToFloat(k4.z);float w2=uintBitsToFloat(k4.w);uint Zb=a0&e3;if(Zb!=0u){Z7=int(M9.x);r1=M9.y;N9=M9.z;}if(Z7!=O9){int ac=j4+Z7-O9;Y3 bc=D1(AC,U4(ac));if((Q4(bc.w)&(e3|0xffffu))!=(a0&(e3|0xffffu))){bool Qe=v2==.0||Xb.x!=.0;if(Qe){j4=int(Yb);V4=D1(AC,U4(j4));}}else{j4=ac;V4=bc;}a0=(Q4(V4.w)&~e3)|Zb;}float X0;
#ifdef HB
float Q6;float n1;if((a0&q3)==S7&&P6==V7){uint cc=Q4(V4.z);float v3=float(cc&0xffffu);float R1=float(cc>>16);S c8=S(-v3-1.,R1-v3+1.);if((a0&e3)!=0u)c8=-c8;Y3 dc=D1(AC,U4(j4+c8.x));Y3 Q9=D1(AC,U4(j4+c8.y));if((Q4(Q9.w)&(e3|0xffffu))!=(Q4(dc.w)&(e3|0xffffu))){Q9=D1(AC,U4(int(Yb)));}Q6=F5(dc.z);float ec=F5(Q9.z);n1=ec-Q6;if(abs(n1)>a3)n1-=M7*sign(n1);float R9=R1+1.-float(Ab);float fc=clamp(round(abs(n1)/a3*R9),1.,R9-1.);float R6=R9-fc;if(v3<=R6){n1=-(a3*sign(n1)-n1);R1=R6;if(v3==R6)r1=-r1;}else if(v3==R6+1.){v3=.0;R1=.0;r1=.0;}else{v3-=R6+2.;R1=fc;}if(v3==R1){X0=ec;}else{X0=Q6+n1*(v3/R1);}}else
#endif
{X0=F5(V4.z);}c J2=c(sin(X0),-cos(X0));c gc=F5(V4.xy);c d8=c(0,0);if(w2!=.0){w2=max(w2,(B9/3.)/length(P0(O0,J2)));}if(v2!=.0){r1*=sign(determinant(O0));if((a0&U7)!=0u)r1=min(r1,.0);if((a0&Fb)!=0u)r1=max(r1,.0);float l4=w2!=.0?w2:Vb(O0,J2)*I3;e hc=1.;if(l4>v2&&w2==.0){hc=A4(v2)/A4(l4);v2=l4;}c W4=J2*(v2+l4);
#ifndef AB
float x=r1*(v2+l4);E1.xy=(1./(l4*2.))*(c(x,-x)+v2)+.5;E1.zw=l6(.0);
#endif
uint S9=a0&q3;if(S9>R7){int S6=2;if((a0&C9)==0u)S6=-S6;if((a0&e3)!=0u)S6=-S6;S Re=U4(j4+S6);Y3 Se=D1(AC,Re);float Te=F5(Se.z);float T6=abs(Te-X0);if(T6>a3)T6=M7-T6;bool e8=(a0&C9)!=0u;bool Ue=(a0&U7)!=0u;float ic=T6*(e8==Ue?-.5:.5)+X0;c f8=c(sin(ic),-cos(ic));float T9=Vb(O0,f8);float U6=cos(T6*.5);float U9;if((S9==ne)||(S9==oe&&U6>=.25)){float Ve=(a0&T7)!=0u?1.:.25;U9=v2*(1./max(U6,Ve));}else{U9=v2*U6+T9*.5;}float V9=U9+T9*I3;if((a0&Eb)!=0u){float jc=v2+l4;float We=l4*.125;if(jc<=V9*U6+We){float Xe=jc*(1./U6);W4=f8*Xe;}else{c W9=f8*V9;c Ye=c(dot(W4,W4),dot(W9,W9));W4=P0(Ye,inverse(W(W4,W9)));}}c Ze=abs(r1)*W4;float kc=(V9-dot(Ze,f8))/(T9*(I3*2.));
#ifndef AB
if((a0&U7)!=0u)E1.y=kc;else E1.x=kc;
#endif
}
#ifndef AB
E1.xy*=hc;E1.y=max(E1.y,1e-4);if(w2!=.0){E1.x=L6-E1.x;}
#endif
d8=P0(O0,r1*W4);if(P6!=V7)return false;}else{
#ifndef AB
E1=g(N9,-1.,.0,.0);
#ifdef HB
if(w2!=.0){E1.y=L6;E1.z=Rb;E1.w=N9;if((a0&q3)==S7&&P6==V7){if(n1<.0){Q6+=n1;n1=-n1;}float w3=X0-Q6;w3=mod(w3+x6,M7)-x6;w3=clamp(w3,.0,n1);if(w3>n1*.5){w3=n1-w3;}c Y7=c(sin(w3),cos(w3));
#if 0
float F1=1.+.33*log2(x6/(a3-min(n1,a3-a3/16.)));g af=Sb(n1,Y7,.5*(F1/3.));float bf=k3(af f1);float cf=G5(bf);float df=(.5-cf)*(B9*2.);float ef=F1/max(df,F1);r1*=ef;
#endif
E1=Sb(n1,Y7,r1);}d8=P0(O0,(r1*w2)*J2);}else
#endif
{d8=sign(P0(r1*J2,inverse(O0)))*I3;}if(bool(a0&e3)!=bool(a0&pe)){E1*=g(-1.,+1.,+1.,+1.);}
#endif
if(P6==Ib)gc=Xb;if((a0&Db)!=0u&&P6!=Hb){return false;}}Pe=P0(O0,gc)+d8+O1;
#ifdef AB
M m4=E0(LB,L2*4u+2u);O6=W1(m4.x);
#else
E1.xy=mix(E1.xy,c(1.,-1.),Yd(A.fe!=0u));
#endif
return true;}
#endif
#if defined(BB)&&defined(DB)
d c Da(U S5,q1(uint)L2
#ifdef AB
,q1(Y)O6
#else
,q1(e)ff
#endif
R5){L2=floatBitsToUint(S5.z)&0xffffu;
#ifdef AB
M m4=E0(LB,L2*4u+2u);O6=W1(m4.x);
#else
ff=p9(floatBitsToInt(S5.z)>>16);
#endif
c T5=S5.xy;W O0=X1(uintBitsToFloat(E0(LB,L2*4u)));M k4=E0(LB,L2*4u+1u);c O1=uintBitsToFloat(k4.xy);T5=P0(O0,T5)+O1;return T5;}
#endif
#if defined(BB)&&defined(CB)
d c Ca(U S5,q1(uint)L2,
#ifdef AB
q1(Y)O6,
#endif
q1(c)gf R5){L2=floatBitsToUint(S5.z)&0xffffu;M m4=E0(LB,L2*4u+2u);
#ifdef AB
O6=W1(m4.x);
#endif
c T5=S5.xy;U V6=uintBitsToFloat(m4.yzw);gf=T5*V6.x+V6.yz;return T5;}
#endif
)===";
} // namespace glsl
} // namespace gpu
} // namespace rive