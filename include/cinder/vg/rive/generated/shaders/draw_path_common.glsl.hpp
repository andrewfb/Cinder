#pragma once

#include "draw_path_common.glsl.exports.h"

namespace rive {
namespace gpu {
namespace glsl {
const char draw_path_common[] = R"===(#define P6 -2.
#define Wb -1.5
#define Xb .25
#define e8 1e3
#define Yb (e8*e8)
#ifdef BB
j3 kb(A2,ue,EC);
#ifdef IB
Q5(A2,K6,RC);
#endif
k3 g4 v4(Lb,Re,LB);M4(Z8,Ka,HC);N4(a9,La,NB);v4(Mb,Se,XC);h4
#endif
#if defined(IB)||defined(AB)
w4(K6,z9)
#endif
#ifdef GB
W2 z2(A2,Nb,VC);
#if defined(IB)||defined(AB)
Q5(A2,K6,RC);
#endif
#ifdef AB
#ifdef WD
R9(A2,L6,FC);
#elif defined(XD)
S9(A2,L6,FC);
#elif defined(YD)
z2(A2,L6,FC);
#else
W4(A2,L6,FC);
#endif
#endif
z2(l3,w6,UB);
#if defined(CB)&&defined(HB)&&!defined(FB)
Q6(ID);
#endif
X2 w4(Nb,g9)
#ifdef AB
w4(L6,T9)
#endif
i4 A3(l3,x6,z3)j4
#endif
#ifdef GB
e bool B5(g B){return B.y>=.0;}e bool B5(G B){return B.y>=.0;}
#endif
#if defined(GB)&&defined(IB)
e bool i9(g B){return B.x<Wb;}e bool F7(g B){return B.y<Wb;}
#endif
#ifdef BB
g Zb(float U9,c f8,float C1){c R5=(1.-f8*abs(C1))*.5;float G3,X4;if(abs(U9-A6)<1./e8){G3=.0;X4=.0;}else{float V9=tan(U9);G3=sign(A6-U9)/max(abs(V9),1./Yb);X4=G3>=.0?R5.y-(1.-R5.x)*V9:R5.y+R5.x*V9;}g B;B.x=max(R5.x,.0)+Xb;B.y=-R5.y+P6;B.z=G3;B.w=X4;return B;}
#endif
#ifdef IB
e d T2(g B p3){d G3=B.z;d X4=max(B.w,.0);d S5=G3>=.0?m4(X4):.0;if(abs(G3)<e8){d x=abs(B.x)-Xb;d y=-B.y+P6;d N2=(y-X4)*0.5984134206;i t=X4+N2*T0(0.20888568955,0.62665706865,1.04442844776,1.46219982687);i u=t*-G3+(y*G3+x);i Te=T0(m4(u[0]),m4(u[1]),m4(u[2]),m4(u[3]));i ac=t*5.09593080173+-2.54796540086;i Ue=exp2(-ac*ac);S5+=dot(Te,Ue)*N2;}return S5*sign(B.x);}e d w3(g B p3){float S5=1.;float Ve=(1.-P6)+B.x;S5-=m4(Ve);float We=1.-B.y;S5-=m4(We);return S5;}
#endif
#if defined(GB)&&defined(AB)
e d I7(c W9,c d4 p3){c T5=round(W9);i B;
#ifdef WD
B=uintBitsToFloat(P(x4(FC,T9,T5,d4)));
#elif defined(XD)
p4 Xe=p4(x4(FC,T9,T5,d4));B=g(Xe)*(1./Pb);
#elif defined(YD)
Y O1=Y(T5);o4 Ye=Zd(lb(FC,O1,.xyzw));B=T0(a8,-a8,255.,-255.)*Ye;
#else
B=T0(x4(FC,T9,T5,d4));
#endif
B=T0(I5(B.x),I5(B.y),I5(B.z),I5(B.w));B.xw=mix(B.xw,B.yz,k1(W9.x+.5-T5.x));B.x=mix(B.w,B.x,k1(W9.y+.5-T5.y));return m4(B.x);}
#endif
#if defined(BB)&&defined(TC)
e Y Y4(int bc){return Y(bc&((1<<Ab)-1),bc>>Ab);}e float cc(d0 G0,c Ze){c W1=H0(G0,Ze);return(abs(W1.x)+abs(W1.y))*(1./dot(W1,W1));}e bool p7(g R6,g X9,int O,A1(uint)P2,A1(c)af
#ifndef CB
,A1(g)I1
#else
,A1(V)S6
#endif
U5){int g8=int(R6.x);float C1=R6.y;float Y9=R6.z;int dc=floatBitsToInt(R6.w)>>2;int T6=floatBitsToInt(R6.w)&3;int Z9=min(g8,dc-1);int y4=O*dc+Z9;k4 Z4=G1(EC,Y4(y4));uint f0=U4(Z4.w);uint h8=max(f0&Hb,1u);P aa=A0(XC,h8-1u);c ec=uintBitsToFloat(aa.xy);P2=aa.z&0xffffu;uint fc=aa.w;d0 G0=N1(uintBitsToFloat(A0(LB,P2*4u)));P z4=A0(LB,P2*4u+1u);c j1=uintBitsToFloat(z4.xy);float B2=uintBitsToFloat(z4.z);float C2=uintBitsToFloat(z4.w);uint gc=f0&m3;if(gc!=0u){g8=int(X9.x);C1=X9.y;Y9=X9.z;}if(g8!=Z9){int hc=y4+g8-Z9;k4 ic=G1(EC,Y4(hc));if((U4(ic.w)&(m3|0xffffu))!=(f0&(m3|0xffffu))){bool bf=B2==.0||ec.x!=.0;if(bf){y4=int(fc);Z4=G1(EC,Y4(y4));}}else{y4=hc;Z4=ic;}f0=(U4(Z4.w)&~m3)|gc;}float e1;
#ifdef IB
float U6;float w1;if((f0&C3)==W7&&T6==Z7){uint jc=U4(Z4.z);float H3=float(jc&0xffffu);float Y1=float(jc>>16);Y i8=Y(-H3-1.,Y1-H3+1.);if((f0&m3)!=0u)i8=-i8;k4 kc=G1(EC,Y4(y4+i8.x));k4 ba=G1(EC,Y4(y4+i8.y));if((U4(ba.w)&(m3|0xffffu))!=(U4(kc.w)&(m3|0xffffu))){ba=G1(EC,Y4(int(fc)));}U6=H5(kc.z);float lc=H5(ba.z);w1=lc-U6;if(abs(w1)>i3)w1-=Q7*sign(w1);float ca=Y1+1.-float(Bb);float mc=clamp(round(abs(w1)/i3*ca),1.,ca-1.);float V6=ca-mc;if(H3<=V6){w1=-(i3*sign(w1)-w1);Y1=V6;if(H3==V6)C1=-C1;}else if(H3==V6+1.){H3=.0;Y1=.0;C1=.0;}else{H3-=V6+2.;Y1=mc;}if(H3==Y1){e1=lc;}else{e1=U6+w1*(H3/Y1);}}else
#endif
{e1=H5(Z4.z);}c O2=c(sin(e1),-cos(e1));c nc=H5(Z4.xy);c j8=c(0,0);if(C2!=.0){C2=max(C2,(J9/3.)/length(H0(G0,O2)));}if(B2!=.0){C1*=sign(determinant(G0));if((f0&Y7)!=0u)C1=min(C1,.0);if((f0&Gb)!=0u)C1=max(C1,.0);float A4=C2!=.0?C2:cc(G0,O2)*S3;d oc=1.;if(A4>B2&&C2==.0){oc=J4(B2)/J4(A4);B2=A4;}c a5=O2*(B2+A4);
#ifndef CB
float x=C1*(B2+A4);I1.xy=(1./(A4*2.))*(c(x,-x)+B2)+.5;I1.zw=m6(.0);
#endif
uint da=f0&C3;if(da>V7){int W6=2;if((f0&K9)==0u)W6=-W6;if((f0&m3)!=0u)W6=-W6;Y cf=Y4(y4+W6);k4 df=G1(EC,cf);float ef=H5(df.z);float X6=abs(ef-e1);if(X6>i3)X6=Q7-X6;bool k8=(f0&K9)!=0u;bool ff=(f0&Y7)!=0u;float pc=X6*(k8==ff?-.5:.5)+e1;c l8=c(sin(pc),-cos(pc));float ea=cc(G0,l8);float Y6=cos(X6*.5);float fa;if((da==ne)||(da==oe&&Y6>=.25)){float gf=(f0&X7)!=0u?1.:.25;fa=B2*(1./max(Y6,gf));}else{fa=B2*Y6+ea*.5;}float ga=fa+ea*S3;if((f0&Fb)!=0u){float qc=B2+A4;float hf=A4*.125;if(qc<=ga*Y6+hf){float jf=qc*(1./Y6);a5=l8*jf;}else{c ha=l8*ga;c kf=c(dot(a5,a5),dot(ha,ha));a5=H0(kf,inverse(d0(a5,ha)));}}c lf=abs(C1)*a5;float rc=(ga-dot(lf,l8))/(ea*(S3*2.));
#ifndef CB
if((f0&Y7)!=0u)I1.y=rc;else I1.x=rc;
#endif
}
#ifndef CB
I1.xy*=oc;I1.y=max(I1.y,1e-4);if(C2!=.0){I1.x=P6-I1.x;}
#endif
j8=H0(G0,C1*a5);if(T6!=Z7)return false;}else{
#ifndef CB
I1=g(Y9,-1.,.0,.0);
#ifdef IB
if(C2!=.0){I1.y=P6;I1.z=Yb;I1.w=Y9;if((f0&C3)==W7&&T6==Z7){if(w1<.0){U6+=w1;w1=-w1;}float I3=e1-U6;I3=mod(I3+A6,Q7)-A6;I3=clamp(I3,.0,w1);if(I3>w1*.5){I3=w1-I3;}c f8=c(sin(I3),cos(I3));
#if 0
float J1=1.+.33*log2(A6/(i3-min(w1,i3-i3/16.)));g mf=Zb(w1,f8,.5*(J1/3.));float nf=T2(mf P0);float of=I5(nf);float pf=(.5-of)*(J9*2.);float qf=J1/max(pf,J1);C1*=qf;
#endif
I1=Zb(w1,f8,C1);}j8=H0(G0,(C1*C2)*O2);}else
#endif
{j8=sign(H0(C1*O2,inverse(G0)))*S3;}if(bool(f0&m3)!=bool(f0&pe)){I1*=g(-1.,+1.,+1.,+1.);}
#endif
if(T6==Jb)nc=ec;if((f0&Eb)!=0u&&T6!=Ib){return false;}}af=H0(G0,nc)+j8+j1;
#ifdef CB
P B4=A0(LB,P2*4u+2u);S6=R1(B4.x);
#else
I1.xy=mix(I1.xy,c(1.,-1.),Yd(q.fe!=0u));
#endif
return true;}
#endif
#if defined(BB)&&defined(EB)
e c S8(X V5,A1(uint)P2
#ifdef CB
,A1(V)S6
#else
,A1(d)rf
#endif
U5){P2=floatBitsToUint(V5.z)&0xffffu;
#ifdef CB
P B4=A0(LB,P2*4u+2u);S6=R1(B4.x);
#else
rf=A9(floatBitsToInt(V5.z)>>16);
#endif
c W5=V5.xy;d0 G0=N1(uintBitsToFloat(A0(LB,P2*4u)));P z4=A0(LB,P2*4u+1u);c j1=uintBitsToFloat(z4.xy);W5=H0(G0,W5)+j1;return W5;}
#endif
#if defined(BB)&&defined(AB)
e c R8(X V5,A1(uint)P2,
#ifdef CB
A1(V)S6,
#endif
A1(c)sf U5){P2=floatBitsToUint(V5.z)&0xffffu;P B4=A0(LB,P2*4u+2u);
#ifdef CB
S6=R1(B4.x);
#endif
c W5=V5.xy;X Z6=uintBitsToFloat(B4.yzw);sf=W5*Z6.x+Z6.yz;return W5;}
#endif
)===";
} // namespace glsl
} // namespace gpu
} // namespace rive